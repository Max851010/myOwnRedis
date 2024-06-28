#include <iostream>
#include <cassert>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <vector>
#include "libraries/HelperLibrary.h"


// global variables
const size_t k_max_msg = 4096;

// Conn preparation for state machine
enum {
  STATE_REQ = 0,
  STATE_RES = 1,
  STATE_END = 2, // deletion for a connection
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


//static void serverDo(int client_fd);
static int32_t oneRequest(int client_fd);
static void setFdToNonblock(int fd);
static int32_t newConnection(std::vector<Conn *> &fd2conn, int server_fd);
static void connectionIO(Conn *conn);
int main() {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd < 0) {
    HelperLibrary::MsgHelpers::die("Fail to create socket object!");
    return 1;
  }

  // Configure the socket
  int reuse = 1;
  if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    HelperLibrary::MsgHelpers::die("setsockopt failed!");
    return 1;
  }

  struct sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = ntohs(1234);
  server_addr.sin_addr.s_addr =  ntohl(0); // 0.0.0.0

  if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    HelperLibrary::MsgHelpers::die("Failed to bind to port 1234");
    return 1;
  }

  if(listen(server_fd, SOMAXCONN) != 0) {
    HelperLibrary::MsgHelpers::die("Failed to bind to port 1234");
    return 1;
  }

  // A map of all client connection, index is the fd.
  // If there is no enough idex for the fd in new connection, should resize the vector; 
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
  while(true) {
    poll_args.clear();
    // put the server fd in the first one
    struct pollfd server_pollfd = {server_fd, POLLIN, 0}; // event: if there is data to read on this one (connection request)
    poll_args.push_back(server_pollfd);
    
    for(Conn *conn: fd2conn) {
      if(!conn) {
        continue;
      }
      struct pollfd pfd = {};
      pfd.fd = conn->fd;
      pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
      pfd.events = pfd.events | POLLERR;
      poll_args.push_back(pfd);
    }

    // arg1: gets a pointer to the underlying array of pollfd structures stored in the poll_args vector.
    // nfds_t: unsigned long int, it's the size of poll_args
    int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
    if(rv < 0) {
      HelperLibrary::MsgHelpers::die("There is something wrong in the function poll()!");
    }

    // start from 1 cuz the first one is server_fd
    for(size_t i = 1; i < poll_args.size(); i++) {
      if(poll_args[i].revents) {
        Conn *conn = fd2conn[poll_args[i].fd];
        connectionIO(conn);
        if(conn->state == STATE_END) {
          // Destroy this connection
          fd2conn[conn->fd] = NULL;
          close(conn->fd);
          free(conn);
        }
      }
    }

    // Try to accept a new connection
    if(poll_args[0].revents) {
      (void)newConnection(fd2conn, server_fd);
    }

  }

  return 0;
}


static void setFdToNonblock(int fd) {
  errno = 0;
  // Flags are bit mask
  int flags = fcntl(fd, F_GETFL, 0); // F_GETFL: get the current file status flags for the fd.
  if(errno) {
    HelperLibrary::MsgHelpers::die("There is something wrong with the function fcntl(fd, F_GETFL, 0).");
    return ;
  }
  flags |= O_NONBLOCK; // set to nonblocking mode
  (void)fcntl(fd, F_SETFL, flags); // F_SETFL: set the modified flags for the fd.
  if(errno) {
    HelperLibrary::MsgHelpers::die("There is something wrong with the function fcntl(fd, F_SETFL, flags).");
    return ;
  }
}


static void setFdToNonblock(int fd);
static void connPut(std::vector<Conn *> &fd2conn, struct Conn *conn);
static int32_t newConnection(std::vector<Conn *> &fd2conn, int server_fd) {
  struct sockaddr_in client_addr = {};
  socklen_t sock_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &sock_len);
  if(client_fd < 0) {
    HelperLibrary::MsgHelpers::error("Error: Failed to connect to the server fd.");
    return -1;
  }
  // Set the client fd to nonBlocking mode;
  setFdToNonblock(client_fd);

  // Create the Conn struct for the client_fd
  struct Conn *conn = (struct Conn*)malloc(sizeof(struct Conn));
  if(!conn) {
    close(client_fd);
    HelperLibrary::MsgHelpers::error("Error: Failed to create the conn struct for the client_fd, connection closed!");
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
  if(fd2conn.size() <= (size_t)conn->fd) {
    fd2conn.resize(conn->fd + 1);
  }
  fd2conn[conn->fd] = conn;
}


