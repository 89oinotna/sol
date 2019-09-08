#include "libosserver.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex variabile di mutua esclusione


void cleanup_thread_handler(void *arg) { 
    t_cleanUpTh* cl=(t_cleanUpTh*)arg;
    pthread_mutex_unlock(&mutex); 
    removeClient(cl->connectedClient, cl->client);
}

static void gestore(int sig) { 
    char cwd[256];
    switch(sig){
        case SIGINT:
            /*//char* s;
            
            if (getcwd(cwd, sizeof(cwd)) == NULL) perror("getcwd() error");
            system(cwd);*/
            if(getcwd(cwd, sizeof(cwd))){
                strcat(cwd, "/tmp");
                //fprintf(stderr, "CWD: %s\n", cwd);
                rmrf(cwd);
            }
            //snprintf(cwd, sizeof(cwd), "s/tmp", getcwd(cwd, sizeof(cwd)));
            
            
            cleanup();
            exit(EXIT_SUCCESS);
            break;
        break;
        default:
            break;
    }
}

static void signal_manager() {
    struct sigaction sa;
    // resetto la struttura
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore;
    // sa.sa_flags = ERESTART;

    int notused;
   
    SYSCALLP(notused, sigaction(SIGINT, &sa, NULL), "Error sigaction");
}

void cleanup() { unlink(SOCKNAME); }

int os_start(){
    cleanup();
    if (mkdir("data", 0777) == -1 && errno != EEXIST) exit(1);
    if (mkdir("tmp", 0777) == -1 && errno != EEXIST) exit(1);
    signal_manager();       //delete on exit
    int listenfd = -1;
    SYSCALLP(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "Error socket");

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);
    
    int notused;
    SYSCALLP(notused, bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)), "Error bind");
    SYSCALLP(notused, listen(listenfd, MAXBACKLOG), "Error listen");
    return listenfd;
}

/*
Inizializza un client da inserire nella lista
return pointer a client
 */
t_client *initClient(long fd) {
    t_client *client;// = (t_client *)malloc(sizeof(t_client));
    MALLOC(client, (t_client *)malloc(sizeof(t_client)), "MALLOC");
    client->next = NULL;
    client->name = NULL;
    client->fd = fd;
    // n_client++;
    return client;
}

/*
Verifica se un client è già connesso
return 1 se connesso 0 altrimenti
 */
int Connected(t_clientList* connectedClient, char *name) {
    //fprintf(stderr, "Connected:  %s\n", name);

    t_client *curr = connectedClient->head;

    while (curr != NULL) {
        // fprintf(stderr, "%s = %s\n", name, curr->name);
        if (equal(name, curr->name)) return 1;
        curr = curr->next;
    }

    return 0;
}

/*
Aggiunge il client alla lista
return pointer al client
 */
