//
// Created by xinbochao on 19-10-6.
//

#ifndef SHELL_PHRASOR_H
#define SHELL_PHRASOR_H

#include <stdbool.h>
#define MAX_ARGUMENT_NUM 16

typedef struct command {
    char **args; //array of argument, args[0] is filename
    int inputfd;
    int outputfd;
    bool background;
    struct command *prev;
    struct command *next;
    int status;
} command;

bool parse_src_string(const char *cmd, command **header);
void myfree(command *header);

/* These are all the auxiliary functions:
 *
bool handle_vertical_bar_and_null(command **header, command **iter, const char *cmd,
                                  char **args, char *inputfile, char *outputfile, int *index);
bool check_vertical_bar(const char *src, int index);

bool handle_ampersand(command **header, command **iter, const char *cmd, char **args,
                      char *inputfile, char *outputfile, int index);
bool check_ampersand(const char *src, int index);

bool handle_redirect_sign(command **header, const char *cmd, char *token, char special, const char *delimiters,
                          int *index, char *inputfile, char *outputfile);
bool check_redirect_sign(command **header, const char *cmd, char *token, const char *delimiters, int *index, char special);

char my_strtok(const char *src, char *dest, const char *delimiters, int *index);
bool write_back(char **args, char *token);
void clear_mem(char **args, char *inputfile, char *outputfile, char *token, command **header);
void initialize_command(command **header, char **args, char *inputfile, char *outputfile, bool background,
                        command *prev, command *next);
 */
#endif //SHELL_PHRASOR_H
