#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 0x901f

int main() {
    /*  Socket - sockaddr_in
        Bind
        Listen
        Accept
        Receive client stream - recv
        Char buffer
        Open file
        Send file between file descriptors
        Close file
    */
    // fd -> file descriptor
    // AF_INET -> IPv4 family
    // SOCK_STREAM -> basic 2-way TCP stream type socket, http
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        AF_INET, 
        PORT, // 8080 in backwards Hex
        0
    };
    bind(server_fd, &addr, sizeof(addr));

    // up to 10 backlogged connection requests allowed
    listen(server_fd, 10);


    int client_fd = accept(server_fd, 0, 0);
    char buffer[256] = {0};
    recv(client_fd, buffer, 256, 0);

    /* request format:  
        GET /filename 
        _____--------_
        skip   read  trailing whitespace
    */
    //           skip first 5 bytes/chars "GET /"..
    char* file = buffer + 5;

    // convert trailing whitespace into null terminator
    *strchr(file, ' ') = 0;

    int opened_fd = open(file, O_RDONLY);
    sendfile(client_fd, opened_fd, 0, 256);
    close(opened_fd);
    close(client_fd);
    close(server_fd);
}