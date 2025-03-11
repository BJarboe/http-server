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

// for multithread functionality
void handle_client(int client_fd);


int main() {
    /*  Useful man pages:
        Socket
        sockaddr_in
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

    printf("Initializing..\n");

    // fd -> file descriptor
    // AF_INET -> IPv4 family
    // SOCK_STREAM -> TCP
    // 0 -> Protocol number.  system will select default
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Server creation failed: %s...\n", strerror(errno));
    }

    // htons -> host to network, short (8080 = 0x1f90 --htons-> 0x901f)
    // htonl for long
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

    while (1) {
        printf("Server on standbye..\n");

        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            printf("Failed to connect.. %s\n", strerror(errno));
            return 1;
        }

        printf("Client Connection Success!\n");

        // fork -> pid_t    duplicates the calling process
        // returns PID of parrent, 0 to child on success, -1 on failure
        if (!fork()) {
            close(server_fd);
            handle_connection(client_fd);
            close(client_fd);
            return 0;
        }
        close(client_fd);
        return 0;
    }
}


void handle_connection(int client_fd) {
    /**recv() client data and store it in a buffer
     * return the message length in bytes, -1 otherwise
     * 
     * Parse the request
     *      "GET /this/path HTTP/1.1..."
     * method ^     
     * the path will indicate which resource the client is requesting
     * 
     * depending on the parse, send() the appropriate resource and return
     */
}