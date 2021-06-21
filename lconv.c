/* lconv front-end */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const char *finName[3] = { "sensor-ax.txt", "sensor-ay.txt", "sensor-az.txt" };
    char cmd[256];
    int i;

    printf("## lconv front-end ##\n");
    if (argc == 2 || argc == 3) {
        sprintf(cmd, "mkdir %s", argv[1]);
        system(cmd);
    }
    else {
        printf("usage: %s [dest-directory [window_size]]\n", argv[0]);
        return -1;
    }

    for (i = 0; i < 3; i++) {
        strcpy(cmd, "./lconv1 ");
        strcat(cmd, finName[i]);
        system(cmd);
    }
    
    if (argc == 3) {
        sprintf(cmd, "./lconv3 %s", argv[2]);
    }
    else
        sprintf(cmd, "./lconv3");
    system(cmd);

    if (argc >= 2) {
        sprintf(cmd, "mv *.txt ./%s", argv[1]);
        system(cmd);
    }
    return 0;
}