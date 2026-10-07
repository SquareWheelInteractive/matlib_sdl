default:
	gcc -DDEBUG_INFO main.c matlib/*.c engine/*.c external/glad/glad.c -I. -I./engine/ -I./external/ -lSDL3 -lm -g --std=c99
