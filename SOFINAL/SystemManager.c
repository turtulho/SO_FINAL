// autores:
// David Cameijo Pinheiro 2021246865
// Guilherme Fernandes Figueiredo 2021236787

#include "dados.h"


#define USER_PIPE "/tmp/user_pipe"
#define BACK_PIPE "/tmp/back_pipe"
#define EPSILON 0.000001
#define BUFFER_SIZE 124 



typedef struct {
    Queue *video_streaming_queue;
    Queue *other_services_queue;
    int* engine_pipes; // Array of pipe descriptors for authorization engines         // Matriz de pipes para comunicação com engines
    int num_engines;        // Número total de engines
    Auth_Engine_Manager* auth_engine_manager;
    int max_video_wait;
    int max_others_wait;
    
} ThreadsData;

// Semaphore for synchronizing access to the queues
sem_t queue_sem;

sem_t *log_sem; // Semaphore for synchronizing access to the log file

sem_t *engines_sem;

sem_t *alertas;

pid_t auth_man_process;
pid_t monitor_process;
int *engine_pids;
int num_auth_engines;

Auth_Engine_Manager* auth_engine_manager;
SharedMemory* shared_mem;

int shmid1, shmid2;


int create_ipcs(){
  int mqid;
  mqid = msgget((key_t)1235, IPC_CREAT|0666);
  
  return mqid;

}

void sigint_handler() {
    printf("\nSIGINT RECEIVED. CLEANING RESOURCES....\n");
    if (unlink(USER_PIPE) == -1) {
        write_log("ERROR UNLINKING USER PIPE\n");
        // Handle error if needed
    } else {
        write_log("USER PIPE UNLINKED.\n");
    }
    if (unlink(BACK_PIPE) == -1) {
        write_log("ERROR UNLINKING BACK PIPE\n");
        // Handle error if needed
    } else {
        write_log("BACK PIPE UNLINKED.\n");
    }

    if (shmdt(auth_engine_manager) == -1) {
        perror("Error detaching shared memory 1");
    }
    if (shmdt(shared_mem) == -1) {
        perror("Error detaching shared memory 2");
    }
    if (shmctl(shmid1, IPC_RMID, NULL) == -1) {
        perror("Error removing shared memory 1");
    }
    if (shmctl(shmid2, IPC_RMID, NULL) == -1) {
        perror("Error removing shared memory 2");
    }
    write_log("SHARED MEMORY DETACHED AND REMOVED SUCESSFULY!\n");

    for(int i=0;i<num_auth_engines;i++){
        kill(engine_pids[i],SIGTERM);
    }
    write_log("ENGINE PROCESSES DESTROYED!\n");

    kill(auth_man_process, SIGTERM);
    write_log("AUTH REQ MANAGER PROCESS DESTROYED!\n");
    //kill(pid_monitor_engine, SIGTERM);

    

    // destroy semaphores
    sem_destroy(&queue_sem);
    sem_destroy(log_sem);
    sem_destroy(engines_sem);

    write_log("SEMAPHORES DESTROYED\n");
    
    exit(EXIT_SUCCESS);
}

int count_hashes(const char *input) {
    int count = 0;  // Initialize count to zero
    size_t length = strlen(input);  // Get the length of the input string

    for (size_t i = 0; i < length; i++) {  // Loop through the string
        if (input[i] == '#') {  // If the character is a '#'
            count++;  // Increment the count
        }
    }

    return count;  // Return the total count of '#'
}


void clear_file(const char *filename) {
    FILE *file = fopen(filename, "w"); // Open file in write mode
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fclose(file); // Close the file after truncating or creating
}

void write_log(char *s){

    if (sem_wait(log_sem) == -1) {
        perror("Error waiting on semaphore");
        exit(EXIT_FAILURE);
    }

	FILE* file = fopen("log.txt", "a"); 
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro");
        exit(1);
    }
    
    time_t now;
    struct tm *local;
    time(&now);

    // Set the timezone to Portugal/Lisbon
    setenv("TZ", "Europe/Lisbon", 1); // Set to Portugal/Lisbon timezone
    tzset(); // Update timezone information


    local = localtime(&now);
    
    fprintf(file,"%02d:%02d:%02d %s\n", local->tm_hour, local->tm_min, local->tm_sec,s);
   
    
	printf("%02d:%02d:%02d %s\n", local->tm_hour, local->tm_min, local->tm_sec,s);
	
    fclose(file);

    if (sem_post(log_sem) == -1) {
        perror("Error releasing semaphore");
        exit(EXIT_FAILURE);
    }
   
}

