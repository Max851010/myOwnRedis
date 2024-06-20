#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
static void serverDo(int client_fd);
static int32_t readFull(int fd, char *buf, size_t n);
static int32_t readAll(int fd, const char *buf, size_t n);

int main() {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd < 0) {
    std::cerr << "Fail to create socket object!\n"; 
    return 1;
  }

  // Configure the socket
  int reuse = 1;
  if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = ntohs(1234);
  server_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0

  if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 1234\n";
    return 1;
  }

  if(listen(server_fd, SOMAXCONN) != 0) {
    std::cerr << "Failed to listen to the port\n";
    return 1;
  }

  // Accept connections
  while(true) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if(client_fd < 0) {
      std::cerr << "Failed to listen to the port\n";
      continue;
    }
    while(true) {
      int32_t err = oneRequest(clinet_fd);
      if(err) {
        std::cerr << "The request is failed in the 'oneRequest()' function!\n";
        break;
      }
    }
    close(client_fd);
  }
  
  return 0;
}

static void serverDo(int client_fd) {
  char rbuf[64] = {};
  if(read(client_fd, rbuf, sizeof(rbuf) - 1) < 0) {
    std::cerr << "Server failed to read from the buffer!\n";
  }
  printf("Client says: %s\n", rbuf);
  char wbuf[] = "World! \n";
  write(client_fd, wbuf, strlen(wbuf));
}

// Two helper functions to do in the oneRequest() function
static int32_t readAll(int fd, char *buf, size_t n) {
  while(n > 0) {
    ssize_t rv = read(fd, buf, n);
    if(rv <= 0) {
      std::cerr << "Error, or unexpected EOF in the readFull() function!\n";
      return -1;
    }
    assert((size_t)rv <= 0);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}


static int32_t readAll(int fd, const char *buf, size_t n) {
  while(n > 0) {
    ssize_t rv = write(fd, buf, n);
    if(rv <= 0) {
      std::cerr << "Error, or unexpected EOF in the readFull() function!\n";
      return -1;
    }
    assert((size_t)rv <= 0);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t oneRequest(int client_fd) {
  return
}