static void stateReq(Conn *conn);
static void stateRes(Conn *conn);
static void connectionIO(Conn *conn) {
  if(conn->state == STATE_REQ) {
    // The state "STATE_REQ" is for reading
    stateReq(conn);
  } else if (conn->state == STATE_RES) {
    stateRes(conn);
  } else {
    HelperLibrary::MsgHelpers::error("Unexpected error happend in the function connectionIO()!");
    assert(0);
  }
}


static bool fillBuffer(Conn *conn);
static void stateReq(Conn *conn) {
  while(fillBuffer(conn)) {}
}


static bool tryOneRequest(Conn *conn);
static bool fillBuffer(Conn *conn) {
  assert(conn->rbuf_size < sizeof(conn->rbuf));
  ssize_t rv = 0;
  // This loop ensure read behavior to be save if there is signal interruption.
  // For non-block mode.
  do {
    size_t cap = sizeof(conn->rbuf) - conn->rbuf_size; // available space left
    // Read cap bytes of data from fd and store in the rbuf
    rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    // EINTR: the read call was interrupted by a signal before it could read any data.
  } while(rv < 0 && errno == EINTR);

  // EAGAIN:  indicates the read operation would block because there is no data available to read at the moment.
  if(rv < 0 && errno == EAGAIN) {
    HelperLibrary::MsgHelpers::error("fillBuffer(): Got EAGAIN, stop to fill the buffer.");
    return false;
  }

  if(rv < 0) {
    HelperLibrary::MsgHelpers::error("Unexpected error happended in the function read()!");
    conn->state = STATE_END;
    return false;
  }

  if(rv == 0) {
    if(conn->rbuf_size > 0) {
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
  while(tryOneRequest(conn)){}
  return (conn->state == STATE_REQ);
}


static void stateRes(Conn *conn);
// parse the request from the buffer
static bool tryOneRequest(Conn *conn) {
  // Not enough data in the buffer
  if(conn->rbuf_size < 4) {
    HelperLibrary::MsgHelpers::error("tryOneRequest() failed: not enough data in the buffer, will try again in the next iteration!");
    return false;
  }

  uint32_t len = 0;
  memcpy(&len, &conn->rbuf[0], 4);
  if(len > k_max_msg) {
    HelperLibrary::MsgHelpers::error("tryOneRequest() failed: the request length is too long!");
    conn->state = STATE_END;
    return false;
  }

  if(4 + len > conn->rbuf_size) {
    HelperLibrary::MsgHelpers::error("tryOneRequest() failed: not enough data in the buffer, will try again in the next iteration!");
    std::cerr << "\n";
    return false;
  }

  // Got one request, print it out
  printf("Client says: %.*s\n", len, &conn->rbuf[4]);

  // Generate response
  memcpy(&conn->wbuf[0], &len, 4);
  memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
  conn->wbuf_size = 4 + len;

  // If there is remaining data, move to the start of the buffer and remove the current request
  size_t remain = conn->rbuf_size - 4 - len;
  if(remain) {
    memmove(&conn->rbuf, &conn->rbuf[4 + len], remain);
  }
  conn->rbuf_size = remain;

  conn->state = STATE_RES;
  stateRes(conn);
  return (conn->state == STATE_REQ);
}

static bool flushBuffer(Conn *conn);
static void stateRes(Conn *conn) {
  while(flushBuffer(conn)){}
}


static bool flushBuffer(Conn *conn){
  ssize_t rv = 0;
  do {
    size_t remain = conn->wbuf_size - conn->wbuf_sent;
    rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    // EINTR: if a signal occurred while the system call was in progress;
  } while(rv < 0 && errno == EINTR);

  if(rv < 0 && errno == EAGAIN) {
    HelperLibrary::MsgHelpers::error("flushBuffer(): Got EAGAIN, stop to flush the buffer.");
    return false;
  }

  if(rv < 0) {
    HelperLibrary::MsgHelpers::error("flushBuffer(): Unexpected error happended in the function write()!");
    conn->state = STATE_END;
    return false;
  }

  conn->wbuf_sent += (size_t)rv;
  assert(conn->wbuf_sent <= conn->wbuf_size);
  if(conn->wbuf_sent == conn->wbuf_size) {
    // The response is fully sent, set the state back
    conn->state = STATE_REQ;
    conn->wbuf_sent = 0;
    conn->wbuf_size = 0;
    return false;
  }

  // Still got some data in wbuf, do it again
  return true;
}
