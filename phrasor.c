//
// Created by xinbochao on 19-10-6.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "phrasor.h"


/*
 * Free memory allocate for each command node
 * remember to free itself!
 */
void _myfree(command *cmd) {
    if(cmd->inputfile)
        free(cmd->inputfile);
    if(cmd->outputfile)
        free(cmd->outputfile);

    assert(cmd->args);
    if(cmd->args) {
        int i = 0;
        while(cmd->args[i]){
            free(cmd->args[i]);
            ++i;
        }
    }
    free(cmd->args);
    free(cmd);
}

/*
 * Free memory allocate for an array of command
 */
void myfree(command *header)
{
    if(!header)
        return;
    for (command *it = header; it != NULL ; it = header) {
        header = header->next;
        _myfree(it);
    }
}

/*
 * replace for  a == '|' || a == '&' || a == '<' || a == '>'
 */
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

/*
 * initialize a new command node and linked it to the linked list
 * remember to free args[i]! Otherwise, we cannot construct next command
 */
void initialize_command(command **header, char **args, char *inputfile, char *outputfile, bool background, command *prev,
                        command *next)
{
    (*header) = malloc(sizeof(command));
    (*header)->background = background;
    (*header)->prev = prev;
    (*header)->next = next;
    (*header)->status = -1; // use -1 indicate this value has not been modified

    if(!strlen(inputfile)){
        (*header)->inputfile = NULL;
    } else {
        (*header)->inputfile = malloc((strlen(inputfile) + 1) * sizeof(char));
        strcpy((*header)->inputfile, inputfile);
    }

    if(!strlen(outputfile)){
        (*header)->outputfile = NULL;
    } else {
        (*header)->outputfile = malloc((strlen(outputfile) + 1) * sizeof(char));
        strcpy((*header)->outputfile, outputfile);
    }

    int argnum = 0;
    while (args[argnum]) ++argnum;
    (*header)->args = malloc((argnum+1) * sizeof(char*));
    for (int i = 0; i < argnum; ++i) {
        (*header)->args[i] = malloc((strlen(args[i]) + 1) * sizeof(char));
        strcpy((*header)->args[i], args[i]);
        free(args[i]);
        args[i] = NULL;
    }
    (*header)->args[argnum] = NULL;
}

int get_next_nonspace_char_pos(const char *src, int index)
{
    while(isspace(src[++index]));
    return index;
}

/*
 * if there is no command behind '|', error!
 */
bool check_vertical_bar(const char *src, int index)
{
    int next = get_next_nonspace_char_pos(src, index);

    if(ischar(src[next], "|&<>") || next >= strlen(src)){
        fprintf(stderr, "Error: missing command\n");
        return false;
    }

    return true;
}

/*
 * if there are anything behind '&', error!
 */
bool check_ampersand(const char *src, int index)
{
    int next = get_next_nonspace_char_pos(src, index);

    if(next >= strlen(src))
        return true;

    fprintf(stderr, "Error: mislocated background sign\n");
    return false;
}

/*
 * clear memory for parsor function
 * Notice that if header == NULL,
 * it indicates we successfully construct command linked list.
 * Therefore, no need to free header.
 * If not, we need to free it and set it back to NULL
 */
void clear_mem(char **args, char *inputfile, char *outputfile, char *token, command **header) {
    int argnum = 0;
    while(args[argnum]){
        free(args[argnum]);
        ++argnum;
    }
    free(args);
    free(inputfile);
    free(outputfile);
    free(token);
    if(header) {
        myfree(*header);
        *header = NULL;
    }
}
/*
 * If token == NULL, do nothing and return false;
 * If token != NULL, copy token into args, free token,
 * and set it back to NULL.
 * args should be a null-terminated array of string
 */
