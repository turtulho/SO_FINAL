// autores:
// David Cameijo Pinheiro 2021246865
// Guilherme Fernandes Figueiredo 2021236787


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <signal.h>
#define MAXBUF 200

// Definição do ID do BackOffice User
#define BACKOFFICE_ID 1
#define BACK_PIPE "/tmp/back_pipe"
typedef struct
{
  /* Message type */
  long int id_process;
  char mensagem[MAXBUF];
 

} msg_queue;
int mqid;
void function_dataStats(){
    printf("vai ser executado o dataStats\n");
    return;
}
void function_reset(){
    printf("vai ser executado o functionReset\n");
    return;

}
// Função para lidar com os sinais de interrupção (SIGINT)
void handle_sigint(int sig) {
    printf("Received SIGINT. Exiting...\n");
    exit(EXIT_SUCCESS);
}
void *read_queue(int a){
    
        msg_queue message;
        
        char formatted_message[100];
        
        while(1){
        
        msgrcv(mqid, &message, sizeof(message), BACKOFFICE_ID,1 );
        
        
        if (strlen(message.mensagem) > 0) {
            
            printf("%s\n", message.mensagem);
            }
        

        
        }

    
        
    
    

}

int main() {
    // Registrar o tratamento de sinal para SIGINT
    signal(SIGINT, handle_sigint);
    
    int fd;
    pthread_t thread_leitura_fila;
    // Nome do named pipe para comunicação com o Authorization Request Manager
    fd = open(BACK_PIPE, O_WRONLY);
    //ID_backoffice_user#[data_stats | reset]
    // Loop principal
    mqid = msgget((key_t)1235, 0666);
    pthread_create(&thread_leitura_fila,NULL,read_queue,NULL);
    while (1) {
        char command[100];
        printf("Digite o comando (data_stats | reset): ");
        fgets(command, sizeof(command), stdin);
        char mensagem[200];
        snprintf(mensagem, MAXBUF, "%d#%s", BACKOFFICE_ID,command );
        if(strcmp(command, "data_stats\n") == 0) {
            write(fd, mensagem, strlen(mensagem) + 1);
            memset(mensagem,0,MAXBUF);
            
            
        } else if(strcmp(command, "reset\n") == 0) {
           write(fd, mensagem, strlen(mensagem) + 1);
           memset(mensagem,0,MAXBUF);
        }
        else {
            printf("Comando inválido\n");
        }

        

        // Remover o caractere de nova linha do final da string de comando
        command[strcspn(command, "\n")] = 0;

        // Construir a mensagem para enviar para o named pipe
        char message[100];
        printf("BackOfficeId:%d, comando:%s\n",BACKOFFICE_ID,command);

        



        
        // Abrir o named pipe para escrita
        
        // Escrever a mensagem no named pipe
        

        

        // Esperar um pouco para a próxima iteração
         // Simulando uma pausa entre os comandos
    }
    // Fechar o file pointer do named pipe
       pthread_join(thread_leitura_fila,NULL);

    return 0;
}
