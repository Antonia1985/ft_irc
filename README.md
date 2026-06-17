*This project has been created as part of the 42 curriculum by didimitr.*

ft_irc
An IRC server written in C++98.

It is built from the ground up using non-blocking sockets and a single poll() event
loop. You connect to it with a real IRC client (or nc) and can authenticate, set a
nickname, join channels, exchange private and channel messages, and run channel-operator
commands.


====================================================================
DESCRIPTION
====================================================================

Internet Relay Chat (IRC) is a text-based, real-time communication protocol. Clients
connect to a server to exchange direct messages and to join group channels where every
message is forwarded to all members.

This project implements the server side of that protocol. It does NOT implement an IRC
client and does NOT implement server-to-server communication. The goal is to understand
low-level network programming (TCP/IP, sockets, non-blocking I/O, event multiplexing) and
the IRC message format, by reproducing the behaviour a real IRC server exposes to a
reference client.

The server:
  - handles multiple clients simultaneously without blocking or hanging
  - performs all I/O through a single poll() loop (accept, read and write)
  - uses non-blocking file descriptors and never forks
  - supports a password, nicknames, usernames, channels, private/channel messaging, and
    the channel-operator command set (KICK, INVITE, TOPIC, MODE)


====================================================================
FEATURES
====================================================================

  - Password-protected connection (PASS)
  - Nickname and username registration (NICK, USER)
  - Channel creation and joining (JOIN)
  - Private messages and channel broadcasts (PRIVMSG)
  - Operators vs. regular users
  - Channel-operator commands: KICK, INVITE, TOPIC, MODE
  - Channel modes: i, t, k, o, l
  - Robust message framing: partial packets are buffered and reassembled; both CRLF and
    a bare LF line ending are accepted
  - Graceful client disconnection and clean shutdown (no leaks)


====================================================================
INSTRUCTIONS
====================================================================

Requirements
  - A C++ compiler supporting C++98 (c++ / g++ / clang++)
  - make
  - A Unix-like system (Linux or macOS)

Build

    make          builds the server               -> ./ircserv
    make client   builds the bundled test client  -> ./client
    make clean    removes object files
    make fclean   removes objects and binaries
    make re       fclean + build

The server is compiled with -Wall -Wextra -Werror -std=c++98.

Run the server

    ./ircserv <port> <password>

  <port>      the TCP port the server listens on (1-65535)
  <password>  the connection password every client must provide

Example:

    ./ircserv 6667 mypassword

Connect a client

The server has been tested with irssi (the reference client), with nc, and with the
bundled test_client.

  irssi:

    /connect 127.0.0.1 6667 mypassword
    /nick alice
    /join #general
    /msg #general hello everyone

  netcat (raw protocol, use -C so it sends CRLF):

    nc -C 127.0.0.1 6667

    PASS mypassword
    NICK alice
    USER alice 0 * :Alice
    JOIN #general
    PRIVMSG #general :hello everyone

  bundled test client:

    ./client 6667

  It connects to 127.0.0.1 on the given port, forwards what you type (terminated with
  CRLF), and prints the server's replies prefixed with "SERVER >".


====================================================================
SUPPORTED COMMANDS
====================================================================

    PASS      Provide the connection password           PASS mypassword
    NICK      Set or change the nickname                NICK alice
    USER      Set the username and realname             USER alice 0 * :Alice
    JOIN      Join (or create) a channel                JOIN #general
    PART      Leave a channel                           PART #general :bye
    PRIVMSG   Send a message to a user or channel       PRIVMSG #general :hi
    TOPIC     View or set a channel topic               TOPIC #general :Welcome
    MODE      View or change channel modes              MODE #general +itk key
    KICK      Eject a user from a channel (operator)    KICK #general bob :reason
    INVITE    Invite a user to a channel (operator)     INVITE bob #general
    QUIT      Disconnect from the server                QUIT :leaving
    PING      Keep-alive; the server replies with PONG  PING :token

Channel modes

    i    Invite-only channel
    t    Only operators may change the topic
    k    Channel key (password) required to join    MODE #c +k <key>
    o    Give / take channel-operator privilege     MODE #c +o <nick>
    l    User limit on the channel                  MODE #c +l <number>


