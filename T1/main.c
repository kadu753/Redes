#include "roteamento.h"

pair tabelaRoteamento[NROUT];
pacotes caixaMensagens[TAMCAIXA];
roteador roteadores[NROUT];

int main(int argc, char *argv[]){
	int opcoes = -1;
	tipo infoRoteador;
	pacotes auxEnviar;

	if(argc < 2)
		die("Faltam argumentos, informe o [ID] do roteador a ser instanciado");
	else if(argc > 2)
		die("Muitos argumentos, informe apenas o [ID] do roteador a ser instanciado");
	infoRoteador.id = toInt(argv[1]);
	if(infoRoteador.id < 0 || infoRoteador.id >= NROUT)
		die("Não existe um roteador com este [ID]");

	infoRoteador.qtdMsg = infoRoteador.qtdNova = 0;
	infoRoteador.tamMsg = (int)sizeof(struct sockaddr_in);

	pegarEnlace(infoRoteador.id, tabelaRoteamento);
	configurarRoteadores(infoRoteador.id, roteadores);
	
	if((infoRoteador.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		die("Não foi possível criar o Socket!");
	}
	memset((char*) &socketRoteador, 0, sizeof(socketRoteador));
	socketRoteador.sin_family = AF_INET;
	socketRoteador.sin_port = htons(roteadores[infoRoteador.id].porta);
	socketRoteador.sin_addr.s_addr = htonl(INADDR_ANY);

	pthread_create(&receberID, NULL, receber, &infoRoteador);

	while(1){
		system("clear");
		switch(opcoes){
			case -1:
				menu(roteadores[infoRoteador.id].id, roteadores[infoRoteador.id].porta, roteadores[infoRoteador.id].ip, infoRoteador.qtdNova);
				while(scanf("%d", &opcoes) != 1)
					getchar();
				break;
			case 1:
				enviarMensagem(auxEnviar, infoRoteador.id, infoRoteador.sock, infoRoteador.tamMsg);
				opcoes = -1;
				break;
			case 2:
				infoRoteador.qtdNova = 0;
				printarCaixaEntrada(infoRoteador.qtdMsg, caixaMensagens);
				opcoes = -1;
				break;
			case 3:
				printRoteamento(infoRoteador.id, tabelaRoteamento);
				opcoes = -1;
				break;
			case 4:
				opcoes = -1;
				break;
			case 0:
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

void enviarMensagem(pacotes auxEnviar, int idRoteador, int auxSock, int tamMsg){
	printf("Insira o roteador destino\n");
	while(scanf("%d", &auxEnviar.destino) != 1)			//Somente aceita um número inteiro
		getchar();
		system("clear");
		printf("Insira o roteador destino\n");
	while(1){
		getchar();
		if(auxEnviar.destino < 0 || auxEnviar.destino >= NROUT){
			printf("Roteador inexistente\n");
		}else{
			system("clear");
			printf("Insira uma mensagem com no máximo %d caracteres\n", MSG);
			scanf("%[^\n]", auxEnviar.mensagem);
			if(strlen(auxEnviar.mensagem) <= MSG){
				auxEnviar.origem = idRoteador;
				transmitirPacote(auxEnviar, auxSock, tamMsg);
				break;
			}else{
				printf("Mensagem estourou o limite de caracteres\n");
				sleep(2);
				system("clear");
			}
		}
	}
}

void transmitirPacote(pacotes auxEnviar, int auxSock, int tamMsg){
	int auxDestino = 0;
	auxDestino = tabelaRoteamento[auxEnviar.destino].salto;
	socketRoteador.sin_port = htons(roteadores[auxDestino].porta);
	if(inet_aton(roteadores[auxDestino].ip, &socketRoteador.sin_addr) == 0){
		printf("Não foi possível obter o endereco do destinatario.\n");
	}else{
		if(sendto(auxSock, &auxEnviar, sizeof(auxEnviar), 0, (struct sockaddr*) &socketRoteador, tamMsg) == -1){
			printf("Falha no envio do pacote\n");
		}else{
			strcpy(LOG, "Pacote enviado para #");
			char aux[2];
			aux[1] = 0;
			aux[0] = auxEnviar.destino + '0';
			strcat(LOG, aux);
			strcat(LOG, " através de #");
			aux[0] = tabelaRoteamento[auxEnviar.destino].salto + '0';
			strcat(LOG, aux);
		}
	}
}

void *receber(void *info){
	tipo *infoRoteador = (tipo*)info;
	pacotes auxReceber;
	char aux[1];

	if((infoRoteador->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		die("Não foi possível criar o Socket!");
	}
	memset((char*) &socketRoteador, 0, sizeof(socketRoteador));
	socketRoteador.sin_family = AF_INET;
	socketRoteador.sin_port = htons(roteadores[infoRoteador->id].porta);
	socketRoteador.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(infoRoteador->sock, (struct sockaddr*) &socketRoteador, sizeof(socketRoteador)) == -1)
		die("Não foi possível conectar o socket com a porta\n");

	while(1){
		if(recvfrom(infoRoteador->sock, &auxReceber, sizeof(auxReceber), 0, (struct sockaddr*) &socketRoteador, (uint*)&infoRoteador->tamMsg) == -1){
			printf("Falha no recebimento do pacote\n");
		}else{
			if(auxReceber.destino == infoRoteador->id){
				caixaMensagens[infoRoteador->qtdMsg++] = auxReceber;
				infoRoteador->qtdNova++;
				strcpy(LOG, "Recebido um pacote do roteador ");
				aux[0] = auxReceber.origem + '0';
				strcat(LOG, aux);
			}else{
				transmitirPacote(auxReceber, infoRoteador->sock, infoRoteador->tamMsg);
			}
			menu(roteadores[infoRoteador->id].id, roteadores[infoRoteador->id].porta, roteadores[infoRoteador->id].ip, infoRoteador->qtdNova);
		}
	}
}