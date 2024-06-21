#include <iostream>
#include <unistd.h>
#include <cassert>
#include "HelperLibrary.h"
namespace HelperLibrary {
  // Two helper functions to do in the oneRequest() function
  int32_t IOHelpers::readAll(int fd, char *buf, size_t n) {
    while(n > 0) {
      ssize_t rv = read(fd, buf, n);
      if(rv <= 0) {
        std::cerr << "Error, or unexpected EOF in the readAll() function!\n";
        return -1;
      }
      assert((size_t)rv <= n);
      n -= (size_t)rv;
      buf += rv;
    }
    return 0;
  }
  
  
  int32_t IOHelpers::writeAll(int fd, const char *buf, size_t n) {
    while(n > 0) {
      ssize_t rv = write(fd, buf, n);
      if(rv <= 0) {
        std::cerr << "Error, or unexpected EOF in the writeAll() function!\n";
        return -1;
      }
      assert((size_t)rv <= n);
      n -= (size_t)rv;
      buf += rv;
    }
    return 0;
  }
}
