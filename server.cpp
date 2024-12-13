#include "libraries/Common.h"
#include "libraries/HashTable.h"
#include "libraries/HelperLibrary.h"
#include <arpa/inet.h>
#include <assert.h>
#include <cstddef>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/ip.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

const size_t k_max_msg = 4096;

// Conn preparation for state machine
enum {
  STATE_REQ = 0,
  STATE_RES = 1,
  STATE_END = 2, // deletion for a connection
};

enum {
  RES_OK = 0,
  RES_ERR = 1,
  RES_NX = 2, // Not exist
};

enum {
  ERR_UNKNOWN = 1,
  ERR_2BIG = 2,
};

struct Conn {
  int fd = -1;
  uint32_t state = 0; // STATE_REQ or STATE_RES
  // buffer for reading
  // rbuf_size: the current amount of data already in the buffer
  size_t rbuf_size = 0;
  uint8_t rbuf[4 + k_max_msg];
  // buffer for writing
  // wbuf_size: the current amount of data already in the buffer
  size_t wbuf_size = 0;
  size_t wbuf_sent = 0;
  uint8_t wbuf[4 + k_max_msg];
};

// Structure for the key
struct Entry {
  struct hashTableNode HTNode;
  std::string key;
  std::string value;
};

static struct {
  hashMap HMap;
} global_data;

static bool entryEQ(hashTableNode *lhs, hashTableNode *rhs) {
  struct Entry *le = container_of(lhs, struct Entry, HTNode);
  struct Entry *re = container_of(rhs, struct Entry, HTNode);
  return le->key == re->key;
}

// static void serverDo(int client_fd);
static void setFdToNonblock(int fd);
static int32_t newConnection(std::vector<Conn *> &fd2conn, int server_fd);
static void connectionIO(Conn *conn);
int main() {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    HelperLibrary::MsgHelpers::die("Fail to create socket object!");
    return 1;
  }

  // Configure the socket
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    HelperLibrary::MsgHelpers::die("setsockopt failed!");
    return 1;
  }

  struct sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = ntohs(1234);
  server_addr.sin_addr.s_addr = ntohl(0); // 0.0.0.0

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    HelperLibrary::MsgHelpers::die("Failed to bind to port 1234");
    return 1;
  }

  if (listen(server_fd, SOMAXCONN) != 0) {
    HelperLibrary::MsgHelpers::die("Failed to bind to port 1234");
    return 1;
  }

  // A map of all client connection, index is the fd.
  // If there is no enough idex for the fd in new connection, should resize the
  // vector;
  std::vector<Conn *> fd2conn;

  // Set the listening fd to nonblocking mode
  setFdToNonblock(server_fd);

  // The event loop using poll()
  /*
   * struct pollfd {
   *   int fd; // file descriptor
   *   short events; // events are listened to (input parameter, bit mask)
   *   short revents; // events really happened (output parameter)
   * }
   */
  std::vector<struct pollfd> poll_args;
  while (true) {
    poll_args.clear();
    // put the server fd in the first one
    struct pollfd server_pollfd = {
        server_fd, POLLIN,
        0}; // event: if there is data to read on this one (connection request)
    poll_args.push_back(server_pollfd);

    for (Conn *conn : fd2conn) {
      if (!conn) {
        continue;
      }
      struct pollfd pfd = {};
      pfd.fd = conn->fd;
      pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
      pfd.events = pfd.events | POLLERR;
      poll_args.push_back(pfd);
    }

    // arg1: gets a pointer to the underlying array of pollfd structures stored
    // in the poll_args vector. nfds_t: unsigned long int, it's the size of
    // poll_args
    int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
    if (rv < 0) {
      HelperLibrary::MsgHelpers::die(
          "There is something wrong in the function poll()!");
    }

    // start from 1 cuz the first one is server_fd
    for (size_t i = 1; i < poll_args.size(); i++) {
      if (poll_args[i].revents) {
        Conn *conn = fd2conn[poll_args[i].fd];
        connectionIO(conn);
        if (conn->state == STATE_END) {
          // Destroy this connection
          fd2conn[conn->fd] = NULL;
          close(conn->fd);
          free(conn);
        }
      }
    }

    // Try to accept a new connection
    if (poll_args[0].revents) {
      (void)newConnection(fd2conn, server_fd);
    }
  }

  return 0;
}

static void setFdToNonblock(int fd) {
  errno = 0;
  // Flags are bit mask
  int flags = fcntl(
      fd, F_GETFL, 0); // F_GETFL: get the current file status flags for the fd.
  if (errno) {
    HelperLibrary::MsgHelpers::die(
        "There is something wrong with the function fcntl(fd, F_GETFL, 0).");
    return;
  }
  flags |= O_NONBLOCK; // set to nonblocking mode
  (void)fcntl(fd, F_SETFL,
              flags); // F_SETFL: set the modified flags for the fd.
  if (errno) {
    HelperLibrary::MsgHelpers::die("There is something wrong with the function "
                                   "fcntl(fd, F_SETFL, flags).");
    return;
  }
}

