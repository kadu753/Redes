#include "roteamento.h"

void pegarEnlace(int id, pair tabelaRoteamento[]){
	int matriz[nRout][nRout], r1, r2, dist;

	memset(matriz, 0, sizeof(matriz));
	FILE *arquivo = fopen("enlaces.config", "r");
	if(!arquivo) die("Não foi possível abrir o arquivo de enlaces\n");
	while(fscanf(arquivo, "%d %d %d\n", &r1, &r2, &dist) != EOF)
		matriz[r1-start][r2-start] = matriz[r2-start][r1-start] = dist; //Arestas são bidirencionais
	fclose(arquivo);

	djikstra(id, tabelaRoteamento, matriz);
}

void djikstra(int id, pair tabelaRoteamento[], int matriz[][nRout]){
	int i, conhecido[nRout], aux = 0, custo, atual = id, caminho[nRout];
	memset(conhecido, 0, sizeof(conhecido));
	for(i = 0; i < nRout; i++) tabelaRoteamento[i].distancia = INF; //Todas as distancias são setadas para 0
	tabelaRoteamento[id].salto = id; // O próximo salto é ele mesmo
	tabelaRoteamento[id].distancia = 0; // A distancia para ele mesmo é 0
	conhecido[id] = 1;

	while(aux != nRout){
		custo = INF;
		for(i = 0; i < nRout; i++){
			if(conhecido[i] == 0 && matriz[atual][i] != 0){
				if(matriz[atual][i] + tabelaRoteamento[atual].distancia < tabelaRoteamento[i].distancia){
					tabelaRoteamento[i].distancia = matriz[atual][i] + tabelaRoteamento[atual].distancia;
					tabelaRoteamento[i].salto = atual;
					caminho[i] = atual;
				}
			}
		}
		for(i = 0; i < nRout; i++){
			if(tabelaRoteamento[i].distancia < custo && conhecido[i] == 0){
				atual = i;
				custo = tabelaRoteamento[i].distancia;
			}
		}
		conhecido[atual] = 1;
		aux++;
	}
	for(i = 0; i < nRout; i++){
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

void printRoteamento(int id, pair tabelaRoteamento[]){
	for(int i = 0; i < nRout; i++){
		printf("\t%c\t%c\t%d\n", i+65, tabelaRoteamento[i].salto+65, tabelaRoteamento[i].distancia);
	}
}