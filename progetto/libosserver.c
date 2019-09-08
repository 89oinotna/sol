/**
 * @file libosserver.c
 * @author Antonio Zegarelli
 * @brief 
 * @version 0.1
 * @date 2019-07-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "libosserver.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex variabile di mutua esclusione
static int listenfd; //fd principale del server


/**
 * @brief funzione gestire il segnale
 * 
 * @param sig segnale da gestire
 */
static void gestore(int sig) { 
    
    rmrf("tmp");  //Rimuovo tmp
    
    
    close(listenfd);
    cleanup(SOCKNAME);
    exit(EXIT_SUCCESS);
    /*switch(sig){
        case SIGINT:
            
            
            if(getcwd(cwd, sizeof(cwd))){ //getcwd restituisce il path della working dir
                strcat(cwd, "/tmp"); //Creo la stringa con il percorso
                rmrf(cwd);  //Rimuovo tmp
            }
            
            close(listenfd);
            cleanup(SOCKNAME);
            exit(EXIT_SUCCESS);
            break;
        break;
        
        default:
            break;
    }*/
}

/**
 * @brief funzione per la gestione dei segnali
 * 
 */
static void signalManager() {
    struct sigaction new_actn, old_actn;
     //Ignoro SIGPIPE
    new_actn.sa_handler = SIG_IGN;
    sigemptyset (&new_actn.sa_mask);
    new_actn.sa_flags = 0;
    int notused;
    SYSCALLP(notused, sigaction (SIGPIPE, &new_actn, &old_actn), "Error sigaction");  

    struct sigaction sa;
    // resetto la struttura
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore;

    
    SYSCALLP(notused, sigaction(SIGINT, &sa, NULL), "Error sigaction");
    SYSCALLP(notused, sigaction(SIGTERM, &sa, NULL), "Error sigaction");
    SYSCALLP(notused, sigaction(SIGQUIT, &sa, NULL), "Error sigaction");
}

void cleanup() { unlink(SOCKNAME); }


int os_start(){
    cleanup();
    //creazione cartelle
    if (mkdir("data", 0777) == -1 && errno != EEXIST) exit(1);
    if (mkdir("tmp", 0777) == -1 && errno != EEXIST) exit(1);
    signalManager();       
    //socket
    SYSCALLP(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "Error socket");

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);
    
    int notused;
    SYSCALLP(notused, bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)), "Error bind");
    SYSCALLP(notused, listen(listenfd, MAXBACKLOG), "Error listen");
    fprintf(stderr, "-------Server Started-------\n");
    fflush(NULL);
    return listenfd;
}


client_t *initClient(long fd) {
    client_t *client;
    MALLOC(client, (client_t *)malloc(sizeof(client_t)), "MALLOC");
    client->next = NULL;
    client->name = NULL;
    client->fd = fd;
    
    return client;
}


int connected(clientList_t* connectedClient, char *name) {
    //fprintf(stderr, "connected:  %s\n", name);

    client_t *curr = connectedClient->head;
    //scorro la lista
    while (curr != NULL) {
        if (equal(name, curr->name)) return 1;
        curr = curr->next;
    }

    return 0;
}


client_t *addClient(clientList_t* connectedClient, client_t *client, char *name) {
    //fprintf(stderr, "%s ", name);
    myErrno=ECLI;
    
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK

    if (connectedClient->head == NULL) {    //lista vuota
        connectedClient->head = client;
        client->name= (char *)malloc(sizeof(char) * strlen(name) + 1);
        if(client->name==NULL){
            pthread_mutex_unlock(&mutex);
            return client;
        } 
        strcpy(client->name, name);
        connectedClient->nClient=1;
        pthread_mutex_unlock(&mutex);
        return client;
    }

    if (connected(connectedClient, name)) { //Controllo che non sia già connesso
        //fprintf(stderr, "GIA CONNESSO");
        myErrno=EACONN;
        pthread_mutex_unlock(&mutex);
        return client;
    }

    client_t *curr = connectedClient->head;

    while (curr->next != NULL) curr = curr->next; //Scorro la lista
    
    client->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
    if(client->name==NULL) {
        
        pthread_mutex_unlock(&mutex);
        return client;
    }

    strcpy(client->name, name);

    curr->next = client;

    connectedClient->nClient++;
    pthread_mutex_unlock(&mutex);  // Rilascio della LOCK
    myErrno=SUCC;
    return client;
}

