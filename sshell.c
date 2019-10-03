#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#define MAX_SIZE 512

enum {
    TOO_MANY_ARGS;
    NO_DIR; // EXEC
    NO_CMD; // EXEC
    MISSING_CMD;
    MISLOCATE_INPUT;
    NO_INPUT_FILE;
    CANT_OPEN_INPUT_FILE;
    MISTLOCATE_OUTPUT;
    NO_OUTPUT_FILE;
    CANT_OPEN_OUTPUT_FILE;
    MISLOCATE_BACKGROUND_SIGN;
    ACTIVE_JOBS_NOT_FINISH; //EXEC
};

typedef struct command {
    char **args; //array of argument, args[0] is filename
    char *inputfile;
    char *outputfile;
    int argnum;
    int ordernum;
    int totalcmd;
} singalcmd;

/*
 * Free memory allocate for each singalcmd object
 */
void _myfree(singalcmd cmd) {
    if(cmd.inputfile)
        free(cmd.inputfile);
    if(cmd.outputfile)
        free(cmd.inputfile);

    assert(cmd.args);
    if(cmd.args) {
        int i = 0;
        while(cmd.args[i]){
            free(cmd.args[i]);
            ++i;
        }
    }
    free(cmd.args);
}

/*
 * Free memory allocate for an array of singalcmd
 * singalcmd **cmds? Care of Memory issue!
 */
void myfree(singalcmd *cmds, int cmdnum) {
    for (int i = 0; i < cmdnum; ++i)
        _myfree(cmds[i]);
    free(cmds);
}

/*
 * This function is for build and check
 * initialize singal command
 * If input command string is not valid,
 * we output error message.
 */
bool initialize_singal_command(singalcmd *cmd, int ordernum, int totalnum) {

}

/*
 * Parse the source string
 * If source string is invalid,
 * we print error message and return false.
 * If source string is vaild,
 * we divide it into different commands and
 * call initialize_singal_command function to
 * build cmds
 */
bool parse_src_string(const char *src, singalcmd *cmds, int *cmdnum)
{
    char *dupcmd = malloc((strlen(src)+1) * sizeof(char));
    char *delimiters = "|";
    char *saveptr;
    char **tmp = malloc(strlen(src) * sizeof(char*));
    strcpy(dupcmd, src);

    (*cmdnum) = 0;
    for (char *token = strtok_r(dupcmd, delimiters, &saveptr); token != NULL ; token = strtok_r(NULL, delimiters, &saveptr)) {
        tmp[(*cmdnum)] = token;
        ++(*cmdnum);
    }

    cmds = malloc((*cmdnum) * sizeof(singalcmd));
    for (int i = 0; i < *cmdnum; ++i) {
        if(initialize_singal_command(&cmds[i], i, *cmdnum)) {
            myfree(cmds, *cmdnum);
            free(dupcmd);
            return false;
        }
    }

    free(dupcmd);
    return true;
}

int main(int argc, char *argv[])
{
    char *src = malloc(MAX_SIZE * sizeof(char)); // FREE
    //char **args = malloc(MAX_SIZE * sizeof(char*)); // FREE

    singalcmd *cmds = NULL;
    int cmdnum;

    do {
        printf("sshell$ ");

        size_t size = MAX_SIZE;
        __ssize_t charsnum = getline(&src, &size, stdin);
        if(charsnum == -1) // no input or error
            continue;
        src[charsnum-1] = '\0'; // delete trailing newline char

        if(!parse_src_string(src, cmds, &cmdnum))
            continue;

        assert(cmds);
        for (int i = 0; i < cmdnum; ++i) {
            if(strcmp(cmds[i].args[0], "exit") == 0)
                goto label;

            int pid = fork();
            int status;

            if (pid == 0)
                execvp(cmds[i].args[0], cmds[i].args);
            else {
                wait(&status);
                fprintf(stderr, "Return status value for '%s': %d\n", src, WEXITSTATUS(status));
            }
        }
        myfree(cmds, cmdnum);
        continue;
        /*
         * I know it is ugly.
         * Laugh me if you have better solution. :)
         */
        label:
            myfree(cmds, cmdnum);
            break;
    } while(true);

    free(src);

    fprintf(stderr, "Bye...\n");
    return EXIT_SUCCESS;
}