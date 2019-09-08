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
    "Errore Server offline"
};

const char* re="^[A-Za-z0-9]{1,32}$";

int startsWith(const char* str, const char* cmpstr) {
    int slen;
    if (str == NULL || cmpstr == NULL) return 0;
    slen = strlen(cmpstr);
    if (strlen(str) < slen) return 0;
    for (int i = 0; i < slen; i++) {
        if (str[i] != cmpstr[i]) return 0;
    }
    return 1;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
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

char* getErrMsg(){
    const char* err=myStrerror(myErrno);
    if(errno!=0)perror(err);
    else fprintf(stderr, "%s\n", err);
    long len=sizeof(char)*((strlen("KO  \n")+strlen(err)+1));
    char *msg=(char*)malloc(len);
    if(msg==NULL){
        return "KO Errore Memoria \n";
    }
    //MALLOC(msg, (char*)malloc(len), "Malloc: ");
    snprintf(msg, len, "KO %s \n", err);
    //free(err);
    return msg;
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

/*void setMyErrno(int e){
    int* getvalue=&myErrno;
    if(getvalue!=NULL){
        *getvalue=e;
    }
    
}*/

const char* myStrerror(int err){
    int e=err-200;
    const char* buf;
    //fprintf(stderr,"E: %d \tERR: %d\n", e, err);
    if (err==EUNK) {
        //buf=(char*) malloc (sizeof(char)*(1+strlen(myErrlist[1])));
        /*/MALLOC(buf, (char*)malloc(1+strlen(myErrlist[1])), "Malloc: ");
        strcpy(buf, strerror(err));*/
        buf=strerror(errno); //NON é TSAFE
        
    } 
    else {
        myErrno=err;
        //buf=(char*)malloc(sizeof(char)*(1+strlen(myErrlist[err])));
        /* MALLOC(buf, (char*)malloc(1+strlen(myErrlist[e])), "Malloc: ");
        strcpy(buf, myErrlist[e]);*/
        buf=myErrlist[e];
    }
    return buf;
}

void myPerror(char* s){
    int* getvalue=&myErrno;
    if(/*myErrno*/ getvalue==NULL || *getvalue==0)return;
    const char* err=myStrerror(*getvalue);
    fprintf(stderr, "%s: %s\n", s, err);
    //free(err);
}

void setMyErrnoFromString(char* e){
    
    for(int i=0; i<MYERR_LEN; i++){
       
        if(startsWith(e, myErrlist[i])){
            //setMyErrno(i+200);
            myErrno=i+200;
            return;
        }
    }
    myErrno=EUNK;
    //setMyErrno(EUNK);
}


/*Returns the size if exists */
long fileExists(char* pathname){
    struct stat st;
    int res=stat(pathname, &st);
    if(res==-1&&errno==ENOENT)return 0;
    else if(res==-1&&errno!=ENOENT) return -1;
    long size = st.st_size;
    //int res=access(pathname, F_OK);
    return size;
}

int isDot(const char dir[]) {
    int l = strlen(dir);

    if ((l > 0 && dir[l - 1] == '.')) return 1;
    return 0;
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

