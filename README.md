# HTTP Server
## Synopsis
HTTP server implemented in C that handles GET and POST requests.  This project showcases skills in socket api network programming, C-string parsing, and understanding of network protocols.
## Skills Exemplified
- C
- Socket programming
- HTTP protocol handling
- File I/O
- C-String manipulation
- Memory management
- Error handling
- Process management
- Concurrency
- Linux system programming
## Software tools
- GCC compiler
- Make
- Socket API
## Usage
Compile the server with
```bash
make
```
or
```bash
gcc server.c -o server
```

Run compiled binary with optional directory argument to specify root directory.

```bash
./server --directory /path/to/directory
```
Run server tests with curl.

**GET**

```bash
curl -v http://localhost:8080/
```

**Echo Request**

```bash
curl -v http://localhost:8080/echo/hello
```

**User-Agent Request**
```bash
curl -v http://localhost:8080/user-agent
```

**File Upload**
```bash
curl -v -X POST http://localhost:8080/files/upload.txt -d 'Hello World'
```

**File Download**
```bash
curl -v http://localhost:8080/files/upload.txt
```
