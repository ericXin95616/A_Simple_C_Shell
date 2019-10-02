#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[])
{
        char *cmd = "/bin/date -u";
        char *originStr = malloc(100);
        strcpy(originStr, cmd);
        char **args = malloc(100);

        args[0] = strtok(originStr, " ");

        args[1] = strtok(NULL, " ");

        int pid = fork();
        int status;

        if(pid == 0)
            execl(args[0], args[1], (char*) NULL);

        waitpid(pid, &status, 0);
        fprintf(stdout, "Return status value for '%s': %d\n", cmd, WIFEXITED(status));

        return EXIT_SUCCESS;
}