void erro(char *msg) {
    perror( msg);
    write_log(msg);
    exit(1);
}

Configuration read_config(const char* filename) {
    Configuration config;

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        erro( "Erro ao abrir o arquivo de configuracao.\n");
    }

    fscanf(file, "%d\n", &config.mobile_users);
    if(config.mobile_users < 1){
        erro("Mobile users inferior a 1.\n");
    }
    fscanf(file, "%d\n", &config.queue_pos);
    if(config.queue_pos < 0){
        erro("queue_pos inferior a 0.\n");
    }
    fscanf(file, "%d\n", &config.auth_servers);
    if(config.auth_servers < 1){
        erro("auth_servers inferior a 1.\n");
    }
    fscanf(file, "%d\n", &config.auth_proc_time);
    if(config.auth_proc_time <= 0){
        erro("auth_proc_time inferior ou igual a 0.\n");
    }
    fscanf(file, "%d\n", &config.max_video_wait);
    if(config.max_video_wait <= 0){
        erro("max_video_wait inferior a 0.\n");
    }
    fscanf(file, "%d\n", &config.max_others_wait);
    if(config.max_others_wait <= 0){
        erro("max_others_wait inferior ou igual a 0.\n");
    }

    fclose(file);

    return config;
}

// Function to send requests to an available Authorization Engine
void send_request_to_authorization_engine(int write_fd, Pedido_User request) {
    ssize_t bytes_written = write(write_fd, &request, sizeof(request));
    if (bytes_written == -1) {
        perror("Error writing to unnamed pipe");
    }
    
}

// Process commands based on their source
void process_user_command(const char *message,ThreadsData *data) {
    int id;
    int tipo;
    int quantDados;
    
    
    
    ;
    if (count_hashes(message) == 1){
        char *data = strtok(message, "#");
        if (data != NULL) {
            id=atoi(data);  // Copy the first part to `id`
            }
        data = strtok(NULL, "#");  // Get the next token
        if (data != NULL) {
            if(id!=1){
                quantDados=atoi(data);
                tipo=0;
                }
            else{
                    if (strcmp(data, "reset") == 0){
                        tipo=0;
                        quantDados=-1;
                    }
                    if(strcmp(data, "data_stats") == 0){
                        tipo=1;
                        quantDados=-1;}
                }
            }   
        }
    else{
        char *data = strtok(message, "#");
        if (data != NULL) {
            id=atoi(data);  // Copy the first part to `id`
            }

        data = strtok(NULL, "#");  // Get the next token
        if (data != NULL) {
            if (strcmp(data, "VIDEO") == 0){
                    tipo=1;
                    }
            if(strcmp(data, "MUSIC") == 0){
                    tipo=2;
                    }
            if(strcmp(data, "SOCIAL") == 0){
                    tipo=3;
                    }
            }

        data = strtok(NULL, "#");  // Get the next token
        if (data != NULL) {
            quantDados=atoi(data);
        }
    
    }
    char msg[BUFFER_SIZE];
    sprintf(msg,"PEDIDO DO MOBILE USER ID = %d Tipo=%d Quantidade de Dados=%d\n ",id,tipo,quantDados);
    write_log(msg);
    //printf("MENSAGEM PROCESSADA ...\n ID = %d\nTipo=%d\nQuantidade de Dados=%d\n",id,tipo,quantDados);
    Pedido_User pedido = create_pedido_user(id,tipo,quantDados);
    if(pedido.UserID !=1 && pedido.Tipo==1){
        if(!is_full(data->video_streaming_queue)){
            enqueue(data->video_streaming_queue,pedido);
            sem_post(&queue_sem);
            }
        }
    else{
        if(!is_full(data->other_services_queue)){
            enqueue(data->other_services_queue,pedido);
            sem_post(&queue_sem);
            }
        }

}

