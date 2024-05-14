#ifndef DADOS_H
#define DADOS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>
#include <sys/select.h>


typedef struct {
    int mobile_users; // número de Mobile Users que serão suportados pelo simulador
    int queue_pos; // número de slots nas filas que são utilizadas para armazenar os pedidos de autorização e os comandos dos utilizadores (>=0)
    int auth_servers; // número de Authorization Engines que devem ser lançados inicialmente (>=1)
    unsigned int auth_proc_time; // período (em ms) que o Authorization Engine demora para processar os pedidos
    unsigned int max_video_wait;  // tempo máximo (em ms) que os pedidos de autorização do serviço de vídeo podemaguardar para serem executados (>=1)
    unsigned int max_others_wait; //tempo máximo (em ms) que os pedidos de autorização dos serviços de música e de redes sociais, bem como os comandos podem aguardar para serem executados (>=1)
} Configuration;

typedef struct{
    int UserID;
    int PlafondInicial;
    int SaldoAtualizado;
    int alerta;
    //int alerta_anterior;
} UserData;

typedef struct{

    int totaldata;
    int totalreq;

} Service_stats;



typedef struct{

    pthread_mutex_t mutex_user_info;  // Mutex para sincronização
    Service_stats stats[3];
    int num_users;
    UserData Users[];
    
} SharedMemory;

typedef struct{
    pthread_mutex_t mutex_flags; 
    int extra_auth_engine_flag;
    int num_engines;
    int auth_engine_flags[];
    

} Auth_Engine_Manager;

typedef struct{
    int UserID;
    int Tipo;
    int QuantDados;
    clock_t StartTime;
} Pedido_User;

// Definition of Queue

typedef struct {
    Pedido_User *data;  // Pointer to the queue's array
    int front;  // Front index
    int rear;  // Rear index
    int count;  // Number of items in the queue
    int max_size;  // Maximum capacity
    pthread_mutex_t mutex; // Mutex for synchronization
} Queue;

typedef struct
{
  //Message type /
  long int id;
  char mensagem[200]                  
} msgqueue;
sem_t semaphore1;


int init_queue(Queue *queue, int size);
int is_full(const Queue *queue);
int is_empty(const Queue *queue);
int is_below_or_equal_50(const Queue *queue);  // New function to check 50% capacity
int enqueue(Queue *queue, Pedido_User pedido);
Pedido_User dequeue(Queue *queue);
Pedido_User create_pedido_user(int user_id, int tipo, int quant_dados);
void free_pedido_user(Pedido_User *pedido);
void reset_stats(SharedMemory* shared_mem);
void register_user(int user_id, int initial_plafond, SharedMemory* shared_mem);
void consume_service(int user_id,int serviceid,int data, SharedMemory* shared_mem);
void show_data_stats(SharedMemory* shared_mem, int msgqid);
void create_engines(Auth_Engine_Manager* auth_engine_manager,int num_engines,int engine_sleep, SharedMemory* num_auth_engines, int (*pipes)[2]);
void process_request(Pedido_User request, SharedMemory* shared_mem, Auth_Engine_Manager* auth_engine_manager,int engine_id,int msgqid);
void write_log(char *s);

#endif // DADOS_H