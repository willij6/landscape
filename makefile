# Huh, let's try and write a makefile
CC = gcc
LIBS = -lGL -lGLU -lglut -lm
SRC = main.c

view: heights.txt drainage.txt program
	./program


heights.txt: height_gen.py slopes
	./height_gen.py

drainage.txt: height_gen.py slopes
	./height_gen.py

program: $(SRC)
	$(CC) $(SRC) $(LIBS) -o program