void authorization_engine(int read_fd, int id, int sleeptime, SharedMemory* shared_mem, Auth_Engine_Manager* engine_manager,int msgqid) {
    int engine_id = id;
    fd_set read_set;
    Pedido_User request;
    char msg[BUFFER_SIZE];
    sprintf(msg,"AUTH ENGINE %d CREATED\n",engine_id);
    write_log(msg);
    //pthread_mutex_lock(&engine_manager->mutex_flags);
    engine_manager->auth_engine_flags[engine_id] = 0;
    //pthread_mutex_unlock(&engine_manager->mutex_flags);

    while (1) {
        FD_ZERO(&read_set);
        FD_SET(read_fd, &read_set);
        
        // Use select to wait for data to be available on the pipe
        int ready = select(read_fd + 1, &read_set, NULL, NULL, NULL);
        
        if (ready == -1) {
            perror("Error in select");
            continue; // Continue to the next iteration of the loop
        }

        if (FD_ISSET(read_fd, &read_set)) {
            ssize_t bytes_read = read(read_fd, &request, sizeof(request));
            if (bytes_read < 0) {
                perror("Error reading from pipe");
                continue; // Continue to the next iteration of the loop
            } else if (bytes_read == 0) {
                // No data read, handle accordingly
                continue; // Continue to the next iteration of the loop
            }

            // Process the request
            //pthread_mutex_lock(&engine_manager->mutex_flags);
            //engine_manager->auth_engine_flags[engine_id] = 1; // Mark the engine as busy
            //pthread_mutex_unlock(&engine_manager->mutex_flags);
            
            write_log("ENGINE PROCESSING REQUEST...");

            engine_manager->auth_engine_flags[engine_id]=1;

            process_request(request,shared_mem,engine_manager,engine_id,msgqid); // Process the request

            usleep(sleeptime);

            
            int working_engines=0;
            for(int i = 0;i < engine_manager->num_engines+1;i++){
                if(engine_manager->auth_engine_flags[i]==1){
                    working_engines++;
                    }
                }
           // pthread_mutex_lock(&engine_manager->mutex_flags);
            engine_manager->auth_engine_flags[engine_id] = 0; //mete o motor pronto a trabalhar
           // pthread_mutex_unlock(&engine_manager->mutex_flags);
            if(working_engines == engine_manager->num_engines){
                sem_post(engines_sem);
            }
            //pthread_mutex_unlock(engine_manager->mutex_flags);
            write_log("ENGINE PROCESSED REQUEST\n");
        }
    }

    // Close the pipe when done (if needed)
    close(read_fd);
    write_log("AUTHORIZATION ENGINE EXITING...\n");
}
    
void envia_alertas(SharedMemory *shared_mem,int mqid) {
    
    
    for (int i = 0; i < shared_mem->num_users; i++){
        
        float a=1.0 - (float)shared_mem->Users[i].SaldoAtualizado / (float)shared_mem->Users[i].PlafondInicial;
        msgqueue msg;
        if(shared_mem->Users[i].alerta==1){

        if(a>=0.8-EPSILON && a<0.9-EPSILON){
            
            
            snprintf(msg.mensagem,200,"ALERT:%d has exceeded 80 of his initial balance.\n",shared_mem->Users[i].UserID);
            
            msg.id=shared_mem->Users[i].UserID;
            
            msgsnd(mqid, &msg, sizeof(msg), 0);
            shared_mem->Users[i].alerta=0;
            
        }
        if(a>=0.9-EPSILON && a<1){ 
            printf("a enviar alerta no msgnd dos 90 por cento\n");
            snprintf(msg.mensagem,200,"ALERT:%d has exceeded 90 of his initial balance.\n",shared_mem->Users[i].UserID);
            msg.id=shared_mem->Users[i].UserID;
            msgsnd(mqid, &msg, sizeof(msg), 0);
            shared_mem->Users[i].alerta=0;
        }
        if((float)shared_mem->Users[i].SaldoAtualizado==0){
            
            snprintf(msg.mensagem,200,"ALERT:%d has exceeded 100 his initial balance.\n",shared_mem->Users[i].UserID);
            msg.id=shared_mem->Users[i].UserID;
            msgsnd(mqid, &msg, sizeof(msg), 0);
            shared_mem->Users[i].alerta=0;
        }
        }

    }

}
pid_t monitor_engine(int mqid,SharedMemory* shared_mem) {
    pid_t monitor_process;
    monitor_process = fork();
    if (monitor_process == -1) {
        erro("Erro ao criar processo do Monitor Engine");
        exit(EXIT_FAILURE);
    }
    if (monitor_process == 0) {
        // Código do processo filho para o Monitor Engine
        write_log("PROCESS MONITOR ENGINE CREATED!.\n");

        // Aqui será implementada a lógica para monitorar o plafond de dados dos Mobile Users
        // e gerar alertas ou estatísticas conforme necessário
        
        while (1)
        {
            //sem_wait(&semaphore1);
            envia_alertas(shared_mem,mqid);
            //sem_post(&semaphore1);
            /* code */
        }
        

        exit(1);

    }
    waitpid(monitor_process,NULL,0);
    return monitor_process;

}


