#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>

#define NROUT 6
#define START 1
#define INF 12345678
#define MSG 100
#define TAMCAIXA 10

char LOG[100];

typedef struct{
	int salto, distancia;
}pair;

struct sockaddr_in socketRoteador;

pthread_t receberID;							// Thread que recebe mensagens

typedef struct{
	int id, porta;
	char ip[20]; 
}roteador;

typedef struct{
	int id;						// ID Roteador
	int sock;					// Socket para enviar mensagem
	int qtdNova;				// Qtd de novas mensagens
	int qtdMsg;					// Qtd de mensagens recebidas
	int tamMsg;					// Tamanho da mensagem
}tipo;

typedef struct{
	int origem;
	int destino;
	char mensagem[MSG];
}pacotes;

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