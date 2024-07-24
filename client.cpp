#include "libraries/HelperLibrary.h"
#include <arpa/inet.h>
#include <cassert>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// global variables
const size_t k_max_msg = 4096;

enum {
  SER_NIL = 0,
  SER_ERR = 1,
  SER_STR = 2,
  SER_INT = 3,
  SER_ARR = 4,
};

static int32_t query(int fd, const char *text);
static int32_t sendReq(int client_fd, const std::vector<std::string> &cmd);
static int32_t readRes(int client_fd);
static uint32_t resHandler(const uint8_t *data, size_t size);
int main(int argc, char **argv) {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd < 0) {
    std::cerr << "Failed to create client socket!\n";
    return 1;
  }

  struct sockaddr_in client_addr = {};
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = ntohs(1234);
  client_addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
  if (connect(client_fd, (const struct sockaddr *)&client_addr,
              sizeof(client_addr))) {
    std::cerr << "Failed to connect to the server!\n";
    return 1;
  }
  std::cout << "Connected to the server!\n";

  // Multiple pipelined requests
  std::vector<std::string> cmd;
  for (int i = 1; i < argc; i++) {
    cmd.push_back(argv[i]);
  }
  int32_t error = sendReq(client_fd, cmd);
  if (error) {
    goto L_DONE;
  }
  error = readRes(client_fd);
  if (error) {
    goto L_DONE;
  }

L_DONE:
  close(client_fd);
  return 0;
}

static int32_t sendReq(int client_fd, const std::vector<std::string> &cmd) {
  uint32_t textLen = 4;

  for (const std::string &s : cmd) {
    textLen += 4 + s.size();
  }

  if (textLen > k_max_msg) {
    HelperLibrary::MsgHelpers::error("The query text length is too long!");
    return -1;
  }
  char wbuf[4 + k_max_msg];
  memcpy(&wbuf[0], &textLen, 4);
  uint32_t n = cmd.size();
  memcpy(&wbuf[4], &n, 4);
  size_t pos = 8;

  for (const std::string &s : cmd) {
    uint32_t tmp_pos = (uint32_t)s.size();
    memcpy(&wbuf[pos], &tmp_pos, 4);
    memcpy(&wbuf[pos + 4], s.data(), s.size());
    pos += 4 + s.size();
  }

  int32_t error =
      HelperLibrary::IOHelpers::writeAll(client_fd, wbuf, 4 + textLen);
  if (error) {
    HelperLibrary::MsgHelpers::error(
        "Something wrong happens in the function writeAll().");
    return error;
  }
  return 0;
}

static int32_t readRes(int client_fd) {
  // 4 bytes header
  char rbuf[4 + k_max_msg + 1];
  errno = 0;
  int32_t error = HelperLibrary::IOHelpers::readAll(client_fd, rbuf, 4);
  if (error) {
    if (errno == 0) {
      HelperLibrary::MsgHelpers::error("EOF in the function query()!");
    } else {
      HelperLibrary::MsgHelpers::error(
          "Error happens in the function readAll()!");
    }
    return error;
  }

  uint32_t text_len = 0;
  memcpy(&text_len, rbuf, 4);
  if (text_len > k_max_msg) {
    HelperLibrary::MsgHelpers::error("The request length is too long!");
    return -1;
  }

  // reply body
  error = HelperLibrary::IOHelpers::readAll(client_fd, &rbuf[4], text_len);
  if (error) {
    HelperLibrary::MsgHelpers::error(
        "Error happens in the function readAll() when getting the reply body!");
    return error;
  }
  // Print the result
  uint32_t rescode = resHandler((uint8_t *)&rbuf[4], text_len);
  if (text_len < 0) {
    HelperLibrary::MsgHelpers::error("Bad request!");
    rescode = -1;
  }
  return rescode;
}

static uint32_t resHandler(const uint8_t *data, size_t size) {
  if (size < 1) {
    HelperLibrary::MsgHelpers::error("Bad response!");
    return -1;
  }
  switch (data[0]) {
  case SER_STR:
    if (size < 1 + 4) {
      HelperLibrary::MsgHelpers::error("Bad response!");
      return -1;
    }
    {
      uint32_t len = 0;
      memcpy(&len, &data[1], 4);
      if (size < 1 + 4 + len) {
        HelperLibrary::MsgHelpers::error("Bad response!");
        return -1;
      }
      printf("(str) %.*s\n", len, &data[1 + 4]);
      return 1 + 4 + len;
    }
  case SER_INT:
    if (size < 1 + 8) {
      HelperLibrary::MsgHelpers::error("Bad response!");
      return -1;
    }
    {
      uint64_t value = 0;
      memcpy(&value, &data[1], 8);
      printf("(int) %llu\n", value);
      return 1 + 8;
    }
  case SER_ARR:
    if (size < 1 + 4) {
      HelperLibrary::MsgHelpers::error("Bad response!");
      return -1;
    }
    {
      uint32_t len = 0;
      memcpy(&len, &data[1], 4);
      printf("(arr) len = %u\n", len);
      size_t arr_bytes = 1 + 4;
      for (uint32_t i = 0; i < len; i++) {
        int32_t rv = resHandler(&data[arr_bytes], size - arr_bytes);
        if (rv < 0) {
          return rv;
        }
        arr_bytes += (size_t)rv;
      }
      printf("(arr) end\n");
      return (int32_t)arr_bytes;
    }
  case SER_NIL: {
    printf("(Nil)\n");
    return 1;
  }
  case SER_ERR:
    if (size < 1 + 8) {
      HelperLibrary::MsgHelpers::error("Bad response!");
      return -1;
    }
    {
      int32_t code = 0;
      uint32_t len = 0;
      memcpy(&code, &data[1], 4);
      memcpy(&len, &data[1 + 4], 4);
      if (size < 1 + 8 + len) {
        HelperLibrary::MsgHelpers::error("Bad response!");
        return -1;
      }
      printf("(err) %d %.*s\n", code, len, &data[1 + 8]);
      return 1 + 8 + len;
    }
  default: {
    HelperLibrary::MsgHelpers::error("Bad response!");
    return -1;
  }
  }
}
