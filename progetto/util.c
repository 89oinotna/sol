/*
 * @file util.c
 * @author Antonio Zegarelli
 * @brief 
 * @version 0.1
 * @date 2019-07-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "util.h"

const char* myErrlist[]={
    "Successful Operation",
    "Unknown Error",
    "Errore lettura dato",
    "Errore connessione client",
    "Errore comando non riconosciuto",
    "Errore nome dato",
    "Errore invio messaggio",
    "Errore lettura messaggio",
    "Errore salvataggio dato",
    "Errore nome client",
    "Errore cancellazione dato",
    "Errore Server offline",
    "Errore dato non esistente",
    "Errore client già connesso"
};

const char* re="^[A-Za-z0-9]{1,32}$"; //stringa di soli caratteri o numeri di lunghezza da 1 a 32

int startsWith(const char* str, const char* cmpstr) {
    int slen;
    if (str == NULL || cmpstr == NULL) return 0;
    slen = strlen(cmpstr);
    // fprintf(stderr, "str: %s\n", str);
    if (strlen(str) < slen) return 0;
   
    for (int i = 0; i < slen; i++) {
        //fprintf(stderr, "str: %c, cmp: %c ", str[i] , cmpstr[i]);
        if (str[i] != cmpstr[i]) return 0;
    }
    //fprintf(stderr, "END\n");
    return 1;
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS); //FWD_DEPTH flag means that the contents of the directory will be removed before the directory itself is passed to remove()
}

char *getDirPath(char *username, int s) {
    char *path;
    if(s==DATA){
        int lenPath = sizeof(char) * (strlen("data/") + strlen(username) + 1);
        //path = (char *)malloc(lenPath);
        MALLOC(path, (char*)malloc(lenPath), "Malloc");
        snprintf(path, lenPath, "data/%s", username);
    }
    else{
        int lenPath = sizeof(char) * (strlen("tmp/") + strlen(username) + 1);
        //(char *)malloc(lenPath);
        MALLOC(path, (char*)malloc(lenPath), "Malloc");
        snprintf(path, lenPath, "tmp/%s", username);
    }
    
    return path;
}

char *getFilePath(char *fileName, char *username, int s) {
    char *dir = getDirPath(username, s);
    int lenPath = sizeof(char) * (strlen(dir) + strlen(fileName) + 2+4); //+4 per .dat
    char *path;// = (char *)malloc(lenPath);
    MALLOC(path, (char*)malloc(lenPath), "Malloc");
    snprintf(path, lenPath, "%s/%s.bin", dir, fileName);
    //fprintf(stderr, "PATH: %s\n", path);

    free(dir);
    return path;
}

int equal(const char *str, const char *cmpstr) {
    if (str == NULL || cmpstr == NULL) return 0;
    return strcmp(str, cmpstr) != 0 ? 0 : 1;
}

const char* getErrMsg(){
    const char* err=myStrerror();
    if(errno!=0)perror(err);
    else fprintf(stderr, "%s\n", err);
    
    return err;
}

void  destr_fn(void *parm){
   //fprintf(stderr, "Destructor function invoked\n");
   free(parm);
}

static int initialized=0;

int* _MyErrnoFun(){
    if(!initialized){
        int status;
        if ((status = pthread_key_create(&_myErrno, destr_fn )) < 0) {
            printf("pthread_key_create failed, errno=%d", errno);
            exit(1);
        }
        atexit(destroyKey);
        initialized=1;
        
    }
    if(pthread_getspecific(_myErrno)==NULL){
        int* mE=(int*)malloc(sizeof(int));
        *mE=SUCC;
        pthread_setspecific(_myErrno, (void *)mE);
        
    }
    return (int*)pthread_getspecific(_myErrno);    
}

void destroyKey(){
    //fprintf(stderr, "destroy all");
    free(&myErrno);
    pthread_key_delete(_myErrno);
}



const char* myStrerror(){
    int e=myErrno-200;
    const char* buf;
    //fprintf(stderr,"E: %d \tERR: %d\n", e, err);
    if (myErrno==EUNK&&errno!=0) {
        buf=strerror(errno);
        
    } 
    else {
        buf=myErrlist[e];
    }
    return buf;
}

void myPerror(char* s){
    int* getvalue=&myErrno;
    if(/*myErrno*/ getvalue==NULL || *getvalue==0)return;
    const char* err=myStrerror();
    fprintf(stderr, "%s: %s\n", s, err);
}

void setMyErrnoFromString(char* e){
    
    for(int i=0; i<MYERR_LEN; i++){
       
        if(startsWith(e, myErrlist[i])){
            
            myErrno=i+200;
            return;
        }
    }
    myErrno=EUNK;
    
}


/*Returns the size if exists */
long fileExists(char* pathname){
    struct stat st;
    int res=stat(pathname, &st);
    if(res==-1&&errno==ENOENT){

        myErrno=EDATANF;
        return 0;
    }
    else if(res==-1&&errno!=ENOENT) {
        myErrno=EUNK;
        return -1;
    }
    long size = st.st_size;
    //int res=access(pathname, F_OK);
    return size;
}




int match(const char *string, const char *pattern)
{
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) return 0;
    int status = regexec(&re, string, 0, NULL, 0);
    regfree(&re);
    if (status != 0) return 0;
    return 1;
}

