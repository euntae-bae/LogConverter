/* lconv front-end */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <sys/wait.h>

int main(int argc, char *argv[])
{
    //pid_t pid;
    const char *finName[3] = { "sensor-ax.txt", "sensor-ay.txt", "sensor-az.txt" };
    char cmd[256];
    int i;

    printf("## lconv front-end ##\n");
    if (argc == 2) {
        sprintf(cmd, "mkdir %s", argv[1]);
        system(cmd);
    }
    else if (argc > 2) {
        printf("usage: %s [dest-directory]\n", argv[0]);
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
        strcpy(cmd, "./lconv1 ");
        strcat(cmd, finName[i]);
        system(cmd);
    }
    //strcpy(cmd, "./lconv2 ");
    //strcat(cmd, argv[1]);
    system("./lconv2");
    
    if (argc == 2) {
        sprintf(cmd, "mv *.txt ./%s", argv[1]);
        system(cmd);
    }
    
    return 0;
}