default:
	gcc main.c matlib/matlib.c matlib/skeleton.c glad/glad.c -I. -lSDL3 -lm -g --std=c11