static void setFdToNonblock(int fd);
static void connPut(std::vector<Conn *> &fd2conn, struct Conn *conn);
static int32_t newConnection(std::vector<Conn *> &fd2conn, int server_fd) {
  struct sockaddr_in client_addr = {};
  socklen_t sock_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sock_len);
  if (client_fd < 0) {
    HelperLibrary::MsgHelpers::error(
        "Error: Failed to connect to the server fd.");
    return -1;
  }
  // Set the client fd to nonBlocking mode;
  setFdToNonblock(client_fd);

  // Create the Conn struct for the client_fd
  struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
  if (!conn) {
    close(client_fd);
    HelperLibrary::MsgHelpers::error("Error: Failed to create the conn struct "
                                     "for the client_fd, connection closed!");
    return -1;
  }

  conn->fd = client_fd;
  conn->state = STATE_REQ;
  conn->rbuf_size = 0;
  conn->wbuf_size = 0;
  conn->wbuf_sent = 0;
  (void)connPut(fd2conn, conn);
  return 0;
}

static void connPut(std::vector<Conn *> &fd2conn, struct Conn *conn) {
  // Question: is hashing better?
  // Resize because the key of fd2conn is fd number
  if (fd2conn.size() <= (size_t)conn->fd) {
    fd2conn.resize(conn->fd + 1);
  }
  fd2conn[conn->fd] = conn;
}

static void stateReq(Conn *conn);
static void stateRes(Conn *conn);
static void connectionIO(Conn *conn) {
  if (conn->state == STATE_REQ) {
    // The state "STATE_REQ" is for reading
    stateReq(conn);
  } else if (conn->state == STATE_RES) {
    stateRes(conn);
  } else {
    HelperLibrary::MsgHelpers::error(
        "Unexpected error happend in the function connectionIO()!");
    assert(0);
  }
}

static bool fillBuffer(Conn *conn);
static void stateReq(Conn *conn) {
  while (fillBuffer(conn)) {
  }
}

static bool oneRequest(Conn *conn);
static bool fillBuffer(Conn *conn) {
  assert(conn->rbuf_size < sizeof(conn->rbuf));
  ssize_t rv = 0;
  // This loop ensure read behavior to be save if there is signal interruption.
  // For non-block mode.
  do {
    size_t cap = sizeof(conn->rbuf) - conn->rbuf_size; // available space left
    // Read cap bytes of data from fd and store in the rbuf
    rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    // EINTR: the read call was interrupted by a signal before it could read any
    // data.
  } while (rv < 0 && errno == EINTR);

  // EAGAIN:  indicates the read operation would block because there is no data
  // available to read at the moment.
  if (rv < 0 && errno == EAGAIN) {
    HelperLibrary::MsgHelpers::error(
        "fillBuffer(): Got EAGAIN, stop to fill the buffer.");
    return false;
  }

  if (rv < 0) {
    HelperLibrary::MsgHelpers::error(
        "Unexpected error happended in the function read()!");
    conn->state = STATE_END;
    return false;
  }

  if (rv == 0) {
    if (conn->rbuf_size > 0) {
      HelperLibrary::MsgHelpers::error("Unexpected EOF!");
    } else {
      HelperLibrary::MsgHelpers::error("EOF!");
    }
    conn->state = STATE_END;
    return false;
  }

  conn->rbuf_size += (size_t)rv;
  assert(conn->rbuf_size <= sizeof(conn->rbuf));

  // Pipelining: The read buffer may contain multiple requests
  while (oneRequest(conn)) {
  }
  return (conn->state == STATE_REQ);
}

static void stateRes(Conn *conn);
static void parseRequest(std::vector<std::string> &cmd, std::string &out);
static int32_t parseHelper(const uint8_t *data, size_t req_len,
                           std::vector<std::string> &cmd);
static void outErr(std::string &out, int32_t error_code,
                   const std::string &msg);