void* sender_f(void* arg) {
    // Implement thread 1 logic here
    write_log("THREAD SENDER CREATED!\n");
    ThreadsData *data= (ThreadsData *)arg;
    
      while (1) {
        
        // Wait until there's a request in either queue
        sem_wait(&queue_sem);

        Pedido_User request;

        // Prioritize video streaming requests
        if (!is_empty(data->video_streaming_queue)) {
            request = dequeue(data->video_streaming_queue);
        } else {
            request = dequeue(data->other_services_queue);
        }
        
        write_log("SENDER: RETRIEVED REQUEST FROM QUEUE\n");
        
        clock_t cur_time = clock();

        double elapsed_time = (double)(cur_time - request.StartTime) * 1000.0 / CLOCKS_PER_SEC;

        int passou_o_tempo=0;

        if (request.UserID != 1 && request.Tipo == 1){
            if(elapsed_time > data->max_video_wait){
                write_log("PEDIDO DE AUTORIZAÇAO EXCEDEU O TEMPO\n");
                passou_o_tempo+=1;}
            }
        else{
            if(elapsed_time > data->max_others_wait){
                write_log("PEDIDO DE AUTORIZAÇAO EXCEDEU O TEMPO\n");
                passou_o_tempo+=1;}
        }

        if (passou_o_tempo == 0){

        sem_wait(engines_sem);

        
        //pthread_mutex_lock(&data->auth_engine_manager->mutex_flags);
        int working_engines=0;
        
        for(int i = 0 ; i < data->num_engines+1; i++){
            if(data->auth_engine_manager->auth_engine_flags[i] == 1){
                working_engines++;
                    }
                }
            
        for(int i = 0 ; i < data->num_engines+1; i++){
            if(data->auth_engine_manager->auth_engine_flags[i] == 0){

                if(working_engines == data->num_engines-1){
                    printf("/nENVIADO REQUEST PARA ULTIMO ENGINE %d/n",i);
                    send_request_to_authorization_engine(data->engine_pipes[i],request);
                    //pthread_mutex_unlock(&data->auth_engine_manager->mutex_flags);
                    break;
                    }
                else{
                    char msg[BUFFER_SIZE];
                    sprintf(msg,"SENDER: TIPO %d ENVIADO REQUEST (ID = %d) PARA ENGINE %d\n",request.Tipo,request.UserID,i);
                    write_log(msg);
                    send_request_to_authorization_engine(data->engine_pipes[i],request);
                    //pthread_mutex_unlock(&data->auth_engine_manager->mutex_flags);
                    sem_post(engines_sem);
                    
                    break;
                    }
                }
            }}
        
        
      }
    return NULL;
    }


   



