@echo off
SetLocal EnableDelayedExpansion

SET CFLAGS=-ggdb

SET LIBS=-LC:\dev\radiance_cascades_2d\lib -lglfw3 -lUser32 -lGdi32 -lShell32 -lmsvcrt -lopengl32
SET INCLUDES=-IC:\dev\radiance_cascades_2d\include

REM SET PREPROCESSOR_DEFINITIONS=-D _CRT_SECURE_NO_WARNINGS

SET EXE=.\bin\cascades.exe
SET EXE_INSTANT=.\bin\cascades_instant.exe

SET SRCS=.\src\main.c .\src\glad.c
SET SRCS_INSTANT=.\src\main_instant.c .\src\glad.c

RMDIR /s /q bin
MKDIR bin 

ECHO Compiling...
gcc %CFLAGS% %INCLUDES% %SRCS% -o %EXE% %LIBS%
IF %errorlevel% NEQ 0 exit /b %errorlevel%

ECHO Compiling instant...
gcc %CFLAGS% %INCLUDES% %SRCS_INSTANT% -o %EXE_INSTANT% %LIBS%
IF %errorlevel% NEQ 0 exit /b %errorlevel%
