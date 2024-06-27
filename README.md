# Build

```
  clang++ -std=c++11 -c libraries/HelperLibrary.cpp -o HelperLibrary.o && clang++ -std=c++11 server.cpp helperLibrary.o -o myOwnRedis && clang++ -std=c++11 client.cpp HelperLibrary.o -o client
```
