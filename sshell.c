#define _XOPEN_SOURCE 700
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
#include <libgen.h>
#include "parsor.h"

#define MAX_SIZE 512
#define BUILDIN_FAILURE 256
#define BUILDIN_SUCCESS 0

/*
 * call getcwd to get current working directory
 * Notice that getcwd will allocate memory if success!
 * so we must free it at the end
 */
void execute_pwd(job *first_job)
{
    first_job->cmd->finished = true;
    char *currentDir = NULL;
    currentDir = getcwd(currentDir, MAX_SIZE * sizeof(char));

    if(currentDir) {
        printf("%s\n", currentDir);
        first_job->cmd->status = BUILDIN_SUCCESS;
        free(currentDir);
        return;
    }
    //if getcwd fail, we set status 256 so that WEXITSTATUS(status) == 1
    first_job->cmd->status = BUILDIN_SUCCESS;
}

/*
 * filename provided can be relative path,
 * we want to modify it to get absolut path
 * Notice that we do not need to implement
 * "cd -", "cd ~" commands.
 */
char * get_dest_dir(char *destDir, const char *filename)
{
    destDir = getcwd(destDir, MAX_SIZE * sizeof(char));
    // if filename is absolute path, nothing need to be done
    if(filename[0] == '/') {
        strcpy(destDir, filename); // dont directly return filename, it will cause mem_leak
        return destDir;
    }
    // for relative path, modify it according to different situations
    if(strcmp(filename, "..") == 0) {
        destDir = dirname(destDir);
    } else if(filename[0] == '.') {
        strcat(destDir, &filename[1]);
    } else {
        strcat(destDir, "/");
        strcat(destDir, filename);
    }
    return  destDir;
}

/*
 * first call get_dest_dir to get absolute path
 * and chdir. Remember to free destDir!
 */
void execute_cd(job *first_job)
{
    // args[0] is "cd", args[1] should be filename
    first_job->cmd->finished = true;
    char *destDir = NULL;
    destDir = get_dest_dir(destDir, first_job->cmd->args[1]);

    int returnVal = chdir(destDir);
    free(destDir);
    // 0 indicates success
    if(!returnVal) {
        first_job->cmd->status = BUILDIN_SUCCESS;
        return;
    }
    // returnVal -1 if fail
    fprintf(stderr, "Error: no such directory\n");
    first_job->cmd->status = BUILDIN_FAILURE; // set status to 256, so that WEXITSTATUS(status) will return 1
}

/*
 * For exit command, we first need to check if there is any job
 * left in the linked list. If that's the case, we should print
 * error message. If it is not, we free everything and quit
 */
void execute_exit(job *first_job)
{
    first_job->cmd->finished = true;
    if(first_job->next) {
        first_job->cmd->status = BUILDIN_FAILURE;
        printf("Error: active jobs still running\n");
        return;
    }

    myfree(first_job);
    fprintf(stderr, "Bye...\n");
    exit(EXIT_SUCCESS);
}

void launch_new_process(command *iter)
{
    if(iter->inputfd != -1) {
        dup2(iter->inputfd, STDIN_FILENO);
        close(iter->inputfd);
    }

    if(iter->outputfd != -1) {
        dup2(iter->outputfd, STDOUT_FILENO);
        close(iter->outputfd);
    }

    execvp(iter->args[0], iter->args);
    fprintf(stderr, "Error: command not found\n");
    exit(EXIT_FAILURE);
}

/*
 * buildin commands will not be called as background commands
 * Neither will they be a part of pipeline commands
 * So we handle them first
 */
bool execute_buildin_commands(job *first_job)
{
    if(strcmp(first_job->cmd->args[0], "exit") == 0) {
        execute_exit(first_job);
        return true;
    }

    if (strcmp(first_job->cmd->args[0], "cd") == 0) {
        execute_cd(first_job);
        return true;
    }

    if (strcmp(first_job->cmd->args[0], "pwd") == 0) {
        execute_pwd(first_job);
        return true;
    }

    return false;
}

/*
 * If the job is a background job, we use WNOHANG
 * to tell parent process not wait for that new process.
 * If the job is a foreground job, we do not use WNOHANG
 * If the pid return from waitpid is equal to child's pid,
 * it means that child finish, we therefore set the finished
 * flag of command to true.
 */
void wait_handler(job *first_job, command *currentCmd)
{
    int returnVal;
    int status = 0;
    if(first_job->background)
        returnVal = waitpid(currentCmd->pid, &status, WNOHANG);
    else
        returnVal = waitpid(currentCmd->pid, &status, 0);

    if(returnVal == currentCmd->pid) {
        currentCmd->finished = true;
        currentCmd->status = status;
    }
}

/*
 * Execute jobs stored in linked list header
 * For every command, we first exam whether it is bulletin command
 * If it is, execute them and return, because we can assume bullet command will not in pipeline.
 * If it is not, call execvp to execute them.
 * Notice that when command is exit, we need to free memory and exit right away!
 */
