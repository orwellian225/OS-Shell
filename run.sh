#!/bin/sh

cp ./src/main.c ./Wits-Shell-Tester/witsshell.c
gcc ./Wits-Shell-Tester/witsshell.c -o ./Wits-Shell-Tester/witsshell
cd ./Wits-Shell-Tester/
./test-witsshell.sh