bool write_back(char **args, char *token){
    if(strlen(token) == 0)
        return true;

    int argnum = 0;
    while(args[argnum]) ++argnum;
    if(argnum >= 16)
        return false;

    args[argnum] = malloc((strlen(token)+1) * sizeof(char));
    strcpy(args[argnum], token);
    token[0] = '\0';
    return true;
}
/*
 * search src string from position *index
 * if src[*index] is a delimiter, returns it, but not update *index
 * if src[*index] is a space, ignore it, searching next meaningful char
 * if that char is delimiter, returns it
 * if that char is the beginning of a token
 * we copy that token to the dest, now src[*index] is pointting at space or delimiter
 */
char my_strtok(const char *src, char *dest, const char *delimiters, int *index){
    if(ischar(src[*index], delimiters) || src[*index] == '\0')
        return src[(*index)];
    // ignore space, searching next char
    while(isspace(src[*index]))
        ++(*index);

    if(ischar(src[*index], delimiters))
        return src[(*index)];

    int destIndex = 0;
    while(src[*index] != '\0' && !ischar(src[*index], delimiters) && !isspace(src[*index])) {
        dest[destIndex] = src[*index];
        ++destIndex;
        ++(*index);
    }
    //trailing null character
    dest[destIndex] = '\0';
    return src[*index];
}

/*
 * check if redirect sign is valid or not
 * update index, and call my_strtok to get filename.
 * if token is null, error: no input/output file
 * if the special char after token is '|' or '\0',
 * we need to check mislocated error.
 * we also need to use fopen() to see if we have
 * access to that file
 */
bool check_redirect_sign(command **header, const char *cmd, char *token, const char *delimiters, int *index, char special) {
    assert(special == '<' || special == '>');
    ++(*index);
    char nextSpecial = my_strtok(cmd, token, delimiters, index);
    // if nextSpecial is space, we want to find next delimiter
    while(isspace(nextSpecial))
        nextSpecial = cmd[++(*index)];
    // check mislocated error
    if(nextSpecial == '|' && *header) { //command in middle
        if(special == '<')
            fprintf(stderr, "Error: mislocated input redirection\n");
        else
            fprintf(stderr, "Error: mislocated output redirection\n");
        return false;
    }

    if(nextSpecial == '|' && !(*header) && special == '>') { //first command in many command
        fprintf(stderr, "Error: mislocated output redirection\n");
        return false;
    }

    if(nextSpecial == '\0' && *header && special == '<') { // last command in many command
        fprintf(stderr, "Error: mislocated input redirection\n");
        return false;
    }

    // No filename
    if(!strlen(token)) {
        if(special == '<')
            fprintf(stderr, "Error: no input file\n");
        else
            fprintf(stderr, "Error: no output file\n");
        return false;
    }

    // Cannot open file
    if(special == '<' && !fopen(token, "r")) {
        fprintf(stderr, "Error: cannot open input file\n");
        return false;
    }

    if(special == '>' && !fopen(token, "w")) {
        fprintf(stderr, "Error: cannot open output file\n");
        return false;
    }

    return true;
}

/*
 * For '<', '>', we call their corresponding check function.
 * If everything is fine, we copy token(redirected filename)
 * into inputfile/outputfile.
 * Notice that index is updated in check function!
 */
bool handle_redirect_sign(command **header, const char *cmd, char *token, char special, const char *delimiters,
                          int *index, char *inputfile, char *outputfile)
{
    if(!check_redirect_sign(header, cmd, token, delimiters, index, special))
        return false;

    if(special == '<')
        strcpy(inputfile, token);
    else
        strcpy(outputfile, token);
    token[0] = '\0';
    return true;
}

/*
 * For '&', we need to check if there is anything behind it.
 * if any, & is misplaced.
 * otherwise, we begin to construct new command and insert it.
 * Notice, background flag is set to true.
 * No need to update index, because there is nothing left in cmd
 */
