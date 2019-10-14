//
// Created by xinbochao on 19-10-6.
//

#ifndef PHRASOR_H
#define PHRASOR_H

#include <stdbool.h>
#include "job.h"

#define MAX_ARGUMENT_NUM 16

bool parse_src_string(char *cmd, job **header);

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
 */
#endif //PHRASOR_H
