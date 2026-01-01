# Compilatore e flag
CC = gcc
# -Iinclude dice al compilatore di cercare i .h nella cartella include
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

# Cartelle
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
EX_DIR = examples

# Lista dei file sorgente della LIBRERIA (cobra.c, cobra_math.c)
LIB_SRCS = $(wildcard $(SRC_DIR)/*.c)
# Trasforma i .c in .o (file oggetto)
LIB_OBJS = $(LIB_SRCS:.c=.o)

# Nome dell'eseguibile finale
TARGET = $(BIN_DIR)/game

# Regola di default (cosa succede se scrivi solo "make")
all: create_dirs $(TARGET)

# Regola per creare l'eseguibile
# Compila il main.c collegandolo con gli oggetti della libreria
$(TARGET): $(EX_DIR)/main.c $(LIB_OBJS)
	$(CC) $(CFLAGS) $(EX_DIR)/main.c $(LIB_OBJS) -o $(TARGET)
	@echo "Compilazione completata! Esegui con: ./$(TARGET)"

# Regola generica per compilare i file .c della libreria in .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Crea la cartella bin se non esiste
create_dirs:
	@mkdir -p $(BIN_DIR)

# Pulisce tutto (utile prima di caricare su git se non hai il gitignore settato bene)
clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
	rm -rf $(BIN_DIR)
