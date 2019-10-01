#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
        char *cmd = "/bin/date -u";
        int retval;

        retval = system(cmd);
        fprintf(stdout, "Return status value for '%s': %d\n", cmd, retval);

        return EXIT_SUCCESS;
}
