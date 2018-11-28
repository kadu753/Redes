#include "roteador.h"

roteadorVizinho_t infoVizinhos[N_ROTEADORES];
distSalto_t tabelaRoteamento[N_ROTEADORES][N_ROTEADORES]; //Tabela de roteamento do nó
filaPacotes_t entrada, saida; //Filas de entrada e saida de pacotes
informacoesRoteador_t infoRoteador;
int listaVizinhos[N_ROTEADORES]; //Lista de vizinhos do roteador
int slen = sizeof(socketRoteador); //Tamanho do endereço

int main(int argc, char *argv[]){
	int opcoes = -1;
	char logCaminho[20] = "./logs/log";
	char mensagemCaminho[20] = "./messages/message";

	// Certifica-se que os argumentos são válidos ao iniciar o programa
	if (argc < 2)
		die("Argumentos insuficientes, informe o ID do roteador a ser instanciado");
	if (argc > 2) {
		die("Argumentos demais, informe apenas o ID do roteador a ser instanciado");
	}
	infoRoteador.id = toint(argv[1]);
	if (infoRoteador.id < 0 || infoRoteador.id >= N_ROTEADORES) {
		die("Não existe um roteador com este id, confira o arquivo de configuração");
	}

	//Adiciona id do roteador ao caminho dos arquivos de mensagem e log
	strcat(logCaminho, argv[1]);
	strcat(mensagemCaminho, argv[1]);

	//Cria o arquivo para armazenar os logs
	if (!(logs = fopen(logCaminho, "w+"))) die("Falha ao criar arquivo de log");

	//Cria o arquivo para armazenar as mensagens
	if (!(messages = fopen(mensagemCaminho, "w+"))) die("Falha ao criar arquivo de mensagens");

	//Rotina de inicializacao do roteador
	inicializar(&infoRoteador, infoVizinhos, tabelaRoteamento, listaVizinhos, &entrada, &saida, &logMutex, &mensagemMutex, &novidadeMutex);

	if((infoRoteador.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		die("Não foi possível criar o Socket!");
	}
	
	memset((char*) &socketRoteador, 0, sizeof(socketRoteador));
	socketRoteador.sin_family = AF_INET;
	socketRoteador.sin_port = htons(infoRoteador.porta);
	socketRoteador.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(infoRoteador.sock, (struct sockaddr*) &socketRoteador, sizeof(socketRoteador)) == -1)
		die("Não foi possível conectar o socket com a porta\n");

	pthread_create(&threadEnviar, NULL, enviar, NULL); //Cria thread emitidora
	pthread_create(&threadReceber, NULL, receber, NULL); //Cria thread receptora
	pthread_create(&threadDesempacotar, NULL, desempacotador, NULL); //Cria thread desempacotadora
	pthread_create(&threadInformacao, NULL, trocarInformacao, NULL); //Cria thread atualizadora
	pthread_create(&threadVivacidade, NULL, checarVivacidade, NULL);

	while(1){
		switch(opcoes){
			case -1:
				menu(infoRoteador);
				while(scanf("%d", &opcoes) != 1)
					getchar();
				break;
			case 1:
				enviarMensagem(infoRoteador);
				opcoes = -1;
				break;

			case 2:
				printf("-------------------MESSAGES--------------------\n");
				printarArquivo(messages, &mensagemMutex);
				printf("---------------------END-----------------------\n");
				getchar();
				getchar();
				opcoes = -1;
				break;

			case 3:
				printf("----------------------LOG----------------------\n");
				printarArquivo(logs, &logMutex);
				printf("----------------------END----------------------\n");
				getchar();
				getchar();
				opcoes = -1;
				break;

			case 4:
				printRoteamento(tabelaRoteamento, NULL);
				getchar();
				getchar();
				opcoes = -1;
				break;

			case 0:
				close(infoRoteador.sock);
				fclose(logs);
				fclose(messages);
				exit(1);
				break;

			default:
				system("clear");
				printf("Essa opção não existe\nRetornando para o menu\n");
				sleep(2);
				opcoes = -1;
			break;
		}
	}
	return 0;
}

