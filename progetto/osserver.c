#define _POSIX_C_SOURCE 200112L
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


t_clientList connectedClient;
long n_items;
long total_size;

int sendErrorMsg(long fd){
    char *msg=getErrMsg();
    //fprintf(stderr,"ERRno: %d", errno);
    int result=myWrite(fd, msg, strlen(msg)*sizeof(char));
    free(msg);
    return result;
}

int sendOkMsg(long fd){
    int result=myWrite(fd, "OK \n", 5 * sizeof(char));
    return result;
}



/*Funzione che gestisce le operazioni del objstore */
t_client *manageRequest(char *buf, t_client *client) {
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
            //setMyErrno(ECMD);
            sendErrorMsg(client->fd);
            free(client->name);
            free(client);
        }
    } 
    else {
        // se è registrato
        if (equal(comand, "STORE")) {  // Se il file esiste lo sovrascrive (potemo fa come ce pare)
            // str = strtok(NULL, " ");   // nomefile
            char *fileName = strtok_r(NULL, " ", &saveptr);
            char *fileLen = strtok_r(NULL, " ", &saveptr);
            long fileLength = strtol(fileLen, NULL, 10);

            char *fileData = strtok_r(NULL, "\0", &saveptr)+2; // parte di <data> (strtok di \n per dati con spazi)
            
            //fprintf(stderr, "IO BOH %s\n", buf);
            int fExists=os_store(&connectedClient, client, fileData, fileName, fileLength);
            memset(buf, '\0', BUFFER_SIZE);
            if(fExists!=-1){
                sendOkMsg(client->fd);
                if(fExists==0){
                //fprintf(stderr, "NOT EXISTS: %d", fExists);
                    total_size+= fileLength;
                    n_items++;
                }
                else{
                    //fprintf(stderr, "EXISTS: %d", fExists);
                    total_size+= fileLength-fExists;
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
            long data_len = strlen(data);

            int numOfDigits = log10(data_len) + 1;                          // Numero di char che servono per scrivere lenData
            char *snum = (char *)malloc((numOfDigits + 1) * sizeof(char));  // stringa per contenere lenData
            if(snum==NULL){
                free(data);
                //sendErrorMsg(client->fd, errno);
                myErrno=EUNK;
                sendErrorMsg(client->fd);
                return client;
            }
            sprintf(snum, "%ld", data_len);

            long response_len = strlen("DATA") + strlen(snum) + strlen(data) + 4 + 1;
            
            char *response= (char *)malloc(response_len * sizeof(char));
            if(response==NULL){
                free(data);
                free(snum);
                //sendErrorMsg(client->fd, errno);
                myErrno=EUNK;
                sendErrorMsg(client->fd);
                return client;
            }
            snprintf(response, response_len, "DATA %s \n %s", snum, data);

            //fprintf(stderr, "\nReponse message: %s", response);
            myWrite(client->fd, response, response_len * sizeof(char));

            free(snum);
            free(data);
            free(response);

        } 
        else if (equal(comand, "DELETE")) {
            // create pathname
            char *dataName = strtok_r(NULL, " ", &saveptr);
            int res=os_delete(client, dataName);
            if (res != 0) {
                sendOkMsg(client->fd);
                n_items--;
                total_size-=res;
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
        else {
            // TODO send reply incorrect request / not logged in
        }
    }
    return client;
}



/*COnto i file presenti nello store */
void count_items(char *nomedir) {
    DIR *dir;
    if ((dir = opendir(nomedir)) == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *file;

    while ((file = readdir(dir)) != NULL) {
        struct stat statbuf;
        char filename[512];
        strncpy(filename, nomedir, strlen(nomedir) + 1);
        strncat(filename, "/", 2);
        strncat(filename, file->d_name, strlen(file->d_name) + 1);

        if (isDot(filename)) continue;

        if (stat(filename, &statbuf) == -1) {
            perror("Stat");
            return;
        }

        // recursive print if file = directory
        if (S_ISDIR(statbuf.st_mode))
            count_items(filename);
        else {
            n_items++;
            total_size += statbuf.st_size;
        }
    }

    closedir(dir);
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

void signal_manager() {
    struct sigaction sa;
    // resetto la struttura
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore;
    // sa.sa_flags = ERESTART;

    int notused;
   
    SYSCALLP(notused, sigaction(SIGUSR1, &sa, NULL), "Error sigaction");
}

int main(int argc, char *argv[]) {
    
    
    int listenfd=os_start();
    count_items("data");
    signal_manager();
    int connfd=-1;
    received=0;
    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1 && errno == EINTR) {
            perror("Error accept");
        }
        if (received == 0){
            fprintf(stderr, "ACCEPT FD: %d", connfd);
            spawn_thread(&connectedClient, connfd, manageRequest);
        }
        else {
            fprintf(stderr,
                    "\n--------Segnale ricevuto--------\n\
                Clienti connessi: %d\n\
                Oggetti store: %ld\n\
                Size totale store: %ld\n\n\n",
                    connectedClient.nClient, n_items, total_size);
            received = 0;
        }
    }

    unlink(SOCKNAME);

    return 0;
}
