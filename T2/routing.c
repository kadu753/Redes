#include "routing.h"

//Função com rotina de inicialização dos roteadores
void inicializar(informacoesRoteador_t *infoRoteador,
                roteadorVizinho_t infoVizinhos[N_ROTEADORES],
                distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES],
                int listaVizinhos[N_ROTEADORES],
                filaPacotes_t *entrada,
                filaPacotes_t *saida,
                pthread_mutex_t *logMutex,
                pthread_mutex_t *mensagemMutex,
                pthread_mutex_t *novidadeMutex){
  int i, j, auxLista;
  int id = infoRoteador->id;

  //Inicializa o vetor de informações de vizinhos e a tabela de roteamento
  for(i = 0; i < N_ROTEADORES; i++){
    infoVizinhos[i].novidade = 0;
    infoVizinhos[i].id = infoVizinhos[i].porta = infoVizinhos[i].saltoOriginal = -1;
    infoVizinhos[i].custo = INF;
    for(j = 0; j < N_ROTEADORES; j++){
      tabelaRoteamento[i][j].distancia = INF;
      tabelaRoteamento[i][j].proxSalto = -1;
    }
  }

  infoVizinhos[id].saltoOriginal = id;

  configurarEnlace(infoRoteador, listaVizinhos, infoVizinhos);
  configurarRoteadores(infoRoteador, infoVizinhos);

  //Custo de um nó para ele mesmo é 0, via ele mesmo
  tabelaRoteamento[id][id].distancia = 0;
  tabelaRoteamento[id][id].proxSalto = id;

  //Preenche o vetor de distâncias inicial do nó
  for(i = 0; i < infoRoteador->qtdVizinhos; i++){
    auxLista = listaVizinhos[i];
    tabelaRoteamento[id][auxLista].distancia = infoVizinhos[auxLista].custo;
    tabelaRoteamento[id][auxLista].proxSalto = infoVizinhos[auxLista].id;
  }

  //Inicializa as filas de entrada e saida
  entrada->inicio = saida->inicio = entrada->fim = saida->fim = 0;
  pthread_mutex_init(&(entrada->mutex), NULL);
  pthread_mutex_init(&(saida->mutex), NULL);

  //Inicializa mutex dos arquivos de log e mensagens e de checagem de queda de nó
  pthread_mutex_init(logMutex, NULL);
  pthread_mutex_init(mensagemMutex, NULL);
  pthread_mutex_init(novidadeMutex, NULL);
}

void configurarEnlace(informacoesRoteador_t *infoRoteador,
                      int listaVizinhos[N_ROTEADORES],
                      roteadorVizinho_t infoVizinhos[N_ROTEADORES]){
  int id = infoRoteador->id;
  int r1, r2, dist;
  FILE *arquivo = fopen("enlaces.config", "r");
  if(!arquivo) die("Não foi possível abrir o arquivo de enlaces\n");
  while(fscanf(arquivo, "%d %d %d\n", &r1, &r2, &dist) != EOF){
    if(r2 == id){
      r2 = r1;
      r1 = id;
    }
    if(r1 == id){
      listaVizinhos[infoRoteador->qtdVizinhos++] = r2;
      infoVizinhos[r2].id = infoVizinhos[r2].saltoOriginal = r2;
      infoVizinhos[r2].custo = infoVizinhos[r2].custoOriginal = dist;
    }
  }
  fclose(arquivo);
}

void configurarRoteadores(informacoesRoteador_t *infoRoteador,
                          roteadorVizinho_t infoVizinhos[N_ROTEADORES]){
  int id = infoRoteador->id;
  int novoID, novaPorta;
  char novoIP[TAM_IP];
  FILE *arquivo = fopen("roteador.config", "r");
  if(!arquivo) die("Não foi possível abrir o arquivo de roteadores\n");
  while(fscanf(arquivo, "%d %d %s\n", &novoID, &novaPorta, novoIP) != EOF){
    if(novoID == id){
      infoRoteador->porta = novaPorta;
      strcpy(infoRoteador->ip, novoIP);
    }
    if(infoVizinhos[novoID].id != -1){
      infoVizinhos[novoID].porta = novaPorta;
      strcpy(infoVizinhos[novoID].ip, novoIP);
    }
  }
  fclose(arquivo);
}

void duplicarPacote(pacote_t *original, pacote_t *copia){
  copia->controle = original->controle;
  copia->origem = original->origem;
  copia->destino = original->destino;
  strcpy(copia->mensagem, original->mensagem);
  for(int i = 0; i < N_ROTEADORES; i++){
    copia->vetorDistancia[i].distancia = original->vetorDistancia[i].distancia;
    copia->vetorDistancia[i].proxSalto = original->vetorDistancia[i].proxSalto;
  }
}

void enfileirarPacote(filaPacotes_t *saida, distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES], 
                      informacoesRoteador_t infoRoteador, int listaVizinhos[N_ROTEADORES]){
  pthread_mutex_lock(&(saida->mutex));
  for(int i = 0; i < infoRoteador.qtdVizinhos; i++, saida->fim++){
    pacote_t *auxPacote = &(saida->filaPacotes[saida->fim]);
    auxPacote->controle = 1;
    auxPacote->origem = infoRoteador.id;
    auxPacote->destino = listaVizinhos[i];
    for(int j = 0; j < N_ROTEADORES; j++){
      auxPacote->vetorDistancia[j].distancia = tabelaRoteamento[infoRoteador.id][j].distancia;
      auxPacote->vetorDistancia[j].proxSalto = tabelaRoteamento[infoRoteador.id][j].proxSalto;
    }
  }
  pthread_mutex_unlock(&(saida->mutex));
}

