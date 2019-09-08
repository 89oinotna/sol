/*
 * @file connection.h
 * @author Antonio Zegarelli
 * @brief 
 * @version 0.1
 * @date 2019-07-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"


#define SOCKNAME "./objstore.sock"
#define MAXBACKLOG 32
#define BUFFER_SIZE 512

/*
 * @brief Write personalizzata
 * 
 * @param fd File Descriptor del client
 * @param msg Messaggio da scrivere
 * @param len Lunghezza messaggio
 * @return lunghezza del messaggio scritto oppure 0 in caso di errore e setta myErrno
 */
int myWrite(long fd, char* msg, size_t len);

/*
 * @brief Read personalizzata
 * 
 * @param fd File Descriptor del client
 * @param buf Buffer dove salvare i dati letti
 * @return lunghezza del messaggio o 0 in caso di errore e setta myErrno
 */
int myRead(long fd, char* buf);
#endif /* CONNECTION_H */
