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

#define MAXNAMELEN 32
#define BUFFER_SIZE 512


typedef struct client {
    char *name;
    long fd;
    struct client *next;
} t_client;

typedef struct clientList {
    t_client* head;
    int nClient;
} t_clientList;

typedef struct cleanUpTh {
    t_clientList* connectedClient;
    t_client* client;
} t_cleanUpTh;



//function for request management
typedef t_client* (*manageRequest_t)(char buf[], t_client* client);

typedef struct{
    long fdc;
    t_clientList* connectedClient;
    manageRequest_t manage;
} args_handler_t;


extern int sendErrorMsg(long fd);

/* inizializza la struttura del client con il suo fd
in caso di errore return null */
t_client *initClient(long fd);

/*Controlla se esiste già un client connesso con nome
return 1 se c'è, 0 altrimenti */
int Connected(t_clientList* connectedClient, char *name);

/*Aggiunge il client alla lista dei client connessi
Return struttura client con il nome inizializzato oppure client->name==NULL in caso di errore */
t_client *addClient(t_clientList* connectedClient, t_client *client, char *name);

/*Rimuove il client dalla lista dei client connessi e elimina client dalla memoria*/
void removeClient(t_clientList* connectedClient, t_client *client);

/*Funzione gestione thread */
void *threadF(void *arg);

/*Funzione per gestire la creazione del thread */
void spawn_thread(t_clientList* connectedClient, long connfd, void *manageRequest);

//void gestore();
//void signal_manager();

void cleanup_thread_handler(void *arg);

/*Write personalizzata */
int myWrite(long fd, char* msg, size_t len);

/*Read personalizzata */
int myRead(long fd, char* buf);

/*Salvataggio da tmp a data */
int save(char* fileToWrite, char* fileToWriteN);

void disconnectAll(t_clientList* connectedClient);

int os_start();

/*Ritorna null se ho errori
ritorna il client senza nome se non è stato aggiunto
ritorna client inizializzato con name se aggiunto
 */
t_client* os_register(t_clientList* connectedClient, t_client * client, char* user);

long os_store(t_clientList* connectedClient, t_client* client, char* fileData, char* fileName, long fileLength);

void* os_retrieve(t_client* client, char* fileName);

long os_delete(t_client* client, char* dataName);

void os_leave(t_clientList* connectedClient, t_client* client);

void cleanup();

#endif 