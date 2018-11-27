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
#define TAM_FILA 1123456 //Tamanho máximo da fila =~ 1123456
#define TAM_MENSAGEM 100 //Tamanho maximo da mensagem
#define TAM_IP 30 //Tamanho máximo de um endereço
#define N_ROTEADORES 4 //Numero de Roteadores
#define INF 12345678

struct sockaddr_in socketRoteador;

pthread_t threadEnviar;
pthread_t threadReceber;
pthread_t threadDesempacotar;
pthread_t threadInformacao;
pthread_mutex_t logMutex;
pthread_mutex_t mensagemMutex;
pthread_mutex_t novidadeMutex;

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
  char mensagem[TAM_MENSAGEM];
  distSalto_t vetorDistancia[N_ROTEADORES];
}pacote_t;

//Fila de pacotes
typedef struct{
  int inicio;
  int fim;
  pacote_t filaPacotes[TAM_FILA];
  pthread_mutex_t mutex;
}filaPacotes_t;

void menu(informacoesRoteador_t infoRoteador);
void enviarMensagem(informacoesRoteador_t infoRoteador);
void die(char* msg);
int toint(char *str);
void inicializar(informacoesRoteador_t *infoRoteador,
                roteadorVizinho_t infoVizinhos[N_ROTEADORES],
                distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES],
                int listaVizinhos[N_ROTEADORES],
                filaPacotes_t *entrada,
                filaPacotes_t *saida,
                pthread_mutex_t *logMutex,
                pthread_mutex_t *mensagemMutex,
                pthread_mutex_t *novidadeMutex);
          
void configurarRoteadores(informacoesRoteador_t *infoRoteador,
                          roteadorVizinho_t infoVizinhos[N_ROTEADORES]);

void configurarEnlace(informacoesRoteador_t *infoRoteador,
                      int listaVizinhos[N_ROTEADORES],
                      roteadorVizinho_t infoVizinhos[N_ROTEADORES]);

void enfileirarPacote(filaPacotes_t *saida, distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES], 
                      informacoesRoteador_t infoRoteador, int listaVizinhos[N_ROTEADORES]);

void printarArquivo(FILE *arquivo, pthread_mutex_t *mutex);
void* enviar(void *nothing);
void* desempacotador(void *nothing);
void* receber(void *nothing);
void* trocarInformacao(void *nothing);
void timeStamp();
void duplicarPacote(pacote_t *original, pacote_t *copia);
void printRoteamento(distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES], FILE *arquivo);

#endif