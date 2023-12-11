#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

#define MSG_NOT_ARGUMENTS "arguments should be 2: host file\n"
#define BUF_SIZE 512

void syscallError(char message[]){
    perror(message);
    exit(EXIT_FAILURE);
}

int checkHost(char* host, struct addrinfo* output){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char buf[BUF_SIZE];

    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(host, NULL, &hints, &result);

    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    memcpy(output,result,400);
	
}

int main(int argc, char* argv[]){
    if (argc != 3){
        write(STDOUT_FILENO,MSG_NOT_ARGUMENTS,34);
        exit(EXIT_FAILURE);
    }
    char* host = argv[1];
    char* filename = argv[2];
    struct addrinfo info;

    checkHost(host,&info);
    
    int sock = socket(info.ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) syscallError("socket: ");

    write(STDOUT_FILENO,"everything ok!!!\n", 20);


    exit(EXIT_SUCCESS);
}