void removeClient(clientList_t* connectedClient, client_t *client) {
    client_t *curr;
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK
    curr = connectedClient->head;
    client_t *prev = NULL;
    if (client == NULL || curr == NULL || client->name == NULL  /*|| n_client == 0*/) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //fprintf(stderr, "{%s}\n", client->name);
    while (curr->next != NULL && client != curr) {  //Cerco il client da rimuovere
        prev = curr;
        curr = curr->next;
    }
    if (prev == NULL) {  // se è il primo della lista
        connectedClient->head = curr->next;
    } else {
        prev->next = curr->next;
    }
    connectedClient->nClient--;
    pthread_mutex_unlock(&mutex); // Rilascio della LOCK
    
    free(curr->name);
    free(curr);  
}

void *threadF(void *arg) {
    
    args_handler_t* args=(args_handler_t*)arg;
    //fprintf(stderr, "New thread started with FD: %ld\n", args->fdc);
    client_t *client = initClient(args->fdc);   //inizializzo il client
    if(client==NULL){
        myErrno=ECLI;
        sendErrorMsg(args->fdc);
        return NULL;
    }
    char buffer[BUFFER_SIZE+1];     //+1 per terminazione
    memset(buffer, '\0', BUFFER_SIZE+1);
    int result = -1;

    do {
        memset(buffer, '\0', BUFFER_SIZE);
        SYSCALL(result, read(args->fdc, buffer, BUFFER_SIZE), EREAD); //Leggo la richiesta
        if (result < 1) {
            fprintf(stderr, "Disconnesso: {%s}\n", client->name);
            break;
        }
        //fprintf(stderr, "RES: %d", result);
        
        client = args->manage(buffer, client); //Gestisco la richista

    } while (client!=NULL);
    
    if(client!=NULL)removeClient(args->connectedClient, client); //Rimuovi il client se è uscito dopo errore sulla read (prob socket chiuso)
    
    close(args->fdc);
    free(args);
    pthread_exit("Thread closed");

    return NULL;
}


void spawn_thread(clientList_t* connectedClient, long connfd, void *manageRequest) {
    pthread_attr_t thattr;
    pthread_t thid;
    args_handler_t* args=(args_handler_t*)malloc(sizeof(args_handler_t));
    args->fdc=connfd;
    args->manage=manageRequest;
    args->connectedClient=connectedClient;
    //fprintf(stderr, "FD: %ld\n", args->fdc);
    
    if (pthread_attr_init(&thattr) != 0) {
        fprintf(stderr, "pthread_attr_init FALLITA\n");
        close(connfd);
        return;
    }
    // settiamo il thread in modalità detached
    if (pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED) != 0) {
        fprintf(stderr, "pthread_attr_setdetachstate FALLITA\n");
        pthread_attr_destroy(&thattr);
        close(connfd);
        return;
    }

    if (pthread_create(&thid, &thattr, threadF, (void *)args) != 0) {
        fprintf(stderr, "pthread_create FALLITA");
        pthread_attr_destroy(&thattr);
        close(connfd);
        return;
    }

}



int save(char* tmpFilePath, char* permFilePath){
    int result;
    SYSCALL(result, rename(tmpFilePath, permFilePath), ESAVE); 
    return 1;
}




client_t* os_register(clientList_t* connectedClient, client_t * client, char* user){
    if(!match(user, re)){ //Controllo sul nome
        myErrno=ENAME;
        return client;
    }

    client = addClient(connectedClient, client, user); //aggiungo il client
    if (client->name==NULL) {
        
        return client;
    }

    //fprintf(stderr, "	INIT MANAGE REQ 2:  (%s)\n", client->name);
    char *tmpDirPath = getDirPath(client->name, TMP); //tmp path
    if (mkdir(tmpDirPath, 0777) == -1 && errno != EEXIST) {
        removeClient(connectedClient, client);
        myErrno=ECLI;
        free(tmpDirPath);
        //free(client->name);
        
        return client;
    }
    char *dirPath = getDirPath(client->name, DATA); //store path
    if (mkdir(dirPath, 0777) == -1 && errno != EEXIST) {
        removeClient(connectedClient, client);
        myErrno=ECLI;
        free(dirPath);
        return client;
    }
    free(dirPath);
    free(tmpDirPath);
    fprintf(stderr, "REGISTERED: Client name {%s} \n", client->name);
    
    return client;
}

