#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define True 1
#define False 0

// https://stackoverflow.com/questions/3301294/scanf-field-lengths-using-a-variable-macro-c-c
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define CMD_CHAR_MAX 32

int main(int argc, char* argv[]) {
    
    char cmd_buffer[CMD_CHAR_MAX + 1];

    while(True) {
        fgets(cmd_buffer, CMD_CHAR_MAX + 1, stdin);

        cmd_buffer[0] = 0; // clear the command buffer
    }
}