====================================================================
USAGE EXAMPLE
====================================================================

    # client A
    PASS mypassword
    NICK alice
    USER alice 0 * :Alice
    JOIN #general                  -> alice creates #general and becomes operator

    # client B
    PASS mypassword
    NICK bob
    USER bob 0 * :Bob
    JOIN #general                  -> alice is notified that bob joined

    # client A
    PRIVMSG #general :hi bob       -> bob receives the message
    MODE #general +t               -> only operators can now change the topic
    TOPIC #general :Rules here     -> broadcast to the channel
    KICK #general bob :spam        -> bob is removed from the channel


====================================================================
PROJECT STRUCTURE
====================================================================

    main.cpp                  entry point, argument validation
    server.{cpp,hpp}          socket setup, the single poll() loop, accept/read/write,
                              cleanup
    parser.{cpp,hpp}          raw line -> ParsedMessage (command + params)
    parsedMessage.{cpp,hpp}   the parsed-message structure
    client.{cpp,hpp}          per-client state (fd, buffers, nick/user, joined channels)
    Channel.{cpp,hpp}         channel state (members, operators, modes, key, limit,
                              invites)
    commandHandler.{cpp,hpp}  command dispatch and implementation of every command
    errors.{cpp,hpp}          numeric replies and error messages
    test_client.cpp           a small raw client used for manual testing
                              (built with "make client")
    Makefile


====================================================================
TECHNICAL CHOICES
====================================================================

  - Single poll() loop. One poll() call watches the listening socket and every client
    socket for read AND write readiness. Reads happen on POLLIN; writes are queued per
    client and flushed on POLLOUT, so the server never blocks inside send().

  - Non-blocking sockets. The listening socket and every accepted socket are set to
    O_NONBLOCK (fcntl(fd, F_SETFL, O_NONBLOCK)); no fork() is used.

  - Message framing. Each client has an incoming buffer; bytes from recv() are appended
    and the buffer is split on line endings, so a command split across several packets is
    reassembled before being processed, and several commands arriving in one packet are
    all handled. Both CRLF and a lone LF are accepted.

  - C++98 only, no external or Boost libraries.


====================================================================
IRC CONCEPTS (REFERENCE)
====================================================================

A condensed summary of the protocol rules relevant to this project.

Client
  - A nickname is 9 characters max (used to distinguish clients from one another).

Server
  - A server must know, for every client: the real name of the host the client runs on,
    the client's username on that host, and the server the client is connected to.

Channels
  - A named group in which all members receive the messages addressed to that channel.
  - The name is 200 characters max.
  - The first character must be & or #.
  - The name cannot contain a space (' '), a control-G (^G, ASCII 7), or a comma (',').

User
  - A user may join several channels at once. A limit of 10 is recommended.

MODE (on join)
  - If the channel does not exist, it is created and the joining user becomes a channel
    operator.
  - If the channel exists, whether the user may join is decided by the channel's current
    modes. For example, if the channel is invite-only (+i), the user can only join if
    they were invited.


====================================================================
APPENDIX: SOCKET PROGRAMMING NOTES
====================================================================

Personal working notes on the networking API used by the server. (IP-address conversion
is included for completeness even though server and client run on the same machine here.)

A file descriptor is just an int.

Address structures

    struct addrinfo {
        int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
        int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
        int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
        int              ai_protocol;  // use 0 for "any"
        size_t           ai_addrlen;   // size of ai_addr in bytes
        struct sockaddr *ai_addr;      // struct sockaddr_in or _in6 (we usually use this)
        char            *ai_canonname; // full canonical hostname
        struct addrinfo *ai_next;      // linked list, next node
    };

    struct sockaddr {
        unsigned short    sa_family;    // address family, AF_xxx
        char              sa_data[14];  // 14 bytes of protocol address
    };
    // A call to getaddrinfo() to fill out your struct addrinfo for you is all you'll
    // need. No need to write it by hand.

    struct sockaddr_in {                // "in" is for internet
        short            sin_family;    // e.g. AF_INET
        unsigned short   sin_port;      // e.g. htons(3490)
        struct in_addr   sin_addr;      // see struct in_addr, below
        char             sin_zero[8];   // pads the structure to the length of a sockaddr
    };

    struct in_addr {
        unsigned long s_addr;           // load with inet_aton()
    };

You usually work on the struct sockaddr_in and then cast it at the last minute to a
struct sockaddr to use connect() or bind().

IP addresses

To convert IPs (IPv4) into a struct in_addr, use inet_pton (Presentation TO Network):

    inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));