void recalcular(int vizinho, distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES], roteadorVizinho_t infoVizinhos[N_ROTEADORES], informacoesRoteador_t infoRoteador){
  pthread_mutex_lock(&logMutex);
  fprintf(logs, "Calculando novo vetor de distancia...\n");
  pthread_mutex_unlock(&logMutex);
  for(int i = 0; i < N_ROTEADORES; i++){
    for(int j = 0; j < N_ROTEADORES; j++){
      if(i == infoRoteador.id){
        if(infoVizinhos[j].saltoOriginal == -1){
          tabelaRoteamento[i][j].distancia = INF;
          tabelaRoteamento[i][j].proxSalto = -1;
        } else {
          tabelaRoteamento[i][j].distancia = infoVizinhos[j].custoOriginal;
          tabelaRoteamento[i][j].proxSalto = infoVizinhos[j].saltoOriginal;
        }
      } else {
        tabelaRoteamento[i][j].distancia = INF;
        tabelaRoteamento[i][j].proxSalto = -1;
      }
    }
  }
  printRoteamento(tabelaRoteamento, NULL);
}

void printRoteamento(distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES], FILE *arquivo){
  if(arquivo){
    fprintf(arquivo, " ________________________________________________\n");
    timeStamp();
    fprintf(arquivo, "|%35s%14s\n", "TABELA DE ROTEAMENTO", "|");
    fprintf(arquivo, "|------------------------------------------------|\n");
    for(int i = 0; i < N_ROTEADORES; i++){
      if(i == 0){
        fprintf(arquivo, "|        |");
      }
      fprintf(arquivo, "|%4d%5s", i, "|");
    }
    fprintf(arquivo, "\n");
    for(int i = 0; i < N_ROTEADORES; i++){
      fprintf(arquivo, "|%4d%5s", i, "|");
      for(int j = 0; j < N_ROTEADORES; j++){
        if(tabelaRoteamento[i][j].distancia != INF){
          fprintf(arquivo, "|%3d[%d]%3s", tabelaRoteamento[i][j].distancia, tabelaRoteamento[i][j].proxSalto, "|");
        } else {
          fprintf(arquivo, "|%4s%5s", "-", "|");
        }
      }
      fprintf(arquivo, "\n");
    }
    fprintf(arquivo, "|________________________________________________|\n");
  } else {
    printf(" ________________________________________________\n");
    printf("|%35s%14s\n", "TABELA DE ROTEAMENTO", "|");
    printf("|------------------------------------------------|\n");
    for(int i = 0; i < N_ROTEADORES; i++){
      if(i == 0){
        printf("|        |");
      }
      printf("|%4d%5s", i, "|");
    }
    printf("\n");
    for(int i = 0; i < N_ROTEADORES; i++){
      printf("|%4d%5s", i, "|");
      for(int j = 0; j < N_ROTEADORES; j++){
        if(tabelaRoteamento[i][j].distancia != INF){
          printf("|%3d[%d]%3s", tabelaRoteamento[i][j].distancia, tabelaRoteamento[i][j].proxSalto, "|");
        } else {
          printf("|%4s%5s", "-", "|");
        }
      }
      printf("\n");
    }
    printf("|________________________________________________|\n");
  }
}

void printarArquivo(FILE *arquivo, pthread_mutex_t *mutex){
  char str[100];
  pthread_mutex_lock(mutex);
  fseek(arquivo, 0, SEEK_SET);
  while(fgets(str, 100, arquivo)){
    printf("%s", str);
  }
  fseek(arquivo, 0, SEEK_END);
  pthread_mutex_unlock(mutex);
}

void menu(informacoesRoteador_t infoRoteador){
  system("clear");
  printf(" ____________________________________________________________\n");
  printf("|%s%d%17s%d%12s%-15s|\n", "Roteador: ", infoRoteador.id, "Porta: ", infoRoteador.porta, "IP: ", infoRoteador.ip);
  printf("|------------------------------------------------------------|\n");
  printf("|%-60s|\n", "1 - Mandar mensagem");
  printf("|%-60s|\n", "2 - Ler Mensagens");
  printf("|%-60s|\n", "3 - Log");
  printf("|%-60s|\n", "4 - Tabela de Roteamento");
  printf("|%-60s|\n", "0 - Sair");
  printf("|____________________________________________________________|\n\n");
}

void timeStamp(){
  time_t     now;
  struct tm  ts;
  char       buf[80];
  
  // Get current time
  time(&now);

  // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
  ts = *localtime(&now);
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  fprintf(logs, "|%38s%12s", buf, "|\n");
}

//Funcao para imprimir mensagens de erro e encerrar o programa
void die(char* msg){
  printf("%s\n", msg);
  exit(1);
};

//Funcao para converter uma string para inteiro
int toint(char *str){
  int i, pot, ans;
  ans = 0;
  for(i = strlen(str) - 1, pot = 1; i >= 0; i--, pot *= 10)
    ans += pot * (str[i] - '0');
  return ans;
}
