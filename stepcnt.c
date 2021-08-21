#include <stdio.h>
#include <stdlib.h>

#define READ_FILE_NAME  "sensor-win.txt"
#define WRITE_FILE_NAME "stepcnt.txt"
#define BUF_SIZE 256

int main(int argc, char **argv)
{
    FILE *fin = NULL;
    //FILE *fout = NULL;
    char finName[BUF_SIZE] = READ_FILE_NAME;
    char buf[BUF_SIZE];
    float timeCnt, vnorm, vMean, avMean;

    if (argc > 3) {
        fprintf(stderr, "usage: stepcnt [<file-info>] [<filename>]\n");
        return -1;
    }

    fin = fopen(READ_FILE_NAME, "rt");
    if (!fin) {
        fprintf(stderr, "E: failed to open file %s\n", READ_FILE_NAME);
        return -1;
    }

    fgets(buf, BUF_SIZE, fin);
    puts(buf);

    while (!feof(fin)) {
        fscanf(fin, "%f\t%f\t%f\t%f\n", &timeCnt, &vnorm, &vMean, &avMean);
        printf("%f\t%f\t%f\t%f\n", timeCnt, vnorm, vMean, avMean);
    }
    printf("실험 회차: %s\n", argv[1]);
    printf("평균 보행 속도: %f\n", vMean);
    printf("총 소요시간: %.2f초\n", timeCnt);
    fclose(fin);
    return 0;
}