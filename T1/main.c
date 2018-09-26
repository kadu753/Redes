#include "roteamento.h"

pair tabelaRoteamento[nRout];

int main(){
	pegarEnlace(2, tabelaRoteamento);
	printRoteamento(2, tabelaRoteamento);
	return 0;
}