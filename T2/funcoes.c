#include "roteamento.h"

void inicializar(RoteadorVizinho infoVizinhos, DistSalto tabelaRoteamento[N_ROUT][N_ROUT], int listaVizinhos[N_ROUT], Roteador roteador){
	int i, j;

	for(i = 0; i < N_ROUT; i++){
		infoVizinhos[i].novidade = 0;
		infoVizinhos[i].id = infoVizinhos[i].porta = -1;
		infoVizinhos[i].custo = INF;
		for(j = 0; j < N_ROUT; j++){
			tabelaRoteamento[i][j].distancia = INF;
			tabelaRoteamento[i][j].proxSalto = -1;
		}
	}

	configurarEnlace(listaVizinhos, infoVizinhos, roteador);
	configurarRoteadores();
}

void configurarEnlace(int listaVizinhos[N_ROUT], RoteadorVizinho infoVizinhos, Roteador roteador){
	int r1, r2, dist;

	FILE *arquivo = fopen("enlaces.config", "r");
	if(!arquivo) die("Não foi possível abrir o arquivo de enlaces\n")
	while(fscanf(arquivo, "%d %d %d\n", &r1, &r2, &dist) != EOF){
		if (r2 == id){
			r2 = r1;
			r1 = id;
		}
		if(r1 == id){
			listaVizinhos[roteador.qtdVizinhos++] = r2;
			infoVizinhos[r2].id = r2;
			infoVizinhos[r2].custo = infoVizinhos[r2].custoOriginal = dist;
		}
	}
}

void configurarRoteadores()



void configurarRoteadores(int id, roteador roteadores[]){
	FILE *arquivo = fopen("roteador.config", "r");
	if(!arquivo) die("Não foi possível abrir o arquivo de roteadores\n");
	for(int i = 0; fscanf(arquivo, "%d %d %s\n", &roteadores[i].id, &roteadores[i].porta, roteadores[i].ip) != EOF; i++);
	fclose(arquivo);
}


void djikstra(int id, pair tabelaRoteamento[], int matriz[][NROUT]){
	int i, conhecido[NROUT], aux = 0, custo, atual = id, caminho[NROUT];
	memset(conhecido, 0, sizeof(conhecido));
	for(i = 0; i < NROUT; i++) tabelaRoteamento[i].distancia = INF; //Todas as distancias são setadas para infinito
	tabelaRoteamento[id].salto = id; // O próximo salto é ele mesmo
	tabelaRoteamento[id].distancia = 0; // A distancia para ele mesmo é 0
	conhecido[id] = 1;
	while(aux != NROUT){
		custo = INF;
		for(i = 0; i < NROUT; i++){
			if(conhecido[i] == 0 && matriz[atual][i] != 0){
				if(matriz[atual][i] + tabelaRoteamento[atual].distancia < tabelaRoteamento[i].distancia){
					tabelaRoteamento[i].distancia = matriz[atual][i] + tabelaRoteamento[atual].distancia;
					tabelaRoteamento[i].salto = atual;
					caminho[i] = atual;
				}
			}
		}
		for(i = 0; i < NROUT; i++){
			if(tabelaRoteamento[i].distancia < custo && conhecido[i] == 0){
				atual = i;
				custo = tabelaRoteamento[i].distancia;
			}
		}
		conhecido[atual] = 1;
		aux++;
	}
	for(i = 0; i < NROUT; i++){
		if(i != id){
			tabelaRoteamento[i].salto = backtracking(id, i, caminho);
		}
	}
}

int backtracking(int inicial, int atual, int caminho[]){
	if(caminho[atual] == inicial){
		return atual;
	}
	return backtracking(inicial, caminho[atual], caminho);
}

void die(char* msg){
    printf("%s\n", msg);
    exit(1);
}

int toInt(char *str){
	int i, pot, ans = 0;

    for(i = strlen(str)-1, pot = 1; i >= 0; i--, pot *= 10){
        ans += pot * (str[i] - '0');
    }
    return ans;
}

void printRoteamento(int id, pair tabelaRoteamento[]){
	system("clear");
	printf(" __________________________________________\n");
	printf("|%31s%12s\n", "TABELA DE ROTEAMENTO", "|");
	printf("|------------------------------------------|\n");
	printf("|--- Destino ---|--- Salto ---|--- Custo---|\n");
	for(int i = 0; i < NROUT; i++){
		printf("|%8d       |%7d      |%7d     |\n", i, tabelaRoteamento[i].salto, tabelaRoteamento[i].distancia);
	}
	printf("|------------------------------------------|\n");
	printf("Enter para voltar\n\n");
	getchar();
	getchar();
}

void menu(int id, int porta, char ip[], int novas){
	system("clear");
	printf(" ____________________________________________________________\n");
	printf("|%s%d%17s%d%12s%-15s|\n", "Roteador: ", id, "Porta: ", porta, "IP: ", ip);
	printf("|--------------------%d Mensagens Novas-----------------------|\n", novas);
	printf(" LOG: %s\n", LOG);
	printf("|------------------------------------------------------------|\n");
	printf("|%-60s|\n", "1 - Mandar mensagem");
	printf("|%-60s|\n", "2 - Ler Mensagens");
	printf("|%-60s|\n", "3 - Tabela de Roteamento");
	printf("|%-60s|\n", "4 - Atualizar");
	printf("|%-60s|\n", "0 - Sair");
	printf("|____________________________________________________________|\n\n");
}

void printarCaixaEntrada(int qtdMsg, pacotes caixaMensagens[]){
	system("clear");
	printf(" ____________________________________________________________\n");
	printf("|%38s%23s\n", "CAIXA DE ENTRADA", "|");
	printf("|------------------------------------------------------------|\n");
	if(!qtdMsg){
		printf("|%45s%18s\n", "Você não possui mensagens", "|");
	}else{
		for(int i = 0; i < qtdMsg; i++)
			printf(" Origem [%d]: %s\n", caixaMensagens[i].origem, caixaMensagens[i].mensagem);
	}
	printf("|------------------------------------------------------------|\n");
	printf("Enter para voltar\n\n");
	getchar();
	getchar();
}