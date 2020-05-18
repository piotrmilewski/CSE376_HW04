CC= gcc 
CFLAGS= -g -O2 -Wall -Werror 
LDFLAGS= -lm  
GOBJS= protocol.o
SOBJS= exec.o jobs.o $(GOBJS)
COBJS= client.o $(GOBJS)

all: server client

clean:
	rm -f *.o *.a *.d server client1 client2a client2b client2c client3 domain_socket

client: clients $(COBJS)
	$(CC) -o client1 $(CFLAGS) client1.o $(COBJS) $(LDFLAGS)
	$(CC) -o client2a $(CFLAGS) client2a.o $(COBJS) $(LDFLAGS)
	$(CC) -o client2b $(CFLAGS) client2b.o $(COBJS) $(LDFLAGS)
	$(CC) -o client2c $(CFLAGS) client2c.o $(COBJS) $(LDFLAGS)
	$(CC) -o client3 $(CFLAGS) client3.o $(COBJS) $(LDFLAGS)

clients: $(GOBJS) client1.c client2a.c client2b.c client2c.c client3.c client.h
	$(CC) -c $(CFLAGS) client1.c
	$(CC) -c $(CFLAGS) client2a.c
	$(CC) -c $(CFLAGS) client2b.c
	$(CC) -c $(CFLAGS) client2c.c
	$(CC) -c $(CFLAGS) client3.c

server: server.o $(SOBJS)
	$(CC) -o server $(CFLAGS) server.o $(SOBJS) $(LDFLAGS)

server.o: $(GOBJS) server.c server.h
	$(CC) -c $(CFLAGS) server.c

client.o: $(GOBJS) client.c client.h
	$(CC) -c $(CFLAGS) client.c

protocol.o: protocol.c protocol.h
	$(CC) -c $(CFLAGS) protocol.c

exec.o: exec.c exec.h
	$(CC) -c $(CFLAGS) exec.c

jobs.o: jobs.c jobs.h
	$(CC) -c $(CFLAGS) jobs.c
