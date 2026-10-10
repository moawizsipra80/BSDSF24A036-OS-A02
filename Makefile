CC = gcc
CFLAGS = -Wall -g

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

TARGET = $(BIN_DIR)/ls
SRC = $(SRC_DIR)/lsv1.4.0.c
OBJ = $(OBJ_DIR)/lsv1.4.0.o
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/ls

run: all
	./$(TARGET)

.PHONY: all clean run