EXEC 		= de
CC			= cc
OBJ_DIR 	= obj
SUB_DIR		= doombsp render
INCL		= $(addprefix -I, $(SUB_DIR))

CFLAGS		= -g -std=gnu17
CFLAGS		+= -Wall -Wextra -Wno-missing-field-initializers -Wshadow
CFLAGS		+= -O0
LDFLAGS		= -lSDL2

SRC = $(wildcard *.c $(foreach fd, $(SUB_DIR), $(fd)/*.c))
OBJ = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC))


all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(EXEC)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCL) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(EXEC)