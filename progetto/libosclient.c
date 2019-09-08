#include "libosclient.h"

static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFFER_SIZE];

struct sigaction new_actn, old_actn;


void gestore(int sig) { 
    switch(sig){
        
        case SIGINT:
            //fprintf(stderr,"SIGINT");
            exit(EXIT_SUCCESS);
            break;
        break;
        default:
            break;
    }
}

void signal_manager() {
    //Ignoro SIGPIPE
    new_actn.sa_handler = SIG_IGN;
    sigemptyset (&new_actn.sa_mask);
    new_actn.sa_flags = 0;
    sigaction (SIGPIPE, &new_actn, &old_actn);  

    struct sigaction sa;
    // resetto la struttura
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore;
    // sa.sa_flags = ERESTART;

    int notused;
   
    SYSCALLP(notused, sigaction(SIGINT, &sa, NULL), "Error sigaction");
}



/*
Create a connection between server with SOCKNAME
Sets globally connfd and serv_addr
 */
int os_connect(char* username) {
    signal_manager();
    
    if(!match(username, re)){
        myErrno=ENAME;
        return 0;
    }
    // Connessione socket
    
    SYSCALLE(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;

    SYSCALLE(notused, connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect");
    if(notused!=0) return 0;


    // Creazione Header
    char* tipo = "REGISTER";
    int lenMex = sizeof(char) * (strlen(username) + strlen(tipo) + 3);  // 3 is for space and \n
    char* message ;
    MALLOC(message,(char*)malloc(lenMex), "Malloc");
    snprintf(message, lenMex, "%s %s \n", tipo, username);
    //fprintf(stderr, "Connecting: %s\n", username);

    // Invio request
    SYSCALLW(notused, write(sockfd, message, lenMex), ESND);
    free(message);

    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), EREAD);

    //fprintf(stderr, "%s\n", buffer);
    char* saveptr;
    char* response=strtok_r(buffer, " ", &saveptr);
    if (equal(response, "OK")) {
        //setMyErrno(SUCC);
    
        return 1;
    }
    char *errmsg=strtok_r(NULL, "\n", &saveptr);
    
    setMyErrnoFromString(errmsg);
    //fprintf(stderr, "fadf %s\n", errmsg);
    
    close(sockfd);

    return 0;
}

int os_store(char* dataName, void* block, size_t len) {
    if(!match(dataName, re)){
        myErrno=EPATH;
        return 0;
    }
    char* message;

    char* type = "STORE";

    // Creazione stringa che contiene la lunghezza del dato(size_t -> string)
    long lenData = (long)len;
    int numOfDigits = log10(lenData) + 1;                          // Numero di char che servono per scrivere lenData
    char* snum;  // Stringa per contenere lenData
    MALLOC(snum, (char*)malloc((numOfDigits + 1) * sizeof(char)), "Malloc");
    sprintf(snum, "%ld", lenData);
    // fprintf(stderr, "lunghezza  %s \n", snum);

    // Creazione header
    long lenMexToSend = sizeof(char) * (strlen(type) + lenData + strlen(dataName) + strlen(snum) + 5 +
                                        1);  // lunghezza messaggio (4 spazi + \n + terminazione)
    MALLOC(message,(char*)malloc(lenMexToSend), "Malloc");   // messaggio da inviare

    snprintf(message, lenMexToSend, "%s %s %s \n %s", type, dataName, snum, (char*)block);  // creo la stringa  da inviare

    free(snum);
    //fprintf(stderr, "Message to send: %s \n", message);

    // Invio request
    int notused;
    SYSCALLW(notused, write(sockfd, message, lenMexToSend), ESND);
    //fprintf(stderr, "MSG: %s\n", message);
    free(message);

    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), EREAD);

    //fprintf(stderr, "RESPONSE: %s", buffer);

    char* saveptr;
    char* response=strtok_r(buffer, " ", &saveptr);
    if (equal(response, "OK")) {
        //setMyErrno(SUCC);
        return 1;
    }
    char *errmsg=strtok_r(buffer, " ", &saveptr);
    setMyErrnoFromString(errmsg);

    return 0;
}

