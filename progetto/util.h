
#if !defined(UTIL_H)
#define UTIL_H
#define _XOPEN_SOURCE 500 //per ftw macro
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <regex.h>
#include <ftw.h>

#define TMP 0
#define DATA 1
#define MYERR_LEN 12
#define SUCC   200       //"Succesfull Operation"
#define EUNK    201       //"Unknown Error"
#define EOPEN   202       //"Errore apertura file"
#define ECLI    203       //"Errore connessione client"
#define ECMD    204       //"Errore comando non riconosciuto"
#define EPATH   205       //"Errore nome dato"
#define ESND    206       //"Errore invio messaggio"
#define EREAD   207       //"Errore lettura messaggio"
#define ESAVE   208       //"Errore salvataggio dato"
#define ENAME   209       //"Errore nome client"
#define EDEL    210       //"Errore cancellazione dato"
#define ESRV    211       //"Errore Server offline"



#define SYSCALL(r, c, e)                    \
    if ((r = c) == -1) {  \
        const char* err=myStrerror(e);                  \
        perror(err);\
        return 0;                           \
    }  
#define SYSCALLW(r, c, e)                    \
    if ((r = c) == -1) {\
        free(message);\
        const char* err;\
        if(errno==SIGPIPE||errno==EPIPE){\
            err=myStrerror(ESRV);                  \
        }                    \
        else{\
            err=myStrerror(e);    \
        } \
        perror(err);\
        return 0;                           \
    }  

#define SYSCALLE(r, c, e)                   \
    if ((r = c) == -1) { \
        perror(e); \
        myErrno=ESRV;\
        return 0;\
    }       

#define SYSCALLP(r, c, e)                   \
    if ((r = c) == -1) {                    \
        perror(e);                          \
        exit(EXIT_FAILURE);                 \
    }                              
                         


#define CK_ZERO(r, c, e)                    \
    if ((r = c) == 0) {\
        myPerror(e);                        \
        if(r==0 && myErrno==ESRV) exit(EXIT_FAILURE);\
    }                                   

#define CK_EQ(X, val, str)      \
    if ((X) == val) {           \
        perror(#str);           \
    }

#define MALLOC(r, c, e)             \
    if((r=c)==NULL){                \
        perror(e);                  \
        return 0;                   \
    }



#define myErrno *_MyErrnoFun()


static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	    if ((r=read((int)fd ,bufptr,left)) == -1) {
	        if (errno == EINTR) continue;
	        return -1;
	    }
	    if (r == 0) return 0;   // gestione chiusura socket
        left    -= r;
	    bufptr  += r;
    }
    return size;
}

static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	    if ((r=write((int)fd ,bufptr,left)) == -1) {
	        if (errno == EINTR) continue;
	        return -1;
	    }
	    if (r == 0) return 0;  
        left    -= r;
	    bufptr  += r;
    }
    return 1;
}

//#define SETERR(e) myErrno=e;
extern const char* myErrlist[];

pthread_key_t _myErrno;

void  destr_fn(void *parm);
int* _MyErrnoFun();
void destroyKey();

extern const char* re;

int rmrf(char *path);

/*
Check if str starts with cmpstr
Returns 1 if true, 0 if false
*/
int startsWith(const char* str, const char* cmpstr);

/*Setta myErrno con e 
*/
//void setMyErrno(int e);

void setMyErrnoFromString(char* e);

/*Restituisce la stringa corrispondente al valore di myErrno */
const char* myStrerror(int err);

/*Scrive s seguito da myErrno su stderr 
non è thread safe*/
void myPerror(char* s);

/*Restituisce il path DATA o TMP di username */
char *getDirPath(char *username, int s);

/*Restituisce il path DATA o TMP del file di username */
char *getFilePath(char *fileName, char *username, int s);

/*
Check if str is equal to cmpstr
Returns 1 if true, 0 if false
*/
int equal(const char *str, const char *cmpstr);

/*Restituisce il messaggio da inviare contenente l'errore in myErrno */
char* getErrMsg();

/*Restituisce la grandezza del file se esiste
oppure 0 se non esiste 
oppure -1 se ho errori*/
long fileExists(char* pathname);

//int sendErrorMsg(long fd, int e);

int isDot(const char dir[]);

int match(const char *string, const char *pattern);
#endif 