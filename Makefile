SRC = squirrel-functions.c pool.c ran2.c squirrelactor.c cellactor.c main.c 
LFLAGS=-lm
CC=mpicc

all: 
	$(CC) -o execute $(SRC) $(LFLAGS)

clean:
	rm -f execute pool.o ran2.o squirrel-functions.o main.o
