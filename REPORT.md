# ECS150 Project 1 Report

## Phase 1
  + We implement the fork+exec+wait structure to run the commands the hard way.
  ```C
do {
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
        fprintf(stderr, "Return status value for '%s': %d\n", cmd, 
            WEXITSTATUS(status));
    }
} while(true);
  ```
+ Basically we use pid to *fork* the process
+ for child process we use *execvp* to execute and exit
+ for parent process we use *wait* for waiting child finish execution
+ and use *WEXITSTATUS* to check the execute status and print on terminal.

## Phase 2
  + We implement two utility function to help reading the command.
  + *parse_cmd* helps parsing the command line and saves it in *args*
  + Returns false if and only if __args[0] == "exit"__
  ```C
bool parse_cmd(const char *cmd, char **args, int *argnum)
{
    char *dupcmd = malloc((strlen(cmd)+1) * sizeof(Byte)); // FREE
    char *saveptr;
    strcpy(dupcmd, cmd);

    (*argnum) = 0;
    for (char *token = strtok_r(dupcmd, " ", &saveptr); token != NULL ; token = strtok_r(NULL, " ", &saveptr)) {
        args[*argnum] = malloc((strlen(token)+1)* sizeof(char));
        strcpy(args[*argnum], token);
        ++(*argnum);
    }

    free(dupcmd);
    if(!strcmp(args[0], "exit"))
        return false;
    return true;
}
  ```
  + *check_validity_of_cmd* return true only if command is valid.
  + and print corresponding error messages into *stderr*

## Phrase 3
  + We implement a linked-list like struct __command__ to represent our input
  + because it can be well fit with the parsed token list from input.
  ```C
typedef struct command {
    char **args; //array of argument, args[0] is filename
    int inputfd;
    int outputfd;
    bool background;
    struct command *prev;
    struct command *next;
    int status;
} command;
  ```
  + Each command store the input into __args__ , and parse down to token
  + next token is stored in __next__, previous token is stored in __prev__.
  + *inputfd* and *outputfd* is for redirection i/o file descriptors.
  + *background* is to check the background mode.
  + *status* is for storing WEXITSTATUS and other error status.

## Phrase 4
  + We first handle the buildin command first
  ```C
bool execute_buildin_commands(job *first_job) {
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
  ```
  + There are two different cases
  + the first function is *execute_exit* to handle __exit__
  + we first need to check if there is any job eft in the linked list. 
  + If that's the case, we should print error message. 
  + If it is not, we free everything and quit.
  ```C
void execute_exit(job *first_job) {
    first_job->finished = true;
    if(first_job->next) {
        first_job->cmd->status = 256;
        printf("Error: active jobs still running\n");
        return;
    }

    myfree(first_job);
    fprintf(stderr, "Bye...\n");
    exit(EXIT_SUCCESS);
}
  ```
  + the second function is *execute_cd* to handle __cd__
  + first call *get_dest_dir* to get absolute path
  + then call *chdir* to change directory to that path
  ```C
void execute_cd(job *first_job) {
    // args[0] is "cd", args[1] should be filename
    first_job->finished = true;
    char *destDir = NULL;
    destDir = get_dest_dir(destDir, first_job->cmd->args[1]);

    int returnVal = chdir(destDir);
    free(destDir);
    // 0 indicates success
    if(!returnVal) {
        first_job->cmd->status = returnVal;
        return;
    }
    // returnVal -1 if fail
    fprintf(stderr, "Error: no such directory\n");
    first_job->cmd->status = 256; 
    // set status to 256, so that WEXITSTATUS(status) will return 1
}

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
  ```
  + the third function is *execute_pwd* to handle __pwd__
  ```C
void execute_pwd(job *first_job) {
    first_job->finished = true;
    char *currentDir = NULL;
    currentDir = getcwd(currentDir, MAX_SIZE * sizeof(char));

    if(currentDir) {
        printf("%s\n", currentDir);
        first_job->cmd->status = 0;
        free(currentDir);
        return;
    }
    //if getcwd fail, we set status 256 so that WEXITSTATUS(status) == 1
    first_job->cmd->status = 256;
}
  ```


