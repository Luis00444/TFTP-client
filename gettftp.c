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
#define TFTP_SERVICE "69"

void syscallError(char message[]){
    perror(message);
    exit(EXIT_FAILURE);
}

struct addrinfo* checkHost(char* host){
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
    hints.ai_protocol = IPPROTO_UDP;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(host, TFTP_SERVICE, &hints, &result);

    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    return result;
	
}

void createReadRequest(int socketDescriptor, char filename[], struct sockaddr* addr, int addrlen){
    //here goes the tutorial...
    //first 2 bytes: the optcode, in this case 1 (RRQ)
    //then goes the filename terminated by \0
    //then goes the mode terminated by \0. I will use octect
    char string[BUF_SIZE];
    char optcode[2] = {0,1};
    char mode[] = "octet";
    int size;
    int flags = 0;

    size = sprintf(string,"%s%s\0%s",optcode,filename,mode);

    sendto(socketDescriptor, string, size, flags, addr, addrlen);
}

int main(int argc, char* argv[]){
    if (argc != 3){
        write(STDOUT_FILENO,MSG_NOT_ARGUMENTS,34);
        exit(EXIT_FAILURE);
    }
    char* host = argv[1];
    char* filename = argv[2];
    struct addrinfo* info;

    info = checkHost(host);
    char words[100];
    socklen_t lenUsed = info->ai_addrlen;
    struct sockaddr* addr = info->ai_addr;
    char hostaddr[lenUsed];
	char servaddr[lenUsed];
    int size;

    int s = getnameinfo(
        addr,lenUsed,
        hostaddr,lenUsed,
        servaddr,lenUsed,
        (NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM)
        );
    if (s<0) syscallError("getnameinfo: ");
    
    size = sprintf(words,"this is the host: %s\n", hostaddr);
    write(STDOUT_FILENO,words,size);
    size = sprintf(words,"this is the server: %s\n", servaddr);
    write(STDOUT_FILENO,words,size);
    
    int sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sock < 0) syscallError("socket: ");


    write(STDOUT_FILENO,"everything ok!!!\n", 20);
    


    exit(EXIT_SUCCESS);
}