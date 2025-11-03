CC = g++
CFLAGS = -std=c++17 -Wall -g -Iinclude

# Targets
all: twmailer-server twmailer-client

# Server build
twmailer-server: obj/twmailer-server.o obj/TWMailerServer.o obj/MailStore.o obj/Utils.o
	$(CC) $(CFLAGS) -o twmailer-server obj/twmailer-server.o obj/TWMailerServer.o obj/MailStore.o obj/Utils.o

obj/twmailer-server.o: src/twmailer-server.cpp include/TWMailerServer.hpp include/MailStore.hpp include/Utils.hpp | obj
	$(CC) $(CFLAGS) -c src/twmailer-server.cpp -o obj/twmailer-server.o

obj/TWMailerServer.o: src/TWMailerServer.cpp include/TWMailerServer.hpp include/MailStore.hpp include/Utils.hpp | obj
	$(CC) $(CFLAGS) -c src/TWMailerServer.cpp -o obj/TWMailerServer.o

obj/MailStore.o: src/MailStore.cpp include/MailStore.hpp | obj
	$(CC) $(CFLAGS) -c src/MailStore.cpp -o obj/MailStore.o

obj/Utils.o: src/Utils.cpp include/Utils.hpp | obj
	$(CC) $(CFLAGS) -c src/Utils.cpp -o obj/Utils.o

# Client build
twmailer-client: obj/twmailer-client.o obj/TWMailerClient.o obj/Utils.o
	$(CC) $(CFLAGS) -o twmailer-client obj/twmailer-client.o obj/TWMailerClient.o obj/Utils.o

obj/twmailer-client.o: src/twmailer-client.cpp include/TWMailerClient.hpp include/Utils.hpp | obj
	$(CC) $(CFLAGS) -c src/twmailer-client.cpp -o obj/twmailer-client.o

obj/TWMailerClient.o: src/TWMailerClient.cpp include/TWMailerClient.hpp include/Utils.hpp | obj
	$(CC) $(CFLAGS) -c src/TWMailerClient.cpp -o obj/TWMailerClient.o

# Folder creation
obj:
	mkdir -p obj

# Clean target
clean:
	rm -f obj/*.o twmailer-server twmailer-client
