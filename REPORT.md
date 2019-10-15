# ECS150 Project 1 Report
  + To briefly organize and be informative, we report data structure first.
  + For each Phrase(or Phase), we will report by functions or lines of code.

## Data Structure Design
  + We implement two data structure, __command__ and __job__
  + The data structure __command__ is a linked list to store a command.
  + A __command__ struct includes eight members:
    + __char **args__: array of argument stores command, _args[0]_ is filename.
    + __int inputfd__: store the input file descriptor in _int_ type.
    + __int outputfd__: store the output file descriptor in _int_ type.
    + __struct command *prev__: link to the previous command in pipeline case.
    + __struct command *next__: link to the next command in pipeline case.
    + __int status__: store the return status for error message.
    + __int pid__: store the Process ID for identifying forked child process.
    + __bool finished__: determine whether the process of this command is done.
    ```C
    typedef struct command {
        char **args;
        int inputfd;
        int outputfd;
        struct command *prev;
        struct command *next;
        int status;
        int pid;
        bool finished;
    } command;
    ```
  + The __job__ struct is a linked list to store a job, a line of command.
  + A __job__ struct includes six members:
    + __command *cmd__: store a linked list of command in that job(line).
    + __bool background__:determine whether it should work in background.
    + __bool finished__:determine whether the whole job(line) is executed.
    + __struct job *prev__:link to the previous job in the background case.
    + __struct job *next__:link to the next job in the background case.
    + __char *src__:store the whole job(line) in string for ending message.
    ```C
    typedef struct job {
        command *cmd;
        bool background;
        bool finished;
        struct job *next;
        struct job *prev;
        char *src;
    } job;
    ```

## Phase 1
  + We implement the fork+exec+wait structure to run the commands the hard way.
  + Basically we use pid to *fork* the process,
  + for child process we use *execvp* to execute and exit,
  + for parent process we use *wait* for waiting child finish execution,
  + and use *WEXITSTATUS* to check the execute status and print on terminal.

## Phase 2
  + To read the command, we implement it by some functions. 
  
  ### Functions
  + *readline*:
    + read a single line from stdin
    + if just enter and do not input any command, return false.
    + if enter any command, modify the char _'\n'_  to _'\0'_ and return true.
  ### Variables
  + *MAX_ARGUMENT_NUM*: maximum number of argument is 16.

## Phase 3
  + To parse the command, we implement it by some functions. 
  
  ### Functions
  + *parse_src_string*:
    + parse a job(line of input) into command.
    + use *my_strtok* to read a command until delimiter *"|&<>"*.
      + use *is_char* to detect char of delimiter
    + use *write_back* to write the token(if existed) to *args* in __command__
    + *parse_cmd* helps parsing the command line and saves it in *args*
    + Returns false if and only if __args[0] == "exit"__.

## Phrase 4
  + To execute the buildin commands, we implement it by some functions.

  ### Functions
  + *execute_commands*:
    + Generally execute a job.
    + Execute commands stored in linked list header
    + For every command, we first exam whether it is bulletin command
    + If it is, since bullet command will not in pipeline, execute and return:
      + *execute_buildin_commands*
        + buildin commands will not be called as background commands
        + Neither will they be a part of pipeline commands
        + There are two different cases
        + the first function is *execute_exit* to handle __exit__
          + we first check if there is any job eft in the linked list. 
          + If true, print error message. Otherwise free everything and quit.
        + the second function is *execute_cd* to handle __cd__
          + first call *get_dest_dir* to get absolute path
          + then call *chdir* to change directory to that path
        + the third function is *execute_pwd* to handle __pwd__
          + call *getcwd* to get current directory
          + if *getcwd* fail, set status 256 to set WEXITSTATUS(status) to 1.
    + If it is not, call execvp to execute them.

## Phrase 5 and 6
  + Since input and output redirection implementation has lots of similarity.
  + To redirect I/O, we implement functions in the child process.

  ### Functions
  + *launch_new_process*: 
    + set input file decriptor to STDIN_FILENO if inputfd != -1(as null)
    + set output file decriptor to STDOUT_FILENO if outputfd != -1(as null)
  + *handle_redirect_sign* in *parse_src_string*:
    + After parsing, get file descriptor by *open* then store in __command__
    + *check_redirect_sign*:
      + check if redirect sign is valid or not.

## Phrase 7
  + To read and execute pipe line, we define functions

  ### Functions
  + *handle_vertical_bar_and_null* in *parse_src_string*:
    + Read and handle pipe, first check if there is command after it.
    + If not, command is missing.
    + Otherwise, create new command and insert it into the linked list.
    + By *initialize_command*:
      + constructor of __command__
  + to implement pipeline after reading and parsing, modify execute function
  + initialize pipe read and write line, and pipe, and set the corresponding
  + Lines of code in *execute_commands*:
    ```C
    if(iter->next) {
        pipe(fd);
        iter->outputfd = fd[1];
        iter->next->inputfd = fd[0];
    }
    ```
    + This file descriptor setting is after phase 5 and 6 because
    + it is implemented in *execute_commands*

## Phrase 8
  + To implement background commend, we introduce __job__ and use functions.
  + A job is a line of code that can work in background.
  + A job is finished if all command in that line is finished.
  + Job is a linked list, so move to next job in execution we do
  + Piece of code in *execute_commands*:
    ```C
    if(!first_job){
        first_job = next_job;
    } else {
        first_job->prev = next_job;
        next_job->next = first_job;
        first_job = next_job;
    }
    ```
  ### Functions
  + *initialize_job*: 
    + constructor of struct __job__
    + allocate memory for newjob and initialize newjob's members' value.
  + *check_background_job*:
    + check if any child process finished,
    + if it is, we check if that means any background job finished.
    + using *waitpid(-1, &status, WNOHANG);*
    + flag "WNOHANG" to exam if there is any terminated child process.
    + if it is, call *mark_child_finish*:
      + loop and find the correspondent child process and mark as finished.
    + if it is not, return value of waitpid will be 0.
    + And then we return to the main function.
  + *mark_job_finished*:
    + mark job finished if all child process is marked as finished.
  + *output_finished_job*:
    + we go through every job in the linked list
    + check if the job is finished
    + if it is, we output them and release their memory
    + return new pointer to the first_job
  + *output*:
    + output execute results
  + *myfree*:
    + Free memory allocate for the job.
  + *_myfree*:
    + Free memory allocate for each command node and itself.

    
