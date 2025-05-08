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
#include <string.h>


#define PORT 8080

#define MAX_RESPONSE_SIZE 512
#define MAX_ECHO_SIZE 400

void handle_connection(int client_fd);

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
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        printf("Binding failed: %s...\n", strerror(errno));
        return -1;
    }
    
    int backlogMax = 10;

    // listen in on the port
    if (listen(server_fd, backlogMax) != 0) {
        printf("Listen failed: %s...\n", strerror(errno));
        return -1;
    }
    
    while (1) {
        printf("Server started.\n");
        printf("\tWaiting for connection..\n");

        struct sockaddr_in client_addr;
        unsigned int client_sockaddr_len = sizeof(client_addr);

        // Accept connection
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_sockaddr_len);

        if (client_fd == -1) {
            printf("Connection failure %s.\n", strerror(errno));
            return 1;
        }

        if (!fork()) {
            close(server_fd);
            handle_connection(client_fd);
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }
    return 0;
}

void handle_connection(int client_fd) {
    printf("Handling connection..\n");

    char readBuffer[1024];
    int bytesReceived = recv(client_fd, readBuffer, sizeof(readBuffer), 0);

    if (bytesReceived == -1) {
        printf("Receive failed %s..\n", strerror(errno));
        exit(1);
    }

    char *method = strdup(readBuffer); // GET some/path HTTP/1.1...
    char *content = strdup(readBuffer);
    printf("Content: %s\n", content);

    method = strtok(method, " "); // extract "GET" / "POST" / "PATCH"...

    printf("Method: %s\n", method);

    // extract path
    char *reqPath = strtok(readBuffer, " ");
    reqPath = strtok(NULL, " ");
    // NULL tells strtok to continue tokenizing from the last position, effectively removing the method preceding the path "GET some/path" -> "some/path"

    int bytesSent;

    /**
     * send() sends data to client_fd socket
     * success: returns 0 or number of bytes sent
     * failure: returns -1
     * 
     * \r\n -> Carriage Return / Line Feed (CRLF) 
     * \r moves cursor to beginning of line
     * _\n moves cursor down to next line
     * use these to parse the HTTP request header/body
     */

    // send appropriate resource based on path

    // unspecified resource "/"
    if (strcmp(reqPath, "/") == 0) {
        char *response = "HTTP/1.1 200 OK\r\n\r\n";
        printf("Sending response: %s\n", response);
        bytesSent = send(client_fd, response, strlen(response), 0);
    }
    // Echo
    else if (strncmp(reqPath, "/echo/", 6) == 0) {
    	reqPath = strtok(reqPath, "/");
    	reqPath = strtok(NULL, "");

    	if (reqPath == NULL) reqPath = "";
    	int contentLength = strlen(reqPath);

    	// Truncate if too long
    	if (contentLength > MAX_ECHO_SIZE) {
		contentLength = MAX_ECHO_SIZE;
    	}

    	char truncatedContent[MAX_ECHO_SIZE + 1];
    	strncpy(truncatedContent, reqPath, contentLength);
    	truncatedContent[contentLength] = '\0';

    	char response[MAX_RESPONSE_SIZE];
    	int bytesWritten = snprintf(
		response, sizeof(response),
		"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",
		contentLength, truncatedContent
    	);

    	if (bytesWritten >= sizeof(response)) {
        	fprintf(stderr, "Response too large to send safely.\n");
    	} else {
        	printf("Sending response..\n");
        	bytesSent = send(client_fd, response, bytesWritten, 0);
    	}
    } 
    // User Agent
    else if (strcmp(reqPath, "/user-agent/") == 0) {

        // Parse headers
        reqPath = strtok(NULL, "\r\n");  // reqPath -> HTTP/1.1
        reqPath = strtok(NULL, "\r\n");
        reqPath = strtok(NULL, "\r\n");

        // Parse Body
        char *body = strtok(reqPath, " ");  // body -> "User-Agent: "
        body = strtok(NULL, " ");           // body -> "curl/7.81.0 "
        int contentLength = strlen(body);

        char response[512];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", contentLength, body);
        printf("Sending Response.\n");
        bytesSent = send(client_fd, response, strlen(response), 0);
    }

    // GET Files
    else if (strncmp(reqPath, "/files/", 7) == 0 && strcmp(method, "GET") == 0) {

        // Parse file path
        char *filename = strtok(reqPath, "/");
        filename = strtok(NULL, "");

        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            printf("File not found.\n");
            char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
            bytesSent = send(client_fd, response, strlen(response), 0);
        }
        else {
            printf("Opening file: %s..", filename);
        }
        if (fseek(fp, 0, SEEK_END) < 0) {
            printf("Error seeking file pointer.\n");
        }

        size_t data_size = ftell(fp);

        rewind(fp);

        void *data = malloc(data_size);

        if (fread(data, 1, data_size, fp) != data_size) {
            printf("Error reading file.\n");
        }
        fclose(fp);

        char response[1024];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %ld\r\n\r\n%s", data_size, (char *)data);
        printf("Sending response: %s\n", response);
        bytesSent = send(client_fd, response, strlen(response), 0);
    }
    // POST Files
    else if (strncmp(reqPath, "/files/", 7) == 0 && strcmp(method, "POST") == 0) {
        method = strtok(NULL, "\r\n"); // HTTP 1.1
        method = strtok(NULL, "\r\n"); // Content-type
        method = strtok(NULL, "\r\n"); // User-Agent
        method = strtok(NULL, "\r\n"); // Accept: */*
        method = strtok(NULL, "\r\n"); // Content-Length: X

        char *contentLengthStr = strtok(method, " ");
        contentLengthStr = strtok(NULL, " ");

        int contentLength = atoi(contentLengthStr);

        char *filename = strtok(reqPath, "/");
        filename = strtok(NULL, " ");
        
        content = strtok(content, "\r\n"); // POST /files/example HTTP 1.1
        content = strtok(NULL, "\r\n"); // Host: localhost
        content = strtok(NULL, "\r\n"); // User-Agent: curl/7.81.0
        content = strtok(NULL, "\r\n"); // Accept: */*
        content = strtok(NULL, "\r\n"); // Content-Length: X
        content = strtok(NULL, "\r\n"); // Content-Type
        content = strtok(NULL, "\r\n"); // Content-Type

        printf("\n---\nCreate a file %s with content length: %d\n\n %s\n---\n", filename, contentLength, content);

        FILE *fp = fopen(filename, "wb");
        if (!fp) {
            printf("File couldn't be opened");
            char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
            bytesSent = send(client_fd, response, strlen(response), 0);
        }
        else
            printf("Opening file %s\n", filename);
        
        // Write contents
        if (fwrite(content, 1, contentLength, fp) != contentLength)
            printf("Error writing data.\n");
        
        fclose(fp);
        
        char response[1024];
        sprintf(response, "HTTP/1.1 201 Created\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\n\r\n%s", contentLength, content);
        printf("Sending response: %s\n", response);
        bytesSent = send(client_fd, response, strlen(response), 0);
    }
    else {
        char *response = "HTTP/1.1 404 Not Found\r\n\r\n"; 
		bytesSent = send(client_fd, response, strlen(response), 0);
    }
    if (bytesSent < 0) {
        printf("Send failed.\n");
        exit(1);
    }
}
