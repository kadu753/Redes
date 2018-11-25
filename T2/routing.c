#include "routing.h"

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

//Função com rotina de inicialização dos roteadores
void inicializar(informacoesRoteador_t *infoRoteador,
                roteadorVizinho_t infoVizinhos[NROUT],
                distSalto_t tabelaRoteamento[NROUT][NROUT],
                int neigh_list[NROUT],
                roteador_t roteador[],
                filaPacotes_t *entrada,
                filaPacotes_t *saida,
                pthread_mutex_t *log_mutex,
                pthread_mutex_t *messages_mutex,
                pthread_mutex_t *news_mutex,
                struct sockaddr_in *si_me){
  int i, j, auxLista;
  int id = infoRoteador->id;

  //Inicializa o vetor de informações de vizinhos e a tabela de roteamento
  for(i = 0; i < NROUT; i++){
    infoVizinhos[i].novidade = 0;
    infoVizinhos[i].id = infoVizinhos[i].porta = -1;
    infoVizinhos[i].custo = INF;
    //neigh_info[i].news = 0;
    //neigh_info[i].id = neigh_info[i].port = -1;
    //neigh_info[i].cost = INF;
    for(j = 0; j < NROUT; j++){
      tabelaRoteamento[i][j].distancia = INF;
      tabelaRoteamento[i][j].proxSalto = -1;
    }
  }

  configurarEnlace(id, neigh_list, roteador, infoVizinhos);
  configurarRoteadores(infoRoteador, roteador, infoVizinhos);

  //Custo de um nó para ele mesmo é 0, via ele mesmo
  tabelaRoteamento[id][id].distancia = 0;
  tabelaRoteamento[id][id].proxSalto = id;

  //Preenche o vetor de distâncias inicial do nó
  for(i = 0; i < roteador[id].qtdVizinhos; i++){
    auxLista = neigh_list[i];
    tabelaRoteamento[id][auxLista].distancia = infoVizinhos[auxLista].custo;
    tabelaRoteamento[id][auxLista].proxSalto = infoVizinhos[auxLista].id;
  }
  
  infoRoteador->qtdVizinhos = roteador[id].qtdVizinhos;

  //Inicializa as filas de entrada e saida
  entrada->inicio = saida->inicio = entrada->fim = saida->fim = 0;
  pthread_mutex_init(&(entrada->mutex), NULL);
  pthread_mutex_init(&(saida->mutex), NULL);

  //Inicializa mutex dos arquivos de log e mensagens e de checagem de queda de nó
  pthread_mutex_init(log_mutex, NULL);
  pthread_mutex_init(messages_mutex, NULL);
  pthread_mutex_init(news_mutex, NULL);
}

void configurarEnlace(int id, 
                      int neigh_list[NROUT], 
                      roteador_t roteador[], 
                      roteadorVizinho_t infoVizinhos[NROUT]){
  int r1, r2, dist;
  FILE *arquivo = fopen("enlaces.config", "r");
  if(!arquivo) die("Não foi possível abrir o arquivo de enlaces\n");
  while(fscanf(arquivo, "%d %d %d\n", &r1, &r2, &dist) != EOF){
    if(r2 == id){
      r2 = r1;
      r1 = id;
    }
    if(r1 == id){
      neigh_list[roteador[id].qtdVizinhos++] = r2;
      infoVizinhos[r2].id = r2;
      infoVizinhos[r2].custo = infoVizinhos[r2].custoOriginal = dist;
      //neigh_info[r2].id = r2;
      //neigh_info[r2].cost = neigh_info[r2].orig_cost = dist;
    }
  }
  fclose(arquivo);
}