void* os_retrieve(char* dataName) {
    // Creazione header
    char* type = "RETRIEVE";
    long lenMexToSend = sizeof(char) * (strlen(type) + strlen(dataName) + 3);
    char* message;
    MALLOC(message,(char*)malloc((lenMexToSend) * sizeof(char)), "Malloc");
    snprintf(message, lenMexToSend, "%s %s \n", type, dataName);
    //fprintf(stderr, "Message to send: %s \n", message);

    // Invio request
    int notused;
    SYSCALLW(notused, write(sockfd, message, lenMexToSend * sizeof(char)), ESND);
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), EREAD);
    // fprintf(stderr, "Buffer: %s\n\n", buffer);

    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    // Gestisco risposta di errore
    if (equal(command, "KO")) {
        char *errmsg=strtok_r(NULL, "\n", &saveptr);
        //fprintf(stderr, "RECEIVED: %s\n",errmsg);
        setMyErrnoFromString(errmsg);
        return NULL;
    }
    // Creo il dato da ritornare
    else if (equal(command, "DATA")) {
        
        // Prendo le informazioni dall'Header
        char* lenData = strtok_r(NULL, " ", &saveptr);     // Stringa con la lunghezza del dato
        char* fileData = strtok_r(NULL, "\0", &saveptr)+2;  // Prima parte del dato letta presente nel buffer
        long lenFirstRead = strlen(fileData);              // Lunghezza della prima parte letta

        long fileLength = strtol(lenData, NULL, 10);  // Trasformo la stringa della lunghezza in intero

        long nReadLeft = (long)ceil((double)(fileLength - lenFirstRead) /
                                    BUFFER_SIZE);  // Calcolo quante altre read dovrò fare per leggere l'intero dato
        //fprintf(stderr, "nReadLeft: %ld\n", nReadLeft);

        char* data;  // Alloco il dato da ritornare
        MALLOC(data,(char*)malloc(sizeof(char) * (fileLength + 1)), "Malloc");
        // fileLength+1 o scoppia tutto
        
        int cx = snprintf(data, fileLength + 1, "%s", fileData);  // uso cx per sapere il punto in cui sono arrivato a scrivere

        // Leggo la parte restante
        while (nReadLeft > 0) {
            
            if(nReadLeft==1)memset(buffer, '\0', BUFFER_SIZE);// Azzero il buffer
            int result=read(sockfd, buffer, BUFFER_SIZE);
            if(result==-1){
                free(data);
                myErrno=EREAD;
                return 0;
            }
            cx += snprintf(data + cx, fileLength - cx, "%s", fileData);  // Metto in append su data usando cx

            nReadLeft--;
            
        }

        //fprintf(stderr, "cx: %d \n lenData: %s \n Data: %s\n", cx, lenData, data);
        //fprintf(stderr, "BUFFER: %s", data);
        //setMyErrno(SUCC);
        return data;
    }
    else{
        //fprintf(stderr, "ERROR BUFFER: %s\n", buffer);
        //setMyErrno(ECMD);
        myErrno=ECMD;
        return NULL;
    }
}

int os_delete(char* dataName) {
    // Creo l'header
    char* type = "DELETE";
    long lenMexToSend = sizeof(char) * (strlen(type) + strlen(dataName) + 3);  // Calcolo la lunghezza messaggio
    char* message;
    MALLOC(message,(char*)malloc((lenMexToSend) * sizeof(char)), "Malloc");
    snprintf(message, lenMexToSend, "%s %s \n", type, dataName);
    //fprintf(stderr, "Message to send: %s \n", message);

    // invio request
    int notused;
    SYSCALLW(notused, write(sockfd, message, lenMexToSend * sizeof(char)), ESND);
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), EREAD);

    //fprintf(stderr, "Buffer: %s", buffer);

    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    // Gestisco risposta di errore
    if (equal(command, "KO")) {
        char* errMsg = strtok_r(NULL, "\n", &saveptr);
        setMyErrnoFromString(errMsg);
        return 0;
    } 
    else if (equal(command, "OK")){
        //setMyErrno(SUCC);
        return 1;
    }

    return 0;
}

int os_disconnect() {
    // Creo l'header
    char* type = "LEAVE";
    long lenMexToSend = sizeof(char) * (strlen(type) + 3);
    char* message ;
    MALLOC(message,(char*)malloc((lenMexToSend) * sizeof(char)), "Malloc");
    snprintf(message, lenMexToSend, "%s \n", type);
    //fprintf(stderr, "Message to send: %s \n", message);

    // invio request
    int notused;
    SYSCALLW(notused, write(sockfd, message, strlen(message) * sizeof(char)), ESND);
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), EREAD);
    //fprintf(stderr, "REPLY: %s\n", buffer);
    close(sockfd);       // Chiudo il socket
    if (startsWith(buffer, "OK")) {
        //setMyErrno(SUCC);
        return 1;
    }
    //fprintf(stderr, "ERROR BUFFER: %s\n", buffer);
    //setMyErrno(ECMD);
    myErrno=ECMD;
    
    return 0;
}
