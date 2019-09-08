/*
 * @file util.h
 * @author Antonio Zegarelli
 * @brief 
 * @version 0.1
 * @date 2019-07-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */
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

#define MYERR_LEN 14
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
#define EDATANF 212        //"Errore dato non esistente"
#define EACONN   213         //"Errore client già connesso"


#define SYSCALL(r, c, e)                    \
    if ((r = c) == -1) {  \
        myErrno=e;\
        const char* err=myStrerror();                  \
        perror(err);\
        return 0;                           \
    }  
#define SYSCALLW(r, c, e)                    \
    if ((r = c) == -1) {\
        free(message);\
        const char* err;\
        if(errno==SIGPIPE||errno==EPIPE){\
            myErrno=ESRV;\
            err=myStrerror();                  \
        }                    \
        else{\
            myErrno=e;\
            err=myStrerror();    \
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
    if((r=c)==NULL){\
        myErrno=EUNK;              \
        perror(e);                  \
        return 0;                   \
    }



#define myErrno *_MyErrnoFun()

/*
 * @brief Array che contiene gli errori
 * 
 */
extern const char* myErrlist[];

/*
 * @brief pthread_key_t ogni thread ha i suo _myErrno
 * 
 */
pthread_key_t _myErrno;

/*
 * @brief funzione per fare la free della key quando un thread termina
 * 
 * @param parm key
 */
void  destr_fn(void *parm);

/**
 * @brief funzione di retrieve myerrno
 * 
 * @return int* myErrno
 */
int* _MyErrnoFun();

/*
 * @brief funzione per la cancellazione della key myErrno
 * 
 */
void destroyKey();

/*
 * @brief RegEx per controllare i nomi
 * 
 */
extern const char* re;

/*
 * @brief effettua la rimozione di path
 * 
 * @param path path da eliminare
 * @return 0 oppure -1 in caso di errore
 */
int rmrf(char *path);


/*
 * @brief Controlla se str inizia con cmpstr
 * 
 * @param str 
 * @param cmpstr 
 * @return 1 se è verificato oppure 0 altrimenti
 */
int startsWith(const char* str, const char* cmpstr);

/*
 * @brief Set myErrno From String
 * 
 * @param e 
 */
void setMyErrnoFromString(char* e);

/*
 * @brief Restituisce la stringa corrispondente al valore di myErrno
 * 
 * @return const char* error string
 */
const char* myStrerror();

/*
 * @brief Scrive s seguito da myErrno su stderr 
 * 
 * @param s 
 */
void myPerror(char* s);

/* */

/*
 * @brief Restituisce il path
 * 
 * @param username nome utente
 * @param s TMP oopure DATA
 * @return char* path
 */
char *getDirPath(char *username, int s);

/*
 * @brief Restituisce il path del file di username
 * 
 * @param fileName 
 * @param username 
 * @param s DATA o TMP 
 * @return char* path
 */
char *getFilePath(char *fileName, char *username, int s);

/*
 * @brief Controlla che str sia uguale a cmpstr
 * 
 * @param str 
 * @param cmpstr 
 * @return 1 se true oppure 0 se false
 */
int equal(const char *str, const char *cmpstr);

/*
 * @brief Restituisce il messaggio di errore associato a myErrno
 * 
 * @return const char* error
 */
const char* getErrMsg();

/*
 * @brief Controlla l'esistenza di pathname
 * 
 * @param pathname 
 * @return grandezza del file se esiste,
 *           0 se non esiste 
 *           oppure -1 se ho errori
 */
long fileExists(char* pathname);


/*
 * @brief funzione per il controllo della RegEx
 * 
 * @param string 
 * @param pattern RegEx
 * @return int 
 */
int match(const char *string, const char *pattern);

#endif 