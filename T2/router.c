#include "routing.h"

roteador_t roteador[NROUT];
informacoesRoteador_t infoRoteador;
roteadorVizinho_t infoVizinhos[NROUT];










int neigh_list[NROUT]; //Lista de vizinhos do roteador
int slen = sizeof(si_me); //Tamanho do endereço
dist_t routing_table[NROUT][NROUT]; //Tabela de roteamento do nó
pack_queue_t in, out; //Filas de entrada e saida de pacotes
pthread_mutex_t log_mutex, messages_mutex, news_mutex;
FILE *logs, *messages;


int main(int argc, char *argv[]){
  int menu_option = -1;
  int dest;
  char message[MAX_MESSAGE];
  char log_path[20] = "./logs/log";
  char message_path[20] = "./messages/message";
  package_t pck;

  // Certifica-se que os argumentos são válidos ao iniciar o programa
  if (argc < 2)
    die("Argumentos insuficientes, informe o ID do roteador a ser instanciado");
  if (argc > 2) {
    die("Argumentos demais, informe apenas o ID do roteador a ser instanciado");
  }
  infoRoteador.id = toint(argv[1]);
  if (infoRoteador.id < 0 || infoRoteador.id >= NROUT) {
    die("Não existe um roteador com este id, confira o arquivo de configuração");
  }

  //Adiciona id do roteador ao caminho dos arquivos de mensagem e log
  strcat(log_path, argv[1]);
  strcat(message_path, argv[1]);

  //Cria o arquivo para armazenar os logs
  if (!(logs = fopen(log_path, "w+"))) die("Falha ao criar arquivo de log");

  //Cria o arquivo para armazenar as mensagens
  if (!(messages = fopen(message_path, "w+"))) die("Falha ao criar arquivo de mensagens");

  //Rotina de inicializacao do roteador
  inicializar(&infoRoteador, infoVizinhos, routing_table, neigh_list, roteador, &in, &out, &log_mutex, &messages_mutex, &news_mutex, &si_me);

  if((infoRoteador.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
    die("Não foi possível criar o Socket!");
  }
  memset((char*) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(roteador[infoRoteador.id].porta);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(infoRoteador.sock, (struct sockaddr*) &si_me, sizeof(si_me)) == -1)
    die("Não foi possível conectar o socket com a porta\n");

  pthread_create(&sender_id, NULL, sender, NULL); //Cria thread emitidora
  pthread_create(&receiver_id, NULL, receiver, NULL); //Cria thread receptora
  pthread_create(&unpacker_id, NULL, unpacker, NULL); //Cria thread desempacotadora
  pthread_create(&refresher_id, NULL, refresher, NULL); //Cria thread atualizadora
  //pthread_create(&pulse_checker_id, NULL, pulse_checker, NULL); //Cria thread checadora de vivicidade

  while(1){
    system("clear");
    if (menu_option <= -1){
      if (menu_option == -2) { printf("Opção inválida\n\n"); }

      printf("ROTEADOR %d\n", infoRoteador.id);
      printf("------------------------------------------------------\n");
      printf("? - Atualizar\n");
      printf("1 - Informações sobre o roteador\n");
      printf("2 - Log\n");
      printf("3 - Ler Mensagens\n");
      printf("4 - Escrever Mensagem\n");
      printf("5 - Sair\n\n");
      scanf("%d", &menu_option);
      continue;
    }

    if (menu_option == 1){
      info(infoRoteador.id, infoRoteador.porta, infoRoteador.ip, infoRoteador.qtdVizinhos, neigh_list, infoVizinhos, routing_table);
      menu_option = back_option_menu(1);
      continue;
    }

    if (menu_option == 2){
      printf("----------------------LOG----------------------\n");
      print_file(logs, &log_mutex);
      printf("----------------------END----------------------\n");
      menu_option = back_option_menu(2);
      continue;
    }

    if (menu_option == 3){
      printf("-------------------MESSAGES--------------------\n");
      print_file(messages, &messages_mutex);
      printf("---------------------END-----------------------\n");
      menu_option = back_option_menu(3);
      continue;
    }

    if (menu_option == 4){
      printf("Insira o roteador de destino: ");
      scanf("%d", &dest);
      getchar();
      printf("Insira uma mensagem de no máximo %d caracteres:\n", MAX_MESSAGE);
      fgets(message, MAX_MESSAGE, stdin);
      pck.dest = dest; pck.control = 0;
      pck.orig = infoRoteador.id;
      strcpy(pck.message, message);
      pthread_mutex_lock(&out.mutex);
      copy_package(&pck, &out.queue[out.end++]); //Enfilera o novo pacote na fila de saida
      pthread_mutex_unlock(&out.mutex);

      printf("Mensagem encaminhada!\n");
      sleep(3);
      menu_option = -1;
      continue;
    }

    if (menu_option == 5){
      system("clear");
      break;
      continue;
    }
    menu_option = -2;
  }

  //Fecha sockets e arquivos
  close(infoRoteador.sock);
  fclose(logs);
  fclose(messages);

  return 0;
}

// Função auxiliar do menu. Retorna -1 caso o usuário deseje voltar ou +fallback_option+ caso o usuário queira atualizar
int back_option_menu(int fallback_option) {
  int option;
  printf("\nInsira 0 para voltar, 1 para atualizar\n");
  scanf("%d", &option);
  return !option ? -1 : fallback_option;
}

void* sender(void *nothing){
  int new_dest;
  package_t *pck;

  pthread_mutex_lock(&log_mutex);
  fprintf(logs, "[SENDER] Enviador iniciado!\n");
  pthread_mutex_unlock(&log_mutex);
  while(1){
    pthread_mutex_lock(&out.mutex);
    while(out.begin != out.end){
      pck = &(out.queue[out.begin]); //Aponta o pacote a ser enviado

      //TO CHECK
      new_dest = routing_table[infoRoteador.id][pck->dest].nhop; //Pega o próximo destino (next hop)

      si_me.sin_port = htons(infoVizinhos[new_dest].porta); //Atribui a porta do pacote a ser enviado
      if (inet_aton(infoVizinhos[new_dest].ip , &si_me.sin_addr) == 0 && !CLEAR_LOG){
        pthread_mutex_lock(&log_mutex);
        fprintf(logs, "[SENDER] Falha ao obter endereco do destinatario\n");
        pthread_mutex_unlock(&log_mutex);
      }
      else{
        //Envia para o socket requisitado(socket, dados, tamanho dos dados, flags, endereço, tamanho do endereço)
        if (sendto(infoRoteador.sock, pck, sizeof(*pck), 0, (struct sockaddr*) &si_me, slen) == -1){
          pthread_mutex_lock(&log_mutex);
          fprintf(logs, "[SENDER] Falha ao enviar Pacote de %s ao nó %d\n"
                  ,out.queue[out.begin].control ? "controle" : "dados", out.queue[out.begin].dest);
          pthread_mutex_unlock(&log_mutex);
        }
        else{
          if (!CLEAR_LOG || (CLEAR_LOG && !pck->control)){
            pthread_mutex_lock(&log_mutex);
            fprintf(logs, "[SENDER] Pacote de %s enviado com sucesso para o nó %d\n"
                    ,out.queue[out.begin].control ? "controle" : "dados", out.queue[out.begin].dest);
            pthread_mutex_unlock(&log_mutex);
          }
        }
      }
      out.begin++;
    }
    pthread_mutex_unlock(&out.mutex);
  }
}

void* unpacker(void *nothing){
  int i, retransmit, changed;
  package_t *pck;

  pthread_mutex_lock(&log_mutex);
  fprintf(logs, "[UNPACKER] Desempacotador iniciado!\n");
  pthread_mutex_unlock(&log_mutex);

  while(1){
    pthread_mutex_lock(&in.mutex);
    while(in.begin != in.end){
      pck = &in.queue[in.begin];
      if (pck->dest == infoRoteador.id){ //Se o pacote é pra mim
        if (!CLEAR_LOG || (CLEAR_LOG && !pck->control)){
          pthread_mutex_lock(&log_mutex);
          fprintf(logs, "[UNPACKER] Processando pacote de %s vindo de %d\n",
            pck->control ? "controle" : "mensagem", pck->orig);
          pthread_mutex_unlock(&log_mutex);
        }
        if (pck->control){
          //Marca que ouviu falar dele
          pthread_mutex_lock(&news_mutex);
          infoVizinhos[pck->orig].novidade = 1;
          pthread_mutex_unlock(&news_mutex);
          for(i = retransmit = changed = 0; i < NROUT; i++){
            //Se o vetor de distancias que o no enviou eh diferente do o no possui, atualiza
            if (routing_table[pck->orig][i].dist != pck->dist_vector[i].dist ||
               routing_table[pck->orig][i].nhop != pck->dist_vector[i].nhop){
              routing_table[pck->orig][i].dist = pck->dist_vector[i].dist;
              routing_table[pck->orig][i].nhop = pck->dist_vector[i].nhop;
              changed = 1;
              //Se a distancia ate o destino, mais o custo ate o no for maior o que ja tem, relaxa
              if (pck->dist_vector[i].dist + infoVizinhos[pck->orig].custo < routing_table[infoRoteador.id][i].dist){
                routing_table[infoRoteador.id][i].dist = pck->dist_vector[i].dist + infoVizinhos[pck->orig].custo;
                routing_table[infoRoteador.id][i].nhop = pck->orig;
                retransmit = 1;
              }
            }
          }
          if (changed){
            pthread_mutex_lock(&log_mutex);
            fprintf(logs, "[UNPACKER] A tabela foi atualizada:\n");
            print_rout_table(routing_table, logs, 1);
            pthread_mutex_unlock(&log_mutex);
          }
          if (retransmit){
            pthread_mutex_lock(&log_mutex);
            fprintf(logs, "[UNPACKER] O vetor de distancias mudou, enfileirando atualização pros vizinhos.\n");
            queue_dist_vec(&out, neigh_list, routing_table, infoRoteador.id, infoRoteador.qtdVizinhos);
            pthread_mutex_unlock(&log_mutex);
          }
        }
        else{ //Se é uma mensagem
          pthread_mutex_lock(&messages_mutex);
          fprintf(messages, "Roteador %d: ", pck->orig);
          fprintf(messages, "%s", pck->message);
          pthread_mutex_unlock(&messages_mutex);
        }
      }
      else{ //Se não é pra mim
        if (!CLEAR_LOG || (CLEAR_LOG && !pck->control)){
          pthread_mutex_lock(&log_mutex);
          fprintf(logs, "[UNPACKER] Roteando pacote com origem %d, para %d, via %d\n", pck->orig, pck->dest, routing_table[infoRoteador.id][pck->dest].nhop);
          pthread_mutex_unlock(&log_mutex);
        }
        pthread_mutex_lock(&out.mutex);
        copy_package(pck, &out.queue[out.end++]); //Enfilero ele na fila de saida
        pthread_mutex_unlock(&out.mutex);
      }
      in.begin++;
    }
    pthread_mutex_unlock(&in.mutex);
  }
}

void* receiver(void *nothing){
  package_t received;

  pthread_mutex_lock(&log_mutex);
  fprintf(logs, "[RECEIVER] Receptor iniciado!\n");
  pthread_mutex_unlock(&log_mutex);

  while(1){
    if ((recvfrom(infoRoteador.sock, &received, sizeof(received), 0, (struct sockaddr *) &si_me,
      (socklen_t * restrict ) &slen)) == -1) {
      printf("[RECEIVER] Erro ao receber pacote\n");
      continue;
    }

    if (!CLEAR_LOG || (CLEAR_LOG && !received.control)){
      pthread_mutex_lock(&log_mutex);
      fprintf(logs, "[RECEIVER] Pacote recebido de %d\n", received.orig);
      pthread_mutex_unlock(&log_mutex);
    }

    pthread_mutex_lock(&in.mutex);
    copy_package(&received, &in.queue[in.end++]); //Coloca o pacote no final da fila de recebidos
    pthread_mutex_unlock(&in.mutex);
    //print_pack_queue(&in);
  }
}

void *refresher(void *nothing){
  while(1){
    if (!CLEAR_LOG) {
      fprintf(logs, "[REFRESHER] Enfileirando atualizações de vetor de distância\n");
    }
    queue_dist_vec(&out, neigh_list, routing_table, infoRoteador.id, infoRoteador.qtdVizinhos);
    sleep(REFRESH_TIME);
  }
}

void* pulse_checker(void *nothing){
  int i, j, neigh, recalculate, through, new_min;

  while(1){
    sleep(TOLERANCY);

    if (!CLEAR_LOG){
      pthread_mutex_lock(&log_mutex);
      fprintf(logs, "[PULSE_CHECKER] Checando...\n");
      pthread_mutex_unlock(&log_mutex);
    }

    pthread_mutex_lock(&news_mutex);
    for(i = 0; i < infoRoteador.qtdVizinhos; i++){
      recalculate = 0;
      neigh = neigh_list[i];
      if (infoVizinhos[neigh].novidade){
        infoVizinhos[neigh].novidade = 0;
        if (infoVizinhos[neigh].custo == INF){
          pthread_mutex_lock(&log_mutex);
          fprintf(logs, "[PULSE_CHECKER] Parece que o nó %d Ressucitou!\n", infoVizinhos[neigh].id);
          pthread_mutex_unlock(&log_mutex);
          routing_table[infoRoteador.id][neigh].dist = infoVizinhos[neigh].custoOriginal;
          routing_table[infoRoteador.id][neigh].nhop = neigh;
          infoVizinhos[neigh].custo = infoVizinhos[neigh].custoOriginal;
          recalculate = 1;
        }
      }
      else if (infoVizinhos[neigh].custo != INF){
        pthread_mutex_lock(&log_mutex);
        fprintf(logs, "[PULSE_CHECKER] Parece que o nó %d morreu!\n", infoVizinhos[neigh].id);
        pthread_mutex_unlock(&log_mutex);
        routing_table[infoRoteador.id][neigh].dist = INF;
        routing_table[infoRoteador.id][neigh].nhop = -1;
        infoVizinhos[neigh].custo = INF;
        recalculate = 1;
      }
      if (recalculate){
        fprintf(logs, "[PULSE_CHECKER] Calculando novo vetor de distancia...\n");
        for(i = 0; i < NROUT; i++){
          if (routing_table[infoRoteador.id][i].nhop == neigh){
            new_min = INF; through = -1;
            for(j = 0; j < NROUT; j++){
              if (routing_table[j][i].dist + infoVizinhos[j].custo < new_min){ //JI?
                new_min = routing_table[j][i].dist + infoVizinhos[j].custo;
                through = j;
              }
            }
            routing_table[infoRoteador.id][i].dist = new_min;
            routing_table[infoRoteador.id][i].nhop = through;
          }
        }
      }
    }
    pthread_mutex_unlock(&news_mutex);
    if (recalculate) {
      queue_dist_vec(&out, neigh_list, routing_table, infoRoteador.id, infoRoteador.qtdVizinhos);
    }
  }
}
