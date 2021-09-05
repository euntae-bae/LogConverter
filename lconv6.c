/* lconv6: 3축 가속도 신호의 SVM을 출력하는 프로그램
 * 입력: sensor-ax-out.txt, sensor-ay-out.txt, sensor-az-out.txt
 * 출력: sensor-svm.txt
**/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define PROGRAM_VERSION "6"

double getNorm(double x, double y, double z) {
	return sqrt((x * x) + (y * y) + (z * z));
}

int main(void)
{
    const char *readFileList[] = { "sensor-ax-out.txt", "sensor-ay-out.txt", "sensor-az-out.txt" };
    const char *writeFile = "sensor-svm.txt";
    FILE *fpList[3] = { 0, };
    FILE *fpWrite = NULL;
    int i;

	printf("## log converter version %s ##\n", PROGRAM_VERSION);

    for (i = 0; i < 3; i++) {
        fpList[i] = fopen(readFileList[i], "rt");
        if (!fpList[i]) {
            fprintf(stderr, "E: failed to open file %s\n", readFileList[i]);
            return -1;
        }
    }

    fpWrite = fopen(writeFile, "wt");
    if (!fpWrite) {
        fprintf(stderr, "E: failed to open file %s\n", writeFile);
        return -1;
    }

    double timeCnt;
    double blankTime;
    double ax, ay, az;
    double anorm; // SVM

    while (!feof(fpList[0])) {
        fscanf(fpList[0], "%lf\t%lf\n", &timeCnt, &ax);
        fscanf(fpList[1], "%lf\t%lf\n", &blankTime, &ay);
        fscanf(fpList[2], "%lf\t%lf\n", &blankTime, &az);
        //printf("%.2lf\t%lf\t%lf\t%lf\n", timeCnt, ax, ay, az);
        
        anorm = getNorm(ax, ay, ax);
        fprintf(fpWrite, "%.2lf\t%lf\n", timeCnt, anorm);
    }

    fclose(fpWrite);
    for (i = 0; i < 3; i++)
        fclose(fpList[i]);
    return 0;
}