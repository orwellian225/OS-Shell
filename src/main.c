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

#define DEFAULT_PATH "/bin/"
#define CMD_CHAR_MAX 1024

#define PARALLEL_TOKEN "&"
#define REDIRECT_TOKEN ">"
#define SEPERATOR_CHAR ' '

#define EXIT_CMD "exit"
#define CD_CMD "cd"
#define PATH_CMD "path"

/**
 *
 * Tasks
 *
 * [p] Batch Mode Input - Specify a file as an arg and just execute the file
 * [x] Interactive Mode Input - Enter while loop waiting for execution
 * [x] Built in commands
 *      [p] exit - call exit() 
 *      [x] cd - change the directory with chdir()
 *      [x] path - overwrite the search directory by the specified args
 * [ ] Output Redirection - Move the ouput into a specified file, if it doesn't exist then create it
 * [x] Parallel Execution - Move the cmd into the background
 * [ ] Error handling - Write "An error has occurred\n" into stdout
 */

typedef struct {
    char** argv;
    size_t argc;
} cmd_t;

void mode_interactive(void);
void mode_batch(const char* batch_filepath);

void handle_line(char* cmdline_buffer);
void handle_excmd(cmd_t* cmd); // External commands i.e. programs
void handle_incmd(cmd_t* cmd); // Internal commands

// GLOBAL VARIABLES - NO TOUCHY
char* search_path;

int main(int argc, char* argv[]) {
    search_path = malloc((strlen(DEFAULT_PATH) + 1) * sizeof(char));
    strlcpy(search_path, DEFAULT_PATH, strlen(DEFAULT_PATH) + 1);

    if (argc == 1) {
        mode_interactive();
    } else if (argc == 2) {
        mode_batch(argv[1]);
    } else {
        const char* arg_error = "Incorrect arguments provided for witsh.\n";
        write(STDERR_FILENO, arg_error, strlen(arg_error));
    }
}

void mode_interactive() {
    char cmd_buffer[CMD_CHAR_MAX + 1];

    while(True) {


        char cwd[128];
        getcwd(cwd, 128);
        fputs("witsh: ", stdout);
        fputs(cwd, stdout);
        fputs(" >> ", stdout);
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
                tokens[cmd_start_idx + j] = NULL;
            }

            cmd_start_idx = i + 1;
            ++cmd_idx;
        }
    }

    free(tokens);

    pid_t* child_pids = malloc(num_cmds * sizeof(pid_t));
    for (size_t i = 0; i < num_cmds; ++i) {
        if (strcmp(parallel_cmds[i].argv[0], EXIT_CMD) != 0 && 
            strcmp(parallel_cmds[i].argv[0], CD_CMD) != 0 && 
            strcmp(parallel_cmds[i].argv[0], PATH_CMD) != 0) {

            pid_t fork_result = fork();

            if (fork_result == 0) {
                handle_excmd(&parallel_cmds[i]);
                exit(EXIT_SUCCESS);
                break; // don't execute the rest of the loop iterations if it is a child
            } else {
                child_pids[i] = fork_result;
            }
        } else {
            handle_incmd(&parallel_cmds[i]);
        }
    }

    wait(NULL);
    free(parallel_cmds);
    free(child_pids);
}


void handle_excmd(cmd_t* cmd) {
    
}

void handle_incmd(cmd_t* cmd) {
    if (strcmp(cmd->argv[0], EXIT_CMD) == 0) {
        exit(EXIT_SUCCESS);
    }

    if (strcmp(cmd->argv[0], CD_CMD) == 0) {
        if (cmd->argc != 2) {
            // Error handling 
            return;
        }

        chdir(cmd->argv[1]);
        return;
    }
    
    if (strcmp(cmd->argv[0], PATH_CMD) == 0) {
        free(search_path); // Clean up old memory

        size_t new_path_length = 0;
        for (size_t i = 1; i < cmd->argc; ++i) {
            new_path_length += strlen(cmd->argv[i]) + 1;
        }

        search_path = malloc(new_path_length * sizeof(char));
        strlcpy(search_path, cmd->argv[1], new_path_length);
        strlcat(search_path, ":", new_path_length);

        for(size_t i = 2; i < cmd->argc; ++i) {
            strlcat(search_path, cmd->argv[i], new_path_length);
            strlcat(search_path, ":", new_path_length);
        }

        return;
    }
}