t_client *addClient(t_clientList* connectedClient, t_client *client, char *name) {
    //fprintf(stderr, "%s ", name);
    /*t_cleanUpTh cl;
    cl.client=client;
    cl.connectedClient=connectedClient;*/
    //pthread_cleanup_push(cleanup_thread_handler, &cl);
    
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK

    if (connectedClient->head == NULL) {
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

    if (Connected(connectedClient, name)) {
        //fprintf(stderr, "GIA CONNESSO");
        pthread_mutex_unlock(&mutex);
        return client;
    }

    t_client *curr = connectedClient->head;

    while (curr->next != NULL) curr = curr->next;
    /*
    t_client *new = (t_client *)malloc(sizeof(t_client));*/
    client->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
    if(client->name==NULL) {
        pthread_mutex_unlock(&mutex);
        return client;
    }
    // new->next = NULL;

    strcpy(client->name, name);

    curr->next = client;

    connectedClient->nClient++;
    pthread_mutex_unlock(&mutex);  // Rilascio della LOCK
    //pthread_cleanup_pop(0);
    return client;
}

void removeClient(t_clientList* connectedClient, t_client *client) {
    t_client *curr;
    /*t_cleanUpTh cl;
    cl.client=client;
    cl.connectedClient=connectedClient;*/
    //pthread_cleanup_push(cleanup_thread_handler, &cl);
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK
    curr = connectedClient->head;
    t_client *prev = NULL;
    if (client == NULL || curr == NULL || client->name == NULL  /*|| n_client == 0*/) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //fprintf(stderr, "{%s}\n", client->name);
    while (curr->next != NULL && client != curr) {
        prev = curr;
        curr = curr->next;
    }
    if (prev == NULL) {  // se è il primo della lista
        connectedClient->head = curr->next;
    } else {
        prev->next = curr->next;
    }
    fprintf(stderr, "   rmv client: n:{%d}, name: {%s} \n", connectedClient->nClient, curr->name);
    connectedClient->nClient--;
    pthread_mutex_unlock(&mutex); // Rilascio della LOCK
    //pthread_cleanup_pop(0);
    free(curr->name);
    free(curr);  
}

void *threadF(void *arg) {
    printf("New thread started\n");
    args_handler_t* args=(args_handler_t*)arg;
    fprintf(stderr, "FD: %ld\n", args->fdc);
    t_client *client = initClient(args->fdc);
    if(client==NULL){
        //setMyErrno(ECLI);
        myErrno=ECLI;
        sendErrorMsg(args->fdc);
        return NULL;
    }
    char buffer[BUFFER_SIZE+1];     //+1 per terminazione
    memset(buffer, '\0', BUFFER_SIZE+1);
    int result = -1;
    /*t_cleanUpTh cl;
    cl.client=client;
    cl.connectedClient=args->connectedClient;*/
    do {
        memset(buffer, '\0', BUFFER_SIZE);
        SYSCALL(result, read(args->fdc, buffer, BUFFER_SIZE), EREAD);
        if (result < 1) break;
        //fprintf(stderr, "RES: %d", result);
        //DEBUG_BUFFER(buffer, result);
        
        //if (client == NULL) break;
        client = args->manage(buffer, client);
        //fprintf(stderr, "	Thread F: %s %d \n", client->name, result);
    } while (client!=NULL);
    //free(buffer);
    
    if(client!=NULL)removeClient(args->connectedClient, client);
    
    close(args->fdc);
    free(args);
    pthread_exit("Thread closed");

    return NULL;
}

void printThrdError(int connfd, char *msg) {
    fprintf(stderr, "%s", msg);
    close(connfd);
}

void spawn_thread(t_clientList* connectedClient, long connfd, void *manageRequest) {
    pthread_attr_t thattr;
    pthread_t thid;
    args_handler_t* args=(args_handler_t*)malloc(sizeof(args_handler_t));
    args->fdc=connfd;
    args->manage=manageRequest;
    args->connectedClient=connectedClient;
    fprintf(stderr, "FD: %ld\n", args->fdc);
    
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

int myWrite(long fd, char* msg, size_t len){
    int result;
    SYSCALL(result, write(fd, msg, len), ESND);
    return 1;
}

int myRead(long fd, char* buf){
    int result;
    SYSCALL(result, read(fd, buf, BUFFER_SIZE), EREAD);
    return 1;
}

int save(char* fileToWrite, char* fileToWriteN){
    int result;
    SYSCALL(result, rename(fileToWrite, fileToWriteN), ESAVE); 
    return 1;
}

void disconnectAll(t_clientList *connectedClient){
    t_client* curr=connectedClient->head;
    while(curr){
        t_client* next=curr->next;
        //setMyErrno(ESRV);
        myErrno=ESRV;
        sendErrorMsg(curr->fd);
        removeClient(connectedClient, curr);
        curr=next;
    }
} 


t_client* os_register(t_clientList* connectedClient, t_client * client, char* user){
    //char* re="^[A-Za-z0-9]{2,32}$";
    if(!match(user, re)){
        myErrno=ENAME;
        return client;
    }

    client = addClient(connectedClient, client, user);
    //int result;
    if (client->name==NULL) {
        //sendErrorMsg(client->fd, ECLI);
        //setMyErrno(ECLI);
        myErrno=ECLI;
        
        return client;
    }
    //fprintf(stderr, "	INIT MANAGE REQ 2:  (%s)\n", client->name);
    char *tmpDirPath = getDirPath(client->name, TMP); //tmp path
    if (mkdir(tmpDirPath, 0777) == -1 && errno != EEXIST) {
        removeClient(connectedClient, client);
        //sendErrorMsg(client->fd, ECLI);
        //setMyErrno(ECLI);
        myErrno=ECLI;
        free(tmpDirPath);
        //free(client->name);
        
        return client;
    }
    char *dirPath = getDirPath(client->name, DATA); //store path
    if (mkdir(dirPath, 0777) == -1 && errno != EEXIST) {
        removeClient(connectedClient, client);
        //sendErrorMsg(client->fd, ECLI);
        //setMyErrno(ECLI);
        myErrno=ECLI;
        free(dirPath);
        //free(client->name);
        return client;
    }
    free(dirPath);
    free(tmpDirPath);
    //fprintf(stderr, "	REGISTER: %s %d \n", client->name, result);
    
    return client;
}

long os_store(t_clientList* connectedClient, t_client* client, char* fileData, char* fileName, long fileLength){
    if(!match(fileName, re)){
        myErrno=EPATH;
        return 0;
    }
    
    char *fileToWrite = getFilePath(fileName, client->name, TMP);   //used as tmp file
    char *fileToWriteN = getFilePath(fileName, client->name, DATA);
    long lenFirstRead = strlen(fileData);
    //fprintf(stderr, "FNAME: %s\n FLEN: %s\n FLEN: %ld\n FTW: %s\n", fileName, fileLen, fileLength, fileToWrite);
    
    int result;
    int count=lenFirstRead;
    
    FILE *fp1;
    CK_EQ(fp1 = fopen(fileToWrite, "wb"), NULL, EOPEN);

    if (fp1 == NULL) {
        //sendErrorMsg(client->fd, EOPEN);
        //setMyErrno(EOPEN);
        myErrno=EOPEN;
        free(fileToWrite);
        free(fileToWriteN);
        return -1;
    }
    long nReadLeft = (long)ceil((double)(fileLength - lenFirstRead) / BUFFER_SIZE);
    int res = fwrite(fileData, sizeof(char), lenFirstRead, fp1);
    //fprintf(stderr, "RES:%d\n", res);
    //fprintf(stderr, "IO BOH");
    char buf[BUFFER_SIZE];
    while (nReadLeft > 0) {
        if(nReadLeft==1)memset(buf, '\0', BUFFER_SIZE);
        //fprintf(stderr, "RES: %d\n", result);
        
        
        res=myRead(client->fd, buf);
        if(!res){
            //sendErrorMsg(client->fd, EREAD);
            //setMyErrno(EREAD);
            myErrno=EREAD;
            free(fileToWrite);
            free(fileToWriteN);
            fclose(fp1);
            return -1;
        }
        
        //count+=result;
        //fprintf(stderr, "%s\n", buf);
        res=fwrite(buf, sizeof(char), (nReadLeft > 1) ? sizeof(char) * BUFFER_SIZE : sizeof(char) * ((fileLength - lenFirstRead) % BUFFER_SIZE),
                fp1);
        //fprintf(stderr, "COUNT: %d\n", res);
        nReadLeft--;
    }
    
    //
    fclose(fp1);
    
    //fprintf(stderr, "ftwn: %s\n", fileToWriteN);
    long fExists=fileExists(fileToWriteN);
    if(fExists==-1){
        //sendErrorMsg(client->fd, ESAVE);
        //setMyErrno(ESAVE);
        myErrno=ESAVE;
        return -1;
    }
    res=save(fileToWrite, fileToWriteN);    //move from tmp to the store
    free(fileToWrite);
    free(fileToWriteN);
    if(!res){
            //sendErrorMsg(client->fd, ESAVE);
            //setMyErrno(ESAVE);
            myErrno=ESAVE;
            return 0;
    }
        // add file length
    
    
    return fExists;
}

void* os_retrieve(t_client* client, char* fileName){
    int path_len = strlen("data") + strlen(client->name) + strlen(fileName) + 2 + 1 + 4; //+4 .dat
            char *pathname = (char *)malloc(path_len * sizeof(char));
            if(pathname==NULL){
                //sendErrorMsg(client->fd, errno);
                //setMyErrno(EUNK);
                myErrno=EUNK;
                return NULL;
            }

            snprintf(pathname, path_len, "%s/%s/%s.bin", "data", client->name, fileName);

            //fprintf(stderr, "Pathname: %s", pathname);

            // open the file
            FILE *fpr;
            fpr = fopen(pathname, "rb");

            // file error
            int result = 0;
            if (fpr == NULL) {
                //sendErrorMsg(client->fd, EOPEN);
                //setMyErrno(EOPEN);
                myErrno=EOPEN;
                free(pathname);
                return NULL;
            }

            // get file size
            /*struct stat st;
            stat(pathname, &st);
            st.st_size*/
            long file_size = fileExists(pathname);
            free(pathname);
            // read the file and prepare data message
            char *data = (char *)malloc(file_size * sizeof(char) + 1);
            if(data==NULL){
                //sendErrorMsg(client->fd, errno);
                //setMyErrno(EUNK);
                myErrno=EUNK;
                return NULL;
            }
            char out;
            int counter = 0;
            while ((out = fgetc(fpr)) != EOF) data[counter++] = (char)out;
            data[counter] = '\0';
            fclose(fpr);
            return data;
            
}

long os_delete(t_client* client, char* dataName){
    int path_len = strlen("data") + strlen(client->name) + strlen(dataName) + 2 + 1 + 4;
    char *pathname= (char *)malloc(path_len * sizeof(char));
    if(pathname==NULL){
        //sendErrorMsg(client->fd, errno);
        //setMyErrno(EUNK);
        myErrno=EUNK;
        return 0;
    }
    snprintf(pathname, path_len, "data/%s/%s.bin", client->name, dataName);

    //fprintf(stderr, "Pathname: %s", pathname);

    long file_size = fileExists(pathname);

    // delete file
    int result=remove(pathname);
    free(pathname);
    if (result == 0) {
        
        return file_size;
    } else {
        //sendErrorMsg(client->fd, EDEL);
        //setMyErrno(EDEL);
        myErrno=EDEL;
        return 0;
    }
}

void os_leave(t_clientList* connectedClient, t_client* client){
    removeClient(connectedClient, client);
}

