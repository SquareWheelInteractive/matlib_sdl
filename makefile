default:
	gcc -DDEBUG_INFO main.c matlib/*.c renderer/*.c external/glad/glad.c -I. -I./renderer/ -I./engine/ -I./external/ -lSDL3 -lm -O0 -g --std=c99
