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

#define PARALLEL_TOKEN '&'
#define REDIRECT_TOKEN '>'
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
    char* redirect_file;
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

    //Specified batch file does not exist
    if (access(batch_filepath, F_OK) != 0) {
        PRINT_ERROR;
        exit(EXIT_FAILURE);
    }

    FILE* batch_file = fopen(batch_filepath, "r");
    char cmd_buffer[CMD_CHAR_MAX + 1];

    // Misc error handling
    if (batch_filepath == NULL ) {
        PRINT_ERROR;
        exit(EXIT_FAILURE);
    }

    while (fgets(cmd_buffer, CMD_CHAR_MAX + 1, batch_file) != NULL) {
        handle_line(cmd_buffer);
    }

    fclose(batch_file);
}

void handle_line(char* cmdline_buffer) {

    // EOF handling
    for(size_t i = 0; i < strlen(cmdline_buffer) - 1; ++i) {
        if (cmdline_buffer[i] == EOF) {
            fputs("\n", stdout); // to avoid weird formatting
            exit(EXIT_SUCCESS);
        }
    }

    // Command Line preprocessing
    cmdline_buffer[strcspn(cmdline_buffer, "\n")] = 0; //Remove newline char
    strip_extra_spaces(cmdline_buffer);

    if (strlen(cmdline_buffer) == 0) { return; } // Empty string handling
    if (cmdline_buffer[0] == PARALLEL_TOKEN && strlen(cmdline_buffer) == 1) {
        return;
    } // If only the Parallel Token is input, then just exit without an error
    if (cmdline_buffer[strlen(cmdline_buffer) - 1] == '&') {
        cmdline_buffer[strlen(cmdline_buffer) - 1] = '\0';
    }

    // Parallel Command Split
    char** parallel_cmdlines = NULL;
    char* current_cmd;
    size_t num_parallel_cmds = 1, parallel_cmd_idx = 0;
    
    for (size_t i = 0; i < strlen(cmdline_buffer) - 1; ++i) {
        if (cmdline_buffer[i] == PARALLEL_TOKEN) { ++num_parallel_cmds; }
    }

    parallel_cmdlines = malloc(num_parallel_cmds * sizeof(char*));
    
    while ((current_cmd = strsep(&cmdline_buffer, "&")) != NULL) {
        parallel_cmdlines[parallel_cmd_idx] = current_cmd;
        ++parallel_cmd_idx;
    }

    // Command token split
    cmd_t* parallel_cmds = malloc(num_parallel_cmds * sizeof(cmd_t));
    for (size_t i = 0; i < num_parallel_cmds; ++i) {
        char** tokens = NULL;
        char* current_cmdline;
        char* current_token;
        char* redirect_file = NULL;
        size_t num_tokens = 1, token_idx = 0;

        strip_extra_spaces(parallel_cmdlines[i]);

        current_cmdline = parallel_cmdlines[i];
        if (strchr(parallel_cmdlines[i], REDIRECT_TOKEN) != NULL) {
            current_cmdline = strsep(&parallel_cmdlines[i], ">");
            redirect_file = strsep(&parallel_cmdlines[i], ">");

            // If no command is specified before the redirect
            if (strlen(current_cmdline) == 0) {
                PRINT_ERROR;
                return;
            }

            strip_extra_spaces(current_cmdline);
            strip_extra_spaces(redirect_file);


            // If anymore redirect tokens are found then throw an error
            // Or if there are more than one specified output files
            if (strsep(&parallel_cmdlines[i], ">") != NULL || strchr(redirect_file, SEPERATOR_CHAR) != NULL) {
                PRINT_ERROR;
                return;
            }

        }

        for (size_t j = 0; j < strlen(current_cmdline) - 1; ++j) { 
            if (current_cmdline[j] == SEPERATOR_CHAR) { ++num_tokens; }
        }

        tokens = malloc(num_tokens * sizeof(char*));
        while ((current_token = strsep(&current_cmdline, " ")) != NULL) {
            tokens[token_idx] = current_token; 
            ++token_idx;
        }

        parallel_cmds[i].argv = tokens;
        parallel_cmds[i].argc = num_tokens;
        parallel_cmds[i].redirect_file = redirect_file;

    }
    free(parallel_cmdlines);

    /** Error Handling - Parallel Commands
     *
     *  - ERORR CAUSE - EXPECTED OUTPUT
     *  - Only Parallel Command token found - no error - just return
     *  - Parallel Token at end of single command - Just ignore last token
     *  - No whitespace between parallel token and command - split into seperate commands 
     */

    pid_t* child_pids = (pid_t*) malloc(num_parallel_cmds * sizeof(pid_t));
    for (size_t i = 0; i < num_parallel_cmds; ++i) {
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

    for (size_t i = 0; i < num_parallel_cmds; ++i) {
        waitpid(child_pids[i], NULL, 0);
    }

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

    if (cmd->redirect_file != NULL) {
        FILE* output_file = fopen(cmd->redirect_file, "w"); 
        if (output_file == NULL) {
            PRINT_ERROR;
            return;
        }
        dup2(fileno(output_file), fileno(stdout));
    }

    // No command is found - done after redirect to handle case where a redirect might not have a command
    if (cmd->argc == 0) { 
        PRINT_ERROR;
        return;
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
        free(search_paths); 

        num_search_paths = cmd->argc - 1;
        search_paths = malloc(num_search_paths * sizeof(char*));
        for(size_t i = 1; i < cmd->argc; ++i) {

            // Check there exists a forward slash in as the last char in the string
            // i.e. checking its a valid path
            if (cmd->argv[i][strlen(cmd->argv[i]) - 1] == '/') {
                size_t new_path_len = strlen(cmd->argv[i]) + 1;
                search_paths[i - 1] = (char*) malloc(new_path_len * sizeof(char));
                strlcpy(search_paths[i - 1], cmd->argv[i], new_path_len);
            } else {
                size_t new_path_len = strlen(cmd->argv[i]) + 2;
                search_paths[i - 1] = (char*) malloc(new_path_len * sizeof(char));
                strlcpy(search_paths[i - 1], cmd->argv[i], new_path_len);
                strlcat(search_paths[i - 1], "/", new_path_len + 1);
            }
            
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

  size_t end = strlen(string) - 1;
  while (string[end] == SEPERATOR_CHAR) {
    --end;
  }
  string[end + 1] = '\0';
}
