# Build

```
  clang++ -c libraries/HelperLibrary.cpp -o HelperLibrary.o && clang++ server.cpp helperLibrary.o -o myOwnRedis && clang++ client.cpp HelperLibrary.o -o client
```
