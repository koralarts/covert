#GCC + FLAGS
GCC = gcc
FLAGS = -W -Wall -pedantic

#DIRECTORIES
SDIR = ./src
BDIR = ./bin

#RELEASE
release: server client

#SERVER
server:
	$(GCC) $(FLAGS) -o $(BDIR)/server $(SDIR)/server.c

#CLIENT
client:
	$(GCC) $(FLAGS) -o $(BDIR)/client $(SDIR)/client.c
        
#CLEAN
clean:
	rm -f $(BDIR)/*
        
#MAKE DIRECTORIES
dir:
	mkdir -p $(BDIR)
