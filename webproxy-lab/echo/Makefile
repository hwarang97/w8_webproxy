CC = gcc
CFLAGS = -Wall -Wextra -g

all: echoclient echoserver

echoclient: echoclient.c csapp.c
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.c

echoserver: echoserver.c csapp.c echo.c
	$(CC) $(CFLAGS) -o echoserver echoserver.c csapp.c echo.c

clean:
	rm -f echoclient echoserver *.o
