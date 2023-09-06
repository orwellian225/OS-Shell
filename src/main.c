#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <ctype.h>

#define DEFAULT_PATH "/bin/"
#define DEFAULT_PATH_COUNT 1

#define CMD_CHAR_MAX 1024

#define PARALLEL_TOKEN "&"
#define REDIRECT_TOKEN ">"
#define SEPERATOR_CHAR ' '

#define EXIT_CMD "exit"
#define CD_CMD "cd"
#define PATH_CMD "path"

#define PRINT_ERROR fputs("An error has occurred\n", stderr);

#define bool uint8_t
#define true 1
#define false 0

/**
 *
 * Tasks
 *
 * [x] Batch Mode Input - Specify a file as an arg and just execute the file
 * [x] Interactive Mode Input - Enter while loop waiting for execution
 * [x] Built in commands
 *      [x] exit - call exit() 
 *      [x] cd - change the directory with chdir()
 *      [x] path - overwrite the search directory by the specified args
 * [x] External commands
 * [x] Output Redirection - Move the ouput into a specified file, if it doesn't exist then create it
 * [x] Parallel Execution - Move the cmd into the background
 * [p] Error handling - Write "An error has occurred\n" into stderr
 */

typedef struct {
    char** argv;
    size_t argc;
} cmd_t;

void mode_interactive(void);
void mode_batch(const char* batch_filepath);

bool is_incmd(cmd_t* cmd);
void strip_extra_spaces(char* string);

void handle_line(char* cmdline_buffer);
void handle_excmd(cmd_t* cmd); // External commands i.e. programs
void handle_incmd(cmd_t* cmd); // Internal commands

// GLOBAL VARIABLES - NO TOUCHY
char** search_paths;
size_t num_search_paths;

int main(int argc, char* argv[]) {
    search_paths = malloc(1 * sizeof(char*)); // Only one initial path entry
    search_paths[0] = DEFAULT_PATH;
    num_search_paths = DEFAULT_PATH_COUNT;

    if (argc == 1) {
        mode_interactive();
    } else if (argc == 2) {
        mode_batch(argv[1]);
    } else {
        PRINT_ERROR;
        exit(1);
    }
}

void mode_interactive() {
    char cmd_buffer[CMD_CHAR_MAX + 1];

    while(true) {
        char cwd[128];
        getcwd(cwd, 128);

        fputs("witsh: ", stdout); fputs(cwd, stdout); fputs(" >> ", stdout);
        fgets(cmd_buffer, CMD_CHAR_MAX + 1, stdin);
        
        handle_line(cmd_buffer);
        cmd_buffer[0] = 0; // clear the command buffer
    }
}

void mode_batch(const char* batch_filepath) {
    FILE* batch_file = fopen(batch_filepath, "r");
    char cmd_buffer[CMD_CHAR_MAX + 1];

    if (batch_filepath == NULL) {
        PRINT_ERROR;
    }

    while (fgets(cmd_buffer, CMD_CHAR_MAX + 1, batch_file) != NULL) {
        handle_line(cmd_buffer);
    }

    fclose(batch_file);
}

