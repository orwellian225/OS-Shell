#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "util.h"

#define True 1
#define False 0

// https://stackoverflow.com/questions/3301294/scanf-field-lengths-using-a-variable-macro-c-c
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define CMD_CHAR_MAX 1024


#define PARALLEL_TOKEN "&"
#define REDIRECT_TOKEN ">"

/**
 *
 * Tasks
 *
 * [p] Batch Mode Input - Specify a file as an arg and just execute the file
 * [x] Interactive Mode Input - Enter while loop waiting for execution
 * [ ] Built in commands
 *      [ ] exit - call exit() 
 *      [ ] cd - change the directory with chdir()
 *      [ ] path - overwrite the search directory by the specified args
 * [ ] Output Redirection - Move the ouput into a specified file, if it doesn't exist then create it
 * [ ] Parallel Execution - Move the cmd into the background
 * [ ] Error handling - Write "An error has occurred\n" into stdout
 */

void mode_interactive(void);
void mode_batch(const char* batch_filepath);

void handle_line(char* cmdline_buffer);
void handle_cmd(char** cmd_tokens);

typedef struct {
    size_t num_args;

    char* cmd_token;
    char** arg_tokens;

    /* bool has_redirect; */
    /* char* redirect_filepath; */
} cmd_tokens_t;

int main(int argc, char* argv[]) {
    if (argc == 1) {
        mode_interactive();
    } else if (argc == 2) {
        mode_batch(argv[1]);
    } else {
        const char* arg_error = "Incorrect arguments provided for witsh.\n";
        write(STDERR_FILENO, arg_error, strlen(arg_error));
    }
}

void mode_interactive(void) {
    char cmd_buffer[CMD_CHAR_MAX + 1];

    while(True) {
        fputs("witsh >> ", stdout);
        fgets(cmd_buffer, CMD_CHAR_MAX + 1, stdin);
        handle_line(cmd_buffer);
        cmd_buffer[0] = 0; // clear the command buffer
    }
}

void mode_batch(const char* batch_filepath) {
    printf("%s\n", batch_filepath);
}

void handle_line(char* cmdline_buffer) {

    char** tokens = malloc(0);
    size_t num_tokens = 0;
    char* current_token;

    // Remove new line symbol
    cmdline_buffer[strcspn(cmdline_buffer, "\n")] = 0;

    while ((current_token = strsep(&cmdline_buffer, " ")) != NULL) {
        ++num_tokens;
        size_t token_len = strlen(current_token);
        tokens = realloc(tokens, sizeof(void*));
        tokens[num_tokens - 1] = current_token; 
    }
    cmdline_buffer = NULL;

}
