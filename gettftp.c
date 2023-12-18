#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>

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

int createReadRequest(int socketDescriptor, char filename[], struct sockaddr* addr, int addrlen){
    //here goes the tutorial...
    //first 2 bytes: the optcode, in this case 1 (RRQ)
    //then goes the filename terminated by \0
    //then goes the mode terminated by \0. I will use octect
    char sedingBuffer[BUF_SIZE];
    char writeBuffer[BUF_SIZE];
    char optcode[2] = {0,1};
    int sizeString;
    ssize_t sizeSend;
    ssize_t sizeRecv;
    int sendflags = 0;
    int recvflags = 0;
    int aknowledge = 1;

    char buffer[BUF_SIZE];
    char recvAck[2];
    char excpAck[2];
    char ErrorCode[2] = {0,5};
    char optcodeAck[2] = {0,4};

    sizeString = formatPacket(sedingBuffer,optcode,filename);
    sizeSend = sendto(socketDescriptor, sedingBuffer, sizeString, sendflags, addr, addrlen);
    if (sizeSend < 0) syscallError("sendto: ");

    ssize_t fd = open(strcat("./",filename), O_WRONLY | O_CREAT);

    do{
        sizeRecv = recvfrom(socketDescriptor,buffer,BUF_SIZE,recvflags,addr,addrlen);
        if (sizeRecv < 0) syscallError("recvfrom: ");
        excpAck[0] = 0;
        excpAck[1] = aknowledge;
        recvAck[0] = buffer[2];
        recvAck[1] = buffer[3];

        if(strncmp(buffer,ErrorCode,2)){
            lookError(buffer);
            break;
        } 
        
        if(!(strcmp(excpAck,recvAck))){
            break;
        }

        strncpy(writeBuffer, buffer + 4, sizeRecv-4);
        if(write(fd, writeBuffer, sizeRecv-4) == -1) syscallError("Write: ");
        
        sizeString = formatPacket(sedingBuffer,optcode,filename);
        sizeSend = sendto(socketDescriptor, sedingBuffer, sizeString, sendflags, addr, addrlen);
        if (sizeSend < 0) syscallError("sendto: ");

    }while(sizeRecv >511);

    write(STDOUT_FILENO,"file transfer successfull",26);
    close(fd);
}

int formatPacket(char output[],char optcode[], char filename[]){
    char mode[] = "octet";
    int lenFilename = strlen(filename);
    int lenPacket = lenFilename + 2 + 5 + 2; //lenfile + lenOptmode + lenMode + 2*\0
    char packet[lenPacket];
    
    packet[0] = optcode[0];
    packet[1] = optcode[1];
    for(int i = 0; i < lenFilename; i++){
        if(filename[i] != "\0") packet[2+i] = filename[i];
    }
    packet[2+lenFilename] = "\0";
    for(int i = 0; i < 5; i++){
        packet[3+lenFilename+i] = mode[i];
    }
    packet[lenPacket-1] = "\0";

    memcpy(output,packet,lenPacket);
    return lenPacket;
}
void lookError(char buffer[]){
    char Message[100];
    strncpy(Message, buffer + 4, EOF);
    write(STDOUT_FILENO,Message,100);
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

    createReadRequest(sock, filename, addr, lenUsed);

    write(STDOUT_FILENO,"everything ok!!!\n", 20);
    


    exit(EXIT_SUCCESS);
}