#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define MSG_NOT_ARGUMENTS "Usage: %s <server_ip> <filename>\n"
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

void sendWRQ(int sockfd, struct sockaddr_in *serveraddr, const char *filename) {
    char buffer[512];
    buffer[0] = 0x00;      // First byte of opcode
    buffer[1] = 0x02;      // Second byte of opcode for WRQ

    // Copy filename and mode into the buffer
    strcpy(buffer + 2, filename);
    strcpy(buffer + 2 + strlen(filename) + 1, "octet");

    // Calculate the length of the packet
    int len = 2 + strlen(filename) + 1 + strlen("octet") + 1;

    ssize_t bytes_sent = sendto(sockfd, buffer, len, 0, (struct sockaddr *)serveraddr, sizeof(*serveraddr));
    if (bytes_sent == -1) {
        perror("sendto failed");
    } else if (bytes_sent != len) {
        printf("Only %zd out of %d bytes sent\n", bytes_sent, len);
    } else {
        printf("Data sent successfully\n");
    }
}


void sendFile(int sockfd, struct sockaddr_in *serveraddr, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    char buffer[512];
    int block = 1;
    int n = sizeof(*serveraddr);
    socklen_t serverlen = sizeof(*serveraddr);
    while (1) {
        int bytesRead = fread(buffer + 4, 1, 512 - 4, file);
        if (bytesRead < 0) {
            perror("Error reading file");
            break;
        }
        buffer[0] = 0;
        buffer[1] = 3; // Opcode for DATA
        buffer[2] = (block >> 8) & 0xFF;
        buffer[3] = block & 0xFF;
        n = sendto(sockfd, buffer, bytesRead + 4, 0, (struct sockaddr *)serveraddr, serverlen);
        if (n < 0) {
            perror("Error sending DATA");
            break;
        }
        n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)serveraddr, &serverlen);
        if (n < 0) {
            perror("Error receiving ACK");
            break;
        }
        // Check for ACK
        if (buffer[1] != 4 || buffer[2] != (block >> 8) || buffer[3] != (block & 0xFF)) {
            printf("Invalid ACK\n");
            break;
        }
        if (bytesRead < 512 - 4) {
            // Last block
            break;
        }
        block++;
    }
    fclose(file);
}

int main(int argc, char* argv[]){
    if (argc != 3){
        write(STDOUT_FILENO, MSG_NOT_ARGUMENTS,34);
        exit(EXIT_FAILURE);
    }
    char* host = argv[1];
    char* filename = argv[2];
    struct addrinfo* info;

    info = checkHost(host);
    char words[100];
    socklen_t lenUsed = info -> ai_addrlen;
    struct sockaddr* addr = info -> ai_addr;
    char hostaddr[lenUsed];
	char servaddr[lenUsed];
    int size;

    int s = getnameinfo(
        addr, lenUsed,
        hostaddr, lenUsed,
        servaddr, lenUsed,
        (NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM)
    );
    if (s<0) syscallError("getnameinfo: ");
    
    size = sprintf(words, "this is the host: %s\n", hostaddr);
    write(STDOUT_FILENO, words, size);
    size = sprintf(words, "this is the server: %s\n", servaddr);
    write(STDOUT_FILENO, words, size);
    
    int sockfd = socket(info -> ai_family, info -> ai_socktype, info -> ai_protocol);
    if (sockfd < 0) {
        perror("Couldn't create socket");
        return 1;
    }   
    write(STDOUT_FILENO,"everything ok!!!\n", 20);
    
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(69); // TFTP typically uses port 69
    if (inet_pton(AF_INET, argv[1], &serveraddr.sin_addr) <= 0) {
        perror("inet_pton error");
        return 1;
    }

    sendFile(sockfd, &serveraddr, filename);
    close(sockfd);
    exit(EXIT_SUCCESS);
}