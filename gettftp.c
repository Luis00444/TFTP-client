#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define MSG_NOT_ARGUMENTS "arguments should be 2: host file\n"

int main(int argc, char* argv[]){
    if (argc != 3){
        write(STDOUT_FILENO,MSG_NOT_ARGUMENTS,34);
        exit(EXIT_FAILURE);
    }
    
    

    exit(EXIT_SUCCESS);
}