#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>

/* constantes */
#define MAXLI 2048

/* variables globales */
char cmd[MAXLI];
char path[MAXLI];
char cheminHistory[MAXLI];
extern char **environ;
char *home;

/* programme principal */
void mbash();

char *which();
void commandeGenerale();
int estArrierePlan();

/* commandes built-in */
void cd();
void pwd();
void history();
int replace_$$_pid();
int replace_tild_home();
int getHisoryLength();
char* getHistory(int index);

/* commande du prompt */
void prompt();

int main(int argc, char** argv) {

    home = getenv("HOME");
    strcpy(cheminHistory, home);
    strncat(cheminHistory, "/.bash_history", strlen("/.bash_history")+1);

    while (1) {

        int tailleCommande = 0;

        struct termios term, term_orig;

        tcgetattr(STDIN_FILENO, &term_orig); // sauvegarder les paramètres d'entrée originaux
        term = term_orig;
        term.c_lflag &= ~(ICANON | ECHO); // désactiver l'entrée canonique et l'écho
        term.c_cc[VMIN] = 1; // un seul caractère est nécessaire pour retourner
        term.c_cc[VTIME] = 0; // bloquer jusqu'à ce qu'un caractère soit lu
        tcsetattr(STDIN_FILENO, TCSANOW, &term); // appliquer les nouveaux paramètres

        prompt();
        int continu = 1;
        int posCurseur = 0;

        for (int i = 0; i < MAXLI; ++i) {
            cmd[i] = '\0';
        }

        int nbHistory = getHisoryLength()-1;

        while (continu) {
            char c = getchar();
            switch (c) {
                case 27:
                    getchar();
                    switch (getchar()) {
                        case 'A':
                            // flèche du haut
                            if (nbHistory > 1) {
                                // supprime l'affichage déjà présent jusqu'au prompt
                                while (tailleCommande > 0) {
                                    printf("\b \b");
                                    cmd[tailleCommande--] = '\0';
                                    posCurseur--;
                                }
                                nbHistory--;
                                char* commande_history = getHistory(nbHistory);
                                commande_history[strcspn(commande_history, "\n")] = 0;
                                char c = commande_history[tailleCommande];
                                // imprime l'ancienne commande pésente dans l'historique et l'affecte dans la variable cmd
                                while (c != '\0') {
                                    putchar(c);
                                    cmd[tailleCommande] = c;
                                    tailleCommande++;
                                    posCurseur++;
                                    c = commande_history[tailleCommande];
                                }
                            }
                            break;
                        case 'B':
                            // flèche du bas
                            if (nbHistory < getHisoryLength()-1) {
                                // supprime tout l'affichage
                                while (tailleCommande > 0) {
                                    printf("\b \b");
                                    cmd[--tailleCommande] = '\0';
                                    posCurseur--;
                                }
                                nbHistory++;
                                char* commande_history = getHistory(nbHistory);
                                commande_history[strcspn(commande_history, "\n")] = 0;
                                char c = commande_history[tailleCommande];
                                while (c != '\0') {
                                    putchar(c);
                                    cmd[tailleCommande] = c;
                                    tailleCommande++;
                                    posCurseur++;
                                    c = commande_history[tailleCommande];
                                }
                            }
                            break;
                    }
                    break;
                case '\x7f' :
                case '\x08' :
                    // si la touche backspace est appuyée, on supprime un caractère de l'affichage
                    if (tailleCommande > 0) {
                        printf("\b \b");
                        cmd[--tailleCommande] = '\0';

                    }
                    break;
                case '\n':
                    continu = 0;
                    putchar(c);
                    break;
                default :
                    putchar(c);
                    cmd[tailleCommande] = c;
                    tailleCommande++;
                    posCurseur++;
                    break;
            }
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &term_orig); // rétablir les paramètres d'entrée originaux


        if (*cmd != 0) {
            mbash();
        }
    }
    return 0;
}

void mbash() {

    /* partie servant pour la commande history de mbash */
    FILE *file;
    file = fopen(cheminHistory, "a");
    if (file != NULL) {
        fprintf(file, "%s\n", cmd);
    }
    fclose(file);

    /* remplace $$ par le pid de mbash */
    replace_$$_pid(cmd);
    /* remplace ~ par le home de l'utilisateur */
    replace_tild_home(cmd);

    char *arguments = NULL;
    char *cmd_copy = strdup(cmd);
    char *commande = strtok_r(cmd_copy, " ", &arguments);

    /* commandes built-in */
    if (strcmp(commande, "cd")==0) {
        cd(arguments);
    }
    else if (strcmp(commande, "pwd")==0) {
        pwd();
    }
    else if (strcmp(commande, "exit")==0) {
        exit(0);
    }
    else if (strcmp(commande, "history")==0) {
        history();
    }
        /* la commande n'est pas une commande built-in */
    else  {
        pid_t pid = fork();
        if (pid == 0) {
            if (estArrierePlan(cmd)) {
                int pidArrierrePlan = fork();
                if (pidArrierrePlan == 0) {
                    commandeGenerale(cmd);
                    exit(0);
                } else {
                    printf("pid du processus en arrière-plan : %d\n", pidArrierrePlan);
                    exit(0);
                }
            } else {
                commandeGenerale(cmd);
                exit(0);
            }
        } else {
            wait(NULL);
        }
    }
}


