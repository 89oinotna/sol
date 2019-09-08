/*
 * @file osserver.c
 * @author Antonio Zegarelli
 * @brief 
 * @version 0.1
 * @date 2019-07-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500 //per ftw macro
#include "libosserver.h"
#include "util.h"
#include <ctype.h> //testing and mapping char
#include <dirent.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ftw.h>

clientList_t connectedClient;
long nItems;
long totalSize;





/*Funzione che gestisce le operazioni dell' objstore */
client_t *manageRequest(char *buf, client_t *client) {
    char *saveptr;
    char *comand = strtok_r(buf, " ", &saveptr);

    if (client->name == NULL) {
        if (equal(comand, "REGISTER")) {
            char *user = strtok_r(NULL, " ", &saveptr);
            client=os_register(&connectedClient, client, user);
            if(client==NULL){
                sendErrorMsg(client->fd);
                return NULL;
            }
            else if(client->name==NULL){
                sendErrorMsg(client->fd);
                free(client);
                return NULL;
            }
            else{
                sendOkMsg(client->fd);
            }
           
        } 
        else { 
            //fprintf(stderr, "ERROR BUFFER: %s\n", buf);
            myErrno=ECMD; 
            sendErrorMsg(client->fd);
            free(client->name);
            free(client);
        }
    } 
    else {
        // se è registrato
        if (equal(comand, "STORE")) {  // Se il file esiste lo sovrascrive
            
            char *fileName = strtok_r(NULL, " ", &saveptr);
            char *fileLen = strtok_r(NULL, " ", &saveptr);
            long fileLength = strtol(fileLen, NULL, 10);

            char *fileData = strtok_r(NULL, "\0", &saveptr)+2; // parte di <data> 
            
            
            int fExists=os_store(client, fileData, fileName, fileLength);
            memset(buf, '\0', BUFFER_SIZE);
            if(fExists!=-1){
                sendOkMsg(client->fd);
                if(fExists==0){
                //fprintf(stderr, "NOT EXISTS: %d", fExists);
                    totalSize+= fileLength;
                    nItems++;
                }
                else{
                    //fprintf(stderr, "EXISTS: %d", fExists);
                    totalSize+= fileLength-fExists;
                }
            }
            else{
                sendErrorMsg(client->fd);
            }
            
        } 
        else if (equal(comand, "RETRIEVE")) {
            // create pathname
            char *fileName = strtok_r(NULL, " ", &saveptr);
            char* data=os_retrieve(client, fileName);
            if(data==NULL){
                sendErrorMsg(client->fd);
                return client;
            }

            // prepare response message
            long lenData = strlen(data);

            int numOfDigits = log10(lenData) + 1;                          // Numero di char che servono per scrivere lenData
            char *sLenData = (char *)malloc((numOfDigits + 1) * sizeof(char));  // stringa per contenere lenData
            if(sLenData==NULL){
                free(data);
                myErrno=EUNK;
                sendErrorMsg(client->fd);
                return client;
            }
            sprintf(sLenData, "%ld", lenData);

            long response_len = strlen("DATA") + strlen(sLenData) + strlen(data) + 4 + 1;
            
            char *response= (char *)malloc(response_len * sizeof(char));
            if(response==NULL){
                free(data);
                free(sLenData);
                myErrno=EUNK;
                sendErrorMsg(client->fd);
                return client;
            }
            snprintf(response, response_len, "DATA %s \n %s", sLenData, data);

            //fprintf(stderr, "\nReponse message: %s", response);
            myWrite(client->fd, response, (response_len) * sizeof(char));

            free(sLenData);
            free(data);
            free(response);

        } 
        else if (equal(comand, "DELETE")) {
            // create pathname
            char *dataName = strtok_r(NULL, " ", &saveptr);
            int res=os_delete(client, dataName);
            if (res != 0) {
                sendOkMsg(client->fd);
                nItems--;
                totalSize-=res;
            }
            else{
                sendErrorMsg(client->fd);
            }
        
               

          
        } 
        else if (equal(comand, "LEAVE")) {
            //fprintf(stderr, "uscito");
            sendOkMsg(client->fd);
            os_leave(&connectedClient, client);
            
            return NULL;
        } 
        
    }
    return client;
}



static int cnt_fun(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    if(typeflag==FTW_F){ //Se è un file
        nItems++;
        totalSize += sb->st_size;
    }
    return 0;
}

/*
 * @brief funzione per le sttistiche del server, conta i file e la grandezza totale
 * 
 * @param path 
 * @return int 
 */
int count(char *path)
{
    return nftw(path, cnt_fun, 64, FTW_DEPTH | FTW_PHYS); //FWD_DEPTH flag means that the contents of the directory will be removed before the directory itself is passed to remove()
}

volatile sig_atomic_t received=0;

void gestore(int sig) { 
    switch(sig){
        case SIGUSR1:
            received = 1;
            break;
        default:
            break;
    }
}

void signalManager() {
    struct sigaction sa;
    // resetto la struttura
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore;

    int notused;
   
    SYSCALLP(notused, sigaction(SIGUSR1, &sa, NULL), "Error sigaction");
}

int main(int argc, char *argv[]) {
    
    
    int listenfd=os_start();
    count("data");  //Controllo i file
    
    //countItems("data");
    signalManager();
    int connfd=-1;
    received=0;
    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1 && errno == EINTR) {
            perror("Error accept");
        }
        if (received == 0){
            spawn_thread(&connectedClient, connfd, manageRequest);
        }
        else {
            fprintf(stderr,
                    "\n--------Segnale ricevuto--------\n\
                Client connessi: %d\n\
                Oggetti presenti: %ld\n\
                Grandezza totale: %ld\n\n\n",
                    connectedClient.nClient, nItems, totalSize);
            received = 0;
        }
    }

    return 0;
}