void enviarMensagem(informacoesRoteador_t infoRoteador){
	system("clear");
	pacote_t auxPacote;
	printf("Insira o roteador destino\n");
	while(scanf("%d", &auxPacote.destino) != 1)			//Somente aceita um número inteiro
		getchar();
		system("clear");
		printf("Insira o roteador destino\n");
	while(1){
		getchar();
		if(auxPacote.destino < 0 || auxPacote.destino >= N_ROTEADORES){
			printf("Roteador inexistente\n");
		}else{
			system("clear");
			printf("Insira uma mensagem com no máximo %d caracteres\n", TAM_MENSAGEM);
			scanf("%[^\n]", auxPacote.mensagem);
			strcat(auxPacote.mensagem, "\n");
			if(strlen(auxPacote.mensagem) <= TAM_MENSAGEM){
				auxPacote.controle = 0;
				auxPacote.origem = infoRoteador.id;
				printf("Pacote enviado para saida do roteador\n");
				pthread_mutex_lock(&saida.mutex);
				duplicarPacote(&auxPacote, &saida.filaPacotes[saida.fim++]);
				pthread_mutex_unlock(&saida.mutex);
				sleep(2);
				break;
			}else{
				printf("Mensagem estourou o limite de caracteres\n");
				sleep(2);
				system("clear");
			}
		}
	}
}

void* enviar(void *info){
	int auxDestino;
	pacote_t *auxEnviar;

	pthread_mutex_lock(&logMutex);
	fprintf(logs, "Enviar foi iniciado\n");
	pthread_mutex_unlock(&logMutex);
	while(1){
		pthread_mutex_lock(&saida.mutex);
		while(saida.inicio != saida.fim){
			auxEnviar = &(saida.filaPacotes[saida.inicio]);
			auxDestino = tabelaRoteamento[infoRoteador.id][auxEnviar->destino].proxSalto;
			socketRoteador.sin_port = htons(infoVizinhos[auxDestino].porta);
			if(inet_aton(infoVizinhos[auxDestino].ip, &socketRoteador.sin_addr) == 0){
				pthread_mutex_lock(&logMutex);
				fprintf(logs, "Enviar - não foi possível obter o endereco do destinatario.\n");
				pthread_mutex_unlock(&logMutex);
			}else{
				if(sendto(infoRoteador.sock, auxEnviar, sizeof(*auxEnviar), 0, (struct sockaddr*) &socketRoteador, slen) == -1){
					printf("Falha ao enviar o seu pacote\n");
					pthread_mutex_lock(&logMutex);
					fprintf(logs, "Enviar - Falha no envio do pacote\n");
					pthread_mutex_unlock(&logMutex);
				}else{
					if(!auxEnviar->controle){
						pthread_mutex_lock(&logMutex);
						fprintf(logs, "Pacote enviado para #%d\n", auxEnviar->destino);
						pthread_mutex_unlock(&logMutex);
					}
				}
			}
			saida.inicio++;
		}
		pthread_mutex_unlock(&saida.mutex);
	}
}

