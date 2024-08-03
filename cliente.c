#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <linux/if_packet.h>

#include "API-raw-socket.h"
#include "frame.h"

#define BUFFER_SIZE 1024


void init_frame(frame_t *frame, unsigned int sequencia, unsigned int tipo) {
        frame->marcador_inicio = BIT_INICIO;
        frame->sequencia = sequencia;
        frame->tipo = tipo;
        frame->tamanho = strlen(frame->data);
        frame->crc = 0;
}

void MenuCliente(){
    printf("-------------------\n");
    printf("Cliente Iniciado\n");
    printf(" 1 - Listar arquivos\n");
    printf(" 2 - Baixar Arquivo\n");
    printf(" 3 - Sair\n");
}

void lista(int soquete){
    frame_t frameSend;
    frame_t frameRecv;
    init_frame(&frameSend, 0, TIPO_LISTA);

    //envia o tipo lista
    if (send(soquete, &frameSend, sizeof(frameSend), 0) == -1)
    {
        perror("Erro ao enviar mensagem! \n");
    }

    // esperando resposta do servidor
    while(1){
        recv(soquete, &frameRecv, sizeof(frameRecv), 0);

        switch(frameRecv.tipo) {
            case TIPO_ACK:
                printf("ACK\n");
                memset(&frameRecv, 0, sizeof(frameRecv));
                break;
            case TIPO_MOSTRA_NA_TELA:
                memset(&frameSend, 0, sizeof(frameSend));
                init_frame(&frameSend, 0, TIPO_ACK);
                if (send(soquete, &frameSend, sizeof(frameSend), 0) == -1)
                {
                        perror("Erro ao enviar mensagem! \n");
                }
                printf("Lista de arquivos: %s\n", frameRecv.data);
                break;
        }
    }
}

void baixar(int soquete, char* nome_arquivo){
    frame_t frameSend;
    frame_t frameRecv;
    char *separador;
    char buffer_nome_arquivo[256];
    char data[256];
    // FILE *arquivo = fopen(nome_arquivo, "wb");
    //if (arquivo == NULL) {
        //perror("Erro ao abrir arquivo para escrita");
        //return;
        //}

    init_frame(&frameSend, 0, TIPO_BAIXAR);
    strcpy(frameSend.data, nome_arquivo);
    if (send(soquete, &frameSend, sizeof(frameSend), 0) == -1)
        {
            perror("Erro ao enviar mensagem! \n");
        }
    while(1){
        recv(soquete, &frameRecv, sizeof(frameRecv), 0);

        switch(frameRecv.tipo) {
            case TIPO_ACK:
                printf("ACK\n");
                memset(&frameRecv, 0, sizeof(frameRecv));
                break;
            case TIPO_DESCRITOR_ARQUIVO:
                separador = strtok(frameRecv.data, "\n");
                if (separador != NULL) {
                    strncpy(buffer_nome_arquivo, separador, sizeof(buffer_nome_arquivo) - 1);
                    buffer_nome_arquivo[sizeof(buffer_nome_arquivo) - 1] = '\0';

                    separador = strtok(NULL, "\n");
                    if (separador != NULL) {
                        strncpy(data, separador, sizeof(data) - 1);
                        data[sizeof(data) - 1] = '\0';
                    }
                }
                printf("Nome do arquivo: %s\n", buffer_nome_arquivo);
                printf("Data: %s\n", data);

                memset(&frameSend, 0, sizeof(frameSend));
                init_frame(&frameSend, 0, TIPO_ACK);
                if (send(soquete, &frameSend, sizeof(frameSend), 0) == -1)
                {
                    perror("Erro ao enviar mensagem! \n");
                }
                break;
        }
    }
}


int main(int argc, char **argv) {
    int soquete = cria_raw_socket("eno1");
    if (soquete == -1) {
        perror("Erro ao criar socket");
        exit(-1);
    }

    char arg[100];
    char comando[10];
    char nome_arquivo[504];

    while(1) {
        MenuCliente();
        if (fgets(arg, sizeof(arg), stdin) != NULL) {
            strtok(arg, "\n"); // Remove a nova linha

            if (sscanf(arg, "%s %89[^\n]", comando, nome_arquivo) == 2) {
                if (strcmp(comando, "2") == 0) {
                    printf("Baixando arquivo %s...\n", nome_arquivo);
                    baixar(soquete, nome_arquivo);
                } else {
                    printf("Mande o nome do filme após o número da operação.\n");
                }
            } else if (strcmp(arg, "1") == 0) {
                lista(soquete);
            } else if (strcmp(arg, "3") == 0) {
                printf("Saindo...\n");
                break;
            } else {
                printf("Opção inválida, tente novamente.\n");
            }
        } else {
            printf("Erro ao ler entrada.\n");
            break;
        }
    }

    close(soquete); //diz que a operacao terminou ver na imagem
    return 0;
}