The old way was inet_addr() or inet_aton(), but they are obsolete and don't work with
IPv6. To go the other way (network to presentation) for IPv4:

    char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
    struct sockaddr_in sa;      // pretend this is loaded with something

    inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
    printf("The IPv4 address is: %s\n", ip4);

inet_ntoa() was used instead of these macros in the past. This does not work with DNS
lookups like "www.example.com" -- for that you need getaddrinfo().

Setting up the address structures for a server listening on port 3490 (this only fills
out the structure, it does not listen yet):

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;       // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        // Note: gai_strerror cannot be used in ft_irc, but is handy for an early check
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    // ... use it until you no longer need it ...

    freeaddrinfo(servinfo);          // free the linked list

The socket lifecycle

    #include <sys/types.h>
    #include <sys/socket.h>

    int socket(int domain, int type, int protocol);

You feed socket() with the result of getaddrinfo():

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // ai_family   : AF_INET, AF_INET6, AF_UNSPEC
    // ai_socktype : SOCK_STREAM, SOCK_DGRAM
    // ai_protocol : 0 for any (or IPPROTO_TCP, IPPROTO_UDP, ...)

It returns a socket descriptor for later system calls.

bind() -- attach the socket to a local port (server side; a client only does connect()):

    int bind(int sockfd, struct sockaddr *my_addr, int addrlen);

    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;       // auto fill the host's IP

    getaddrinfo(NULL, "3490", &hints, &res);   // no DNS, listen on 3490
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bind(sockfd, res->ai_addr, res->ai_addrlen);

To stop the socket from hogging the port (which makes bind() fail on restart):

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

connect() -- used mostly by clients:

    int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);

As a client you don't need to bind(); the kernel gives you a port, and when you send() to
a server it gets what it needs to send() back.

listen() -- the server waits for incoming connections:

    int listen(int sockfd, int backlog);
    // sockfd  : the result of socket()
    // backlog : number of connections allowed in the queue

accept() -- accept a queued connection:

    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

The listening socket keeps listening; accept() returns a NEW socket on which you send()
and recv(). addr is local storage for the peer address. Full server sketch:

    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>

    #define MYPORT  "3490"  // the port users connect to
    #define BACKLOG 10      // pending-connection queue size

    int main(void)
    {
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        struct addrinfo hints, *res;
        int sockfd, new_fd;

        // !! don't forget error checking for these calls !!
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        getaddrinfo(NULL, MYPORT, &hints, &res);

        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        bind(sockfd, res->ai_addr, res->ai_addrlen);
        listen(sockfd, BACKLOG);

        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    }

send() / recv()

    int send(int sockfd, const void *msg, int len, int flags);

flags is usually 0. send() may not send everything at once -- check its return value and
send the rest if needed.

    int recv(int sockfd, void *buf, int len, int flags);

flags is usually 0. It returns the number of bytes read; 0 means the connection was
closed.

When finished, close() the socket descriptor (closesocket() on Windows). shutdown() gives
more control but is not needed here.

Non-blocking I/O and poll()

For ft_irc we cannot use a blocking accept(): while it waits it can do nothing else. We
set the descriptor to non-blocking right after creating it:

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

On its own, this would busy-wait and burn CPU. So instead we use poll() to ask which
descriptors are ready:

    #include <poll.h>

    int poll(struct pollfd fds[], nfds_t nfds, int timeout);

    struct pollfd {
        int   fd;        // file descriptor
        short events;    // requested events
        short revents;   // returned events
    };


====================================================================
RESOURCES
====================================================================

  - Beej's Guide to Network Programming
    https://beej.us/guide/bgnet/html/split/intro.html#intro

  - RFC 1459 (Internet Relay Chat Protocol)
    https://datatracker.ietf.org/doc/html/rfc1459

Use of AI

AI assistance was used during this project for the following tasks:

  - Testing: drafting a manual test plan covering registration, channels, the operator
    commands, and edge cases (partial packets, malformed input, abrupt disconnects).

  - Review and debugging: compiling under the required flags, running the server under
    valgrind, and stress-testing edge cases; identifying and applying a set of targeted
    fixes (for example: Makefile relinking, CRLF/LF line handling, propagation of a NICK
    change to channel members, and registration edge cases).

  - Documentation: restructuring this README.

The IRC protocol logic, the network implementation, and the overall architecture were implemented by the author;
AI was used as a reviewer and assistant, and all code is understood and owned by the
author(s).
