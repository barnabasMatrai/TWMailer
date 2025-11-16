# ---------------------------------
# COMPILER AND FLAGS
# ---------------------------------
CC = g++
CFLAGS = -std=c++17 -Wall -g -Iinclude
LDFLAGS = -pthread -lldap -llber

OBJDIR = obj

# ---------------------------------
# OBJECT FILES
# ---------------------------------

SERVER_OBJS = \
    $(OBJDIR)/twmailer-server.o \
    $(OBJDIR)/TWMailerServer.o \
    $(OBJDIR)/MailStore.o \
    $(OBJDIR)/Utils.o \
    $(OBJDIR)/AuthManager.o \
    $(OBJDIR)/Blacklist.o

CLIENT_OBJS = \
    $(OBJDIR)/twmailer-client.o \
    $(OBJDIR)/TWMailerClient.o \
    $(OBJDIR)/Utils.o

# ---------------------------------
# DEFAULT TARGET
# ---------------------------------
all: twmailer-server twmailer-client


# ---------------------------------
# SERVER BUILD
# ---------------------------------
twmailer-server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o twmailer-server $(SERVER_OBJS) $(LDFLAGS)

$(OBJDIR)/twmailer-server.o: src/twmailer-server.cpp \
                             include/TWMailerServer.hpp \
                             include/MailStore.hpp \
                             include/Utils.hpp \
                             include/AuthManager.hpp \
                             include/Blacklist.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/twmailer-server.cpp -o $(OBJDIR)/twmailer-server.o

$(OBJDIR)/TWMailerServer.o: src/TWMailerServer.cpp \
                            include/TWMailerServer.hpp \
                            include/MailStore.hpp \
                            include/Utils.hpp \
                            include/AuthManager.hpp \
                            include/Blacklist.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/TWMailerServer.cpp -o $(OBJDIR)/TWMailerServer.o

$(OBJDIR)/MailStore.o: src/MailStore.cpp include/MailStore.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/MailStore.cpp -o $(OBJDIR)/MailStore.o

$(OBJDIR)/Utils.o: src/Utils.cpp include/Utils.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/Utils.cpp -o $(OBJDIR)/Utils.o

$(OBJDIR)/AuthManager.o: src/AuthManager.cpp include/AuthManager.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/AuthManager.cpp -o $(OBJDIR)/AuthManager.o

$(OBJDIR)/Blacklist.o: src/Blacklist.cpp include/Blacklist.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/Blacklist.cpp -o $(OBJDIR)/Blacklist.o



# ---------------------------------
# CLIENT BUILD
# ---------------------------------
twmailer-client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o twmailer-client $(CLIENT_OBJS) $(LDFLAGS)

$(OBJDIR)/twmailer-client.o: src/twmailer-client.cpp \
                             include/TWMailerClient.hpp \
                             include/Utils.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/twmailer-client.cpp -o $(OBJDIR)/twmailer-client.o

$(OBJDIR)/TWMailerClient.o: src/TWMailerClient.cpp \
                            include/TWMailerClient.hpp \
                            include/Utils.hpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c src/TWMailerClient.cpp -o $(OBJDIR)/TWMailerClient.o



# ---------------------------------
# CREATE OBJ DIRECTORY
# ---------------------------------
$(OBJDIR):
	mkdir -p $(OBJDIR)



# ---------------------------------
# CLEAN
# ---------------------------------
clean:
	rm -f $(OBJDIR)/*.o twmailer-server twmailer-client
