/*
** http_server.cpp -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <fstream>
#include <iostream>
#include <string>

#define BUFFER_SIZE 1024    // how many bytes to be able to recv at once
#define BACKLOG 10   // how many pending connections queue will hold

using std::string;
using std::ifstream;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET_ADDRSTRLEN];
    int rv;

    if (argc != 2) {
        fprintf(stderr,"usage: %s port_number\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1] /* port */, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            
            // Get request
            char buf[BUFFER_SIZE];
            int bytesRecvd;
            if((bytesRecvd = recv(new_fd, buf, BUFFER_SIZE, 0)) < 0) {
                perror("recv failed");
                exit(1);
            }
            buf[bytesRecvd] = '\0';
    
            // Parse request
            string s(buf);
            int first_space = s.find(' ', 0);
            string cmd = s.substr(0, first_space);
            string filename = s.substr(first_space+1, s.find(' ', first_space+1)-first_space-1);
            if(cmd != "GET") {
                string bad_req = "HTTP/1.1 400 Bad Request\r\n\r\nSorry. Only the GET request is accepted, currently.";
                send(new_fd, bad_req.c_str(), bad_req.length(), 0);
            }
            
            // Read in file and send contents
            std::ifstream file(string(".") + filename.c_str(), std::ios::binary);
            if (!file.is_open()) {
                string not_found = "HTTP/1.1 404 Not Found\r\n\r\nOops! Content not found.";
                send(new_fd, not_found.c_str(), not_found.length(), 0);
            } else {
                string file_contents((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
                string response = "HTTP/1.1 200 OK\r\n\r\n";
                response = response + file_contents;
                send(new_fd, response.c_str(), response.length(), 0);
            }
            
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

