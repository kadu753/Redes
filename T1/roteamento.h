#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define nRout 7
#define start 0
#define INF 12345678

typedef struct{
	int salto, distancia;
}pair;


void die(char* msg);
void pegarEnlace(int id, pair tabelaRoteamento[]);
void djikstra(int id, pair tabelaRoteamento[], int matriz[][nRout]);
void printRoteamento(int id, pair tabelaRoteamento[]);
int backtracking(int inicial, int atual, int caminho[]);