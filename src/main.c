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

const char** split_buffer(const char* buffer, const size_t len, const char split_char);

int main(int argc, char* argv[]) {
    
    char cmd_buffer[CMD_CHAR_MAX + 1];

    while(True) {
        fgets(cmd_buffer, CMD_CHAR_MAX + 1, stdin);
        printf("COMMAND: %s", cmd_buffer);
        const char** result = split_buffer(cmd_buffer, CMD_CHAR_MAX + 1, ' ');

        cmd_buffer[0] = 0; // clear the command buffer
    }
}

const char** split_buffer(const char* buffer, const size_t len, const char split_char) {

    size_t word_count = 0;
    for (size_t i = 0; i <= len; ++i) {
        if (buffer[i] == split_char || i == len - 1) { 
            ++word_count;
        }

        if (buffer[i] == '\0') { break; }
    }

    printf("Word Count: %ld\n", word_count);

    return NULL;
}
