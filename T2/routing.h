#ifndef ROUTING_H
#define ROUTING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define REFRESH_TIME 2//Tempo entre os envios periodicos de vetor de distancia aos vizinhos
#define MAX_QUEUE 1123456 //Tamanho máximo da fila =~ 1123456
#define MAX_MESSAGE 200 //Tamanho maximo da mensagem
#define TAM_IP 30 //Tamanho máximo de um endereço
#define NROUT 4 //Numero de Roteadores
#define INF 112345678 //Infinito =~ 10^8

struct sockaddr_in socketRoteador;

pthread_t threadEnviar;
pthread_t threadReceber;
pthread_t threadDesempacotar;
pthread_t refresher_id;
pthread_t pulse_checker_id;
pthread_mutex_t log_mutex;
pthread_mutex_t messages_mutex;
pthread_mutex_t news_mutex;

FILE *logs, *messages;

//Estrutura de vizinho
typedef struct{
  int id;
  int custoOriginal;
  int custo;
  int porta;
  int novidade;
  char ip[TAM_IP];
} roteadorVizinho_t;

//Estrutura de distância, para o vetor de distâncias
typedef struct{
  int distancia;
  int proxSalto;
}distSalto_t;

typedef struct{
  int id;
  int porta;
  int sock;
  int qtdVizinhos;
  char ip[TAM_IP];
}informacoesRoteador_t;

//Estrutura de pacote
typedef struct{
  int controle;
  int destino;
  int origem;
  char mensagem[MAX_MESSAGE];
  distSalto_t vetorDistancia[NROUT];
}pacote_t;

//Fila de pacotes
typedef struct{
  int inicio;
  int fim;
  pacote_t filaPacotes[MAX_QUEUE];
  pthread_mutex_t mutex;
}filaPacotes_t;

void menu(informacoesRoteador_t infoRoteador);
void enviarMensagem(informacoesRoteador_t infoRoteador);
void die(char* msg);
int toint(char *str);
void inicializar(informacoesRoteador_t *infoRoteador,
                roteadorVizinho_t infoVizinhos[NROUT],
                distSalto_t tabelaRoteamento[NROUT][NROUT],
                int listaVizinhos[NROUT],
                filaPacotes_t *entrada,
                filaPacotes_t *saida,
                pthread_mutex_t *log_mutex,
                pthread_mutex_t *messages_mutex,
                pthread_mutex_t *news_mutex);
          
void configurarRoteadores(informacoesRoteador_t *infoRoteador,
                          roteadorVizinho_t infoVizinhos[NROUT]);

void configurarEnlace(informacoesRoteador_t *infoRoteador,
                      int listaVizinhos[NROUT],
                      roteadorVizinho_t infoVizinhos[NROUT]);

void info(int id, int port, char adress[TAM_IP], int neigh_qtty, int listaVizinhos[NROUT],
                roteadorVizinho_t infoVizinhos[NROUT], distSalto_t tabelaRoteamento[NROUT][NROUT]);
void copy_package(pacote_t *a, pacote_t *b);
void queue_dist_vec(filaPacotes_t *saida, int listaVizinhos[NROUT], distSalto_t tabelaRoteamento[NROUT][NROUT],
                    int id, int neigh_qtty);
void print_pack_queue(filaPacotes_t *filaPacotes);
void print_rout_table(distSalto_t tabelaRoteamento[NROUT][NROUT], FILE *file, int infile);
void print_file(FILE *file, pthread_mutex_t *mutex);
void* enviar(void *nothing); //Thread responsavel por enviar pacotes
void* desempacotador(void *nothing);
void* receber(void *nothing);
void* refresher(void *nothing); //Thread responsavel por enfileirar periodicamente pacotes de distancia para todos os vizinhos
int back_option_menu(int fallback_option); // Função auxiliar do menu. Retorna -1 caso o usuário deseje voltar ou +fallback_option+ caso o usuário queira atualizar
void printRoteamento(distSalto_t tabelaRoteamento[NROUT][NROUT], FILE *arquivo);

#endif