#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_SIZE 512

bool parse_cmd(const char *cmd, char **args, int *argnum);
bool check_validity_of_cmd(char **args, const int argnum);

int main(int argc, char *argv[])
{
        char *cmd = malloc(MAX_SIZE * sizeof(char)); // FREE
        char **args = malloc(MAX_SIZE * sizeof(char*)); // FREE

        int argnum;

        do {
            printf("sshell$ ");

            size_t size = MAX_SIZE;
            __ssize_t charsnum = getline(&cmd, &size, stdin);
            if(charsnum == -1) // no input or error
                continue;
            cmd[charsnum-1] = '\0'; // delete trailing newline char

            if(!parse_cmd(cmd, args, &argnum))
                break;

            if(!check_validity_of_cmd(args, argnum))
                continue;

            int pid = fork();
            int status;

            if (pid == 0)
                execvp(args[0], &args[0]);
            else {
                wait(&status);
                fprintf(stderr, "Return status value for '%s': %d\n", cmd, WEXITSTATUS(status));
            }

            for (int i = 0; i <= argnum ; ++i) {
                free(args[i]);
                args[i] = NULL; // seems redundant, delete and you screw everything!
            }

        } while(true);

        free(cmd);
        free(args);

        fprintf(stderr, "Bye...\n");
        return EXIT_SUCCESS;
}

/*
 * Parse the command line
 * Get all the arguments and the number of argument
 * Note that args[0] is not the argument but program
 * name we need to find and exec
 * Returns false if and only if args[0] == "exit"
 */
bool parse_cmd(const char *cmd, char **args, int *argnum)
{
    char *dupcmd = malloc((strlen(cmd)+1) * sizeof(Byte)); // FREE
    char *saveptr;
    strcpy(dupcmd, cmd);

    (*argnum) = 0;
    for (char *token = strtok_r(dupcmd, " ", &saveptr); token != NULL ; token = strtok_r(NULL, " ", &saveptr)) {
        args[*argnum] = malloc((strlen(token)+1)* sizeof(char));
        strcpy(args[*argnum], token);
        ++(*argnum);
    }

    free(dupcmd);
    if(!strcmp(args[0], "exit"))
        return false;
    return true;
}

/*
 * Return true if cmd is vaild / false if cmd is invalid
 * And print corresponding error messages into stderr
 */
bool check_validity_of_cmd(char **args, const int argnum)
{
    return true;
}
