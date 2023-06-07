CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -pedantic -Werror -g

all: proj2

proj2: proj2.c
	$(CC) $(CFLAGS) proj2.c -o proj2 -lpthread -lrt

clean:
	rm proj2 proj2.out
