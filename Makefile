EXEC 		= de
CC			= cc
INCL 		= -Ipanels -Idoombsp

CFLAGS		= -g -std=gnu17
CFLAGS		+= -Wall -Wextra -Wno-missing-field-initializers -Wshadow
CFLAGS		+= -O0

LDFLAGS		= -lSDL2

SRC 		= $(wildcard *.c) $(wildcard panels/*.c) $(wildcard doombsp/*.c)
DEP 		= $(wildcard *.h) $(wildcard panels/*.h) $(wildcard doombsp/*.h)
OBJ_DIR 	= obj
OBJ 		= $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC))


all: obj_dir $(EXEC)

obj_dir:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/panels
	mkdir -p $(OBJ_DIR)/doombsp

$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(EXEC)

$(OBJ_DIR)/%.o: %.c $(DEP)
	$(CC) $(CFLAGS) $(INCL) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(EXEC)