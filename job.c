//
// Created by bochao on 10/10/19.
//
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "job.h"

/*
 * initialize a new command node and linked it to the linked list
 * remember to free args[i]! Otherwise, we cannot construct next command
 */
command *initialize_command(command *header, char **args, int inputfd, int outputfd, command *prev, command *next)
{
    header = malloc(sizeof(command));
    header->prev = prev;
    header->next = next;
    header->status = -1; // use -1 indicate this value has not been modified
    header->inputfd = inputfd;
    header->outputfd = outputfd;
    header->finished = false;
    header->pid = -1; //-1 indicates this process has not been issued

    int argnum = 0;
    while (args[argnum]) ++argnum;
    header->args = malloc((argnum+1) * sizeof(char*));
    for (int i = 0; i < argnum; ++i) {
        header->args[i] = malloc((strlen(args[i]) + 1) * sizeof(char));
        strcpy(header->args[i], args[i]);
        free(args[i]);
        args[i] = NULL;
    }
    header->args[argnum] = NULL;
    return header;
}

/*
 * allocate memory for newjob and initialize newjob's members' value
 */
job *initialize_job(job *newjob, command *cmd, bool background, char *src)
{
    newjob = malloc(sizeof(job));
    newjob->cmd = cmd;
    newjob->background = background;
    newjob->src = src;
    newjob->finished = false;
    newjob->next = NULL;
    newjob->prev = NULL;
    return newjob;
}

/*
 * Free memory allocate for each command node
 * remember to free itself!
 */
void _myfree(command *cmd)
{
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
 * Free memory allocate for the job
 */
void myfree(job *finishedJob)
{
    if(!finishedJob)
        return;
    free(finishedJob->src);
    for (command *it = finishedJob->cmd; it != NULL ; it = finishedJob->cmd) {
        finishedJob->cmd = finishedJob->cmd->next;
        _myfree(it);
    }
    free(finishedJob);
}
