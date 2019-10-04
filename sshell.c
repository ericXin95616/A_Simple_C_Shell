#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#define MAX_SIZE 512

typedef struct command {
    char **args; //array of argument, args[0] is filename
    char *inputfile;
    char *outputfile;
    bool background;
    bool first_element; // allow input redirect
    bool last_element; // allow output redirect
    int argnum;
} singalcmd;

/*
 * Free memory allocate for each singalcmd object
 */
void _myfree(singalcmd *cmd) {
    if(cmd->inputfile)
        free(cmd->inputfile);
    if(cmd->outputfile)
        free(cmd->inputfile);

    assert(cmd->args);
    if(cmd->args) {
        int i = 0;
        while(cmd->args[i]){
            free(cmd->args[i]);
            ++i;
        }
    }
    free(cmd->args);
}

/*
 * Free memory allocate for an array of singalcmd
 * singalcmd **cmds? Care of Memory issue!
 */
void myfree(singalcmd **cmds, int cmdnum)
{
    for (int i = 0; i < cmdnum; ++i) {
        _myfree(cmds[i]);
        cmds[i] = NULL;
    }
}

bool ischar(const char a, const char * list)
{
    if(!list)
        return false;

    for (int i = 0; i < strlen(list); ++i){
        if(list[i] == a)
            return true;
    }
    return false;
}

bool check_command_validity(char *inputfile, char *outputfile, bool first_element, bool last_element, int argnum) {
    if(!first_element && inputfile){
        printf("Error: mislocated input redirection\n");
        return false;
    }

    if(!last_element && outputfile){
        printf("Error: mislocated output redirection\n");
        return false;
    }

    if(inputfile && !fopen(inputfile, "r")){
        printf("Error: cannot open input file\n");
        return false;
    }

    if(outputfile && !fopen(outputfile, "r")){
        printf("Error: cannot open output file\n");
        return false;
    }
    return true;
}

void initialize_command(singalcmd **cmd, char **args, char *inputfile, char *outputfile,
                        bool background, bool first_element, bool last_element, int argnum)
{
    (*cmd) = malloc(sizeof(singalcmd));
    (*cmd)->background = background;
    (*cmd)->first_element = first_element;
    (*cmd)->last_element = last_element;
    (*cmd)->argnum = argnum;

    (*cmd)->inputfile = malloc((strlen(inputfile) + 1) * sizeof(char));
    strcpy((*cmd)->outputfile, outputfile);
    (*cmd)->outputfile = malloc((strlen(outputfile) + 1) * sizeof(char));
    strcpy((*cmd)->outputfile, outputfile);

    (*cmd)->args = malloc(argnum * sizeof(char*));
    for (int i = 0; i < argnum; ++i) {
        (*cmd)->args[i] = malloc((strlen(args[i]) + 1) * sizeof(char));
        strcpy((*cmd)->args[i], args[i]);
    }
}

int get_next_nonspace_char_pos(const char *src, int index)
{
    while(isspace(src[++index]));
    return index;
}

int get_prev_nonspace_char_pos(const char *src, int index)
{
    while(isspace(src[--index]));
    return index;
}

bool check_vertical_bar(const char *src, int index)
{
    int prev = get_prev_nonspace_char_pos(src, index);
    int next = get_next_nonspace_char_pos(src, index);

    if(prev < 0 || next >= strlen(src)){
        printf("Error: missing command\n");
        return false;
    }

    if(src[prev] == '|' || ischar(src[next], "|&<>")){
        printf("Error: missing command\n");
        return false;
    }

    return true;
}

bool check_input_sign(const char *src, int index)
{
    int prev = get_prev_nonspace_char_pos(src, index);
    int next = get_next_nonspace_char_pos(src, index);

    if(prev < 0){
        printf("Error: missing command\n");
        return false;
    }

    if(next >= strlen(src) || ischar(src[next], "|&<>")){
        printf("Error: no input file\n");
        return false;
    }

    return true;
}

bool check_output_sign(const char *src, int index)
{
    int prev = get_prev_nonspace_char_pos(src, index);
    int next = get_next_nonspace_char_pos(src, index);

    if(prev < 0) {
        printf("Error: missing command\n");
        return false;
    }

    if(next >= strlen(src) || ischar(src[next], "|&<>")){
        printf("Error: no output file\n");
        return false;
    }

    return true;
}

bool check_ampersand(const char *src, int index) {
    int next = get_next_nonspace_char_pos(src, index);

    if(next >= strlen(src))
        return true;

    printf("Error: mislocated background sign\n");
    return false;
}