long os_store(client_t* client, char* fileData, char* fileName, long fileLength){
    if(!match(fileName, re)){ //controllo sul fileName
        myErrno=EPATH;
        return -1;
    }
    
    char *tmpFilePath = getFilePath(fileName, client->name, TMP);   //used as tmp file
    
    long lenFirstRead = strlen(fileData);
    //fprintf(stderr, "FNAME: %s\n FLEN: %s\n FLEN: %ld\n FTW: %s\n", fileName, fileLen, fileLength, tmpFilePath);
    
    
    FILE *fp1;
    CK_EQ(fp1 = fopen(tmpFilePath, "wb"), NULL, EOPEN);

    if (fp1 == NULL) {
        myErrno=EOPEN;
        free(tmpFilePath);
        return -1;
    }
    long nReadLeft = (long)ceil((double)(fileLength - lenFirstRead) / BUFFER_SIZE); //approssima superiormente
    int res = fwrite(fileData, sizeof(char), lenFirstRead, fp1);
    //fprintf(stderr, "RES:%d\n", res);
    char buf[BUFFER_SIZE];
    while (nReadLeft > 0) { //utilizzo il buffer perchè non ho bisogno di allocare tutto
        if(nReadLeft==1)memset(buf, '\0', BUFFER_SIZE);
        
        res=myRead(client->fd, buf);
        if(!res){
            myErrno=EREAD;
            free(tmpFilePath);
            fclose(fp1);
            return -1;
        }
        
        res=fwrite(buf, sizeof(char), (nReadLeft > 1) ? 
                                                        sizeof(char) * BUFFER_SIZE : 
                                                        sizeof(char) * ((fileLength - lenFirstRead) % BUFFER_SIZE), 
                    fp1);
        //fprintf(stderr, "COUNT: %d\n", res);
        nReadLeft--;
    }
    
    
    fclose(fp1);
    char *permFilePath = getFilePath(fileName, client->name, DATA); //path data
   
    long fExists=fileExists(permFilePath); //controllo file
    if(fExists==-1){
        myErrno=ESAVE;
        return -1;
    }
    res=save(tmpFilePath, permFilePath);    //move from tmp to the store
    free(tmpFilePath);
    free(permFilePath);
    if(!res){
            myErrno=ESAVE;
            return -1;
    }
    
    fprintf(stderr, "STORED: Client name {%s}, File Name {%s}, Lenght {%ld}\n", client->name, fileName, fileLength);
    return fExists;
}

void* os_retrieve(client_t* client, char* fileName){
    int path_len = strlen("data") + strlen(client->name) + strlen(fileName) + 2 + 1 + 4; //+2 //, +4 .bin, +1 terminazione
    char *pathname = (char *)malloc(path_len * sizeof(char));
    if(pathname==NULL){
        myErrno=EUNK;
        return NULL;
    }

    snprintf(pathname, path_len, "%s/%s/%s.bin", "data", client->name, fileName);

    //fprintf(stderr, "Pathname: %s", pathname);

    // open the file
    FILE *fpr;
    fpr = fopen(pathname, "rb");

    // file error
    if (fpr == NULL) {
        myErrno=EOPEN;
        free(pathname);
        return NULL;
    }

    // get file size
    long file_size = fileExists(pathname);
    if(file_size<1) {
        return NULL;
    }
    free(pathname);
    // read the file and prepare data message
    char *data = (char *)malloc(file_size * sizeof(char) + 1);
    if(data==NULL){
        myErrno=EUNK;
        return NULL;
    }
    char out;
    int counter = 0;
    while ((out = fgetc(fpr)) != EOF) data[counter++] = (char)out; //leggo
    data[counter] = '\0';
    fclose(fpr);
    fprintf(stderr, "RETRIEVED: Client name {%s}, File Name {%s}, Lenght {%ld}\n", client->name, fileName, file_size);
    return data;
            
}

long os_delete(client_t* client, char* fileName){
    //creo il path
    int path_len = strlen("data") + strlen(client->name) + strlen(fileName) + 2 + 1 + 4;
    char *pathname= (char *)malloc(path_len * sizeof(char));
    if(pathname==NULL){
        myErrno=EUNK;
        return 0;
    }
    snprintf(pathname, path_len, "data/%s/%s.bin", client->name, fileName);

    //fprintf(stderr, "Pathname: %s", pathname);

    long file_size = fileExists(pathname);
    // delete file
    int result=remove(pathname);
    free(pathname);
    if (result == 0) {
        fprintf(stderr, "DELETED: Client name {%s}, File Name {%s}\n", client->name, fileName);
        return file_size;
    } else {
        myErrno=EDEL;
        return 0;
    }
}

void os_leave(clientList_t* connectedClient, client_t* client){
    fprintf(stderr, "LEAVE: Client name {%s} \n", client->name);
    removeClient(connectedClient, client);
}

/*Invia risposta con errore al client */
int sendErrorMsg(long fd){
    const char *err=getErrMsg();
    long len=sizeof(char)*((strlen("KO  \n")+strlen(err)+1));
    char *msg;
    MALLOC(msg, (char*)malloc(len), "Malloc");
    if(msg==NULL){
        return 0;
    }
    snprintf(msg, len, "KO %s \n", err);
    int result=myWrite(fd, msg, strlen(msg)*sizeof(char));
    
    free(msg);
    return result;
}

/*invia ok al client */
int sendOkMsg(long fd){
    int result=myWrite(fd, "OK \n", 5 * sizeof(char));
    //fprintf(stderr, "OK\n");
    return result;
}