void execute_job(job *first_job)
{
    if(execute_buildin_commands(first_job))
        return;

    int fd[2];

    for (command *iter = first_job->cmd; iter != NULL; iter = iter->next) {
        //check if pipeline
        if(iter->next) {
            pipe(fd);
            iter->outputfd = fd[1];
            iter->next->inputfd = fd[0];
        }

        int pid = fork();

        if (pid == 0) {
            launch_new_process(iter);
        } else if(pid > 0){
            iter->pid = pid;
            wait_handler(first_job, iter);
        } else{
            perror("fork");
        }
        // close any inputfd or outputfd
        if(iter->inputfd != -1)
            close(iter->inputfd);
        if(iter->outputfd != -1)
            close(iter->outputfd);
    }

}

/*
 * read a single line from stdin
 */
bool readline(char *src)
{
    size_t size = MAX_SIZE;
    size_t charsnum = getline(&src, &size, stdin);
    //This is for test script
    if (!isatty(STDIN_FILENO)) {
        printf("%s", src);
        fflush(stdout);
    }
    // no input or error
    if(charsnum <= 1)
        return false;
    // replace '\n' with '\0'
    src[charsnum-1] = '\0';
    return true;
}

/*
 * output execute results
 */
void output(const char *src, const command *header)
{
    fprintf(stderr, "+ completed '%s' ", src);
    for (const command *it = header; it != NULL ; it = it->next) {
        fprintf(stderr, "[%d]", WEXITSTATUS(it->status));
    }
    fprintf(stderr, "\n");
}

/*
 * we go through every job in the linked list
 * check if the job is finished
 * if it is, we output them and release their memory
 * return new pointer to the first_job
 */
job* output_finished_job(job* first_job)
{
    if(!first_job)
        return first_job;

    job *tail = first_job;
    while(tail->next)
        tail = tail->next;

    for (job *it = tail; it != first_job; it = tail) {
        tail = tail->prev;
        if(!(it->finished))
            continue;
        //if job is finished
        output(it->src, it->cmd);
        //reconstruct linked list. tail->next == it
        tail->next = it->next;
        if(it->next)
            it->next->prev = tail;
        myfree(it);
    }

    if(first_job->finished) {
        output(first_job->src, first_job->cmd);
        job *tmp = first_job;
        first_job = first_job->next;
        free(tmp);
    }

    return first_job;
}

/*
 * we go through every element in the linked list,
 * find the correspondent child process and mark
 * it as finished
 */
void mark_child_finish(job *first_job, int terminatedChildPid, int status)
{
    for (job *jobIter = first_job; jobIter != NULL; jobIter = jobIter->next) {
        for (command *cmdIter = jobIter->cmd; cmdIter != NULL ; cmdIter = cmdIter->next) {
            if(cmdIter->pid != terminatedChildPid)
                continue;
            //cmdIter->pid == terminatedChildPid
            cmdIter->status = status;
            cmdIter->finished = true;
        }
    }
}

/*
 * we go through every job in the linked list.
 * For each job, check whether all of its child
 * processes is finished. If it is, we mark that
 * job as finished
 */
void mark_job_finished(job *first_job)
{
    for (job *jobIter = first_job; jobIter != NULL ; jobIter = jobIter->next) {
        bool isJobFinish = true;
        for (command *cmdIter = jobIter->cmd; cmdIter != NULL ; cmdIter = cmdIter->next) {
            if(cmdIter->finished)
                continue;
            isJobFinish = false;
            break;
        }
        jobIter->finished = isJobFinish;
    }
}

/*
 * check if any child process finished,
 * if it is, we check if that means any background
 * job finished.
 * We call waitpid with option flag "WNOHANG" to
 * exam if there is any terminated child process.
 * if it is not, return value of waitpid will be 0.
 * And we return to the main function
 */
void check_background_job(job *first_job)
{
    if(!first_job)
        return;
    int terminatedChildPid;
    int status;
    do{
        terminatedChildPid = waitpid(-1, &status, WNOHANG);
        if(terminatedChildPid > 0)
            mark_child_finish(first_job, terminatedChildPid, status);
    }while(terminatedChildPid > 0);
}

int main(int argc, char *argv[])
{
    job *first_job = NULL;
    // this loop only exit if command is "exit"
    do {
        check_background_job(first_job);
        mark_job_finished(first_job);
        first_job = output_finished_job(first_job);

        char *src = malloc(MAX_SIZE * sizeof(char));
        printf("sshell$ ");

        if(!readline(src))
            continue;

        //we first build next job from command line
        //and insert it as the first element in the linked list
        job *next_job = NULL;
        if(!parse_src_string(src, &next_job))
            continue;

        if(!first_job){
            first_job = next_job;
        } else {
            first_job->prev = next_job;
            next_job->next = first_job;
            first_job = next_job;
        }

        //if command is "exit", it will exit from this function
        execute_job(first_job);
    } while(true);
}