void handle_line(char* cmdline_buffer) {

    char** tokens = NULL;
    char* current_token;
    size_t num_tokens = 1, token_idx = 0;

    cmdline_buffer[strcspn(cmdline_buffer, "\n")] = 0; //Remove newline char
    strip_extra_spaces(cmdline_buffer);
    for (size_t i = 0; i < strlen(cmdline_buffer) - 1; ++i) { 
        if (cmdline_buffer[i] == SEPERATOR_CHAR) { ++num_tokens; }
    }
    tokens = malloc(num_tokens * sizeof(char*));
    while ((current_token = strsep(&cmdline_buffer, " ")) != NULL) {
        tokens[token_idx] = current_token; 
        ++token_idx;
    }
    free(cmdline_buffer);

    /** Error Handling - Parallel Commands
     *
     *  - ERORR CAUSE - EXPECTED OUTPUT
     *  - Only Parallel Command token found - no error - output 0
     *  - Parallel Token at end of single command - Just ignore last token
     *  - No whitespace between parallel token and command - split into seperate commands 
     */

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
        if (!is_incmd(&parallel_cmds[i])) {
            pid_t fork_result = fork();

            if (fork_result == 0) {
                handle_excmd(&parallel_cmds[i]);
                exit(EXIT_SUCCESS); // don't execute the rest of the loop iterations if it is a child
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
    
    // To handle redirects, change stdout to the relevant file to output to that
    // file and then remove the two tokens from cmd->argv and any that occur afterwards
    
    /** Error Handling - Redirection
     *
     *  - ERORR CAUSE - EXPECTED OUTPUT
     *  - Multiple Output files - stderr write "An error has occurred"
     *  - Multiple redirect tokens - stderr write "An error has occurred"
     *  - No command before redirect token - stderr write "An error has occurred"
     *
     */

    int saved_stdout = dup(fileno(stdout));
    for (size_t i = 0; i < cmd->argc; ++i) {
        if (strcmp(cmd->argv[i], REDIRECT_TOKEN) == 0) {
            FILE* output_file = fopen(cmd->argv[i + 1], "w"); 
            if (output_file == NULL) {
                PRINT_ERROR;
                return;
            }
            dup2(fileno(output_file), fileno(stdout));

            cmd->argc = i;
            char** new_argv = (char**)malloc(cmd->argc * sizeof(char*));
            for (size_t j = 0; j < cmd->argc; ++j) {
                new_argv[j] = cmd->argv[j];
            }
            free(cmd->argv);
            cmd->argv = new_argv;

            break;
        }
    }

    bool was_found_flag = false;
    for (size_t i = 0; i < num_search_paths; ++i) {

        size_t bin_filepath_length = strlen(search_paths[i]) + strlen(cmd->argv[0]) + 1;
        char* bin_filepath = malloc(bin_filepath_length * sizeof(char));
        strlcpy(bin_filepath, search_paths[i], bin_filepath_length);
        strlcat(bin_filepath, cmd->argv[0], bin_filepath_length);

        if (access(bin_filepath, X_OK) == 0) {
            execv(bin_filepath, cmd->argv);
            was_found_flag = true;
            break;
        }
    }

    if (!was_found_flag) {
        PRINT_ERROR;
    }

    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);

}

void handle_incmd(cmd_t* cmd) {
    if (strcmp(cmd->argv[0], EXIT_CMD) == 0) {
        if (cmd->argc > 1) {
            PRINT_ERROR;
            return;
        }

        exit(EXIT_SUCCESS);
    }

    if (strcmp(cmd->argv[0], CD_CMD) == 0) {
        if (cmd->argc != 2) {
            PRINT_ERROR;
            return;
        }

        if (chdir(cmd->argv[1]) == -1) {
            PRINT_ERROR;
        }
        return;
    }
    
    if (strcmp(cmd->argv[0], PATH_CMD) == 0) {
        // Clean up old memory
        for (size_t i = 0; i < num_search_paths; ++i) {
            free(search_paths[i]);
        }
        free(search_paths); 

        // If no path is specified, then default to bin
        if (cmd->argc == 1) {
            num_search_paths = 1;
            search_paths = malloc(num_search_paths * sizeof(char*));
            search_paths[0] = "/bin/";
            return;
        }

        
        num_search_paths = cmd->argc - 1;
        search_paths = malloc(num_search_paths * sizeof(char*));
        for(size_t i = 1; i < cmd->argc; ++i) {
            search_paths[i - 1] = cmd->argv[i];
        }

        return;
    }
}

bool is_incmd(cmd_t* cmd) {
    return strcmp(cmd->argv[0], EXIT_CMD) == 0 || 
            strcmp(cmd->argv[0], CD_CMD) == 0 || 
            strcmp(cmd->argv[0], PATH_CMD) == 0;
} 

// https://stackoverflow.com/questions/17770202/remove-extra-whitespace-from-a-string-in-c
void strip_extra_spaces(char* string) {
  size_t i, x;

  for(i=x=0; string[i]; ++i) {
    if(string[i] != SEPERATOR_CHAR || (i > 0 && string[i-1] != SEPERATOR_CHAR)) {
      string[x++] = string[i];
    }
  }

  string[x] = '\0';
}
