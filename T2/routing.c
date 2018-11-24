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
void inicializar(Tipo infoRoteador,
                neighbour_t neigh_info[NROUT],
                dist_t routing_table[NROUT][NROUT],
                int neigh_list[NROUT],
                Roteador roteador[],
                pack_queue_t *in,
                pack_queue_t *out,
                pthread_mutex_t *log_mutex,
                pthread_mutex_t *messages_mutex,
                pthread_mutex_t *news_mutex,
                struct sockaddr_in *si_me){
  int i, j, auxLista;
  int id = infoRoteador.id;

  printf("%d\n", id);
  //Inicializa o vetor de informações de vizinhos e a tabela de roteamento
  for(i = 0; i < NROUT; i++){
    neigh_info[i].news = 0;
    neigh_info[i].id = neigh_info[i].port = -1;
    neigh_info[i].cost = INF;
    for(j = 0; j < NROUT; j++){
      routing_table[i][j].dist = INF;
      routing_table[i][j].nhop = -1;
    }
  }

  configurarEnlace(id, neigh_list, roteador, neigh_info);
  configurarRoteadores(id, roteador, neigh_info);

  //Custo de um nó para ele mesmo é 0, via ele mesmo
  routing_table[id][id].dist = 0;
  routing_table[id][id].nhop = id;

  //Preenche o vetor de distâncias inicial do nó
  for(i = 0; i < roteador[id].qtdVizinhos; i++){
    auxLista = neigh_list[i];
    routing_table[id][auxLista].dist = neigh_info[auxLista].cost;
    routing_table[id][auxLista].nhop = neigh_info[auxLista].id;
  }
  
  //Inicializa as filas de entrada e saida
  in->begin = out->begin = in->end = out->end = 0;
  pthread_mutex_init(&(in->mutex), NULL);
  pthread_mutex_init(&(out->mutex), NULL);

  //Inicializa mutex dos arquivos de log e mensagens e de checagem de queda de nó
  pthread_mutex_init(log_mutex, NULL);
  pthread_mutex_init(messages_mutex, NULL);
  pthread_mutex_init(news_mutex, NULL);
}

void configurarEnlace(int id, 
                      int neigh_list[NROUT], 
                      Roteador roteador[], 
                      neighbour_t neigh_info[NROUT]){
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
      neigh_info[r2].id = r2;
      neigh_info[r2].cost = neigh_info[r2].orig_cost = dist;
    }
  }
  fclose(arquivo);
}

void configurarRoteadores(int id, 
                          Roteador roteador[],
                          neighbour_t neigh_info[NROUT]){
  int novoID, novaPorta;
  char novoIP[TAM_IP];
  FILE *arquivo = fopen("roteador.config", "r");
  if(!arquivo) die("Não foi possível abrir o arquivo de roteadores\n");
  while(fscanf(arquivo, "%d %d %s\n", &novoID, &novaPorta, novoIP) != EOF){
    if(novoID == id){
      infoRoteador.porta = novaPorta;
      //roteador[id].porta = novaPorta;
      strcpy(roteador[id].ip, novoIP);
    }
    if(neigh_info[novoID].id != -1){
      neigh_info[novoID].port = novaPorta;
      strcpy(neigh_info[novoID].adress, novoIP);
    }
  }
  fclose(arquivo);
}

//Imprime informações sobre o roteador
void info(int id, int port, char adress[TAM_IP], int neigh_qtty, int neigh_list[NROUT],
    neighbour_t neigh_info[NROUT], dist_t routing_table[NROUT][NROUT]){
  int i;

  printf("O nó %d, está conectado à porta %d, Seu endereço é %s\n\n", id, port, adress);
  printf("Seus vizinhos são:\n");
  for(i = 0; i < neigh_qtty; i++)
    printf("O roteador %d, com enlace de custo %d, na porta %d, e endereço %s\n", neigh_list[i],
      neigh_info[neigh_list[i]].cost, neigh_info[neigh_list[i]].port, neigh_info[neigh_list[i]].adress);
  printf("\n");

  printf("Essa é sua tabela de roteamento, atualmente:\n");
  print_rout_table(routing_table, NULL, 0);
}

//Copia o pacote a para o pacote b
void copy_package(package_t *a, package_t *b){
  int i;

  b->control = a->control;
  b->dest = a->dest;
  b->orig = a->orig;
  strcpy(b->message, a->message);
  for(i = 0; i < NROUT; i++){
    b->dist_vector[i].dist = a->dist_vector[i].dist;
    b->dist_vector[i].nhop = a->dist_vector[i].nhop;
  }
}

//Enfilera um pacote para cada vizinho do nó, contendo seu vetor de distancia
void queue_dist_vec(
                      pack_queue_t *out,
                      int neigh_list[NROUT],
                      dist_t routing_table[NROUT][NROUT],
                      int id,
                      int neigh_qtty
                    ) {
  int i, j;

  pthread_mutex_lock(&(out->mutex));
  for(i = 0; i < neigh_qtty; i++, out->end++){
    package_t *pck = &(out->queue[out->end]);
    pck->control = 1;
    pck->orig = id;
    pck->dest = neigh_list[i];
    //printf("Enfileirando pacote de vetor de distancia para o destino %d\n", pck->dist);
    //printf("Vetor de distancia enviado: ");
    for(j = 0; j < NROUT; j++){
      pck->dist_vector[j].dist = routing_table[id][j].dist;
      pck->dist_vector[j].nhop = routing_table[id][j].nhop;
      //printf("(%d,%d) ", pck->dist_vector[j].dist, pck->dist_vector[j].nhop);
    }
  }
  pthread_mutex_unlock(&(out->mutex));
}

//Funcao que imprime tudo que está numa fila
void print_pack_queue(pack_queue_t *queue){
  int i, j;
  package_t *pck;

  pthread_mutex_lock(&(queue->mutex));
  printf("%d Pacotes nessa fila:\n", queue->end - queue->begin);
  for(i = queue->begin; i < queue->end; i++){
    pck = &(queue->queue[i]);
    printf("\nPacote %s de controle, Destino: %d, Origem: %d\n", pck->control ? "é" : "não é", pck->dest, pck->orig);
    if (pck->control){
      printf("Vetor de distância do pacote: ");
      for(j = 0; j < NROUT; j++){
        printf("(%d,%d), ", pck->dist_vector[j].dist, pck->dist_vector[j].nhop);
      }
      printf("\n");
    }
    else printf("Mensagem do pacote: %s\n", pck->message);
  }
  printf("\n");
  pthread_mutex_unlock(&(queue->mutex));
}

//Funcao que imprime a tabela de roteamento
void print_rout_table(dist_t routing_table[NROUT][NROUT], FILE *file, int infile){
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
        if (routing_table[i][j].dist != INF) fprintf(file, "%d(%d)| ", routing_table[i][j].dist, routing_table[i][j].nhop);
        else fprintf(file, "I(X)| ");
      }
      else{
        if (routing_table[i][j].dist != INF) printf("%d(%d)| ", routing_table[i][j].dist, routing_table[i][j].nhop);
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