int replace_$$_pid() {
    char *pch;
    pid_t pid = getpid();
    char pid_str[20];
    snprintf(pid_str, sizeof pid_str, "%d", pid);

    pch = strstr(cmd, "$$");
    if (pch) {
        int index = pch - cmd;
        int pid_str_len = strlen(pid_str);
        int str_len = strlen(cmd);
        int new_str_len = str_len - 2 + pid_str_len;
        char new_str[new_str_len+1];

        strncpy(new_str, cmd, index);
        strncpy(new_str+index, pid_str, pid_str_len);
        strncpy(new_str+index+pid_str_len, pch+2, str_len-index-2);
        new_str[new_str_len] = '\0';
        strcpy(cmd, new_str);
        return 1;
    }
    return 0;
}

int replace_tild_home() {
    char *pch;
    if(home == NULL) return 0;
    pch = strstr(cmd, "~");
    if (pch) {
        int index = pch - cmd;
        int home_len = strlen(home);
        int str_len = strlen(cmd);
        int new_str_len = str_len - 1 + home_len;
        char new_str[new_str_len+1];

        strncpy(new_str, cmd, index);
        strncpy(new_str+index, home, home_len);
        strncpy(new_str+index+home_len, pch+1, str_len-index-1);
        new_str[new_str_len] = '\0';
        strcpy(cmd, new_str);
        return 1;
    }
    return 0;
}



int estArrierePlan() {
    int index = 0;
    char carCourant = cmd[index];
    int valRetour = 0;

    while (carCourant != '\0') {
        index++;
        carCourant = cmd[index];
        if (carCourant == '&') {
            valRetour = 1;
            /* supprime la suite de la commande */
            int j = index;
            while (cmd[j] != '\0') {
                cmd[j] = '\0';
                j++;
            }
            break;
        }
    }
    return valRetour;
}

/* lancement de commandes UNIX */

char *which(char *cmd) {
    char *path_env = getenv("PATH");
    char *paths = strdup(path_env);
    char *path;
    char *command_path = NULL;
    while ((path = strsep(&paths, ":")) != NULL) {
        size_t length = strlen(path) + strlen(cmd) + 2;
        command_path = malloc(length);
        if (!command_path) {
            printf("mbash : erreur interne : n'a pas pu allouer de mémoire\n");
            exit(1);
        }
        snprintf(command_path, length, "%s/%s", path, cmd);
        if (access(command_path, X_OK) == 0) {
            return command_path;
        }
        free(command_path);
        command_path = NULL;
    }
    printf("mbash : commande inconnue\n");
    execlp("","");
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

/* Commandes built-in */

void cd(char *chemin) {
    chdir(chemin);
}


void pwd() {
    printf("%s\n",getcwd(path, 1000));
}

void history() {
    char line[256];
    int index = 1;

    FILE *historique;
    historique = fopen(cheminHistory, "r");

    while (fgets(line, sizeof(line), historique)) {
        printf("%d\t%s", index, line);
        index++;
    }

    fclose(historique);
}

int getHisoryLength() {
    char line[256];
    int index = 1;

    FILE *historique;
    historique = fopen(cheminHistory, "r");

    while (fgets(line, sizeof(line), historique)) {
        index++;
    }

    fclose(historique);
    return index;
}

char* getHistory(int index) {
    char line[256];
    int i = 1;
    FILE *historique;
    historique = fopen(cheminHistory, "r");

    while (fgets(line, sizeof(line), historique)) {
        if (i == index) {
            char* retour = (char*) malloc(sizeof(char) * (strlen(line) + 1));
            strcpy(retour, line);
            fclose(historique);
            return retour;
        }
        i++;
    }
    fclose(historique);
    return "";
}

/* affichage du prompt */
void prompt() {
    char *chemin = getcwd(path, 1000);
    int indexChemin = 0;

    char dernierDossier[100];
    int indexDernierDossier = 0;

    char car = chemin[indexChemin];
    while (car != '\0') {
        if (car == '/') {
            for (int i = 0; i < 100; i++) {
                dernierDossier[i] = '\0';
            }
            indexDernierDossier = 0;
            if (chemin[indexChemin+1] == '\0') {
                dernierDossier[0] = '/';
                break;
            }
        } else {
            dernierDossier[indexDernierDossier] = car;
            indexDernierDossier++;
        }
        indexChemin++;
        car = chemin[indexChemin];
    }

    printf(" %s $ ", dernierDossier);
}