void* receiver_f(void* arg) {
    // Implement thread 1receiver logic here
    write_log("THREAD RECEIVER CREATED!\n");

    ThreadsData *data= (ThreadsData *)arg;


    int user_pipe_fd = open(USER_PIPE, O_RDONLY | O_NONBLOCK); // Open in non-blocking mode
    int back_pipe_fd = open(BACK_PIPE, O_RDONLY | O_NONBLOCK);

    if (user_pipe_fd == -1 || back_pipe_fd == -1) {
        perror("Error opening named pipes");
        return NULL;  // Exit if pipes can't be opened
    }

    fd_set read_fds;  // File descriptor set
    char buffer[1024];  // Buffer for reading data
    int buffer_pos = 0;  // Position in the buffer
    ssize_t bytes_read;  // Number of bytes read

    while (1) {  // Main loop
        FD_ZERO(&read_fds);  // Clear the set
        FD_SET(user_pipe_fd, &read_fds);  // Add USER_PIPE to the set
        FD_SET(back_pipe_fd, &read_fds);  // Add BACK_PIPE to the set

        int max_fd = (user_pipe_fd > back_pipe_fd) ? user_pipe_fd : back_pipe_fd;

        // Use select() to wait for data from either pipe
        int retval = select(max_fd + 1, &read_fds, NULL, NULL, NULL);  // Block until data arrives
        if (retval == -1) {
            perror("Error in select()");
            break;
        }

        // Check if there's data from USER_PIPE
        if (FD_ISSET(user_pipe_fd, &read_fds)) {
            bytes_read = read(user_pipe_fd, buffer + buffer_pos, sizeof(buffer) - buffer_pos - 1);  // Read data into the buffer
            if (bytes_read > 0) {
                buffer_pos += bytes_read;
                buffer[buffer_pos] = '\0';  // Null-terminate the buffer

                // Process complete messages separated by '\n'
                char *message_start = buffer;
                char *newline;
                while ((newline = strchr(message_start, '\n')) != NULL) {
                    *newline = '\0';  // Terminate the message
                    write_log("RECEIVER RECEIVED REQUEST FROM USER_PIPE \n");  // Handle the message
                    process_user_command(message_start,data);
                    message_start = newline + 1;  // Move to the next message
                }

                // Move remaining unprocessed data to the beginning of the buffer
                buffer_pos = strlen(message_start);
                memmove(buffer, message_start, buffer_pos);
            }
        }

        // Check if there's data from BACK_PIPE
        if (FD_ISSET(back_pipe_fd, &read_fds)) {
            bytes_read = read(back_pipe_fd, buffer + buffer_pos, sizeof(buffer) - buffer_pos - 1);  // Read data into the buffer
            if (bytes_read > 0) {
                buffer_pos += bytes_read;
                buffer[buffer_pos] = '\0';

                // Process complete messages separated by '\n'
                char *message_start = buffer;
                char *newline;
                while ((newline = strchr(message_start, '\n')) != NULL) {
                    *newline = '\0';  // Terminate the message
                    write_log("RECEIVER RECEIVED REQUEST FROM BACK_PIPE \n");  // Handle the message
                    process_user_command(message_start,data);
                    message_start = newline + 1;  // Move to the next message
                }

                // Move unprocessed data to the beginning of the buffer
                buffer_pos = strlen(message_start);
                memmove(buffer, message_start, buffer_pos);
            }
        }
    }

    // Cleanup
    close(user_pipe_fd);
    close(back_pipe_fd);

    return NULL;
}


