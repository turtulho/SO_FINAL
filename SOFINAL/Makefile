# Compilador
CC = gcc

# Opções de compilação
CFLAGS =  -Wall -Wextra -pedantic -std=c99

# Opções de enlace
LFLAGS =  -lpthread -lrt

# Diretório de inclusão
INC_DIR = .

# Nome do arquivo header
HEADER = dados.h

# Lista de arquivos fonte
SRCS = backofficeUser.c mobileUser.c

# Adiciona SystemManager.c e Queues.c à lista de arquivos fonte
SRCS += SystemManager.c Queues.c

# Lista de arquivos objeto
OBJS = $(SRCS:.c=.o)

# Alvos para os executáveis
TARGETS = backofficeUser mobileUser

# Adiciona SystemManager e Queues aos alvos
TARGETS += SystemManager Queues

# Regras para construir os executáveis
all: $(TARGETS)

# Alvo para backofficeUser
backofficeUser: backofficeUser.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

# Alvo para mobileUser
mobileUser: mobileUser.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

# Alvo para SystemManager
SystemManager: SystemManager.o Queues.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

# Alvo para Queues
Queues: Queues.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

# Regra de compilação de cada arquivo fonte para um arquivo objeto
%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Regra para limpar os arquivos objeto e os executáveis
clean:
	rm -f $(OBJS) $(TARGETS)