#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

void parse_input(char *input, char **args) {
    /**
     * This function takes in a user command line and parses, separates them as tokens and places
     * them into an array of strings
     */
    int i = 0;
    args[i] = strtok(input, " \t\n");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        // Prints the name of the shell
        printf("CS340Shell%% ");
        fflush(stdout);

        // Reads the user input
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        // Remove newline character at end
        input[strcspn(input, "\n")] = '\0';

        
        // Pipes
        if (strchr(input, '|') != NULL) {
            char *commands[10];
            int num_cmds = 0;
            char *output_file = NULL;

            // Check for output redirection (any spacing)
            char *redir_pos = strchr(input, '>');
            if (redir_pos != NULL) {
                *redir_pos = '\0';
                redir_pos++;
                while (*redir_pos == ' ') redir_pos++;
                output_file = strtok(redir_pos, " \t\n");
            }

            commands[num_cmds] = strtok(input, "|");
            while (commands[num_cmds] != NULL && num_cmds < 9) {
                num_cmds++;
                commands[num_cmds] = strtok(NULL, "|");
            }

            int pipe_fd[2], prev_fd = -1;
            pid_t pids[10];

            for (int i = 0; i < num_cmds; i++) {
                char *cmd = commands[i];
                while (*cmd == ' ') cmd++;

                char *cmd_args[MAX_ARGS];
                parse_input(cmd, cmd_args);

                if (cmd_args[0] == NULL) {
                    fprintf(stderr, "Empty command in pipe segment %d\n", i + 1);
                    continue;
                }

                if (i < num_cmds - 1 && pipe(pipe_fd) < 0) {
                    perror("pipe failed");
                    exit(EXIT_FAILURE);
                }

                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork failed");
                    exit(EXIT_FAILURE);
                }

                if (pid == 0) {
                    if (prev_fd != -1) {
                        dup2(prev_fd, STDIN_FILENO);
                        close(prev_fd);
                    }

                    if (i < num_cmds - 1) {
                        close(pipe_fd[0]);
                        dup2(pipe_fd[1], STDOUT_FILENO);
                        close(pipe_fd[1]);
                    } else if (output_file != NULL) {
                        int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd_out < 0) {
                            perror("open output file failed");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd_out, STDOUT_FILENO);
                        close(fd_out);
                    }

                    execv(cmd_args[0], cmd_args);
                    perror("execv failed");
                    exit(EXIT_FAILURE);
                } else {
                    pids[i] = pid;
                    if (prev_fd != -1) close(prev_fd);
                    if (i < num_cmds - 1) {
                        close(pipe_fd[1]);
                        prev_fd = pipe_fd[0];
                    }
                }
            }

            for (int i = 0; i < num_cmds; i++) {
                waitpid(pids[i], NULL, 0);
            }

            continue;
        }

        
        

        // Calling the parse_input function -> refer to comments for function explanation
        parse_input(input, args);

        /**
         * Internal commands, uses the strcmp() function to compare two strings
         * IN THIS CASE, we are comparing args[0] to the internal command that gets called and 
         * checking if they're equal
         */

        // time
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // cd
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "cd: expected argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd failed");
                }
            }
            continue;
        }

        // time
        if (strcmp(args[0], "time") == 0) {
            time_t now = time(NULL);
            printf("%s", ctime(&now));
            continue;
        }

        char *input_file = NULL;
        char *output_file = NULL;
        int i = 0;
        
        // Checks for redirection by parsing the string to check for '<' or '>'
        while (args[i] != NULL) {
            // input
            if (strcmp(args[i], "<") == 0) {
                input_file = args[i + 1];
                args[i] = NULL; // terminate arguments before '<'
                i++;
            // output
            } else if (strcmp(args[i], ">") == 0) {
                output_file = args[i + 1];
                args[i] = NULL; // terminate arguments before '>'
                i++;
            }
            i++;
        }
        

        // Fork child process
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            continue;
        }

        if (pid == 0) {
            // === Child Process ===

            // Handle input redirection
            if (input_file != NULL) {
                int fd_in = open(input_file, O_RDONLY);
                if (fd_in < 0) {
                    perror("open input file failed");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_in, STDIN_FILENO);  // redirect stdin (fd 0)
                close(fd_in);
            }

            // Handle output redirection
            if (output_file != NULL) {
                int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    perror("open output file failed");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_out, STDOUT_FILENO);  // redirect stdout (fd 1)
                close(fd_out);
            }

            // Execute the command
            execv(args[0], args);
            perror("execv failed");
            exit(EXIT_FAILURE);
        } else {
            // === Parent Process ===
            waitpid(pid, NULL, 0);
        }
    }

    return 0;
}