pid_t auth_requests_manager(Configuration config,SharedMemory* shared_mem,Auth_Engine_Manager* auth_engine_manager,int msgqid) {
    pthread_t receiver, sender;
    int res1, res2;
    int max_size = config.queue_pos;
    int num_auth_engines=config.auth_servers;
    int video_time = config.max_video_wait;
    int others_time = config.max_others_wait;
    int engine_sleep = config.auth_proc_time;


    
    auth_man_process = fork();
    if (auth_man_process == -1) {
        erro("ERROR CREATING PROCESS AUTHORIZATION REQUESTS MANAGER.\n");
        exit(EXIT_FAILURE);
    }
    if (auth_man_process == 0) {
        // Child process code for Authorization Requests Manager
        write_log("PROCESS AUTHORIZATION REQUESTS MANAGER CREATED.\n");
        int pipes[num_auth_engines + 1][2];  // Array of pipes (two descriptors each)
        int* engine_pipe_fds_write = malloc(sizeof(int) * (num_auth_engines+1)); // Array to hold write descriptors
        int pids[num_auth_engines];
        engine_pids = (int *)malloc(num_auth_engines * sizeof(int));

         for (int i = 0; i < num_auth_engines+1; i++) {
            if (pipe(pipes[i]) == -1) {  // Create each pipe
                perror("Error creating pipe");
                exit(EXIT_FAILURE);  // Exit if creation fails
            }
            // After creating pipes, store the write ends for further use
        
            engine_pipe_fds_write[i] = pipes[i][1]; // Store write ends
        }
    
    write_log("UNNAMED PIPES CREATED!!\n");

    for(int i = 0; i < num_auth_engines;i++){
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error during fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {  // Child process
            close(pipes[i][1]);  // Close the write end in child
            authorization_engine(pipes[i][0], i, engine_sleep, shared_mem, auth_engine_manager,msgqid);  // Run engine with the read end
        }
        else {  // Parent process
            engine_pids[i] = pid;  // Store the process ID of the child process
            close(pipes[i][0]);  // Close the read end in the parent
        }
    }





        Queue video_streaming_queue;
        Queue other_services_queue;

        


        // Initialize the queues
        if (init_queue(&video_streaming_queue, max_size) != 0) {
        fprintf(stderr, "Failed to initialize video streaming queue.\n");
        return -1;
        }

        if (init_queue(&other_services_queue, max_size) != 0) {
        fprintf(stderr, "Failed to initialize other services queue.\n");
        return -1;
        }

        ThreadsData queues = {
            .video_streaming_queue = &video_streaming_queue,
            .other_services_queue = &other_services_queue,
            .engine_pipes = engine_pipe_fds_write,
            .num_engines = num_auth_engines,
            .auth_engine_manager = auth_engine_manager, // Assign Auth_Engine_Manager pointer
            .max_video_wait = video_time,
            .max_others_wait = others_time,
        };


        write_log("QUEUE VIDEO STREAMING CREATED!\n");
        write_log("QUEUE OTHER SERVICES CREATED!\n");

        // Tentar criar USER_PIPE
        if (mkfifo(USER_PIPE, 0666) == -1) {
            if (errno != EEXIST) { // Se o erro não for por já existir
                perror("Error creating USER_PIPE");
                exit(EXIT_FAILURE);
                }
            else{write_log("USER_PIPE already exists!\n");}
        } else {
            write_log("USER_PIPE CREATED.\n");
        }   

        // Tentar criar BACK_PIPE
        if (mkfifo(BACK_PIPE, 0666) == -1) {
            if (errno != EEXIST) { // Se o erro não for por já existir
                perror("Error creating BACK_PIPE");
                exit(EXIT_FAILURE);
                }
            else{write_log("BACK_PIPE ALREADY EXISTS!!\n");}
            }
        else {

            write_log("BACK_PIPE CREATED.\n");
            }   

        // Create thread Receiver
        

        

        
        res1 = pthread_create(&receiver, NULL, receiver_f, &queues);
        if (res1 != 0) {
            erro("Error creating thread receiver");
            exit(EXIT_FAILURE);
        }

        // Create thread Sender
        res2 = pthread_create(&sender, NULL, sender_f, &queues);
        if (res2 != 0) {
            erro("Error creating thread sender");
            exit(EXIT_FAILURE);
        }

        
        for (int i = 0; i < num_auth_engines; i++) {
        int status;
        waitpid(pids[i], &status, 0);}

        // Wait for threads to finish before exiting
        pthread_join(receiver, NULL);
        pthread_join(sender, NULL);

        // Exit child process
        exit(EXIT_SUCCESS);
    }

    // Parent process continues here
    //waitpid(auth_manager_process, NULL, 0); // Wait for child process to finish
    return auth_man_process;
    }


Auth_Engine_Manager* create_auth_engine_shared_memory(int* shmid, key_t key, int num_engines) {
    size_t shm_size = sizeof(Auth_Engine_Manager) + sizeof(int) * num_engines+1;
    *shmid = shmget(key, shm_size, IPC_CREAT | 0666);
    if (*shmid == -1) {
        perror("Error creating shared memory");
        return NULL;
    }
    Auth_Engine_Manager* shared_mem = (Auth_Engine_Manager*)shmat(*shmid, NULL, 0);
    if (shared_mem == (void*)-1) {
        perror("Error attaching to shared memory");
        return NULL;
    }

    
    pthread_mutexattr_t attr;
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_init(&attr);

    if (pthread_mutex_init(&shared_mem->mutex_flags, &attr) != 0) {
        perror("Error initializing mutex");
        return NULL;
    }

    pthread_mutexattr_destroy(&attr);  // Destroy the mutex attribute
    // Initialize shared memory
    // FLAGS  -1-INDISPONIVEL  0-DISPONIVEL 1-OCUPADO
 
    for (int i = 0; i < num_engines+1; i++) {
        shared_mem->auth_engine_flags[i] = -1;
    }
    shared_mem->num_engines=num_engines;

    return shared_mem;
}


