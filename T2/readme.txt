Author: Felipe Chabatura Neto - 1511100016

-Para compilar use: gcc router.c routing.c -o router -lpthread -Wall -O2

-Configuracoes importantes (Constantes em "routing.h")
  NROUT -> quantidade de nós na rede
  CLEAR_LOG -> Se setado para 1, informações acerca do recebimento,envio e roteamento de Pacotes
  de controle serão omitidas.
  REFRESH_TIME -> tempo em que cada roteador espera antes de mandar seu vetor de distancia aos vizinhos,
  periodicamente

-Nas tabelas de roteamento, o numero fora do parenteses significa o custo até o destino, e o dentro do
  parenteses o próximo salto

-Antes de alterar a topologia da rede, altere a quantidade de nós (NROUT) no arquivo "routing.h"

-A topologia da rede pode ser alterada no arquivo enlaces.config

-Informacoes sobre a porta utilizada por cada roteador, seus endereços, estão no arquivo
roteador.config

-Cada roteador deve ser instanciado executando ./router ID em um terminal diferente,
onde ID é o numero do roteador instanciado

-Uma imagem contendo a topologia default da rede acompanha este arquivo
