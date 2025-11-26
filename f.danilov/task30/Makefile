UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),SunOS)
  LDLIBS = -lsocket -lnsl
endif

server: server.c
client: client.c

clean:
	rm -f server client