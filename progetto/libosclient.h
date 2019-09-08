/*
 * @file libosclient.h
 * @author Antonio Zegarelli
 * @brief 
 * @version 0.1
 * @date 2019-07-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */

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

/*
 * @brief lunghezza massima nomi
 * 
 */
#define MAXNAMELEN 32

/*
 * @brief inizia la connessione all'object store
 * 
 * @param username 
 * @return 1 se la connessione ha avuto successo, 0 altrimenti e setta myErrno
 */
int os_connect(char *username);

/*
 * @brief richiede all'object store la memorizzazione dell'oggetto puntato da *block*
 * 
 * @param dataName 
 * @param block puntatore al dato
 * @param len lunghezza dato
 * @return 1 se ha avuto successo, 0 altrimenti e setta myErrno
 */
int os_store(char *dataName, void *block, size_t len);

/*
 * @brief recupera dall'object store l'oggetto precedentemente memorizzato sotto il nome *name
 * 
 * @param dataName 
 * @return void* puntatore al dato (allocato nell'heap) oppure NULL e setta myErrno
 */
void *os_retrieve(char *dataName);

/*
 * @brief cancella l'oggetto di nome *name* precedentemente memorizzato
 * 
 * @param dataName 
 * @return 1 se ha avuto successo, 0 altrimenti e setta myErrno
 */
int os_delete(char *dataName);

/*
 * @brief chiude la connessione all'object store
 * 
 * @return 1 se ha avuto successo, 0 altrimenti e setta myErrno
 */
int os_disconnect();

#endif 