// parse the request from the buffer
static bool oneRequest(Conn *conn) {
  // Not enough data in the buffer
  if (conn->rbuf_size < 4) {
    HelperLibrary::MsgHelpers::error(
        "oneRequest() failed: not enough data in the buffer, will try again in "
        "the next iteration!");
    return false;
  }

  uint32_t len = 0;
  memcpy(&len, &conn->rbuf[0], 4);
  if (len > k_max_msg) {
    HelperLibrary::MsgHelpers::error(
        "oneRequest() failed: the request length is too long!");
    conn->state = STATE_END;
    return false;
  }

  if (4 + len > conn->rbuf_size) {
    HelperLibrary::MsgHelpers::error(
        "oneRequest() failed: not enough data in the buffer, will try again in "
        "the next iteration!");
    std::cerr << "\n";
    return false;
  }

  // Got one request, generate the response
  // uint32_t rescode = 0;
  // response length
  // uint32_t wlen = 0;
  // arg1: request data
  //       |   4 bytes   |   remaining bytes   |
  //         data length      request data
  // arg4: response data
  //       |   4 bytes   |   4 bytes   |   remaining bytes   |
  //         data length     rescode        response data
  // int32_t err =
  //     parseRequest(&conn->rbuf[4], len, &rescode, &conn->wbuf[4 + 4], &wlen);
  // if (err) {
  //   conn->state = STATE_END;
  //   return false;
  // }
  // because it adds response code
  // wlen += 4;
  // memcpy(&conn->wbuf[0], &wlen, 4);
  // memcpy(&conn->wbuf[4], &rescode, 4);
  // conn->wbuf_size = 4 + wlen;

  // Parse the request
  std::vector<std::string> cmd;
  if (parseHelper(&conn->rbuf[4], len, cmd) != 0) {
    HelperLibrary::MsgHelpers::error("Bad Request");
    conn->state = STATE_END;
    return false;
  }
  // Generate one response after got one request
  std::string out;
  parseRequest(cmd, out);

  // Pack the response into the buffer
  if (4 + out.size() > k_max_msg) {
    out.clear();
    outErr(out, ERR_2BIG, "response is too big");
  }
  uint32_t wlen = (uint32_t)out.size();
  memcpy(&conn->wbuf[0], &wlen, 4);
  memcpy(&conn->wbuf[4], out.data(), out.size());
  conn->wbuf_size = 4 + wlen;

  // If there is remaining data, move to the start of the buffer and remove the
  // current request
  size_t remain = conn->rbuf_size - 4 - len;
  if (remain) {
    memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
  }
  conn->rbuf_size = remain;

  conn->state = STATE_RES;
  stateRes(conn);
  return (conn->state == STATE_REQ);
}

static bool flushBuffer(Conn *conn);
static void stateRes(Conn *conn) {
  while (flushBuffer(conn)) {
  }
}

static bool flushBuffer(Conn *conn) {
  ssize_t rv = 0;
  do {
    size_t remain = conn->wbuf_size - conn->wbuf_sent;
    rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    // EINTR: if a signal occurred while the system call was in progress;
  } while (rv < 0 && errno == EINTR);

  if (rv < 0 && errno == EAGAIN) {
    HelperLibrary::MsgHelpers::error(
        "flushBuffer(): Got EAGAIN, stop to flush the buffer.");
    return false;
  }

  if (rv < 0) {
    HelperLibrary::MsgHelpers::error(
        "flushBuffer(): Unexpected error happended in the function write()!");
    conn->state = STATE_END;
    return false;
  }

  conn->wbuf_sent += (size_t)rv;
  assert(conn->wbuf_sent <= conn->wbuf_size);
  if (conn->wbuf_sent == conn->wbuf_size) {
    // The response is fully sent, set the state back
    conn->state = STATE_REQ;
    conn->wbuf_sent = 0;
    conn->wbuf_size = 0;
    return false;
  }

  // Still got some data in wbuf, do it again
  return true;
}

static void doGet(std::vector<std::string> &cmd, std::string &out);
static void doSet(std::vector<std::string> &cmd, std::string &out);
static void doDel(std::vector<std::string> &cmd, std::string &out);
static void doKeys(std::vector<std::string> &cmd, std::string &out);
static bool cmdIs(std::string &word, const char *cmd);
static void outErr(std::string &out, int32_t error_code,
                   const std::string &msg);
static void parseRequest(std::vector<std::string> &cmd, std::string &out) {
  if (cmd.size() == 1 && cmdIs(cmd[0], "keys")) {
    doKeys(cmd, out);
  } else if (cmd.size() == 2 && cmdIs(cmd[0], "get")) {
    doGet(cmd, out);
  } else if (cmd.size() == 3 && cmdIs(cmd[0], "set")) {
    doSet(cmd, out);
  } else if (cmd.size() == 2 && cmdIs(cmd[0], "del")) {
    doDel(cmd, out);
  } else {
    // Unknown Command
    outErr(out, ERR_UNKNOWN, "Unknown Command");
  }
}

