/* lconv front-end */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    const char *finName[3] = { "sensor-ax.txt", "sensor-ay.txt", "sensor-az.txt" };
    char str[256];
    int i;

    printf("## lconv front-end ##\n");
    if (argc != 2) {
        printf("usage: %s output-file\n", argv[0]);
        return -1;
    }
    /*
    for (i = 0; i < 3; i++) {
        pid = fork();
        if (pid > 0) {
            wait(NULL);
            printf("%s is processed\n", finName[i]);
        }
        else if (pid == 0) {
            execl("./lconv1", finName[i], NULL);
        }
        else {
            fputs("fork failed\n", stderr);
            return -1;
        }
    }
    execl("./lconv2", argv[1], NULL); */

    for (i = 0; i < 3; i++) {
        strcpy(str, "./lconv1 ");
        strcat(str, finName[i]);
        system(str);
    }
    strcpy(str, "./lconv2 ");
    strcat(str, argv[1]);
    system(str);
    
    return 0;
}