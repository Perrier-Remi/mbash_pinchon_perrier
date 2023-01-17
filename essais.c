#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>

#define MAXLI 2048

char cmd[MAXLI];
char path[MAXLI];
int pathidx;
extern char **environ;

char *which(char *cmd);

void mbash();

void commandeGenerale();


int main(int argc, char **argv) {
    while (1) {
        printf("Commande: ");
        fgets(cmd, MAXLI, stdin);
        mbash(cmd);
    }
    return 0;
}

void mbash() {
    pid_t pid = fork();
    if (pid == 0) {
        commandeGenerale(cmd);
        exit(0);
    } else {
        wait(NULL);
    }
}

char *which(char *cmd) {
    char *path_env = getenv("PATH");
    char *paths = strdup(path_env);
    char *path;
    char *command_path = NULL;
    while ((path = strsep(&paths, ":")) != NULL) {
        size_t length = strlen(path) + strlen(cmd) + 2;
        command_path = malloc(length);
        if (!command_path) {
            printf("Error: Failed to allocate memory\n");
            exit(1);
        }
        snprintf(command_path, length, "%s/%s", path, cmd);
        if (access(command_path, X_OK) == 0) {
            return command_path;
        }
        free(command_path);
        command_path = NULL;
    }
    printf("Command not found\n");
    execlp("", "");
}

void commandeGenerale() {
    char *argv[MAXLI];
    char *envp[] = {NULL};

    char *cmd_copy = strdup(cmd);
    cmd_copy = strtok(cmd_copy, "\n");
    int i = 0;
    char *token = strtok(cmd_copy, " ");

    while (token != NULL) {
        argv[i] = token;
        i++;
        token = strtok(NULL, " ");
    }
    argv[i] = NULL;
    char *str = which(argv[0]);

    int exec_result = execve(str, argv, environ);
    if (exec_result == -1) {
        perror("Error executing command");
    }
    free(str);
    free(cmd_copy);

}