void configurarRoteadores(informacoesRoteador_t *infoRoteador, 
                          roteador_t roteador[],
                          roteadorVizinho_t infoVizinhos[NROUT]){
  int id = infoRoteador->id;
  int novoID, novaPorta;
  char novoIP[TAM_IP];
  FILE *arquivo = fopen("roteador.config", "r");
  if(!arquivo) die("Não foi possível abrir o arquivo de roteadores\n");
  while(fscanf(arquivo, "%d %d %s\n", &novoID, &novaPorta, novoIP) != EOF){
    if(novoID == id){
      infoRoteador->porta = novaPorta;
      roteador[id].porta = novaPorta;
      strcpy(roteador[id].ip, novoIP);
      strcpy(infoRoteador->ip, novoIP);
    }
    if(infoVizinhos[novoID].id != -1){
      infoVizinhos[novoID].porta = novaPorta;
      strcpy(infoVizinhos[novoID].ip, novoIP);
      //neigh_info[novoID].port = novaPorta;
      //strcpy(neigh_info[novoID].adress, novoIP);
    }
  }
  fclose(arquivo);
}

//Imprime informações sobre o roteador
void info(int id, int port, char adress[TAM_IP], int neigh_qtty, int neigh_list[NROUT],
    roteadorVizinho_t infoVizinhos[NROUT], distSalto_t tabelaRoteamento[NROUT][NROUT]){
  int i;

  printf("O nó %d, está conectado à porta %d, Seu endereço é %s\n\n", id, port, adress);
  printf("Seus vizinhos são:\n");
  for(i = 0; i < neigh_qtty; i++)
    printf("O roteador %d, com enlace de custo %d, na porta %d, e endereço %s\n", neigh_list[i],
    infoVizinhos[neigh_list[i]].custo, infoVizinhos[neigh_list[i]].porta, infoVizinhos[neigh_list[i]].ip);  
    //neigh_info[neigh_list[i]].cost, neigh_info[neigh_list[i]].port, neigh_info[neigh_list[i]].adress);
  printf("\n");

  printf("Essa é sua tabela de roteamento, atualmente:\n");
  print_rout_table(tabelaRoteamento, NULL, 0);
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
                      int neigh_list[NROUT],
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
    pck->destino = neigh_list[i];
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

//Funcao que imprime tudo que está numa fila
void print_pack_queue(filaPacotes_t *filaPacotes){
  int i, j;
  pacote_t *pck;

  pthread_mutex_lock(&(filaPacotes->mutex));
  printf("%d Pacotes nessa fila:\n", filaPacotes->fim - filaPacotes->inicio);
  for(i = filaPacotes->inicio; i < filaPacotes->fim; i++){
    pck = &(filaPacotes->filaPacotes[i]);
    printf("\nPacote %s de controle, Destino: %d, Origem: %d\n", pck->controle ? "é" : "não é", pck->destino, pck->origem);
    if (pck->controle){
      printf("Vetor de distância do pacote: ");
      for(j = 0; j < NROUT; j++){
        printf("(%d,%d), ", pck->vetorDistancia[j].distancia, pck->vetorDistancia[j].proxSalto);
      }
      printf("\n");
    }
    else printf("Mensagem do pacote: %s\n", pck->mensagem);
  }
  printf("\n");
  pthread_mutex_unlock(&(filaPacotes->mutex));
}

//Funcao que imprime a tabela de roteamento
void print_rout_table(distSalto_t tabelaRoteamento[NROUT][NROUT], FILE *file, int infile){
  int i, j;

  if (infile) fprintf(file, "   ");
  else printf("   ");
  for(i = 0; i < NROUT; i++){
    if (infile) fprintf(file, "  %d %s|",i, i > 0 ? " " : "");
    else printf("  %d %s|",i, i > 0 ? " " : "");
  }
  if (file) fprintf(file, "\n");
  else printf("\n");
  for(i = 0; i < NROUT; i++){
    if (file) fprintf(file, "%d |", i);
    else printf("%d |", i);
    for(j = 0; j < NROUT; j++){
      if (infile){
        if (tabelaRoteamento[i][j].distancia != INF) fprintf(file, "%d(%d)| ", tabelaRoteamento[i][j].distancia, tabelaRoteamento[i][j].proxSalto);
        else fprintf(file, "I(X)| ");
      }
      else{
        if (tabelaRoteamento[i][j].distancia != INF) printf("%d(%d)| ", tabelaRoteamento[i][j].distancia, tabelaRoteamento[i][j].proxSalto);
        else printf("I(X)| ");
      }
    }
    if (file) fprintf(file, "\n");
    else printf("\n");
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
