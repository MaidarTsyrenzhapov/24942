#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h> 

#define   BEL   '\07'

int count = 0;

int main(void){
    void sigcatch(int);

    signal(SIGINT, sigcatch);
    signal(SIGQUIT, sigcatch);

    while (1)
        pause();
    
    return 0;
}

void sigcatch(int sig){
    if (sig == SIGQUIT){
        printf("прозвучало %d раз\n", count);
        exit(0);
    }
    printf("%c\n", BEL);
    count++;
    signal(sig, sigcatch);
}