#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cassert>
#include "libraries/HelperLibrary.h"

// global variables
const size_t k_max_msg = 4096;

static int32_t query(int fd, const char *text);
static int32_t sendReq(int client_fd, const char *text);
static int32_t readRes(int client_fd);
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
  

  // Multiple pipelined requests
  const char *query_list[3] = {"hello1", "hello2", "hello3"};
  for(size_t i = 0; i < 3; i++) {
    int32_t err = sendReq(client_fd, query_list[i]);
    if(err) {
      goto L_DONE; 
    }
  }

  for(size_t i = 0; i < 3; i++) {
    int32_t err = readRes(client_fd);
    if(err) {
      goto L_DONE; 
    }
  }

  L_DONE:
    close(client_fd);
    return 0;
}


static int32_t sendReq(int client_fd, const char *text) {
  uint32_t textLen = (uint32_t)strlen(text);
  if(textLen > k_max_msg) {
    HelperLibrary::MsgHelpers::error("The query text length is too long!");
    return -1;
  }
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, &textLen, 4);
  memcpy(&wbuf[4], text, textLen);

  int32_t error = HelperLibrary::IOHelpers::writeAll(client_fd, wbuf, 4 + textLen);
  if(error) {
    HelperLibrary::MsgHelpers::error("Something wrong happens in the function writeAll().");
    return error;
  }
  return 0;
}


static int32_t readRes(int client_fd) {
  // 4 bytes header
  char rbuf[4 + k_max_msg + 1];
  errno = 0;
  int32_t error = HelperLibrary::IOHelpers::readAll(client_fd, rbuf, 4);
  if(error) {
    if(errno == 0) {
      HelperLibrary::MsgHelpers::error("EOF in the function query()!");
    } else {
      HelperLibrary::MsgHelpers::error("Error happens in the function readAll()!");
    }
    return error;
  }

  uint32_t textLen = 0;
  memcpy(&textLen, rbuf, 4);
  if(textLen > k_max_msg) {
    HelperLibrary::MsgHelpers::error("The request length is too long!");
    return -1;
  }

  // reply body
  error = HelperLibrary::IOHelpers::readAll(client_fd, &rbuf[4], textLen);
  if(error) {
    HelperLibrary::MsgHelpers::error("Error happens in the function readAll() when getting the reply body!");
    return error;
  }
  // assign the end of the string
  rbuf[4 + textLen] = '\0';
  printf("Server says: %s\n", &rbuf[4]);
  return 0;
}
