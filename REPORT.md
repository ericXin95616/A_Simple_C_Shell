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
    for (char *token = strtok_r(dupcmd, " ", &saveptr); token != NULL ; 
        token = strtok_r(NULL, " ", &saveptr)) {
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
        strcpy(destDir, filename); // no directly return filename b/c mem_leak
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

## Phrase 5 and 6
  + Since input and output redirection implementation has lots of similarity.
  + We re-implement the parsing function *parse_src_string*
  + and set the delimiter __"|&<>"__ for different cases.
  + this function also check its vaildity and build command linked list.
  ```C
bool parse_src_string(const char *cmd, command **header) {
    int index = 0;
    char *delimiters = "|&<>";
    int mallocSize = (int)strlen(cmd) * sizeof(char);
    char *token = malloc(mallocSize);
    //input/output file descriptor, -1 indicates NULL file descriptor
    int inputfd = -1;
    int outputfd = -1;

    // set allocated memory back to 0
    memset(token, 0, mallocSize);

    // args should be a NULL-terminated array of strings
    char **args = malloc((MAX_ARGUMENT_NUM + 1) * sizeof(char*));
    for (int i = 0; i <= MAX_ARGUMENT_NUM; ++i)
        args[i] = NULL;

    /*
     * iter indicates the last element in linked list,
     * convenient for us to insert new element.
     */
    command *iter;
    while(index <= strlen(cmd)) {
        // src[index] == special, take next token
        char special = my_strtok(cmd, token, delimiters, &index);

        if(!write_back(args, token)){
            fprintf(stderr, "Error: too many process arguments\n");
            clear_mem(args, token, header);
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
            clear_mem(args, token, header);
            return false;
        }

        // depends on different delimiters, we take different actions
        if(special == '<' || special == '>') {
            if (!handle_redirect_sign(header, cmd, token, special, delimiters, 
                &index, &inputfd, &outputfd)){
                clear_mem(args, token, header);
                return false;
            }
        } else if(special == '&') {
            if(!handle_ampersand(header, &iter, cmd, args, &inputfd, 
                &outputfd, index)){
                clear_mem(args, token, header);
                return false;
            }
        } else if(special == '|' || special == '\0') {
            if(!handle_vertical_bar_and_null(header, &iter, cmd, args, 
                &inputfd, &outputfd, &index)) {
                clear_mem(args, token, header);
                return false;
            }
        }
    }

    // success, no need to free header
    clear_mem(args, token, NULL);
    return true;
}
  ```
  + After parsing command, we get file descriptor as int by *open* function
  + and store it in the struct command
  + and call *handle_redirect_sign* to give to correspond variable.
  ```C
bool handle_redirect_sign(command **header, const char *cmd, char *token,
                          char special, const char *delimiters,
                          int *index, int *inputfd, int *outputfd)
{
    //-1 indicates no file descriptor
    int fd = -1;
    if(!check_redirect_sign(header, cmd, token, delimiters, index, 
        special, &fd))
        return false;

    if(special == '<')
        *inputfd = fd;
    else
        *outputfd = fd;
    token[0] = '\0';
    return true;
}
  ```
  + the function *check_redirect_sign* for validation of the input
  + use *fopen* to check accessablitiy to file
  + use *my_strtok*, similar to *strtok* to get filename.
  ```C
check_redirect_sign(command **header, const char *cmd, char *token,
                    const char *delimiters,
                    int *index, char special, int *fd)
{
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

    if(nextSpecial == '|' && !(*header) && special == '>') { 
        //first command in many command
        fprintf(stderr, "Error: mislocated output redirection\n");
        return false;
    }

    if(nextSpecial == '\0' && *header && special == '<') { 
        // last command in many command
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

    // Check to see if we can open the file
    int _fd;
    if(special == '<') {
        _fd = open(token, O_RDONLY, 0644);
        if(_fd == -1) {
            fprintf(stderr, "Error: cannot open input file\n");
            return false;
        } else {
            *fd = _fd;
            return true;
        }
    }
    // when special == '>'
    _fd = open(token, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if(_fd == -1) {
        fprintf(stderr, "Error: cannot open output file\n");
        return false;
    } else {
        *fd = _fd;
        return true;
    }
}
  ```
  + we define *my_strtok* search src string from position *index
  + if src[*index] is a delimiter, returns it, but not update *index
  + if src[*index] is a space, ignore it, searching next meaningful char
  + if that char is delimiter, returns it
  + if that char is the beginning of a token
  + Copy the token to the dest, now src[*index]is pointing at space or delimiter
  ```C
char my_strtok(const char *src, char *dest, const char *delimiters, int *index){
    if(ischar(src[*index], delimiters) || src[*index] == '\0')
        return src[(*index)];
    // ignore space, searching next char
    while(isspace(src[*index]))
        ++(*index);

    if(ischar(src[*index], delimiters))
        return src[(*index)];

    int destIndex = 0;
    while(src[*index] != '\0' && !ischar(src[*index], delimiters) 
        && !isspace(src[*index])) {
        dest[destIndex] = src[*index];
        ++destIndex;
        ++(*index);
    }
    //trailing null character
    dest[destIndex] = '\0';
    return src[*index];
}
  ```

## Phrase 7
  + We implement it by *handle_vertical_bar_and_null* to read and __pipe__
  + *handle_vertical_bar_and_null* first check if there is command after it,
  + if not, command is missing.
  + otherwise, we begin to create new command and insert it into the linked list
  + For '\0', we do not need to check.
  + update index, so that we will begin construct next command.
  ```C
bool handle_vertical_bar_and_null(command **header, command **iter, 
        const char *cmd,char **args, int *inputfd, int *outputfd, int *index)
{
    if(cmd[*index] != '\0' && !check_vertical_bar(cmd, *index))
        return false;

    //First command, but we dont know if it is the only one
    if(!(*header)) {
        *header = initialize_command(*header, args, *inputfd, *outputfd, 
            false, NULL, NULL);
        *iter = *header;
    } else {
        assert(!(*iter)->next);
        (*iter)->next = initialize_command((*iter)->next, args, *inputfd, 
            *outputfd, false, *iter, NULL);
        *iter = (*iter)->next;
    }
    ++(*index);
    // set them back to initial value
    *inputfd = -1;
    *outputfd = -1;
    return true;
}
  ```
  + to implement pipeline after reading and parsing, we modify execute function
  ```C
void execute_commands(job *first_job) {
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
        }
        // close any inputfd or outputfd
        if(iter->inputfd != -1)
            close(iter->inputfd);
        if(iter->outputfd != -1)
            close(iter->outputfd);
    }
}
  ```

## Phrase 8

  + We implement 