/*
 * Parse the source string
 * If source string is invalid,
 * we print error message and return false.
 * If source string is vaild,
 * we divide it into different commands and
 * call check_and_init_cmd function to
 * build cmds
 */
bool parse_src_string_and_build_cmds(const char *src, singalcmd **cmds, int *cmdnum)
{
    char *delimiters = "|&<>";

    char *inputfile = malloc(strlen(src) * sizeof(char));
    char *outputfile = malloc(strlen(src) * sizeof(char));

    char *arg = malloc((strlen(src) + 1) * sizeof(char));

    char **args = malloc(strlen(src) * sizeof(char*));
    for (int i = 0; i < strlen(src); ++i)
        args[i] = malloc((strlen(src)+1) * sizeof(char));

    (*cmdnum) = 0;
    int _argsindex = 0;
    int preindex = -1;
    int argindex = 0;
    bool input_redirect = false;
    bool output_redirect = false;
    bool background = false;

    while(preindex < strlen(src)) {
        int index = get_next_nonspace_char_pos(src, preindex);
        if(index >= strlen(src))
            break;

        if(((index > preindex + 1) || ischar(src[index], delimiters)) && argindex != 0){
            arg[argindex] = '\0';

            if(input_redirect){
                strcpy(inputfile, arg);
                input_redirect = false;
            } else if(output_redirect){
                strcpy(outputfile, arg);
                output_redirect = false;
            }else {
                strcpy(args[_argsindex], arg);
                ++_argsindex;
            }

            memset(arg, 0, (strlen(src) + 1) * sizeof(char));
            argindex = 0;
        } else if((index == preindex + 1) && !ischar(src[index], delimiters)){
            arg[argindex] = src[index];
            ++argindex;
        }

        switch(src[index]) {
            case '|':
                if(!check_vertical_bar(src, index))
                    return false;
                if(!check_command_validity(inputfile, outputfile, (*cmdnum) == 0, false, _argsindex))
                    return false;
                initialize_command(&(cmds[*cmdnum]), args, inputfile, outputfile, background, (*cmdnum) == 0, false, _argsindex);
                // back to the origin state
                ++(*cmdnum);
                _argsindex = 0;
                input_redirect = false;
                output_redirect = false;
                background = false;
                memset(inputfile, 0, strlen(src) * sizeof(char));
                memset(outputfile, 0, strlen(src) * sizeof(char));
                for (int i = 0; i < strlen(src); ++i)
                    memset(args[i], 0, (strlen(src)+1) * sizeof(char));
                break;
            case '<':
                if(!check_input_sign(src, index))
                    return false;
                input_redirect = true;
                break;
            case '>':
                if(!check_output_sign(src, index))
                    return false;
                output_redirect = true;
                break;
            case '&':
                if(!check_ampersand(src, index))
                    return false;
                background = true;
                break;
            default:
                break;
        }
        preindex = index;
    }

    if(!check_command_validity(inputfile, outputfile, (*cmdnum) == 0, true, _argsindex))
        return false;
    initialize_command(&(cmds[*cmdnum]), args, inputfile, outputfile, background, (*cmdnum) == 0, true, _argsindex);
    ++(*cmdnum);

    free(inputfile);
    free(outputfile);
    for (int i = 0; i < strlen(src); ++i)
        free(args[i]);
    free(args);

    return true;
}

int main(int argc, char *argv[])
{
    char *src = malloc(MAX_SIZE * sizeof(char)); // FREE
    //char **args = malloc(MAX_SIZE * sizeof(char*)); // FREE

    singalcmd **cmds = malloc(MAX_SIZE * sizeof(singalcmd*));
    int cmdnum;

    do {
        printf("sshell$ ");

        size_t size = MAX_SIZE;
        __ssize_t charsnum = getline(&src, &size, stdin);
        if(charsnum == -1) // no input or error
            continue;
        src[charsnum-1] = '\0'; // delete trailing newline char

        if(!parse_src_string_and_build_cmds(src, cmds, &cmdnum))
            continue;

        assert(cmds);
        for (int i = 0; i < cmdnum; ++i) {
            if(strcmp(cmds[i]->args[0], "exit") == 0)
                goto label;

            int pid = fork();
            int status;

            if (pid == 0)
                execvp(cmds[i]->args[0], cmds[i]->args);
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
    free(cmds);

    fprintf(stderr, "Bye...\n");
    return EXIT_SUCCESS;
}
