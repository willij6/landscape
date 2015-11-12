# Huh, let's try and write a makefile
CC = gcc -Wall
LIBS = -lGL -lGLU -lglut -lm
SRC = viewer.c

view: data viewer
	./viewer

data: heights.txt drainage.txt

heights.txt: height_gen.py slopes
	./height_gen.py

drainage.txt: height_gen.py slopes
	./height_gen.py

viewer: $(SRC)
	$(CC) $(SRC) $(LIBS) -o viewer
