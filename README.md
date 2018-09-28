# Implementação de roteadores UDP

Implementação de uma aplicação de transmissão de mensagens de texto. A mensagem  roteada da origem at o destino seguindo a rota computada pelos roteadores

## Compilação

- Abrir o terminal na pasta do projeto e compilar:
    ```
      gcc main.c funcoes.c -o roteador -lpthread -Wall
    ```
### Execução

Para executar deve ser aberto um terminal para cada roteador no terminal deve ser executado o seguinte comando:
```
  ./roteador N
```
Onde N é um inteiro que indica qual roteador será instanciado
- Exemplo:
```
  ./roteador 0
  ./roteador 1
```

### Utilização

A utilização é bem simples, utilizando os passos fornecidos pelo menu você terá acesso às funcionalidaes.

## Configuraço

A topologia estabelecida foi a fornecida pelo professor com a mudança de que os roteadores vão começar com ID 0 e não ID 1

Para configurar os enlaces o arquivo enlaces.config deve ser modificado
O modelo do enlaces.config é o seguinte:
```
  0 1 10
  0 2 15
  1 3 2
  1 4 5
  2 3 2
  3 5 10
  4 5 5
```

Para configurar os roteadors o arquivo roteador.config deve ser modificado
O modelo do roteador.config é o seguinte:
```
  0 25001 127.0.0.1
  1 25002 127.0.0.1
  2 25003 127.0.0.1
  3 25004 127.0.0.1
  4 25005 127.0.0.1
  5 25006 127.0.0.1
```