void* desempacotador(void *info){

	int tabelaAtualizada = 0, vetorAtualizado = 0;
	pacote_t *auxDesempacotar;

	pthread_mutex_lock(&logMutex);
	fprintf(logs, "Desempacotador foi iniciado\n");
	pthread_mutex_unlock(&logMutex);

	while(1){
		pthread_mutex_lock(&entrada.mutex);
		while(entrada.inicio != entrada.fim){
			auxDesempacotar = &entrada.filaPacotes[entrada.inicio];
			if(auxDesempacotar->destino == infoRoteador.id){
				if(!auxDesempacotar->controle){
					pthread_mutex_lock(&logMutex);
					fprintf(logs, "Desempacotando pacote de %d\n", auxDesempacotar->origem);
					pthread_mutex_unlock(&logMutex);
					pthread_mutex_lock(&mensagemMutex);
					fprintf(messages, "Origem [%d]:  %s\n", auxDesempacotar->origem, auxDesempacotar->mensagem);
					pthread_mutex_unlock(&mensagemMutex);
				}else{
					tabelaAtualizada = 0;
					vetorAtualizado = 0;
					pthread_mutex_lock(&novidadeMutex);
					infoVizinhos[auxDesempacotar->origem].novidade = 1;
					pthread_mutex_unlock(&novidadeMutex);
					for(int i = 0; i < N_ROTEADORES; i++){
						if(tabelaRoteamento[auxDesempacotar->origem][i].distancia != auxDesempacotar->vetorDistancia[i].distancia 
							|| tabelaRoteamento[auxDesempacotar->origem][i].proxSalto != auxDesempacotar->vetorDistancia[i].proxSalto){
							tabelaRoteamento[auxDesempacotar->origem][i].distancia = auxDesempacotar->vetorDistancia[i].distancia;
							tabelaRoteamento[auxDesempacotar->origem][i].proxSalto = auxDesempacotar->vetorDistancia[i].proxSalto;
							tabelaAtualizada = 1;
						}
						if(auxDesempacotar->vetorDistancia[i].distancia + infoVizinhos[auxDesempacotar->origem].custo < tabelaRoteamento[infoRoteador.id][i].distancia){
							tabelaRoteamento[infoRoteador.id][i].distancia = auxDesempacotar->vetorDistancia[i].distancia + infoVizinhos[auxDesempacotar->origem].custo;
							tabelaRoteamento[infoRoteador.id][i].proxSalto = auxDesempacotar->origem;
							vetorAtualizado = 1;
						}
					}
					if(tabelaAtualizada){
						pthread_mutex_lock(&logMutex);
						printRoteamento(tabelaRoteamento, logs);
						pthread_mutex_unlock(&logMutex);
					}
					if(vetorAtualizado){
						pthread_mutex_lock(&logMutex);
			            fprintf(logs, "O vetor de distancia  mudou no desempacotador\n");
			            enfileirarPacote(&saida, tabelaRoteamento, infoRoteador, listaVizinhos);
			            pthread_mutex_unlock(&logMutex);
					}
				}
			}else{
				pthread_mutex_lock(&saida.mutex);
				duplicarPacote(auxDesempacotar, &saida.filaPacotes[saida.fim++]);
				pthread_mutex_unlock(&saida.mutex);
			}
			entrada.inicio++;
		}
		pthread_mutex_unlock(&entrada.mutex);
	}
}

void* receber(void *info){
	pacote_t auxReceber;

	pthread_mutex_lock(&logMutex);
	fprintf(logs, "Receber iniciado!\n");
	pthread_mutex_unlock(&logMutex);

	while(1){
		if ((recvfrom(infoRoteador.sock, &auxReceber, sizeof(auxReceber), 0, (struct sockaddr *) &socketRoteador,
			(socklen_t * restrict ) &slen)) == -1) {
			pthread_mutex_lock(&logMutex);
			fprintf(logs, "Falha no recebimento do pacote!\n");
			pthread_mutex_unlock(&logMutex);
		} else {
			if(!auxReceber.controle){
				pthread_mutex_lock(&logMutex);
				fprintf(logs, "Pacote recebido de #%d\n", auxReceber.origem);
				pthread_mutex_unlock(&logMutex);
			}
			pthread_mutex_lock(&entrada.mutex);
			duplicarPacote(&auxReceber, &entrada.filaPacotes[entrada.fim++]);
			pthread_mutex_unlock(&entrada.mutex);
		}
	}
}

void *trocarInformacao(void *info){
	while(1){
		enfileirarPacote(&saida, tabelaRoteamento, infoRoteador, listaVizinhos);
		sleep(TEMPO_CHECAR_REDE);
	}
}

void *checarVivacidade(void *info){
	int vizinho, precisaRecalcular = 0;

	while(1){
		sleep(TEMPO_CHECAR_REDE * 2);
		pthread_mutex_lock(&novidadeMutex);
		precisaRecalcular = 0;
		for(int i = 0; i < infoRoteador.qtdVizinhos; i++){
			vizinho = listaVizinhos[i];
			if(infoVizinhos[vizinho].novidade){
				infoVizinhos[vizinho].novidade = 0;
				if(infoVizinhos[vizinho].custo == INF){
					tabelaRoteamento[infoRoteador.id][vizinho].distancia = infoVizinhos[vizinho].custoOriginal;
					tabelaRoteamento[infoRoteador.id][vizinho].proxSalto = vizinho;
					infoVizinhos[vizinho].custo = infoVizinhos[vizinho].custoOriginal;
				}
			} else if(infoVizinhos[vizinho].custo != INF) {
				infoVizinhos[vizinho].custo = INF;
		        precisaRecalcular = 1;
			}
		}
		if(precisaRecalcular){
			recalcular(vizinho, tabelaRoteamento, infoVizinhos, infoRoteador);
			enfileirarPacote(&saida, tabelaRoteamento, infoRoteador, listaVizinhos);
		} 
		pthread_mutex_unlock(&novidadeMutex);
	}
}