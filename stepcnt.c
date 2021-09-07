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

// 극값을 갖는 점
typedef struct InflectPoint {
    char status;
    int idx;
} InflectPoint;

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

int getMaxFromList(const WinEntry *buf, const InflectPoint *list, int len) {
    int i;
    int maxIdx = list[0].idx;
    for (i = 1; i < len; i++) {
        if (buf[maxIdx].vnorm < buf[list[i].idx].vnorm)
            maxIdx = list[i].idx;
    }
    return maxIdx;
}

int getMinFromList(const WinEntry *buf, const InflectPoint *list, int len) {
    int i;
    int minIdx = list[0].idx;
    for (i = 1; i < len; i++) {
        if (buf[minIdx].vnorm > buf[list[i].idx].vnorm)
            minIdx = list[i].idx;
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

void bubble_sortf(float *list, int len) {
    int i, j;
    float temp;
    for (i = 0; i < len; i++) {
        for (j = 0; j < len - i - 1; j++) {
            if (list[j] > list[j + 1]) {
                temp = list[j + 1];
                list[j + 1] = list[j];
                list[j] = temp;
            }
        }
    }
}

void usage(void) {
    fprintf(stderr, "usage: stepcnt [<file-info>] [<filename>]\n");
}

int main(int argc, char **argv)
{
    FILE *fin = NULL;
    char finName[BUF_SIZE];
    char buf[BUF_SIZE];
    WinEntry bufList[DEFAULT_LIST_SIZE];
    //puts("# step counter");

    if (argc > 3) {
        usage();
        return -1;
    }

    if (argc == 3)
        strcpy(finName, argv[2]);
    else
        strcpy(finName, DEFAULT_READ_FILE_NAME);
    fin = fopen(finName, "rt");
    if (!fin) {
        fprintf(stderr, "E: failed to open file %s\n", DEFAULT_READ_FILE_NAME);
        usage();
        return -1;
    }

#ifdef _DEBUG
    if (argc >= 2)
        printf("실험 회차: %s\n", argv[1]);
#endif 

    /* pass-1 */
    int curIdx = 0;
    int listSize = 0;
    float elapsedTime;
    //WinEntry curEntry;

    fgets(buf, BUF_SIZE, fin); // 첫 줄은 읽어서 버린다.
    while (!feof(fin)) {
        fscanf(fin, "%f\t%f\t%f\t%f\n", &bufList[curIdx].time, &bufList[curIdx].vnorm, &bufList[curIdx].avgCalcV, &bufList[curIdx].avgRealV);
        listSize++;
        curIdx++;
    }
    fclose(fin);
    elapsedTime = bufList[listSize - 1].time;

    /* pass-2 */
    int i;
    int ret;

    InflectPoint *inflList = (InflectPoint*)malloc(sizeof(InflectPoint) * listSize);
    int inflCnt = 0;
    int stepcnt = 0; // 걸음수
    
    int maxCnt = 0, minCnt = 0;
    float maxSum = 0.0f, minSum = 0.0f;
    float maxAvg, minAvg;
    for (i = 1; i < listSize; i++) {
        ret = win_isInflect(bufList, i);
        if (ret == 1) { // +
            //printf("%f\t%f\t\n", bufList[i].time, bufList[i].vnorm);
            inflList[inflCnt].status = '+';
            inflList[inflCnt].idx = i;
            inflCnt++;
            maxSum += bufList[i].vnorm;
            maxCnt++;
        }
        else if (ret == 2) { // -
            //printf("%f\t\t%f\n", bufList[i].time, bufList[i].vnorm);
            inflList[inflCnt].status = '-';
            inflList[inflCnt].idx = i;
            inflCnt++;
            minSum += bufList[i].vnorm;
            minCnt++;
        }
    }
    maxAvg = maxSum / (float)maxCnt;
    minAvg = minSum / (float)minCnt;

    /* 고점(극대)에서 저점(극소)으로 떨어지는 값들을 계산한다. */
    // inflDiffTbl: inflList와 같은 인덱스 체계를 공유한다.
    // diffList: inflDiffTbl로부터 극대에서 극소로 떨어지는 경우의 vnorm delta 값을 추출하여 저장한다. 값을 저장한 후 오름차순으로 정렬하여 사용한다.
    // TODO: inflDiffTbl을 inflList로 통합; InflectPoint 구조체에 diff 멤버를 추가한다.
    float *inflDiffTbl = (float*)malloc(sizeof(float) * inflCnt);
    int startPoint;
    
    float *diffList = NULL;
    int diffCnt = 0;

    for (i = 0; i < inflCnt; i++) {
        inflDiffTbl[i] = bufList[inflList[i].idx].vnorm;
    }
    startPoint = (inflList[0].status == '+') ? 0 : 1;
    for (i = startPoint; (i + 1) < inflCnt; i += 2) {
        inflDiffTbl[i] -= inflDiffTbl[i + 1];
        //diffCnt++;
    }
    for (i = 0; i < inflCnt; i++) {
        if (inflList[i].status == '+')
            diffCnt++;
    }
    // printf("inflDiffTbl:\n");
    // printf("[s idx] vnorm\t\tvnormDiff\n");

    // for (i = 0; i < inflCnt; i++) {
    //     printf("[%c%04d] %f\t%f\n", inflList[i].status, inflList[i].idx, bufList[inflList[i].idx].vnorm, inflDiffTbl[i]);
    // }
    // puts("");
    diffList = (float*)malloc(sizeof(float) * diffCnt);
    curIdx = 0;
    for (i = 0; i < inflCnt; i++) {
        if (inflList[i].status == '+') {
            // printf("[%c%04d] %f\t%f\n", inflList[i].status, inflList[i].idx, bufList[inflList[i].idx].vnorm, inflDiffTbl[i]);
            diffList[curIdx++] = inflDiffTbl[i];
        }
    }
    // printf("> 총 %d개\n\n", i);

    bubble_sortf(diffList, diffCnt);
    // for (i = 0; i < diffCnt; i++)
    //     printf("diffList[%d]: %f\n", i, diffList[i]);

    const int DIFF_MEAN_LIST_SIZE = diffCnt / 3;    // diffList의 1/3개를 각각의 평균의 기준으로 잡는다.
    float sdiffMean = 0.0f, ldiffMean = 0.0f;       // 격차가 작은 경우의 평균과 큰 경우의 평균 (그런 두 가지 상태로 나뉘는 경향이 있다)
    for (i = 0; i < DIFF_MEAN_LIST_SIZE; i++) {
        sdiffMean += diffList[i];
        ldiffMean += diffList[diffCnt - 1 - i];
    }
    sdiffMean /= DIFF_MEAN_LIST_SIZE;
    ldiffMean /= DIFF_MEAN_LIST_SIZE;
    //printf("small diff mean: %f\n", sdiffMean);
    //printf("large diff mean: %f\n", ldiffMean);

    /* 걸음 수 계수 */
    stepcnt = 0;
    // 임계치 thMax, thMin 정의 (평균으로부터 n%)
    const float ratioMax = 0.2f;  // 20%
    const float ratioMin = 0.5f;  // 50%
    const float thMax = ldiffMean * (1.0f - ratioMax); // 정점(Peak, Crest)의 상위 평균에서 30%를 뺀 값
    const float thMin = sdiffMean * (1.0f + ratioMin); // 골(Trough)의 하위 평균에서 30%를 더한 값
    
#ifdef _DEBUG
    printf("large diff mean: %f, small diff mean: %f\n", ldiffMean, sdiffMean);
    printf("thMax: %f, thMin: %f\n", thMax, thMin);
#endif

    for (i = 0; i < inflCnt; i++) {
        curIdx = inflList[i].idx;
#ifdef _DEBUG
        printf("[%c%d] %f: %f\n", inflList[i].status, inflList[i].idx, bufList[curIdx].time, bufList[curIdx].vnorm);
#endif
        if (inflList[i].status == '+' && bufList[curIdx].vnorm >= thMax && (i + 1) < inflCnt) { // 정점과 임계치 조건 검사 
            curIdx = inflList[i + 1].idx;
            if (inflList[i + 1].status == '-' && bufList[curIdx].vnorm <= thMin) { // 골과 임계치 조건 검사
                stepcnt++;
            }
            // if (inflList[i + 1].status == '-')
            //     stepcnt++;
        }
    }
    /*for (i = 0; i < diffCnt; i++) {
        if (diffList[i] <= ldiffMean * 1.3f && diffList[i] >= ldiffMean * 0.7f) {
            stepcnt++;
        }
    } */ 

#ifdef _DEBUG
    printf("stepcnt: %d\n\n", stepcnt * 2);
#else
    printf("%d", stepcnt * 2);
#endif
    /* 첫 세 극소값을 추출한다. */
    // 이 중 가장 작은 점을 첫 번째 걸음이라 가정한다.
    /*
    struct Point {
        int bufListIdx;
        int inflListIdx;
    };
    struct Point tripleMin[3];
    struct Point mmin; // 세 점 중 vnorm이 가장 작은 점
    curIdx = 0;
    i = 0;
    while (curIdx < 3) {
        if (inflList[i].status == '-') {
            tripleMin[curIdx].inflListIdx = i;
            tripleMin[curIdx].bufListIdx = inflList[i].idx;
            curIdx++;
        }
        i++;
    }

    for (i = 0; i < 3; i++) {
        if (bufList[mmin.bufListIdx].vnorm > bufList[tripleMin[i].bufListIdx].vnorm)
            mmin = tripleMin[i];
        printf("bufList[%d]: %f\n", tripleMin[i].bufListIdx, bufList[tripleMin[i].bufListIdx].vnorm);
    }
    printf("[%d]: %f\t%f\n", mmin.bufListIdx, bufList[mmin.bufListIdx].time, bufList[mmin.bufListIdx].vnorm); */


    /* 극값 목록 출력 */
#ifdef _DEBUG
    printf("time\tlocal max\tlocal min\tavg(lmax)\tavg(lmin)\n");
    for (i = 0; i < inflCnt; i++) {
        curIdx = inflList[i].idx;
        if (inflList[i].status == '+') {
            printf("%f\t%f\t\t%f\t%f\n", bufList[curIdx].time, bufList[curIdx].vnorm, maxAvg, minAvg);
        }
        else {
            printf("%f\t\t%f\t%f\t%f\n", bufList[curIdx].time, bufList[curIdx].vnorm, maxAvg, minAvg);
        }
    }
    printf("> 극값 개수: %d\n", inflCnt);
    printf("> 극대: %d\t극소: %d\n", maxCnt, minCnt);
    printf("> 극대 평균: %f\t극소 평균: %f\n", maxAvg, minAvg);
#endif

    // int maxIdx = getMaxFromList(bufList, inflList, inflCnt);
    // int minIdx = getMinFromList(bufList, inflList, inflCnt);
    // printf("maxCnt: %d\tmaxIdx: %d\tmax: %f\n", maxCnt, maxIdx, bufList[maxIdx].vnorm);
    // printf("minCnt: %d\tminIdx: %d\tmin: %f\n", minCnt, minIdx, bufList[minIdx].vnorm);


    //printf("listSize: %d\n", listSize);

    free(diffList);
    free(inflDiffTbl);
    free(inflList);
    return 0;
}