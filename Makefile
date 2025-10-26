# Diretórios
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin

# Compilador e flags
CC = gcc
CFLAGS = -Wall -I$(INC_DIR) -std=c11
LDFLAGS = -lwiringPi -lsqlite3

# Arquivos
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

# Executável
EXEC = $(BIN_DIR)/driver

all: $(EXEC)

# Compilação do executável
$(EXEC): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $(EXEC) $(LDFLAGS)

# Compilação dos objetos
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Cria diretório bin se não existir
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Limpeza
clean:
	rm -f $(BIN_DIR)/*.o $(EXEC)

# Recompila tudo
rebuild: clean all
