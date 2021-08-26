#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_READ_FILE_NAME  "sensor-win.txt"
#define BUF_SIZE                256
#define START_TIME_CNT          0.5
#define UNIT_TIME               0.02
#define DEFAULT_LIST_SIZE       2000

#define TRUE    1
#define FALSE   0

// # 용어
// 변곡점: inflection point
// 극대: maximal, 극소: minimal (global/local)
// 최대: global maximal, 최소: global minimal
// 파장: wave length
// 주기: period

typedef struct WinEntry {
    float time;
    float vnorm;
    float avgCalcV;
    float avgRealV;
} WinEntry;

typedef struct Wave {
    int startIdx;
    int endIdx;
    float period;
} Wave;

// 극값을 갖는 점
typedef InflectPoint {
    int idx;
    char status;
};

void win_dump(const WinEntry *list, int len, int ln) {
    int i;
    for (i = 0; i < len; i++) {
        if (ln)
            printf("%04d:\t", i);
        printf("%f\t%f\t%f\t%f\n", list[i].time, list[i].vnorm, list[i].avgCalcV, list[i].avgRealV);
    }
    printf("\n");
}

float win_maximum(const WinEntry *list, int len) {
    int i;
    float max = list[0].vnorm;
    for (i = 1; i < len; i++) {
        if (max < list[i].vnorm)
            max = list[i].vnorm;
    }
    return max;
}

float win_minimum(const WinEntry *list, int len) {
    int i;
    float min = list[0].vnorm;
    for (i = 1; i < len; i++) {
        if (min > list[i].vnorm)
            min = list[i].vnorm;
    }
    return min;
}

int getMaxFromList(const WinEntry *buf, const int *list, int len) {
    int i;
    int maxIdx = buf[list[0]].vnorm;
    for (i = 1; i < len; i++) {
        if (buf[maxIdx].vnorm < buf[list[i]].vnorm)
            maxIdx = list[i];
    }
    return maxIdx;
}

int getMinFromList(const WinEntry *buf, const int *list, int len) {
    int i;
    int minIdx = buf[list[0]].vnorm;
    for (i = 1; i < len; i++) {
        if (buf[minIdx].vnorm > buf[list[i]].vnorm)
            minIdx = list[i];
    }
    return minIdx;
}
// 1: 위로 볼록 (극대)
// 2: 아래로 볼록 (극소)
// 0: 변곡점 아님
int win_isInflect(const WinEntry *list, int idx) {
    float prevV = list[idx - 1].vnorm;
    float curV = list[idx].vnorm;
    float nextV = list[idx + 1].vnorm;

    if ((curV > prevV) && (curV > nextV))
        return 1;
    else if ((curV < prevV) && (curV < nextV))
        return 2;
    return 0;
}

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
    float elapsedTime;
    WinEntry curEntry;

    fgets(buf, BUF_SIZE, fin); // 첫 줄은 읽어서 버린다.
    while (!feof(fin)) {
        fscanf(fin, "%f\t%f\t%f\t%f\n", &bufList[curIdx].time, &bufList[curIdx].vnorm, &bufList[curIdx].avgCalcV, &bufList[curIdx].avgRealV);
        listSize++;
        curIdx++;
    }
    fclose(fin);
    // win_dump(bufList, listSize, 0);

    elapsedTime = bufList[listSize - 1].time;

    int i;
    int ret;
    int *localMaxList = (int*)malloc(sizeof(int) * listSize);       // 극대점들의 인덱스 저장
    int *localMinList = (int*)malloc(sizeof(int) * listSize);       // 극소점들의 인덱스 저장
    int *negPeakList = (int*)malloc(sizeof(int) * listSize);        // 극대에서 극소로 떨어지는 점들의 인덱스 저장
    float *periodList = (float*)malloc(sizeof(float) * listSize);   // 주기의 
    int maxCnt = 0, minCnt = 0;
    int negPeakCnt = 0;
    int periodCnt = 0;
    int stepcnt = 0; // 걸음수
    //printf("time\tlocal max\tlocal min\n");
    for (i = 1; i < listSize; i++) {
        ret = win_isInflect(bufList, i);
        if (ret == 1) { // +
            //printf("%f\t%f\t\n", bufList[i].time, bufList[i].vnorm);
            localMaxList[maxCnt++] = i;
            if ((i + 1 < listSize) && win_isInflect(bufList, i + 1) == 2) {
                negPeakList[negPeakCnt++] = i + 1;
            }
        }
        else if (ret == 2) { // -
            //printf("%f\t\t%f\n", bufList[i].time, bufList[i].vnorm);
            localMinList[minCnt++] = i;
        }
    }
    /*
    printf("maxCnt: %d\n", maxCnt);
    for (i = 0; i < maxCnt; i++)
        //printf("%d\t%f\t%f\n", localMaxList[i], bufList[localMaxList[i]].time, bufList[localMaxList[i]].vnorm);
        printf("%f\n", bufList[localMaxList[i]].vnorm);
    printf("\nminCnt: %d\n", minCnt);
    for (i = 0; i < minCnt; i++)
        //printf("%d\t%f\t%f\n", localMinList[i], bufList[localMinList[i]].time, bufList[localMinList[i]].vnorm);
        printf("%f\n", bufList[localMinList[i]].vnorm);
    printf("\nnegPeakCnt: %d\n", negPeakCnt);
    for (i = 0; i < negPeakCnt; i++) {
        printf("%f\t%f\n", bufList[negPeakList[i]].time, bufList[negPeakList[i]].vnorm);
    }
    */
   int maxIdx = getMaxFromList(bufList, localMaxList, maxCnt);
   int minIdx = getMinFromList(bufList, localMinList, minCnt);
   printf("maxCnt: %d\tmaxIdx: %d\tmax: %f\n", maxCnt, maxIdx, bufList[maxIdx].vnorm);
   printf("minCnt: %d\tminIdx: %d\tmin: %f\n", minCnt, minIdx, bufList[minIdx].vnorm);
   printf("negPeakCnt: %d\n", negPeakCnt);

    if (argc >= 2)
        printf("실험 회차: %s\n", argv[1]);
    //printf("listSize: %d\n", listSize);
    free(localMaxList);
    free(localMinList);
    free(negPeakList);
    free(periodList);
    
    return 0;
}