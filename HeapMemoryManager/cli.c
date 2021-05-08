#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "memmgr.h"
#include "cli.h"
#include "css.h"

char tokbuf[MAX_INPUT_SIZE];
int toki = 0;

char* tokstr(char* inp) {
    if(inp) {
        strcpy(tokbuf, inp);
        toki = 0;
    }
    char* rbuf = (char*) malloc(MAX_TOKEN_SIZE*sizeof(char));
    int ri = 0;
    
    int quoted = 0;
    for (; tokbuf[toki]; toki++)
    {
        switch(tokbuf[toki]) {
            case '"':
            case '\'':
                if(!quoted) {
                    if(ri > 0) {
                        rbuf[ri] = 0;
                        return rbuf;
                    } else {
                        quoted = 1;
                    }
                } else {
                    rbuf[ri] = 0;
                    toki++;
                    return rbuf;
                }
                break;
            case ' ':
            case '\t':
                if(quoted) rbuf[ri++] = tokbuf[toki];
                else {
                    if(ri > 0) {
                        rbuf[ri] = 0;
                        toki++;
                        return rbuf;
                    }
                }
                break;
            default:
                rbuf[ri++] = tokbuf[toki];
        }
    }

    if(ri == 0) return NULL;
    rbuf[ri] = 0;
    return rbuf;
}

sem_t mutex_deb, mutex_main;

void show_help() {
    printf("Debugger for the memmgr v1.0.\n");
    printf("The following commands are supported:\n");
    printf("deb help\n\tDisplay this help message\n");
    printf("deb register [struct name] [size]\n\tRegister a page family for the given struct of given size with the memory manager.\n");

}

void show_prompt() {
    printf(GREEN_COLOR "memmgr" RESET_COLOR "@" BLUE_COLOR "debug" RESET_COLOR " $ ");
    fflush(stdout);
}

int get_line(char* buf) {
    if(fgets(buf, MAX_INPUT_SIZE, stdin) == NULL) return -1;
    int len = strlen(buf);
    if(buf[len-1] == '\n') buf[len-1] = 0;
    return 0;
}

void display_error() {
    printf("Error: invalid syntax\nType 'deb help' to display the available commands\n");
}

// int get_page_family_name(char* buf, char** tokens) {
//     if(*tokens == NULL) return -1;
//     if((*tokens)[0] != '"' && (*tokens)[0] != '\'') return -1;
//     while(*tokens) {
//         int len = strlen(*tokens);
//         if((*tokens)[len-1] == '"' || (*tokens)[len-1] == '\'') (*tokens)[len-1] = 0;
//             strcat(buf, )
//         tokens++;
//     }
// }

int process_cmd(char* tokens[]) {
    if(tokens[0] == NULL) return 0;

    if(strcmp(tokens[0], "deb") != 0) {
        display_error();
        return 0;
    } 
    if(tokens[1] == NULL) {
        display_error(); 
        return 0;
    }
    if(strcmp(tokens[1], "allocate") == 0) {
        if(tokens[2] == NULL || tokens[3] == NULL) {
            display_error();
            return 0;
        }
        int count = atoi(tokens[3]);
        if(count < 1) {
            display_error();
            return 0;
        }

        if(xmalloc(tokens[2], count)) {
            printf("Successfully allocated memory for %d instances of %s\n", count, tokens[2]);
        } else {
            printf("Error: unable to allocate memory\n");
        }
        
        return 0;
    } else if(strcmp(tokens[1], "free") == 0) {
        if(tokens[2] == NULL) {
            display_error();
            return 0;
        }
        void* ptr = (void*) strtol(tokens[2], NULL, 0);
        printf("%p\n", ptr);
        xfree(ptr);
        return 0;
    } else if(strcmp(tokens[1], "show") == 0) {
        if(tokens[2] == NULL) {
            display_error();
            return 0;
        }
        if(strcmp(tokens[2], "mem") == 0) {
            mm_print_memory_usage(tokens[3]);
        } else if(strcmp(tokens[2], "blocks") == 0) {
            mm_print_block_usage();
        } else if(strcmp(tokens[2], "families") == 0) {
            mgr_print_reg_page_families();
        } else {
            display_error();
        }
        return 0;
    } else if(strcmp(tokens[1], "register") == 0) {
        if(tokens[2] == NULL || tokens[3] == NULL) {
            display_error();
            return 0;
        }   
        size_t size = atoi(tokens[3]);
        mgr_register_page_family(tokens[2], size);
        return 0;
    } else if(strcmp(tokens[1], "continue") == 0) {
        return -1;
    } else {
        display_error();
        return 0;
    }
}

int cli_loop() {
    show_prompt();
    char inpbuf[MAX_INPUT_SIZE];
    if(get_line(inpbuf) == -1) return -1;

    char* tokens[MAX_TOKENS];
    int tokcnt = 0;

    tokens[tokcnt] = tokstr(inpbuf);
    while(tokens[tokcnt]) {
        // printf("%s\n", tokens[tokcnt]);
        tokens[++tokcnt] = tokstr(NULL); 
    }

    return process_cmd(tokens);
    
}

void* cli_main(void* arg) {

    while(1) {
        sem_wait(&mutex_deb);
        // mgr_print_reg_page_families();
        // mm_print_memory_usage(NULL);

        while(cli_loop() == 0) ;

        sem_post(&mutex_main);

    }
}

void cli_init() {
    sem_init(&mutex_deb, 0, 0);
    sem_init(&mutex_main, 0, 0);

    pthread_t deb_thr;
    pthread_create(&deb_thr, NULL, cli_main, NULL);
}

void debug() {
    sem_post(&mutex_deb);
    sem_wait(&mutex_main);
}