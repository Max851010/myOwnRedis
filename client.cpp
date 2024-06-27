#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cassert>
#include "libraries/HelperLibrary.h"

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
  std::cout << "Connected to the server!\n";
  
  
  std::cout << "Doing test1\n";
  int32_t error = query(client_fd, "hello1");
  if(error) {
    goto L_DONE;
  }
  std::cout << "Doing test2\n";
  error = query(client_fd, "hello2");
  if(error) {
    goto L_DONE;
  }
  std::cout << "Doing test3\n";
  error = query(client_fd, "hello3");
  if(error) {
    goto L_DONE;
  }

  L_DONE:
    close(client_fd);
    return 0;
}

static int32_t query(int fd, const char *text) {
  uint32_t textLen = (uint32_t)strlen(text);
  if(textLen > k_max_msg) {
    std::cerr << "Error: The query text length is too long!\n";
    return -1;
  }
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, &textLen, 4);
  memcpy(&wbuf[4], text, textLen);

  int32_t error = HelperLibrary::IOHelpers::writeAll(fd, wbuf, 4 + textLen);
  if(error) {
    std::cerr << "Error: Something wrong happens in the function writeAll()";
    return error;
  }

  // 4 bytes header
  char rbuf[4 + k_max_msg + 1];
  errno = 0;
  error = HelperLibrary::IOHelpers::readAll(fd, rbuf, 4);
  if(error) {
    if(errno == 0) {
      std::cerr << "Error: EOF in the function query()!\n";
    } else {
      std::cerr << "Error: Error happens in the function readAll()!\n";
    }
    return error;
  }

  memcpy(&textLen, rbuf, 4);
  if(textLen > k_max_msg) {
    std::cerr << "Error: the request length is too long!\n";
    return -1;
  }

  // reply body
  error = HelperLibrary::IOHelpers::readAll(fd, &rbuf[4], textLen);
  if(error) {
    std::cerr << "Error: Error happens in the function readAll() when getting the reply body!\n";
    return error;
  }
  // assign the end of the string
  rbuf[4 + textLen] = '\0';
  printf("Server says: %s\n", &rbuf[4]);
  
  return 0;
} 
