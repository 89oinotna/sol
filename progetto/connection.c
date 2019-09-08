#include "connection.h"

int myWrite(long fd, char* msg, size_t len){
    int result;
    SYSCALL(result, write(fd, msg, len), ESND);
    return result;
}

int myRead(long fd, char* buf){
    int result;
    SYSCALL(result, read(fd, buf, BUFFER_SIZE), EREAD);
    return result;
}