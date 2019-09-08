#if !defined(OSCLIENT_H_)
#define OSCLIENT_H_
#define _POSIX_C_SOURCE 200112L //per strtok_r
#include <stdio.h>
#include <string.h>
#include "connection.h"
#include "util.h"
#include <unistd.h>
#include <stddef.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>


#define MAXNAMELEN 32


int os_connect(char *username);
int os_store(char *dataName, void *block, size_t len);
void *os_retrieve(char *dataName);
int os_delete(char *dataName);
int os_disconnect();

#endif 