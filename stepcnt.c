#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_READ_FILE_NAME  "sensor-win.txt"
#define BUF_SIZE                256
#define START_TIME_CNT          0.5
#define UNIT_TIME               0.02
#define DEFAULT_LIST_SIZE       3000

typedef struct WinEntry {
    float time;
    float vnorm;
    float avgCalcV;
    float avgRealV;
} WinEntry;

int main(int argc, char **argv)
{
    FILE *fin = NULL;
    char finName[BUF_SIZE];
    char buf[BUF_SIZE];
    WinEntry bufList[DEFAULT_LIST_SIZE];
    //puts("# step counter");

    if (argc > 3) {
        fprintf(stderr, "usage: stepcnt [<file-info>] [<filename>]\n");
        return -1;
    }

    if (argc == 3)
        strcpy(finName, argv[2]);
    else
        strcpy(finName, DEFAULT_READ_FILE_NAME);
    fin = fopen(finName, "rt");
    if (!fin) {
        fprintf(stderr, "E: failed to open file %s\n", DEFAULT_READ_FILE_NAME);
        return -1;
    }

    // 1-pass
    int curIdx = 0;
    int listSize = 0;
    WinEntry curEntry;

    fgets(buf, BUF_SIZE, fin); // 첫 줄은 읽어서 버린다.
    while (!feof(fin)) {
        fscanf(fin, "%f\t%f\t%f\t%f\n", &bufList[curIdx].time, &bufList[curIdx].vnorm, &bufList[curIdx].avgCalcV, &bufList[curIdx].avgRealV);
        listSize++;
    }

    if (argc >= 2)
        printf("실험 회차: %s\n", argv[1]);
    printf("listSize: %d\n", listSize);

    fclose(fin);
    return 0;
}