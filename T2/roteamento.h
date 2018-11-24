#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>

#define N_ROUT 6
#define INF 12345678
#define TAM_MSG 100

struct sockaddr_in socketRoteador;

typedef struct{
	int proxSalto, distancia;
}DistSalto;

typedef struct{
	int id;
	int custoOriginal
	int custo;
	int porta;
	int novidade;
	char ip[20]; 
}RoteadorVizinho;

typedef struct{
	int id
	int porta;
	int qtdVizinhos;
	char ip[20]; 
}Roteador;

typedef struct{
	int id;						// ID Roteador
	int sock;					// Socket para enviar mensagem
	int qtdNova;				// Qtd de novas mensagens
	int qtdMsg;					// Qtd de mensagens recebidas
	int tamMsg;					// Tamanho da mensagem
}Tipo;

typedef struct{
	int origem;
	int destino;
	char mensagem[TAM_MSG];
	distSalto vetorDistancia[N_ROUT];
}Pacote;

void die(char* msg);
void pegarEnlace(int id, pair tabelaRoteamento[]);
void djikstra(int id, pair tabelaRoteamento[], int matriz[][NROUT]);
void printRoteamento(int id, pair tabelaRoteamento[]);
int backtracking(int inicial, int atual, int caminho[]);
void configurarRoteadores(int id, roteador roteadores[]);
int toInt(char *str);
void *receber(void *info);
void enviarMensagem(pacotes auxEnviar, int idRoteador, int sockRoteador, int tamMsg);
void transmitirPacote(pacotes auxEnviar, int sockRoteador, int tamMsg);
void menu(int id, int porta, char ip[], int novas);
void printarCaixaEntrada(int qtdMsg, pacotes caixaMensagens[]);