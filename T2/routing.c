#include "routing.h"

//Função com rotina de inicialização dos roteadores
void inicializar(informacoesRoteador_t *infoRoteador,
                roteadorVizinho_t infoVizinhos[NROUT],
                distSalto_t tabelaRoteamento[NROUT][NROUT],
                int listaVizinhos[NROUT],
                filaPacotes_t *entrada,
                filaPacotes_t *saida,
                pthread_mutex_t *log_mutex,
                pthread_mutex_t *messages_mutex,
                pthread_mutex_t *news_mutex){
  int i, j, auxLista;
  int id = infoRoteador->id;

  //Inicializa o vetor de informações de vizinhos e a tabela de roteamento
  for(i = 0; i < NROUT; i++){
    infoVizinhos[i].novidade = 0;
    infoVizinhos[i].id = infoVizinhos[i].porta = -1;
    infoVizinhos[i].custo = INF;
    for(j = 0; j < NROUT; j++){
      tabelaRoteamento[i][j].distancia = INF;
      tabelaRoteamento[i][j].proxSalto = -1;
    }
  }

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
  
  //infoRoteador->qtdVizinhos = roteador[id].qtdVizinhos;

  //Inicializa as filas de entrada e saida
  entrada->inicio = saida->inicio = entrada->fim = saida->fim = 0;
  pthread_mutex_init(&(entrada->mutex), NULL);
  pthread_mutex_init(&(saida->mutex), NULL);

  //Inicializa mutex dos arquivos de log e mensagens e de checagem de queda de nó
  pthread_mutex_init(log_mutex, NULL);
  pthread_mutex_init(messages_mutex, NULL);
  pthread_mutex_init(news_mutex, NULL);
}

void configurarEnlace(informacoesRoteador_t *infoRoteador,
                      int listaVizinhos[NROUT],
                      roteadorVizinho_t infoVizinhos[NROUT]){
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
      infoVizinhos[r2].id = r2;
      infoVizinhos[r2].custo = infoVizinhos[r2].custoOriginal = dist;
    }
  }
  fclose(arquivo);
}

void configurarRoteadores(informacoesRoteador_t *infoRoteador,
                          roteadorVizinho_t infoVizinhos[NROUT]){
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

//Imprime informações sobre o roteador
void info(int id, int port, char adress[TAM_IP], int neigh_qtty, int listaVizinhos[NROUT],
    roteadorVizinho_t infoVizinhos[NROUT], distSalto_t tabelaRoteamento[NROUT][NROUT]){
  int i;

  printf("O nó %d, está conectado à porta %d, Seu endereço é %s\n\n", id, port, adress);
  printf("Seus vizinhos são:\n");
  for(i = 0; i < neigh_qtty; i++)
    printf("O roteador %d, com enlace de custo %d, na porta %d, e endereço %s\n", listaVizinhos[i],
    infoVizinhos[listaVizinhos[i]].custo, infoVizinhos[listaVizinhos[i]].porta, infoVizinhos[listaVizinhos[i]].ip);  
    //neigh_info[listaVizinhos[i]].cost, neigh_info[listaVizinhos[i]].port, neigh_info[listaVizinhos[i]].adress);
  printf("\n");

  printf("Essa é sua tabela de roteamento, atualmente:\n");
  printRoteamento(tabelaRoteamento, NULL);
}

//Copia o pacote a para o pacote b
void copy_package(pacote_t *a, pacote_t *b){
  int i;

  b->controle = a->controle;
  b->destino = a->destino;
  b->origem = a->origem;
  strcpy(b->mensagem, a->mensagem);
  for(i = 0; i < NROUT; i++){
    b->vetorDistancia[i].distancia = a->vetorDistancia[i].distancia;
    b->vetorDistancia[i].proxSalto = a->vetorDistancia[i].proxSalto;
  }
}

//Enfilera um pacote para cada vizinho do nó, contendo seu vetor de distancia
void queue_dist_vec(
                      filaPacotes_t *saida,
                      int listaVizinhos[NROUT],
                      distSalto_t tabelaRoteamento[NROUT][NROUT],
                      int id,
                      int neigh_qtty
                    ) {
  int i, j;

  pthread_mutex_lock(&(saida->mutex));
  for(i = 0; i < neigh_qtty; i++, saida->fim++){
    pacote_t *pck = &(saida->filaPacotes[saida->fim]);
    pck->controle = 1;
    pck->origem = id;
    pck->destino = listaVizinhos[i];
    //printf("Enfileirando pacote de vetor de distancia para o destino %d\n", pck->dist);
    //printf("Vetor de distancia enviado: ");
    for(j = 0; j < NROUT; j++){
      pck->vetorDistancia[j].distancia = tabelaRoteamento[id][j].distancia;
      pck->vetorDistancia[j].proxSalto = tabelaRoteamento[id][j].proxSalto;
      //printf("(%d,%d) ", pck->vetorDistancia[j].distancia, pck->vetorDistancia[j].proxSalto);
    }
  }
  pthread_mutex_unlock(&(saida->mutex));
}

void printRoteamento(distSalto_t tabelaRoteamento[NROUT][NROUT], FILE *arquivo){
  if(arquivo){
    fprintf(arquivo, " ________________________________________________\n");
    fprintf(arquivo, "|%35s%14s\n", "TABELA DE ROTEAMENTO", "|");
    fprintf(arquivo, "|------------------------------------------------|\n");
    for(int i = 0; i < NROUT; i++){
      if(i == 0){
        fprintf(arquivo, "|        |");
      }
      fprintf(arquivo, "|%4d%5s", i, "|");
    }
    fprintf(arquivo, "\n");
    for(int i = 0; i < NROUT; i++){
      fprintf(arquivo, "|%4d%5s", i, "|");
      for(int j = 0; j < NROUT; j++){
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
    for(int i = 0; i < NROUT; i++){
      if(i == 0){
        printf("|        |");
      }
      printf("|%4d%5s", i, "|");
    }
    printf("\n");
    for(int i = 0; i < NROUT; i++){
      printf("|%4d%5s", i, "|");
      for(int j = 0; j < NROUT; j++){
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

void print_file(FILE *file, pthread_mutex_t *mutex){
  char str[100];
  pthread_mutex_lock(mutex);
  fseek(file, 0, SEEK_SET);
  while(fgets(str, 100, file)){
    printf("%s", str);
  }
  fseek(file, 0, SEEK_END);
  pthread_mutex_unlock(mutex);
}

void menu(informacoesRoteador_t infoRoteador){
  system("clear");
  printf(" ____________________________________________________________\n");
  printf("|%s%d%17s%d%12s%-15s|\n", "Roteador: ", infoRoteador.id, "Porta: ", infoRoteador.porta, "IP: ", infoRoteador.ip);
  printf("|------------------------------------------------------------|\n");
  printf("|%-60s|\n", "1 - Mandar mensagem");
  printf("|%-60s|\n", "2 - Ler Mensagens");
  printf("|%-60s|\n", "3 - log");
  printf("|%-60s|\n", "4 - info");
  printf("|%-60s|\n", "0 - Sair");
  printf("|____________________________________________________________|\n\n");
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
