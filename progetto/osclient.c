#include "libosclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Da levare

#include <math.h>

#define START_SIZE 100  // start offset
#define INC_SIZE 10000   // modify this to reach 100000 bytes

int totalOp = 0;
int successOp = 0;
int failedOp = 0;

void test1() {
    char data_name[3];
    char data_sing[5] = "123\n";
    char* data = (char*)malloc(sizeof(char) * START_SIZE);
    int res, i = 0;
    //memset(data, '\0', sizeof(data));
    for (i = 0; i < 20; i++) {
        totalOp++;
        long current_size = ((i+1)/2*INC_SIZE)+1;
        if(i==0) current_size=START_SIZE+1;
        data = (char*)realloc(data, (sizeof(char) * current_size));

        sprintf(data_name, "%d", i);
        int cx=0;
        while (current_size - cx  > 0) {
            cx += snprintf(data + cx, current_size - cx, "%s", data_sing);
        }
    //fprintf(stderr, "NAME: %s", data_name);
        CK_ZERO(res, os_store(data_name, data, strlen(data)), "Error STORE");
        
        if (res == 0)
            failedOp++;
        else{
            fprintf(stderr, "RESPONSE: OK\n");
            successOp++;
        }
    }

     free(data);
}
/*
Recuperare oggetti verificando che i contenuti siano corretti**
 */
void test2() {
    char* data_name = "test2";
    char* data_store = "hello";
    char* data_retrieve;
    totalOp++;
    int res;
    CK_ZERO(res, os_store(data_name, data_store, strlen(data_store)), "Error STORE");
    CK_ZERO(data_retrieve, (char*)os_retrieve(data_name), "Error Retrieve");

    if (equal(data_retrieve, data_store)) {
        successOp++;
        free(data_retrieve);
        fprintf(stderr, "Test2 OK\n");
        return;
    }
    free(data_retrieve);
    failedOp++;
    fprintf(stderr, "Test2 KO\n");

}

/*
**cancellare oggetti**
*/
void test3() {
    char* data_name = "test3";
    char* data_store = "hello";
    int res;
    totalOp++;
    CK_ZERO(res, os_store(data_name, data_store, strlen(data_store)), "Error STORE");
    CK_ZERO(res, os_delete(data_name), "Error DELETE");

    if (res != 1) {
        failedOp++;
        fprintf(stderr, "Test3 KO\n");
        return;
    }
    successOp++;
    fprintf(stderr, "Test3 OK\n");
}

int main(int argc, char* argv[]) {
    
    if (argc == 1) {
        fprintf(stderr, "usa: %s stringa \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && strlen(argv[1]) > MAXNAMELEN) {
        fprintf(stderr, "Name can be max %d char\n", MAXNAMELEN);
        exit(EXIT_FAILURE);
    }

    int res = 0;
    char* username = argv[1];
    fprintf(stderr, "CLIENT:%s\n", username);
    CK_ZERO(res, os_connect(username), "Connection error");
    if (res == 0) return 0;

    int run;
    if (argc == 2)
        run = 1;
    else if (argc == 3) {  // caso in cui scelgo di usare i test
        switch (strtol(argv[2], NULL, 10)) {
            case 1:
                test1();
                break;
            case 2:
                test2();
                break;
            case 3:
                test3();
                break;
            default:
                break;
        }
        fprintf(stderr,
                "\n--------Risultati test--------\n\
                Op totali: %d\n\
                Op success: %d\n\
                Op fallite: %d\n\n\n",
                totalOp, successOp, failedOp);

        CK_ZERO(res, os_disconnect(), "Error LEAVE");
        fprintf(stderr, "DISCONNECTED: OK\n");
        exit(EXIT_SUCCESS);
    }

    int scelta = 0;

    while (run) {
        printf(
            "\nSelect operation:\n\
                1)STORE\n\
                2)RETRIEVE\n\
                3)DELETE\n\
                4)LEAVE\n");
        scelta = 0;
        scanf("%d", &scelta);
        char dataName[33];
        char* data;
        switch (scelta) {
            case 1:
                printf("Insert data name:");
                scanf("%s%*c", dataName);
                printf("Insert data:");
                //scanf("%*c%ms", &data);  // ms alloca data dinamicamente
                size_t len = 0;

                //data[0]='\0';
                int c;
                data=(char*)malloc(sizeof(char));
                while((c = getchar()) != '\n' && c != EOF) {
                    data = (char*)realloc(data, (len+2)*sizeof(char)); //reallocating memory
                    data[len] = (char) c; //type casting `int` to `char`
                    data[++len] = '\0'; //inserting null character at the end
                }
                CK_ZERO(res, os_store(dataName, data, strlen(data)), "Error STORE");
                free(data);
                
                break;
            case 2:
                printf("Insert data name:");
                scanf("%s", dataName);
                CK_ZERO(data, os_retrieve(dataName), "Error RETRIEVE");
                
                fprintf(stderr, "DATA: {%s}", data);
                free(data);
                
                break;
            case 3:
                printf("Insert data name:");
                scanf("%s", dataName);
                CK_ZERO(res, os_delete(dataName), "Error DELETE");
                break;
            case 4:
                CK_ZERO(res, os_disconnect(), "Error LEAVE");
                //destroyKey();
                exit(EXIT_SUCCESS);  // Da vedere se terminare il client oppure rendere possibile la connessione
                break;
            default:
                perror("Error");
                break;
        }
        // sleep(3);

        fprintf(stderr, "FINE OP\n");
    }

    return 0;
}
