/**
 * @file libosserver.h
 * @author Antonio Zegarelli
 * @brief 
 * @version 0.1
 * @date 2019-07-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#if !defined(OSSERVER_H_)
#define OSSERVER_H_
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
#include <dirent.h>

/*
 * @brief Lunghezza massima per il nome
 * 
 */
#define MAXNAMELEN 32

/*
 * @brief Lunghezza del buffer
 * 
 */
#define BUFFER_SIZE 512

/*
 * @brief nodo client
 * 
 */
typedef struct client {
    char *name;
    long fd;
    struct client *next;
} client_t;

/*
 * @brief Lista dei client
 * 
 */
typedef struct clientList {
    client_t* head;
    int nClient;
} clientList_t;



/*
 * @brief Funzione per la gestione delle richieste
 * 
 */
typedef client_t* (*manageRequest_t)(char buf[], client_t* client);

/*
 * @brief args da passare al thread gestore del client
 * 
 */
typedef struct{
    long fdc;
    clientList_t* connectedClient;
    manageRequest_t manage;
} args_handler_t;

/*
 * @brief Funzione esterna per inviare messaggi di errore
 * 
 * @param fd File descriptor del client
 * @return int 
 */
extern int sendErrorMsg(long fd);


/*
 * @brief inizializza la struttura del client con il suo fd
 * 
 * @param fd File Descriptor del client
 * @return client_t*, null in caso di errore 
 */
client_t *initClient(long fd);

/*
 * @brief Controlla se esiste già un client connesso con nome
 * 
 * @param connectedClient Lista dei client
 * @param name Nome del client
 * @return 1 se è presente, 0 altrimenti 
 */
int connected(clientList_t* connectedClient, char *name);

/*
 * @brief Aggiunge il client alla lista dei client connessi
 * 
 * @param connectedClient Lista dei client
 * @param client 
 * @param name Nome del client
 * @return client_t* con il nome inizializzato oppure client->name==NULL in caso di errore 
 */
client_t* addClient(clientList_t* connectedClient, client_t *client, char *name);

/*
 * @brief Rimuove il client dalla lista
 * 
 * @param connectedClient Lista dei client
 * @param client 
 */
void removeClient(clientList_t* connectedClient, client_t *client);


/*
 * @brief   Funzione principale del thread che gestisce un client
 *          Se ho problemi nell'inizializzare un client setta myErrno 
 *          e invia un messaggio di errore
 * 
 * @param arg args_handler_t
 */
void *threadF(void *arg);

/*
 * @brief Funzione per gestire la creazione dei thread
 * 
 * @param connectedClient Lista dei client
 * @param connfd File descriptor del client
 * @param manageRequest Funzione di gestione richieste
 */
void spawn_thread(clientList_t* connectedClient, long connfd, void *manageRequest);




/*
 * @brief Sposta il file da tmp a data
 * 
 * @param tmpFilePath Path del file tmp
 * @param permFilePath Path dove spostare il file
 * @return 1 oppure 0 in caso di errore e setta myErrno
 */
int save(char* tmpFilePath, char* permFilePath);

/*
 * @brief Inizializza il server
 * 
 * @return fd File Descriptor del socket
 */
int os_start();

/*
 * @brief Funzione che registra un client
 * 
 * @param connectedClient Lista dei client connessi
 * @param client 
 * @param user Nome del client
 * @return client_t* con name=user oppure name=null in caso di errore e setta myErrno
 */
client_t* os_register(clientList_t* connectedClient, client_t * client, char* user);

/*
 * @brief Funzione per il salvataggio di un dato
 * 
 * @param client 
 * @param fileData Prima parte del dato
 * @param fileName Nome del file
 * @param fileLength Lunghezza totale del dato
 * @return long lunghezza del dato salvato se esiste, 
 *          0 se il file non esiste, 
 *          -1 in caso di errore e setta myErrno
 */
long os_store(client_t* client, char* fileData, char* fileName, long fileLength);

/*
 * @brief Funzione per la lettura di un dato
 * 
 * @param client 
 * @param fileName 
 * @return void* puntatore al dato (allocato nell'heap) oppure NULL in caso di errore e setta myErrno
 */
void* os_retrieve(client_t* client, char* fileName);

/*
 * @brief Funzione per la cancellazione di un dato
 * 
 * @param client 
 * @param dataName 
 * @return long grandezza del file eliminato oppure 0 in caso di errore e setta myErrno
 */
long os_delete(client_t* client, char* fileName);

/*
 * @brief Funzione per la disconnessione di un client
 * 
 * @param connectedClient Lista dei client connessi
 * @param client 
 */
void os_leave(clientList_t* connectedClient, client_t* client);

/*
 * @brief effettua l'unlink del socket
 * 
 */
void cleanup();

/*
 * @brief Invia il messaggio di errore al client
 * 
 * @param fd File descriptor del client
 * @return int lunghezza del messaggio oppure 0 in caso di errore
 */
int sendErrorMsg(long fd);

/*
 * @brief Invia ok al client
 * 
 * @param fd File descriptor del client
 * @return int lunghezza del messaggio oppure 0 in caso di errore
 */
int sendOkMsg(long fd);
#endif 