#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_SIZE 512

//char** generate_path_environ_var();
bool parse_cmd(const char *cmd, char **args, int *argnum);
bool check_validity_of_cmd(char **args, const int argnum);

int main(int argc, char *argv[])
{
        char *cmd = malloc((MAX_SIZE)*sizeof(Byte)); // FREE
        char **args = malloc(MAX_SIZE); // FREE
        //char **envp = generate_path_environ_var(); // FREE
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
                execvp(args[0], &args[1]);
            else {
                wait(&status);
                fprintf(stderr, "Return status value for '%s': %d\n", cmd, WEXITSTATUS(status));
            }
        } while(true);

        free(cmd);

        //free(envp[0]);
        //free(envp);
        int i = 0;
        while(args[i]){
            free(args[i]);
            ++i;
        }
        free(args);

        fprintf(stderr, "Bye...\n");
        return EXIT_SUCCESS;
}

/*
 * Generate an array of string for environment variable $PATH
 */
/*
char** generate_path_environ_var()
{
    char *tmp = getenv("PATH"); // FREE
    char *path = malloc((strlen("PATH=") + strlen(tmp)) * 8); // FREE
    sprintf(path, "PATH=%s", tmp);
    char *envp[] = {path, NULL};
    free(tmp);
    return envp;
}
 */
/*
 * Parse the command line
 * Get all the arguments and the number of argument
 * Note that args[0] is not the argument but program
 * name we need to find and exec
 * Returns false if and only if args[0] == "exit"
 */
bool parse_cmd(const char *cmd, char **args, int *argnum)
{
    char *dupcmd = malloc(MAX_SIZE* sizeof(Byte)); // FREE
    char *token;
    char *saveptr;
    strcpy(dupcmd, cmd);

    int i = 0;
    for ( token = strtok_r(dupcmd, " ", &saveptr); token != NULL ; token = strtok_r(NULL, " ", &saveptr)) {
        args[i] = token;
        printf("Token %d: %s\n", i, token);
        ++i;
    }

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