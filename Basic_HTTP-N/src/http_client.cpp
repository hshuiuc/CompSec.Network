/*
** http_client.cpp -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <fstream>
#include <iostream>
#include <string>
#include <regex>

#define MAXDATASIZE 1024

using namespace std;

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
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"usage: %s hostname_url\n", argv[0]);
        exit(1);
    }

    // split input url into respective parts
    string whole_url = argv[1];
    regex rgx("http://([a-zA-Z0-9.-]+):*([0-9]*)(/.*)");
    smatch matches;
    
    regex_search(whole_url, matches, rgx);
    string hostname = matches[1].str();
    string port = matches[2].str();
    if(port.empty())
        port = "80";
    string path = matches[3].str();

    // input parameters for getaddrinfo()
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("http_client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("http_client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "http_client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("http_client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    
    // Send request
    string request = "GET " + path + " HTTP/1.1\r\nHost: " + hostname + ":" + port + "\r\nConnection: close\r\n\r\n";
    int num_sent = send(sockfd, request.c_str(), request.length(), 0);
    if (num_sent < 0) {
        perror("client: send");
        exit(1);
    }
    
    // Read response
    string response = "";
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) > 0) {
        response += string(buf, numbytes);
    }
    if (numbytes == -1) {
        perror("recv");
        exit(1);
    }

    printf("client: received '%s'\n", response.c_str());

    close(sockfd);
    
    string body;
    string::size_type idx = response.find("\r\n\r\n");
    if (idx != string::npos) {
        body = response.substr(idx + 4);
    } else {
        printf("Could not find body in HTTP response\n");
        exit(1);
    }
    
    ofstream output;
    output.open("output");
    output << body;
    output.close();

    return 0;
}
