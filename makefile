CC = gcc
CFLAGS = -std=c99 -Wall -Werror

myShell : sshell.c phrasor.c phrasor.h
	$(CC) $(CFLAGS) -o myShell sshell.c phrasor.c

.PHONY : clean
clean :
	rm myShell