// Read arguments
static int32_t parseHelper(const uint8_t *data, size_t req_len,
                           std::vector<std::string> &cmd) {
  if (req_len < 4) {
    return -1;
  }
  uint32_t n = 0;
  memcpy(&n, &data[0], 4);
  if (n > k_max_msg) {
    return -1;
  }
  size_t pos = 4;
  while (n--) {
    if (pos + 4 > req_len) {
      return -1;
    }
    uint32_t sz = 0;
    memcpy(&sz, &data[pos], 4);
    if (pos + 4 + sz > req_len) {
      return -1;
    }
    cmd.push_back(std::string((char *)&data[pos + 4], sz));
    pos += (4 + sz);
  }

  if (pos != req_len) {
    return -1;
  }

  return 0;
}

static bool cmdIs(std::string &word, const char *cmd) {
  return strcasecmp(word.c_str(), cmd) == 0;
}

static void keyScan(hashTable *HTable, void (*f)(hashTableNode *, void *),
                    void *arg) {
  if (HTable->size == 0) {
    return;
  }
  for (size_t i = 0; i < HTable->mask + 1; i++) {
    hashTableNode *HTNode = HTable->table[i];
    while (HTNode) {
      f(HTNode, arg);
      HTNode = HTNode->next;
    }
  }
}

static void outStr(std::string &out, const std::string &val);
// void* pointer: means it can point to any type
static void callbackScan(hashTableNode *HTNode, void *arg) {
  // (std::string *)arg: cast arg to type string *
  // *(std::string *)arg: dereferences the type pointer, now it points to the
  //                      actual string object
  std::string &out = *(std::string *)arg;
  outStr(out, container_of(HTNode, Entry, HTNode)->key);
}

static void outArr(std::string &out, uint32_t n);
static void keyScan(hashTable *HTable, void (*f)(hashTableNode *, void *),
                    void *arg);
static void callbackScan(hashTableNode *HTNode, void *arg);
static void doKeys(std::vector<std::string> &cmd, std::string &out) {
  (void)cmd;
  outArr(out, (uint32_t)HMSize(&global_data.HMap));
  keyScan(&global_data.HMap.current_HT, &callbackScan, &out);
  keyScan(&global_data.HMap.previous_HT, &callbackScan, &out);
}

static void outNil(std::string &out);
static void outStr(std::string &out, const std::string &val);
static void doGet(std::vector<std::string> &cmd, std::string &out) {
  Entry key;
  key.key = cmd[1];
  key.HTNode.hash_value = strHash((uint8_t *)key.key.data(), key.key.size());
  hashTableNode *node = HMLookup(&global_data.HMap, &key.HTNode, &entryEQ);
  if (!node) {
    return outNil(out);
  }
  const std::string &val = container_of(node, Entry, HTNode)->value;
  outStr(out, val);
}

static void outNil(std::string &out);
static void doSet(std::vector<std::string> &cmd, std::string &out) {
  Entry key;
  key.key = cmd[1];
  key.HTNode.hash_value = strHash((uint8_t *)key.key.data(), key.key.size());
  hashTableNode *node = HMLookup(&global_data.HMap, &key.HTNode, &entryEQ);
  if (node) {
    container_of(node, Entry, HTNode)->value = cmd[2];
  } else {
    Entry *new_entry = new Entry();
    new_entry->key = key.key;
    new_entry->HTNode.hash_value = key.HTNode.hash_value;
    new_entry->value = cmd[2];
    HMInsert(&global_data.HMap, &new_entry->HTNode);
  }
  return outNil(out);
}

static void outInt(std::string &out, int64_t val);
static void doDel(std::vector<std::string> &cmd, std::string &out) {
  Entry key;
  key.key = cmd[1];
  key.HTNode.hash_value = strHash((uint8_t *)key.key.data(), key.key.size());
  hashTableNode *deleted_node = HMPop(&global_data.HMap, &key.HTNode, &entryEQ);
  if (deleted_node) {
    delete container_of(deleted_node, Entry, HTNode);
  }
  return outInt(out, deleted_node ? 1 : 0);
}

static void outStr(std::string &out, const std::string &val) {
  out.push_back(SER_STR);
  uint32_t len = (uint32_t)val.size();
  // To ensures that the length of the string is stored as a binary
  // representation in the out string.
  out.append((char *)&len, 4);
  out.append(val);
}

static void outInt(std::string &out, int64_t val) {
  out.push_back(SER_INT);
  out.append((char *)&val, 8);
}

static void outErr(std::string &out, int32_t error_code,
                   const std::string &msg) {
  out.push_back(SER_ERR);
  out.append((char *)&error_code, 4);
  uint32_t len = (uint32_t)msg.size();
  out.append((char *)&len, 4);
  out.append(msg);
}

static void outArr(std::string &out, uint32_t n) {
  out.push_back(SER_ARR);
  out.append((char *)&n, 4);
}

static void outNil(std::string &out) { out.push_back(SER_NIL); }