SharedMemory *create_shared_memory(int *shmid, key_t key, int num_users) {
    // Calculate the size of the shared memory segment dynamically
    size_t shm_size = sizeof(SharedMemory) + sizeof(UserData) * num_users;

    // Create the shared memory segment with the given key
    *shmid = shmget(key, shm_size, IPC_CREAT | 0666);
    if (*shmid == -1) {
        perror("Error creating shared memory");
        return NULL;  // Return NULL on failure
    }

    // Attach to the shared memory segment
    SharedMemory *shared_mem = (SharedMemory *)shmat(*shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("Error attaching to shared memory");
        return NULL;  // Return NULL on failure
    }
    
    // Initialize the shared memory
    shared_mem->num_users = num_users;  // Set the number of users
    for (int i = 0; i < num_users; i++) {
        shared_mem->Users[i].UserID = -1;  // Initialize user data
        shared_mem->Users[i].PlafondInicial = -1;
        shared_mem->Users[i].SaldoAtualizado = -1;
        shared_mem->Users[i].alerta=0;
        //shared_mem->Users[i].alerta_anterior=0;
    }

    for (int i = 0; i < 3; i++) {
        shared_mem->stats[i].totaldata = 0;  // Initialize statistics
        shared_mem->stats[i].totalreq = 0;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    if (pthread_mutex_init(&shared_mem->mutex_user_info, &attr) != 0) {
        perror("Error initializing mutex");
        return NULL;
    }

    pthread_mutexattr_destroy(&attr);  // Destroy the mutex attribute
    // Initialize shared memory

    return shared_mem;  // Return the shared memory pointer
}

int main(int argc, char *argv[]){

    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1; // Exit with an error if filename is missing
    }

    char *configfile= argv[1];
    
    //limpa o ficheiro log 
    clear_file("log.txt");
    
    //Abre o semaforo 
    log_sem = sem_open("/log_semaphore", O_CREAT | O_EXCL, 0644, 1);

    if (sem_init(&semaphore1, 0, 1) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    int msgqid=create_ipcs();
    

    if (log_sem == SEM_FAILED) {
        if (errno == EEXIST) {
            // If the semaphore already exists, open it without creating
            log_sem = sem_open("/log_semaphore", 0);
            if (log_sem == SEM_FAILED) {
                perror("Error opening semaphore");
                exit(EXIT_FAILURE);
            }
        } else {
            // If the error is different from "File exists", report it
            perror("Error creating/opening semaphore");
            exit(EXIT_FAILURE);
        }
    }

    

     // Initialize the semaphore with 0 (initially, there are no requests)
    sem_init(&queue_sem, 0, 0);


    write_log("5G_AUTH_PLATFORM SIMULATOR STARTING...\n");


    write_log("PROCESS SYSTEM MANAGER CREATED\n");



    Configuration config=read_config(configfile);

    write_log("CONFIGURATION READ SUCCESSFULY\n");

    

    
    write_log("CREATING SHARED MEMORY... \n");

    key_t key1 = 1233; // Key for the first shared memory segment
    key_t key2 = 5679; // Key for the second shared memory segment


    // Create shared memory
	

    int num_users = config.mobile_users;
    int num_auth_engines = config.auth_servers;

    engines_sem = sem_open("/engines_semaphore", O_CREAT | O_EXCL, 0644, 1);

    if (engines_sem == SEM_FAILED) {
        if (errno == EEXIST) {
            // If the semaphore already exists, open it without creating
            engines_sem = sem_open("/engines_semaphore", 0);
            if (log_sem == SEM_FAILED) {
                perror("Error opening engines semaphore\n");
                exit(EXIT_FAILURE);
            }
        } else {
            // If the error is different from "File exists", report it
            perror("Error creating/opening engines semaphore\n");
            exit(EXIT_FAILURE);
        }
    }
    

    auth_engine_manager = create_auth_engine_shared_memory(&shmid1, key1, num_auth_engines); // 5 engine flags

    shared_mem = create_shared_memory(&shmid2, key2,num_users); // Basic shared memory

    if (auth_engine_manager == NULL || shared_mem == NULL) {
        fprintf(stderr, "Error initializing shared memory segments.\n");
        return EXIT_FAILURE;
    }
   
    auth_man_process = auth_requests_manager(config,shared_mem,auth_engine_manager,msgqid);
    monitor_engine(msgqid,shared_mem);
    
    signal(SIGINT, sigint_handler);
    
    waitpid(auth_man_process, NULL, 0); // Wait for child process to finish

    

    write_log("5G_AUTH_PLATFORM SIMULATOR CLOSING...\n");

    return 0;
}