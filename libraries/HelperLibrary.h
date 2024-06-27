#pragma once
#include <unistd.h>
namespace HelperLibrary {
  class IOHelpers {
    public:
      static int32_t readAll(int fd, char *buf, size_t n);
      static int32_t writeAll(int fd, const char *buf, size_t n);
  }; 
  
  class MsgHelpers {
    public:
      static void error(const char *text);
      static void die(const char *text);
  };
}
