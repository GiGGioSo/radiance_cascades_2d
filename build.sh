#!/bin/sh

LIBS="-lglfw -ldl -lpthread -lm -lGL"
INCLUDES="-Iinclude/"

EXE="./bin/cascades"
SRCS="src/main.c src/glad.c"

rm -rf bin
mkdir bin

echo "Compiling..."
gcc -ggdb $LIBS $INCLUDES -o $EXE $SRCS
