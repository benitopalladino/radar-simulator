CC = gcc
CFLAGS = -Wall -Wextra -O2

all: radar_sim radar_server

radar_sim: radar_sim.c
	$(CC) $(CFLAGS) -o $@ radar_sim.c

radar_server: radar_server.c
	$(CC) $(CFLAGS) -o $@ radar_server.c

test: radar_server
	bash test.sh

clean:
	rm -f radar_sim radar_server *.class