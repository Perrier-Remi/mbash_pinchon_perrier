//
// Created by wartt88 on 16/01/23.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#define MAXLI 2048

char cmd[MAXLI];
char path[MAXLI];
int pathidx;
void mbash();
int main(int argc, char** argv) {
    while (1) {
        printf("Commande: ");
        fgets(cmd, MAXLI, stdin);
        mbash(cmd);
    }
    return 0;
}

void mbash() {
    char *argv[] = {"ls", "-l", "/"};
    char *envp[] = {NULL};
    pid_t pid = fork();
    if(pid == 0){
        execve("/bin/ls", argv, envp);
        exit(0);
    }else{
        wait(NULL);
    }
}