bool handle_ampersand(command **header, command **iter, const char *cmd, char **args,
                      char *inputfile, char *outputfile, int index)
{
    if(!check_ampersand(cmd, index))
        return false;

    //First command and only one command!
    if(!(*header)) {
        initialize_command(header, args, inputfile, outputfile, true, NULL, NULL);
    } else {
        assert(!(*iter)->next);
        initialize_command(&((*iter)->next), args, inputfile, outputfile, true, *iter, NULL);
        *iter = (*iter)->next; // problem here?
    }
    return true;
}

/*
 * For '|', we first need to check if there is command after it.
 * if not, command is missing.
 * otherwise, we begin to create new command and insert it into the linked list.
 * For '\0', we do not need to check.
 * update index, so that we will begin construct next command
 */
bool handle_vertical_bar_and_null(command **header, command **iter, const char *cmd,
                                  char **args, char *inputfile, char *outputfile, int *index)
{
    if(cmd[*index] != '\0' && !check_vertical_bar(cmd, *index))
        return false;

    //First command, but we dont know if it is the only one
    if(!(*header)) {
        initialize_command(header, args, inputfile, outputfile, false, NULL, NULL);
    } else {
        assert(!(*iter)->next);
        initialize_command(&((*iter)->next), args, inputfile, outputfile, false, *iter, NULL);
        *iter = (*iter)->next; // problem here?
    }
    ++(*index);
    return true;
}

/*
 * Parse input string / check its vaildity / build command linked list
 * index is updated inside the calling function(special/handle_*).
 * all the dynamic allocated memory will be free using clear_mem().
 * if we successfully build command linked list, we won't free
 * memory allocated for it(passing NULL indicates no need to free).
 */
bool parse_src_string(const char *cmd, command **header) {
    int index = 0;
    char *delimiters = "|&<>";
    int mallocSize = (int)strlen(cmd) * sizeof(char);
    char *token = malloc(mallocSize);
    char *inputfile = malloc(mallocSize);
    char *outputfile = malloc(mallocSize);

    // set allocated memory back to 0
    memset(token, 0, mallocSize);
    memset(inputfile, 0, mallocSize);
    memset(outputfile, 0, mallocSize);

    // args should be a NULL-terminated array of strings
    char **args = malloc((MAX_ARGUMENT_NUM + 1) * sizeof(char*));
    for (int i = 0; i <= MAX_ARGUMENT_NUM; ++i)
        args[i] = NULL;

    /*
     * iter indicates the last element in linked list,
     * convenient for us to insert new element.
     */
    command **iter = header;
    while(index <= strlen(cmd)) {
        // src[index] == special, take next token
        char special = my_strtok(cmd, token, delimiters, &index);

        if(!write_back(args, token)){
            fprintf(stderr, "Error: too many process arguments\n");
            clear_mem(args, inputfile, outputfile, token, header);
            return false;
        }

        if(isspace(special))
            continue;

        // if input are all spaces
        if(!args[0] && special == '\0')
            return false;

        /*
         * we find a delimiter before any input
         * it should be missing command
         */
        if(!args[0]) {
            fprintf(stderr, "Error: missing command\n");
            clear_mem(args, inputfile, outputfile, token, header);
            return false;
        }

        // depends on different delimiters, we take different actions
        if(special == '<' || special == '>') {
            if (!handle_redirect_sign(header, cmd, token, special, delimiters, &index, inputfile, outputfile)){
                clear_mem(args, inputfile, outputfile, token, header);
                return false;
            }
        } else if(special == '&') {
            if(!handle_ampersand(header, iter, cmd, args, inputfile, outputfile, index)){
                clear_mem(args, inputfile, outputfile, token, header);
                return false;
            }
        } else if(special == '|' || special == '\0') {
            if(!handle_vertical_bar_and_null(header, iter, cmd, args, inputfile, outputfile, &index)) {
                clear_mem(args, inputfile, outputfile, token, header);
                return false;
            }
        }
    }

    // success, no need to free header
    clear_mem(args, inputfile, outputfile, token, NULL);
    return true;
}