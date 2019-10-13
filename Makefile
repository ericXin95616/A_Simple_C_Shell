C = gcc
CFLAGS = -std=c99 -Wall -Werror


sshell : sshell.c parsor.c parsor.h job.c job.h
	$(CC) $(CFLAGS) -o sshell sshell.c parsor.c job.c

.PHONY : clean
clean :
	rm sshell
