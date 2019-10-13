//
// Created by bochao on 10/10/19.
//

#ifndef ECS150_PROJECT1_JOB_H
#define ECS150_PROJECT1_JOB_H

typedef struct command {
    char **args; //array of argument, args[0] is filename
    int inputfd;
    int outputfd;
    struct command *prev;
    struct command *next;
    int status;
    int pid;
} command;

typedef struct job {
    command *cmd;
    bool background;
    bool finished;
    struct job *next;
    struct job *prev;
    char *src;
} job;

void myfree(job *header);
void _myfree(command *cmd);
command *initialize_command(command *header, char **args, int inputfd, int outputfd, command *prev, command *next);
job *initialize_job(job *newjob, command *cmd, bool background, char *src);
#endif //ECS150_PROJECT1_JOB_H
