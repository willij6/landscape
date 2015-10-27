# Huh, let's try and write a makefile
CC = gcc
LIBS = -lGL -lGLU -lglut -lm
SRC = main.c

default: $(SRC)
	$(CC) $(SRC) $(LIBS) -o program
