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

#define USER_PIPE "/tmp/user_pipe"
#define MAX_BUF 1024
#define NUM_SERVICES 3
#define TAMANHO_STRING 20

int id_aleatorio =12;
int contagem_pedidos;
int max_requests;
int fd;
int mqid;
pid_t id;

struct ServiceRequest {
    char service[TAMANHO_STRING];
    int data_to_reserve;
    int pid_mobile_user;
    int time_sleep;
};
typedef struct
{
  /* Message type */
  long int id_process;
  char mensagem[MAX_BUF];
 

} msg_queue;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void handle_sigint(int sig) {
    printf("Recebido SIGINT. Saindo...\n");
    // Aqui você pode adicionar ações de limpeza, se necessário
    exit(EXIT_SUCCESS);
}
void *service_handler(void *arg) {
    struct ServiceRequest *request = (struct ServiceRequest *)arg;
    
   

    
    
    while (contagem_pedidos<max_requests)

    {   
        
        
        char message[MAX_BUF];
        usleep((request->time_sleep*1000)); 
        if (contagem_pedidos >= max_requests) {
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&mutex); 
        
        contagem_pedidos++;
        pthread_mutex_unlock(&mutex);

        // Bloquear o acesso ao pipe
        
        
    
        
        
    
        snprintf(message, MAX_BUF, "%d#%s#%d\n", /*request->pid_mobile_user*/request->pid_mobile_user, request->service, request->data_to_reserve);
        printf("%s",message);
        ssize_t bytes_written=write(fd, message, strlen(message));
         if (bytes_written <= 0) {
        perror("Erro ao escrever no pipe");
        
        close(fd);
        
        
        
       
        }
         
        
    }
    

    


   
    free(request);
    pthread_exit(NULL);
}

void *read_queue(int mqid){
    
        msg_queue message;
        printf("dewddwqe mqid:%d\n",mqid);
        printf("id%d\n",id);
        char formatted_message[100];
        sprintf(formatted_message, "ALERT:%d has exceeded 100 his initial balance.\n", id);
        while(1){
        
        msgrcv(mqid, &message, sizeof(message), id,1 );
        if(strcmp(message.mensagem,formatted_message)==0){
            printf("%s\n",message.mensagem);
            break;
        }
        
        if (strlen(message.mensagem) > 0) {
            
            printf("%s\n", message.mensagem);
            }
        

        
        }

    
        
    
    

}


int main(int argc, char *argv[]) {
    contagem_pedidos=0;
    char *filepath = "/home/user/Desktop/SO";
    
    mqid = msgget((key_t)1235, 0666);
    printf("%d\n",mqid);
    signal(SIGINT, handle_sigint);
    int initial_balance, data_to_reserve;
    int intervals[NUM_SERVICES];
    id = getpid();
    pthread_t threads[NUM_SERVICES];
    pthread_t thread_leitura_fila;

    if (argc != 7) {
        printf("Uso: %s <plafond_inicial> <max_pedidos> <intervalos_VIDEO_MUSIC_SOCIAL> <dados_reservar>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Inicializar variáveis
    initial_balance = atoi(argv[1]);
    max_requests = atoi(argv[2]);
    intervals[0] = atoi(argv[3]);
    intervals[1] = atoi(argv[4]);
    intervals[2] = atoi(argv[5]);
    data_to_reserve = atoi(argv[6]);

    // Inicializar o pipe
    char message[MAX_BUF];

    // Loop principal
   fd = open(USER_PIPE, O_WRONLY);
  
   sprintf(message, "%d#%d\n",id, initial_balance);
    printf("%s\n",message);
    ssize_t bytes_written=write(fd, message, strlen(message));
         if (bytes_written <= 0) {
        perror("Erro ao escrever no pipe");
        
        close(fd);
        
        
        
       
        }
        for (int i = 0; i < NUM_SERVICES; i++) {
            
                struct ServiceRequest *request = malloc(sizeof(struct ServiceRequest));
                if (i == 0)
                    strcpy(request->service, "VIDEO");
                else if (i == 1)
                    strcpy(request->service, "MUSIC");
                    
                else
                    strcpy(request->service, "SOCIAL");
                request->data_to_reserve = data_to_reserve;
                request->pid_mobile_user = id;
                request->time_sleep = intervals[i];
                pthread_create(&threads[i], NULL, service_handler, (void *)request);
            
        }
       pthread_create(&thread_leitura_fila,NULL,read_queue,mqid);
        



        // Aguardar a conclusão de todas as threads
        for (int i = 0; i < NUM_SERVICES; i++) {
            for (int j = 0; j < 3; j++) {
                pthread_join(threads[i], NULL);
            }
        }
        pthread_join(thread_leitura_fila,NULL);


        // Verificar se atingiu o número máximo de solicitações
        
    

    // Limpeza e encerramento do programa
   
    return 0;
}
