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
#include "phrasor.h"

#define MAX_SIZE 512

/*
 * call getcwd to get current working directory
 * Notice that getcwd will allocate memory if success!
 * so we must free it at the end
 */
void execute_pwd(command *cmd) {
    char *currentDir = NULL;
    currentDir = getcwd(currentDir, MAX_SIZE * sizeof(char));

    if(currentDir) {
        printf("%s\n", currentDir);
        free(currentDir);
        return;
    }
    //if getcwd fail, we set status 256 so that WEXITSTATUS(status) == 1
    cmd->status = 256;
}

/*
 * filename provided can be relative path,
 * we want to modify it to get absolut path
 * Notice that we do not need to implement
 * "cd -", "cd ~" commands.
 */
char * get_dest_dir(char *destDir, const char *filename){
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
void execute_cd(command *cmd) {
    // args[0] is "cd", args[1] should be filename
    char *destDir = NULL;
    destDir = get_dest_dir(destDir, cmd->args[1]);

    int returnVal = chdir(destDir);
    free(destDir);
    // 0 indicates success
    if(!returnVal) {
        cmd->status = returnVal;
        return;
    }
    // returnVal -1 if fail
    fprintf(stderr, "Error: no such directory\n");
    cmd->status = 256; // set status to 256, so that WEXITSTATUS(status) will return 1
}

void launch_new_process(command *iter) {
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
bool execute_buildin_commands(command *header, char *src) {
    if(strcmp(header->args[0], "exit") == 0) {
        free(header);
        free(src);
        fprintf(stderr, "Bye...\n");
        exit(EXIT_SUCCESS);
    }

    if (strcmp(header->args[0], "cd") == 0) {
        execute_cd(header);
        return true;
    }

    if (strcmp(header->args[0], "pwd") == 0) {
        execute_pwd(header);
        return true;
    }

    return false;
}
/*
 * Execute commands stored in linked list header
 * For every command, we first exam whether it is bulletin command
 * If it is, execute them and return, because we can assume bullet command will not in pipeline.
 * If it is not, call execvp to execute them.
 * Notice that when command is exit, we need to free memory and exit right away!
 */
void execute_commands(command *header, char *src) {
    if(execute_buildin_commands(header, src))
        return;

    int fd[2];

    //check if the job need to be put in the background
    command *iter = header;
    while(iter->next) iter = iter->next;
    bool background = iter->background;

    for (iter = header; iter != NULL; iter = iter->next) {
        //check if pipeline
        if(iter->next) {
            pipe(fd);
            iter->outputfd = fd[1];
            iter->next->inputfd = fd[0];
        }

        int status;
        int pid = fork();

        if (pid == 0) {
            launch_new_process(iter);
        } else if(pid > 0){
            wait(&status);
            iter->status = status;
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
bool readline(char *src) {
    size_t size = MAX_SIZE;
    size_t charsnum = getline(&src, &size, stdin);
    // TODO: delete this code before handin
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
 * if exit true, output bye
 * if exit false, output execute results
 */
void output(const char *src, const command *header) {
    fprintf(stderr, "+ completed '%s' ", src);
    for (const command *it = header; it != NULL ; it = it->next) {
        fprintf(stderr, "[%d]", WEXITSTATUS(it->status));
    }
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    char *src = malloc(MAX_SIZE * sizeof(char)); // FREE
    command *header = NULL;

    // this loop only exit if command is "exit"
    do {
        printf("sshell$ ");

        if(!readline(src))
            continue;

        if(!parse_src_string(src, &header))
            continue;

        //if command is "exit", it will exit from this function
        execute_commands(header, src);

        output(src, header);
        myfree(header);
        header = NULL;
    } while(true);
}