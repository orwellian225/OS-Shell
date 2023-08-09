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
#define SEPERATOR_CHAR ' '

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
 * [p] Parallel Execution - Move the cmd into the background
 * [ ] Error handling - Write "An error has occurred\n" into stdout
 */

void mode_interactive(void);
void mode_batch(const char* batch_filepath);

void handle_line(char* cmdline_buffer);
void handle_cmd(char** cmd_tokens);

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

    char** tokens = NULL;
    char* current_token;
    size_t num_tokens = 1, token_idx = 0;

    cmdline_buffer[strcspn(cmdline_buffer, "\n")] = 0; //Remove newline char
    for (size_t i = 0; i < strlen(cmdline_buffer) - 1; ++i) { 
        if (cmdline_buffer[i] == SEPERATOR_CHAR) { ++num_tokens; }
    }
    tokens = malloc(num_tokens * sizeof(char*));
    while ((current_token = strsep(&cmdline_buffer, " ")) != NULL) {
        tokens[token_idx] = current_token; 
        ++token_idx;
    }
    free(cmdline_buffer);

    cmd_t* parallel_cmds = NULL;
    size_t num_cmds = 1;
    size_t cmd_idx = 0;
    for (size_t i = 0; i < num_tokens; ++i) { 
        if (strcmp(tokens[i], PARALLEL_TOKEN) == 0) { ++num_cmds; }
    }
    parallel_cmds = malloc(num_cmds * sizeof(cmd_t));

    size_t cmd_start_idx = 0;
    for (size_t i = 0; i < num_tokens; ++i) {
        // Trigger cmd seperation on parallel token or last token in list 
        // This will handle strings without parallel tokens and the last parallel
        // command that doesn't feature the parallel token to trigger its seperation
        if (strcmp(tokens[i], PARALLEL_TOKEN) == 0 || i == num_tokens - 1) { 
            parallel_cmds[cmd_idx] = (cmd_t){ malloc((i - cmd_start_idx + 1) * sizeof(char*)), i - cmd_start_idx + 1 };
            for (size_t j = 0; j < parallel_cmds[cmd_idx].argc; ++j) {
                parallel_cmds[cmd_idx].argv[j] = tokens[cmd_start_idx + j];
            }

            cmd_start_idx = i + 1;
            ++cmd_idx;
        }
    }

}
