# ECS150 Project 2 Report

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
+ We also build utility functions:
+ *parse_cmd* help parsing the command line
+ *check_validity_of_cmd* return true only if command is valid.
