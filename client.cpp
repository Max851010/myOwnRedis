#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cassert>

// Functions
static int32_t query(int fd, const char *text);

// global variables
const size_t k_max_msg = 4096;

int main() {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(client_fd < 0) {
    std::cerr << "Failed to create client socket!\n";
    return 1;
  }

  struct sockaddr_in client_addr = {};
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = ntohs(1234);
  client_addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
  if(connect(client_fd, (const struct sockaddr*)&client_addr, sizeof(client_addr))) {
    std::cerr << "Failed to connect to the server!\n";
    return 1;
  }

  char msg[] = "hello";
  write(client_fd, msg, strlen(msg));
  char rbuf[64] = {};
  if(read(client_fd, rbuf, sizeof(rbuf) - 1) < 0) {
    std::cerr << "Failed to read from client fd\n";
    return 1;
  }
  printf("server says: %s\n", rbuf);
  close(client_fd);

  return 0;
}

static int32_t query(int fd, const char *text) {
  uint32_t textLen = (uint32_t)strlen(text);
  if(textLen > k_max_msg) {
    std::cerr << "Error: the query text length is too long!\n";
    return -1;
  }
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, &textLen, 4);
  memcpy(&wbuf[4], text, textLen);

  int32_t error = readAll;

} 
