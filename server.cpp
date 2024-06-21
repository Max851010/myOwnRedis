#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cassert>
#include "libraries/HelperLibrary.h"

// Functions
//static void serverDo(int client_fd);
static int32_t oneRequest(int client_fd);

// global variables
const size_t k_max_msg = 4096;


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
  server_addr.sin_addr.s_addr =  ntohl(0); // 0.0.0.0

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
      int32_t err = oneRequest(client_fd);
      if(err) {
        std::cerr << "The request is failed in the 'oneRequest()' function!\n";
        break;
      }
    }
    close(client_fd);
  }
  
  return 0;
}

//static void serverDo(int client_fd) {
//  char rbuf[64] = {};
//  if(read(client_fd, rbuf, sizeof(rbuf) - 1) < 0) {
//    std::cerr << "Server failed to read from the buffer!\n";
//  }
//  printf("Client says: %s\n", rbuf);
//  char wbuf[] = "World! \n";
//  write(client_fd, wbuf, strlen(wbuf));
//}


static int32_t oneRequest(int client_fd) {
  char rbuf[4 + k_max_msg + 1];
  int errorNo = 0;
  // Read from the client fd to the buffer
  // 4 bytes header, is used to tell the size of the request
  int32_t error = HelperLibrary::IOHelpers::readAll(client_fd, rbuf, 4);
  if(error) {
    if(errorNo == 0) {
      std::cerr << "Error: EOF in the function oneRequest()!\n";
    } else {
      std::cerr << "Error: Error happens in the function readAll()!\n";
    }
    return error;
  }

  uint32_t reqLen = 0;
  // Copy the first 4 block in the rbuf to the reqLen
  memcpy(&reqLen, rbuf, 4); // assume little endian
  if(reqLen > k_max_msg) {
    std::cerr << "Error: the request length is too long!\n";
  }

  // Get the request body
  error = HelperLibrary::IOHelpers::readAll(client_fd, &rbuf[4], reqLen);
  if(error) {
    std::cerr << "Error: Error happens in the function readAll() when getting the request body!\n";
    return error;
  }
  // assign the end of the string
  rbuf[4 + reqLen] = '\0';
  printf("client says: %s\n", &rbuf[4]);

  uint32_t replyLen = 0;
  // Replay using the same protocol (first 4 bytes are the length)
  const char replyMsg[] = "world";
  char wbuf[4 + sizeof(replyMsg)];
  replyLen = (uint32_t)strlen(replyMsg);
  memcpy(wbuf, &replyLen, 4);
  memcpy(&wbuf[4], replyMsg, replyLen);
  return HelperLibrary::IOHelpers::writeAll(client_fd, wbuf, 4 + replyLen);
}
