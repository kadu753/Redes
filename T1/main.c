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
	infoRoteador.tamMsg = (int)sizeof(si_other);

	//memset((char *) &socketRoteador, 0, sizeof(socketRoteador));

	pegarEnlace(infoRoteador.id, tabelaRoteamento);
	configurarRoteadores(infoRoteador.id, &infoRoteador.sock, &si_me, roteadores);

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
				printf("Not possible\n");
				sleep(2);
				opcoes = -1;
				break;
		}
	}
	return 0;
}

void enviarMensagem(pacotes auxEnviar, int idRoteador, int sockRoteador, int tamMsg){
	printf("Insira o roteador alvo\n");
	while(scanf("%d", &auxEnviar.destino) != 1 && (auxEnviar.destino < 0 || auxEnviar.destino >= NROUT))
		printf("Roteador inexistente\n");
		getchar();
	while(1){
		system("clear");
		printf("Insira uma mensagem com no máximo %d caracteres\n", MSG);
		scanf("%[^\n]", auxEnviar.mensagem);
		if(strlen(auxEnviar.mensagem) <= MSG){
			auxEnviar.origem = idRoteador;
			transmitirPacote(auxEnviar, sockRoteador, tamMsg);
		}else{
			printf("Mensagem estourou o limite de caracteres\n");
			sleep(3);
			system("clear");
		}
		break;
	}
}

void transmitirPacote(pacotes auxEnviar, int sockRoteador, int tamMsg){
	int auxDestino = 0;
	auxDestino = tabelaRoteamento[auxEnviar.destino].salto;
	si_other.sin_port = htons(roteadores[auxDestino].porta);
	if(inet_aton(roteadores[auxDestino].ip, &si_other.sin_addr) == 0){
		printf("Não foi possível obter o endereco do destinatario.\n");
	}else{
		if(sendto(sockRoteador, &auxEnviar, sizeof(auxEnviar), 0, (struct sockaddr*) &si_other, tamMsg) == -1){
			printf("Falha no envio do pacote\n");
		}else{
			printf("Pacote enviado\n");
			strcpy(LOG, "Pacote enviado para #");
			char aux[1];
			aux[0] = auxEnviar.destino;
			strcat(LOG, aux);
			strcat(LOG, " através de #");
			aux[0] = tabelaRoteamento[auxEnviar.destino].salto;
			strcat(LOG, aux);
		}
		sleep(10);
	}
}

void *receber(void *info){
	tipo *infoRoteador = (tipo*)info;
	pacotes auxReceber;
	char aux[1];
	while(1){
		if(recvfrom(infoRoteador->sock, &auxReceber, sizeof(auxReceber), 0, (struct sockaddr*) &si_other, (uint*)&infoRoteador->tamMsg) == -1){
			printf("Falha no recebimento do pacote\n");
		}else{
			if(auxReceber.destino == infoRoteador->id){
				printf("LUL\n");
				caixaMensagens[infoRoteador->qtdMsg++] = auxReceber;
				infoRoteador->qtdNova++;
				strcpy(LOG, "Recebido um pacote do roteador ");
				aux[0] = auxReceber.origem;
				strcat(LOG, aux);
			}else{
				transmitirPacote(auxReceber, infoRoteador->sock, infoRoteador->tamMsg);
			}
		}
	}
}