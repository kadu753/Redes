#include "roteamento.h"

void pegarEnlace(int id, pair tabelaRoteamento[]){
	int matriz[NROUT][NROUT], r1, r2, dist;

	memset(matriz, 0, sizeof(matriz));
	FILE *arquivo = fopen("enlaces.config", "r");
	if(!arquivo) die("Não foi possível abrir o arquivo de enlaces\n");
	while(fscanf(arquivo, "%d %d %d\n", &r1, &r2, &dist) != EOF)
		matriz[r1-START][r2-START] = matriz[r2-START][r1-START] = dist; //Arestas são bidirencionais
	fclose(arquivo);

	djikstra(id, tabelaRoteamento, matriz);
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

void configurarRoteadores(int id, int *sock, struct sockaddr_in *socketRoteador, roteador roteadores[]){
	FILE *arquivo = fopen("roteador.config", "r");
	if(!arquivo) die("Não foi possível abrir o arquivo de roteadores\n");
	for(int i = 0; fscanf(arquivo, "%d %d %s\n", &roteadores[i].id, &roteadores[i].porta, roteadores[i].ip) != EOF; i++);
	fclose(arquivo);

	//Criação do socket
	if((*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		die("Não foi possível criar o Socket!");
	}else{
		socketRoteador->sin_family = AF_INET;
		socketRoteador->sin_port = htons(roteadores[id].porta);	//Atribuir porta
		socketRoteador->sin_addr.s_addr = htonl(INADDR_ANY);		//Atribui o socket
	}

	//Liga o socket a porta
	if(bind(*sock, (struct sockaddr*) socketRoteador, sizeof(*socketRoteador)) == -1)
		die("Não foi possível conectar o socket com a porta\n");
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
	for(int i = 0; i < NROUT; i++){
		printf("\t%c\t%c\t%d\n", i+ASCII, tabelaRoteamento[i].salto+ASCII, tabelaRoteamento[i].distancia);
	}
}

int toInt(char *str){
	int i, pot, ans = 0;

    for(i = strlen(str)-1, pot = 1; i >= 0; i--, pot *= 10){
        ans += pot * (str[i] - '0');
    }
    return ans;
}