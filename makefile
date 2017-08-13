mainmake: src/main.c
		mkdir -p bin
		cc -std=c99 -g -Wall -Werror-implicit-function-declaration src/main.c -lsqlite3 -lpcre -o bin/zeit