dev: main.c
	 @gcc -o main.exe main.c ./utils/window_setup.c -lm && ./main.exe
