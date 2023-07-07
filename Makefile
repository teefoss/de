INCL = -Ipanels -Idoombsp
SRC = *.c panels/*.c doombsp/*.c

all:
	cc -g $(SRC) $(INCL) -lSDL2 -o de
