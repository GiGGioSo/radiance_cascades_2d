#!/bin/sh

LIBS="-lglfw -ldl -lpthread -lm -lGL"
INCLUDES="-Iinclude/"

EXE="./bin/cascades"
EXE_INSTANT="./bin/cascades_instant"
SRCS="src/main.c src/glad.c"
SRCS_INSTANT="src/main_instant.c src/glad.c"

rm -rf bin
mkdir bin

echo "Compiling..."
gcc -ggdb $LIBS $INCLUDES -o $EXE $SRCS

echo "Compiling instant..."
gcc -ggdb $LIBS $INCLUDES -o $EXE_INSTANT $SRCS_INSTANT
