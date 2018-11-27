#include "routing.h"

//roteador_t roteador[NROUT];
roteadorVizinho_t infoVizinhos[NROUT];
distSalto_t tabelaRoteamento[NROUT][NROUT]; //Tabela de roteamento do nó
filaPacotes_t entrada, saida; //Filas de entrada e saida de pacotes
informacoesRoteador_t infoRoteador;
int listaVizinhos[NROUT]; //Lista de vizinhos do roteador
int slen = sizeof(socketRoteador); //Tamanho do endereço

int main(int argc, char *argv[]){
	int opcoes = -1;
	char log_path[20] = "./logs/log";
	char message_path[20] = "./messages/message";

	// Certifica-se que os argumentos são válidos ao iniciar o programa
	if (argc < 2)
		die("Argumentos insuficientes, informe o ID do roteador a ser instanciado");
	if (argc > 2) {
		die("Argumentos demais, informe apenas o ID do roteador a ser instanciado");
	}
	infoRoteador.id = toint(argv[1]);
	if (infoRoteador.id < 0 || infoRoteador.id >= NROUT) {
		die("Não existe um roteador com este id, confira o arquivo de configuração");
	}

	//Adiciona id do roteador ao caminho dos arquivos de mensagem e log
	strcat(log_path, argv[1]);
	strcat(message_path, argv[1]);

	//Cria o arquivo para armazenar os logs
	if (!(logs = fopen(log_path, "w+"))) die("Falha ao criar arquivo de log");

	//Cria o arquivo para armazenar as mensagens
	if (!(messages = fopen(message_path, "w+"))) die("Falha ao criar arquivo de mensagens");

	//Rotina de inicializacao do roteador
	inicializar(&infoRoteador, infoVizinhos, tabelaRoteamento, listaVizinhos, &entrada, &saida, &log_mutex, &messages_mutex, &news_mutex);

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
	pthread_create(&refresher_id, NULL, refresher, NULL); //Cria thread atualizadora

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
				print_file(messages, &messages_mutex);
				printf("---------------------END-----------------------\n");
				getchar();
				getchar();
				opcoes = -1;
				break;

			case 3:
				printf("----------------------LOG----------------------\n");
				print_file(logs, &log_mutex);
				printf("----------------------END----------------------\n");
				getchar();
				getchar();
				opcoes = -1;
				break;

			case 4:
				info(infoRoteador.id, infoRoteador.porta, infoRoteador.ip, infoRoteador.qtdVizinhos, listaVizinhos, infoVizinhos, tabelaRoteamento);
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
	pacote_t auxPacote;
	printf("Insira o roteador destino\n");
	while(scanf("%d", &auxPacote.destino) != 1)			//Somente aceita um número inteiro
		getchar();
		system("clear");
		printf("Insira o roteador destino\n");
	while(1){
		getchar();
		if(auxPacote.destino < 0 || auxPacote.destino >= NROUT){
			printf("Roteador inexistente\n");
		}else{
			system("clear");
			printf("Insira uma mensagem com no máximo %d caracteres\n", MAX_MESSAGE);
			scanf("%[^\n]", auxPacote.mensagem);
			strcat(auxPacote.mensagem, "\n");
			if(strlen(auxPacote.mensagem) <= MAX_MESSAGE){
				auxPacote.controle = 0;
				auxPacote.origem = infoRoteador.id;
				pthread_mutex_lock(&saida.mutex);
				copy_package(&auxPacote, &saida.filaPacotes[saida.fim++]);
				pthread_mutex_unlock(&saida.mutex);
				printf("Mensagem enviada!\n");
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

// Função auxiliar do menu. Retorna -1 caso o usuário deseje voltar ou +fallback_option+ caso o usuário queira atualizar
int back_option_menu(int fallback_option) {
	int option;
	printf("\nInsira 0 para voltar, 1 para atualizar\n");
	scanf("%d", &option);
	return !option ? -1 : fallback_option;
}

void* enviar(void *nothing){
	int auxDestino;
	pacote_t *auxEnviar;

	pthread_mutex_lock(&log_mutex);
	fprintf(logs, "Enviar foi iniciado\n");
	pthread_mutex_unlock(&log_mutex);
	while(1){
		pthread_mutex_lock(&saida.mutex);
		while(saida.inicio != saida.fim){
			auxEnviar = &(saida.filaPacotes[saida.inicio]);
			auxDestino = tabelaRoteamento[infoRoteador.id][auxEnviar->destino].proxSalto;
			socketRoteador.sin_port = htons(infoVizinhos[auxDestino].porta);
			if(inet_aton(infoVizinhos[auxDestino].ip, &socketRoteador.sin_addr) == 0){
				pthread_mutex_lock(&log_mutex);
				fprintf(logs, "Enviar - não foi possível obter o endereco do destinatario.\n");
				pthread_mutex_unlock(&log_mutex);
			}else{
				if(sendto(infoRoteador.sock, auxEnviar, sizeof(*auxEnviar), 0, (struct sockaddr*) &socketRoteador, slen) == -1){
					pthread_mutex_lock(&log_mutex);
					fprintf(logs, "Enviar - Falha no envio do pacote\n");
					pthread_mutex_unlock(&log_mutex);
				}else{
					if(!auxEnviar->controle){
						pthread_mutex_lock(&log_mutex);
						fprintf(logs, "Pacote enviado para #%d\n", auxEnviar->destino);
						pthread_mutex_unlock(&log_mutex);
					}
				}
			}
			saida.inicio++;
		}
		pthread_mutex_unlock(&saida.mutex);
	}
}

void* desempacotador(void *nothing){

	int tabelaAtualizada = 0, vetorAtualizado = 0;
	pacote_t *auxDesempacotar;

	pthread_mutex_lock(&log_mutex);
	fprintf(logs, "Desempacotador foi iniciado\n");
	pthread_mutex_unlock(&log_mutex);

	while(1){
		pthread_mutex_lock(&entrada.mutex);
		while(entrada.inicio != entrada.fim){
			auxDesempacotar = &entrada.filaPacotes[entrada.inicio];
			if(auxDesempacotar->destino == infoRoteador.id){ // O pacote é para mim
				if(!auxDesempacotar->controle){ // Se for uma mensagem
					pthread_mutex_lock(&log_mutex);
					fprintf(logs, "Desempacotando pacote de %d\n", auxDesempacotar->origem);
					pthread_mutex_unlock(&log_mutex);
					pthread_mutex_lock(&messages_mutex);
					fprintf(messages, "Origem [%d]:  %s\n", auxDesempacotar->origem, auxDesempacotar->mensagem);
					pthread_mutex_unlock(&messages_mutex);
				}else{ // É um pacote de controle
					tabelaAtualizada = 0;
					vetorAtualizado = 0;
					pthread_mutex_lock(&news_mutex);
					infoVizinhos[auxDesempacotar->origem].novidade = 1;
					pthread_mutex_unlock(&news_mutex);
					for(int i = 0; i < NROUT; i++){
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
						pthread_mutex_lock(&log_mutex);
						printRoteamento(tabelaRoteamento, logs);
						pthread_mutex_unlock(&log_mutex);
					}
					if(vetorAtualizado){
						pthread_mutex_lock(&log_mutex);
			            fprintf(logs, "O vetor de distancia  mudou no desempacotador\n");
			            queue_dist_vec(&saida, listaVizinhos, tabelaRoteamento, infoRoteador.id, infoRoteador.qtdVizinhos);
			            pthread_mutex_unlock(&log_mutex);
					}
				}
			}else{
				pthread_mutex_lock(&saida.mutex);
				copy_package(auxDesempacotar, &saida.filaPacotes[saida.fim++]);
				pthread_mutex_unlock(&saida.mutex);
			}
			entrada.inicio++;
		}
		pthread_mutex_unlock(&entrada.mutex);
	}
}

void* receber(void *nothing){
	pacote_t auxReceber;

	pthread_mutex_lock(&log_mutex);
	fprintf(logs, "Receber iniciado!\n");
	pthread_mutex_unlock(&log_mutex);

	while(1){
		if ((recvfrom(infoRoteador.sock, &auxReceber, sizeof(auxReceber), 0, (struct sockaddr *) &socketRoteador,
			(socklen_t * restrict ) &slen)) == -1) {
			pthread_mutex_lock(&log_mutex);
			fprintf(logs, "Falha no recebimento do pacote!\n");
			pthread_mutex_unlock(&log_mutex);
		} else {
			if(!auxReceber.controle){
				pthread_mutex_lock(&log_mutex);
				fprintf(logs, "Pacote recebido de #%d\n", auxReceber.origem);
				pthread_mutex_unlock(&log_mutex);
			}
			pthread_mutex_lock(&entrada.mutex);
		    copy_package(&auxReceber, &entrada.filaPacotes[entrada.fim++]); //Coloca o pacote no final da fila de recebidos
		    pthread_mutex_unlock(&entrada.mutex);
		}
	}
}

void *refresher(void *nothing){
	while(1){
		queue_dist_vec(&saida, listaVizinhos, tabelaRoteamento, infoRoteador.id, infoRoteador.qtdVizinhos);
		sleep(REFRESH_TIME);
	}
}