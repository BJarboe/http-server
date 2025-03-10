#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080

int main() {
    /*  Socket - sockaddr_in
        htons
        Bind
        Listen
        Accept
        Receive client stream - recv
        Char buffer
        Open file
        Send file between file descriptors
        Close file
    */
    setbuf(stdout, NULL); // erase buffer for stdout

    // fd -> file descriptor
    // AF_INET -> IPv4 family
    // SOCK_STREAM -> TCP
    // 0 -> Protocol number.  system will select default
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Server creation failed: %s...\n", strerror(errno));
    }

    // htons -> converts values between host and network byte order (8080 = 0x1f90 --htons-> 0x901f)
    // htons for short, htonl for long
    // INADDR_ANY -> accepts any incoming messages (0x00000000)
    struct sockaddr_in addr = {
        .sin_family = AF_INET, 
        .sin_port = htons(PORT),
        .sin_addr = {htonl(INADDR_ANY)}
    };


    /*  Configure socket for reuse
        setsockopt constants represent options:
            level = SOL_SOCKET = 1 --> the socket layer itself
            optname = SO_REUSEADDR = 2 --> allows binding to previously bound address
            optval = reusePort = 1 --> my best guess is this just toggles the option named to ON or 1.
    */
    int reusePort = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reusePort, sizeof(reusePort)) < 0) {
        printf("Couldn't reuse port: %s...\n", strerror(errno));
        return -1;
    }

    // bind server to address
    if (bind(server_fd, &addr, sizeof(addr)) != 0) {
        printf("Binding failed: %s...\n", strerror(errno));
        return -1;
    }
    
    int backlogMax = 10;

    // listen in on the port
    if (listen(server_fd, backlogMax) != 0) {
        printf("Binding failed: %s...\n", strerror(errno));
        return -1;
    }

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