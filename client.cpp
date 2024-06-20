#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
