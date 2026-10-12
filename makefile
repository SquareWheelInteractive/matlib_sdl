default:
	gcc -DDEBUG_INFO main.c matlib/*.c renderer/*.c external/glad/glad.c -I. -I./renderer/ -I./engine/ -I./external/ -lSDL3 -lm -g --std=c99
