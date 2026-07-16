default:
	gcc main.c matlib/*.c external/glad/glad.c -I. -I./external/ -lSDL3 -lm -g --std=c11
