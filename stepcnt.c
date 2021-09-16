#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _SVM_MODE
    #define DEFAULT_READ_FILE_NAME  "sensor-svm.txt"
    #define PROGRAM_VERSION "SVM MODE"
#else
    #define DEFAULT_READ_FILE_NAME  "sensor-win.txt"
    #define PROGRAM_VERSION "WIN MODE"
#endif
#define BUF_SIZE                256
#define START_TIME_CNT          0.5
#define UNIT_TIME               0.02
#define DEFAULT_LIST_SIZE       2000

#define TH_FLAG_L1 1 << 0
#define TH_FLAG_L2 1 << 1
#define TH_FLAG_U1 1 << 2
#define TH_FLAG_U2 1 << 3
#define TH_FLAG_LO  (TH_FLAG_L1 | TH_FLAG_L2)
#define TH_FLAG_UP  (TH_FLAG_U1 | TH_FLAG_U2)

#define TRUE    1
#define FALSE   0

// # 용어
// 변곡점: inflection point
// 극대: maximal, 극소: minimal (global/local)
// 최대: global maximal, 최소: global minimal
// 파장: wave length
// 주기: period
// 골: trough(valley), 마루: crest

typedef struct WinEntry {
    float time;
    float vnorm;
    float avgCalcV;
    float avgRealV;
} WinEntry;

typedef struct SVMEntry {
    float time;
    float vnorm; // svm이라 하는 게 맞지만, 코드 재사용을 위해 vnorm이라는 표현을 사용했다.
} SVMEntry;

// 극점
typedef struct InflectPoint {
    char status;
    int idx;
} InflectPoint;

typedef struct ThInfo {
    char thStatus;   // 극점이 upper보다 위에 있으면 +, lower보다 아래에 있으면 -
    char inflStatus; // 극대인 경우 +, 극소인 경우 -
    int inflIdx;     // inflList상의 인덱스
    int bufIdx;      // bufList상의 인덱스
    float svm;       // bufList.vnorm
} ThInfo;

typedef struct PeakBufferEntry {
    char thStatus;
    int bufIdx;     // bufList Index
    float svm;
} PeakBufferEntry;

#define PEAK_BUFFER_SIZE    50
typedef struct PeakBuffer {
    int upperIdx;
    int lowerIdx;
    PeakBufferEntry upper[PEAK_BUFFER_SIZE];
    PeakBufferEntry lower[PEAK_BUFFER_SIZE];
} PeakBuffer;

bool th_flag_is_on(unsigned char thFlag, unsigned char f) {
    return (thFlag & f) == f;
}

void th_flag_on(unsigned char *thFlag, unsigned char f) {
    *thFlag |= f;
}

void th_flag_off(unsigned char *thFlag, unsigned char f) {
    *thFlag &= ~f;
}

void peak_buffer_init(PeakBuffer *buf) {
    buf->upperIdx = -1;
    buf->lowerIdx = -1;
}

bool peak_upper_empty(const PeakBuffer *buf) {
    return (buf->upperIdx == -1);
}

bool peak_lower_emptry(const PeakBuffer *buf) {
    return (buf->lowerIdx == -1);
}

bool peak_upper_full(const PeakBuffer *buf) {
    return ((buf->upperIdx + 1) == PEAK_BUFFER_SIZE);
}

bool peak_lower_full(const PeakBuffer *buf) {
    return ((buf->lowerIdx + 1) == PEAK_BUFFER_SIZE);
}

bool peak_upper_append(PeakBuffer *buf, PeakBufferEntry e) {
    if (peak_upper_full(buf)) {
#ifdef _DEBUG
        fprintf(stderr, "E: peak upper buffer is full\n");
#endif
        return false;
    }
    buf->upper[++buf->upperIdx] = e;
    return true;
}

bool peak_lower_append(PeakBuffer *buf, PeakBufferEntry e) {
    if (peak_lower_full(buf)) {
#ifdef _DEBUG
        fprintf(stderr, "E: peak lower buffer is full\n");
#endif
        return false;
    }
    buf->lower[++buf->lowerIdx] = e;
    return true;
}

void peak_upper_clear(PeakBuffer *buf) {
    buf->upperIdx = -1;
}

void peak_lower_clear(PeakBuffer *buf) {
    buf->lowerIdx = -1;
}

float peak_upper_max(const PeakBuffer *buf) {
    int i;
    float max;
    max = buf->upper[0].svm;
    for (i = 0; i <= buf->upperIdx; i++) {
        if (max < buf->upper[i].svm)
            max = buf->upper[i].svm;
    }
    return max;
}

float peak_upper_min(const PeakBuffer *buf) {
    int i;
    float min;
    min = buf->upper[0].svm;
    for (i = 0; i <= buf->upperIdx; i++) {
        if (min > buf->upper[i].svm)
            min = buf->upper[i].svm;
    }
    return min;
}

float peak_upper_avg(const PeakBuffer *buf) {
    int i;
    float avg = 0.0f;
    for (i = 0; i <= buf->upperIdx; i++) {
        avg += buf->upper[i].svm;
    }
    avg /= (buf->upperIdx + 1);
    return avg;
}

float peak_lower_max(const PeakBuffer *buf) {
    int i;
    float max;
    max = buf->lower[0].svm;
    for (i = 0; i <= buf->lowerIdx; i++) {
        if (max < buf->lower[i].svm)
            max = buf->lower[i].svm;
    }
    return max;
}

float peak_lower_min(const PeakBuffer *buf) {
    int i;
    float min;
    min = buf->lower[0].svm;
    for (i = 0; i <= buf->lowerIdx; i++) {
        if (min > buf->lower[i].svm)
            min = buf->lower[i].svm;
    }
    return min;
}

float peak_lower_avg(const PeakBuffer *buf) {
    int i;
    float avg = 0.0f;
    for (i = 0; i <= buf->lowerIdx; i++) {
        avg += buf->lower[i].svm;
    }
    avg /= (buf->lowerIdx + 1);
    return avg;
}

void peak_buffer_dump(const PeakBuffer *buf) {
    int i = 0;
    printf("[peak upper buf]: ");
    if (peak_upper_empty(buf))
        printf("(Empty)");
    else {
        for (i = 0; i <= buf->upperIdx; i++)
            printf("%f ", buf->upper[i].svm);
    }
    printf("\n");
    printf("[peak lower buf]: ");
    if (peak_lower_emptry(buf))
        printf("(Empty)");
    else {
        for (i = 0; i <= buf->lowerIdx; i++)
            printf("%f ", buf->lower[i].svm);
    }
    printf("\n");
}

void win_dump(const WinEntry *list, int len, bool ln) {
    int i;
    for (i = 0; i < len; i++) {
        if (ln)
            printf("%04d:\t", i);
        printf("%f\t%f\t%f\t%f\n", list[i].time, list[i].vnorm, list[i].avgCalcV, list[i].avgRealV);
    }
    printf("\n");
}

void svm_dump(const SVMEntry *list, int len, bool ln) {
    int i;
    for (i = 0; i < len; i++) {
        if (ln)
            printf("%04d:\t", i);
        printf("%f\t%f\n", list[i].time, list[i].vnorm);
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

// 주어진 구간이 증가함수인지 판별
bool svm_inc_func(const SVMEntry *buf, int startIdx, int endIdx) {
    int i;
    float prevSVM = buf[startIdx].vnorm;
    for (i = startIdx; i <= endIdx; i++) {
        if (buf[i].vnorm < prevSVM)
            return false;
        prevSVM = buf[i].vnorm;
    }
    return true;
}

// 주어진 구간이 감소함수인지 판별
bool svm_dec_func(const SVMEntry *buf, int startIdx, int endIdx) {
    int i;
    float prevSVM = buf[startIdx].vnorm;
    for (i = startIdx; i <= endIdx; i++) {
        if (buf[i].vnorm > prevSVM)
            return false;
        prevSVM = buf[i].vnorm;
    }
    return true;
}

float svm_get_max(const SVMEntry *buf, int startIdx, int endIdx) {
    int i;
    float max = buf[startIdx].vnorm;
    for (i = startIdx; i <= endIdx; i++) {
        if (max < buf[i].vnorm)
            max = buf[i].vnorm;
    }
    return max;
}

float svm_get_min(const SVMEntry *buf, int startIdx, int endIdx) {
    int i;
    float min = buf[startIdx].vnorm;
    for (i = startIdx; i <= endIdx; i++) {
        if (min > buf[i].vnorm)
            min = buf[i].vnorm;
    }
    return min;
}

// 1: 위로 볼록 (극대)
// 2: 아래로 볼록 (극소)
// 0: 변곡점 아님
#ifdef _SVM_MODE
int svm_isInflect(const SVMEntry *list, int idx) {
#else
int win_isInflect(const WinEntry *list, int idx) {
#endif
    float prevV = list[idx - 1].vnorm;
    float curV = list[idx].vnorm;
    float nextV = list[idx + 1].vnorm;

    if ((curV > prevV) && (curV > nextV))
        return 1;
    else if ((curV < prevV) && (curV < nextV))
        return 2;
    return 0;
}

void bubble_sort(int *list, int len) {
    int i, j;
    int temp;
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
#ifndef _SVM_MODE
    char buf[BUF_SIZE];
#endif
    
#ifdef _DEBUG
    puts("# step counter "PROGRAM_VERSION);
#endif

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
    //float elapsedTime;
    //WinEntry curEntry;

#ifdef _SVM_MODE
    SVMEntry bufList[DEFAULT_LIST_SIZE];
    while (!feof(fin)) {
        fscanf(fin, "%f\t%f\n", &bufList[curIdx].time, &bufList[curIdx].vnorm);
        listSize++;
        curIdx++;
    }
#else
    WinEntry bufList[DEFAULT_LIST_SIZE];
    fgets(buf, BUF_SIZE, fin); // 첫 줄은 읽어서 버린다.
    while (!feof(fin)) {
        fscanf(fin, "%f\t%f\t%f\t%f\n", &bufList[curIdx].time, &bufList[curIdx].vnorm, &bufList[curIdx].avgCalcV, &bufList[curIdx].avgRealV);
        listSize++;
        curIdx++;
    }
#endif
    fclose(fin);
    //elapsedTime = bufList[listSize - 1].time;

#ifdef _DEBUG
    puts("## Dump bufList");
#ifdef _SVM_MODE
    svm_dump(bufList, listSize, true);
#else
    win_dump(bufList, listSize, true);
#endif
#endif

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
#ifdef _SVM_MODE
        ret = svm_isInflect(bufList, i);
#else
        ret = win_isInflect(bufList, i);
#endif
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
    printf("> 극대점 개수: %d\t극소점 개수: %d\n", maxCnt, minCnt);
    printf("> 극대 평균: %f\t극소 평균: %f\n\n", maxAvg, minAvg);
#endif

    float *inflMaxList = (float*)malloc(sizeof(float) * maxCnt);
    float *inflMinList = (float*)malloc(sizeof(float) * minCnt);
    
    int curMaxIdx = 0, curMinIdx = 0;
    for (i = 0; i < inflCnt; i++) {
        if (inflList[i].status == '+')
            inflMaxList[curMaxIdx++] = bufList[inflList[i].idx].vnorm;
        else
            inflMinList[curMinIdx++] = bufList[inflList[i].idx].vnorm;
    }
    bubble_sortf(inflMaxList, maxCnt);
    bubble_sortf(inflMinList, minCnt);

#ifdef _DEBUG
    puts("inflMaxList:");
    for (i = 0; i < maxCnt; i++)
        printf("[%d]: %f\n", i, inflMaxList[i]);
    puts("");
    puts("inflMinList:");
    for (i = 0; i < minCnt; i++)
        printf("[%d]: %f\n", i, inflMinList[i]);
#endif

    /* 극대점들의 상위 1/AVG_RATIO과 중위 1/AVG_RATIO의 평균을 구한다. */
    const int AVG_RATIO = 4;
    const int AVG_MAX_CNT = maxCnt / AVG_RATIO;
    float inflUpperMaxAvg = 0.0f; // 극대점의 상위 AVG_CNT개의 평균
    float inflMidMaxAvg = 0.0f; // 극대점의 중위 AVG_CNT개의 평균

    for (i = 0; i < AVG_MAX_CNT; i++) {
        inflUpperMaxAvg += inflMaxList[maxCnt - 1 - i];
        inflMidMaxAvg += inflMaxList[AVG_MAX_CNT + i];
    }
    inflUpperMaxAvg /= AVG_MAX_CNT;
    inflMidMaxAvg /= AVG_MAX_CNT;

    for (i = 0; i < maxCnt; i++) {
        if (inflMaxList[i] >= inflMidMaxAvg)
            break;
    }
    stepcnt = maxCnt - i;
#ifdef _DEBUG
    printf("inflMaxList[%d]: %f\n", i, inflMaxList[i]);
    printf("%d", stepcnt);
#endif

    /* 극소점들의 하위 및 중위 1/AVG_RATIO의 평균을 구한다. */
    const int AVG_MIN_CNT = minCnt / AVG_RATIO;
    float inflLowerMinAvg = 0.0f;
    float inflMidMinAvg = 0.0f;

    for (i = 0; i < AVG_MIN_CNT; i++) {
        //inflLowerMinAvg += inflMinList[minCnt - 1 - i];
        inflLowerMinAvg += inflMinList[i];
        inflMidMinAvg += inflMinList[AVG_MIN_CNT + i];
    }
    inflLowerMinAvg /= AVG_MIN_CNT;
    inflMidMinAvg /= AVG_MIN_CNT;
#ifdef _DEBUG
    puts("");
    printf("극대점의 상위 %d개 항목의 평균: %f\n", AVG_MAX_CNT, inflUpperMaxAvg);
    printf("극대점의 중위 %d개 항목의 평균: %f\n", AVG_MAX_CNT, inflMidMaxAvg);
    printf("극소점의 하위 %d개 항목의 평균: %f\n", AVG_MIN_CNT, inflLowerMinAvg);
#endif

    /* 적응형 임계값을 적용하여 임계값을 만족하는 극점 추출 */
    
    // 초기 임계값 설정
    const float INIT_TH_RATIO = 0.7f;
    const float INIT_UPPER_TH = inflUpperMaxAvg * INIT_TH_RATIO; // -30%
    const float INIT_LOWER_TH = inflMidMinAvg * (2.0f - INIT_TH_RATIO); // +30%
    //const float INIT_LOWER_TH = inflLowerMinAvg * (2.0f - INIT_TH_RATIO);

    float lowerTh = INIT_LOWER_TH;
    float upperTh = INIT_UPPER_TH;

    ThInfo *thInfoList = (ThInfo*)malloc(sizeof(ThInfo) * inflCnt);
    int thInfoCnt = 0;

#ifdef _DEBUG
    printf("INIT_UPPER_TH: %f, INIT_LOWER_TH: %f\n", INIT_UPPER_TH, INIT_LOWER_TH);
    printf("upperTh: %f, lowerTh: %f\n", upperTh, lowerTh);
#endif 

    PeakBuffer pbuf;
    PeakBufferEntry pe;
    peak_buffer_init(&pbuf);
    //int preMinIdx = -1, curMinIdx = -1;
    
    // 골(trough)과 마루의 값을 얻었는지를 나타내는 상태 플래그
    // TH_FLAG_L1, TH_FLAG_L2, TH_FLAG_U1, TH_FLAG_U2
    unsigned char thFlag = 0x00;

    char prevStatus, curStatus;
    float m1, m2;
    float p1, p2;
    prevStatus = '0';

    /* 임계값을 만족하는 극점들의 리스트를 만든다. */
    for (i = 0; i < inflCnt; i++) {
        InflectPoint curPt = inflList[i];
        float curSVM = bufList[curPt.idx].vnorm;
        if (curSVM >= upperTh || curSVM <= lowerTh) {
            thInfoList[thInfoCnt].inflStatus = curPt.status;
            thInfoList[thInfoCnt].inflIdx = i;
            thInfoList[thInfoCnt].bufIdx = curPt.idx;
            thInfoList[thInfoCnt].svm = bufList[curPt.idx].vnorm;

            if (curSVM >= upperTh)
                thInfoList[thInfoCnt].thStatus = '+';
            else if (curSVM <= lowerTh)
                thInfoList[thInfoCnt].thStatus = '-';

            // pbuf에 삽입될 새로운 원소 pe의 정보를 채운다.
            pe.thStatus = thInfoList[thInfoCnt].thStatus;
            pe.bufIdx = thInfoList[thInfoCnt].bufIdx;
            pe.svm = thInfoList[thInfoCnt].svm;

            if (prevStatus == '0')
                prevStatus = thInfoList[thInfoCnt].thStatus;
            curStatus = thInfoList[thInfoCnt].thStatus;

            // 이전 상태와 현재 상태가 다르면 peak buffer에 저장된 평균값을 m1, m2 또는 p1, p2에 저장하고 버퍼를 비운다.
            if (prevStatus != curStatus) {
                if (curStatus == '+') { // 이전에 골이었다가 마루가 나오는 경우
                    float minAvg = peak_lower_avg(&pbuf); // 점들의 평균을 구한다.
                    peak_lower_clear(&pbuf); // peak 버퍼의 lower를 비운다.
                    peak_upper_append(&pbuf, pe);
                    if (th_flag_is_on(thFlag, TH_FLAG_L1)) { // TH_FLAG_L1 on
                        m2 = minAvg;
                        th_flag_on(&thFlag, TH_FLAG_L2);
                    }
                    else {
                        m1 = minAvg;
                        th_flag_on(&thFlag, TH_FLAG_L1);
                    }
                }
                else {
                    float maxAvg = peak_upper_avg(&pbuf);
                    peak_upper_clear(&pbuf);
                    peak_lower_append(&pbuf, pe);
                    if (th_flag_is_on(thFlag, TH_FLAG_U1)) { // TH_FLAG_U1 on
                        p2 = maxAvg;
                        th_flag_on(&thFlag, TH_FLAG_U2);
                    }
                    else { // TH_FLAG_U1 off
                        p1 = maxAvg;
                        th_flag_on(&thFlag, TH_FLAG_U1);
                    }
                }
            }
            // 이전 상태와 현재 상태가 같으면 peak buffer에 점에 대한 정보를 채운다(append).
            else {
                if (curStatus == '+') {
                    peak_upper_append(&pbuf, pe);
                }
                else {
                    peak_lower_append(&pbuf, pe);
                }
            }
      
#ifdef _DEBUG
            printf("\n# %c%03d/SVM:%f ########################################\n", pe.thStatus, pe.bufIdx, pe.svm);
            printf("lowerTh: %f, upperTh: %f\n", lowerTh, upperTh);
            printf("m1: ");
            if (th_flag_is_on(thFlag, TH_FLAG_L1))
                printf("%f, ", m1);
            else
                printf("(NULL), ");
            printf("m2: ");
            if (th_flag_is_on(thFlag, TH_FLAG_L2))
                printf("%f\n", m2);
            else
                printf("(NULL)\n");
            printf("p1: ");
            if (th_flag_is_on(thFlag, TH_FLAG_U1))
                printf("%f, ", p1);
            else
                printf("(NULL), ");
            printf("p2: ");
            if (th_flag_is_on(thFlag, TH_FLAG_U2))
                printf("%f\n", p2);
            else
                printf("(NULL)\n");

            peak_buffer_dump(&pbuf);
#endif

            // 임계값 갱신
            if (th_flag_is_on(thFlag, TH_FLAG_LO)) {
                lowerTh = (m1 + m2) / 2.0f;
                lowerTh *= 2.3f;
                th_flag_off(&thFlag, TH_FLAG_LO);
#ifdef _DEBUG
                printf(">> new lowerTh: %f\n", lowerTh);
#endif
            }
            if (th_flag_is_on(thFlag, TH_FLAG_UP)) {
                upperTh = (p1 + p2) / 2.0f;
                upperTh *= 0.85f;
                th_flag_off(&thFlag, TH_FLAG_UP);
#ifdef _DEBUG
                printf(">> new upperTh: %f\n", upperTh);
#endif
            }

            prevStatus = curStatus;
            thInfoCnt++;
        }
    }

    /* lowerTh 밑에 있는 점들의 리스트를 만든다. */
    // lowerTh의 값이 정확할수록 lowerIdxList도 정확하게 나온다.
    int *lowerIdxList = (int*)malloc(sizeof(int) * thInfoCnt);
    int *lowerIdxDiffTbl = NULL;
    int lowerCnt = 0;

    for (i = 0; i < thInfoCnt; i++) {
        if (thInfoList[i].thStatus == '-') {
#ifdef _DEBUG
            printf("%03d: [%c/%c] %d %d %f\n", i, thInfoList[i].thStatus, thInfoList[i].inflStatus, thInfoList[i].inflIdx, thInfoList[i].bufIdx, thInfoList[i].svm);
#endif
            lowerIdxList[lowerCnt++] = thInfoList[i].bufIdx;
        }
    }

#ifdef _DEBUG
    for (i = 0; i < lowerCnt; i++) {
        printf("lowerIdxList[%d]: %d\n", i, lowerIdxList[i]);
    }
#endif

    /* 점들 사이의 간격을 구한다. */
    float lowerDiffAvg = 0.0f;
    const int LOWER_DIFF_CNT = lowerCnt - 1;
    lowerIdxDiffTbl = (int*)malloc(sizeof(int) * LOWER_DIFF_CNT);
    for (i = 0; i < LOWER_DIFF_CNT; i++) {
        lowerIdxDiffTbl[i] = -(lowerIdxList[i] - lowerIdxList[i + 1]);
#ifdef _DEBUG
        printf("lowerIdxDiffTbl[%d]: %d\n", i, lowerIdxDiffTbl[i]);
#endif
    }

    /* 점들 사이의 간격의 평균인 lowerDiffAvg를 구한다. */
    bubble_sort(lowerIdxDiffTbl, LOWER_DIFF_CNT);
    for (i = 0; i < (LOWER_DIFF_CNT / AVG_RATIO); i++) {
        lowerDiffAvg += (float)lowerIdxDiffTbl[LOWER_DIFF_CNT - 1 - i];
#ifdef _DEBUG
        printf("lowerIdxDiffTbl[%d]: %d\n", i, lowerIdxDiffTbl[i]);
#endif
    }
    lowerDiffAvg /= (float)i;

    // 조건 검사를 위해 lowerDiffAvg에 어느정도의 오차범위를 적용한 lowerDiffBound를 계산
    float lowerDiffBound = (float)lowerDiffAvg * 0.7f;

    // 평균 간격을 기반으로 걸음수를 계산한다.
    stepcnt = 0;
    // for (i = 0; i < lowerCnt; i++) {
    //     if (lowerIdxDiffTbl[i] >= lowerDiffBound)
    //         stepcnt++;
    // }
    // stepcnt *= 2;

    float stepcntTmp = (float)listSize / lowerDiffAvg;
    stepcntTmp *= 2;
    stepcnt = (int)stepcntTmp;

    /* 걸음수 보완하기 */
    // (1). 첫 번째 골이 등장한 시점과 평균 간격을 비교하여 부족한 걸음수를 보충한다.
    float startTimeStep = (float)lowerIdxList[0] / (lowerDiffAvg / 2.0f);
    stepcnt += (int)startTimeStep;
    
    // (2). 마지막 골이 등장한 시점과 평균 간격을 비교하여 부족한 걸음수를 보충한다.
    // 두 가지 접근 방법:
    // 1. 마지막 골과 마지막 측정 시점의 차이를 구해서 평균 간격과 비교한다.
    // 2. 마지막 골 이후에 상승 곡선이 있는지를 확인한다. (upperTh보다 큰 점이 발견되는지)
    float endTimeDiff = listSize - lowerIdxList[lowerCnt - 1];
    float endTimeStep = endTimeDiff / (lowerDiffAvg / 2.0f);

    stepcnt += (int)endTimeStep;

    printf("%d", stepcnt);
    //printf("%d %f", listSize, lowerDiffAvg);
    //printf(" %d", (int)stepcntTmp);

    free(lowerIdxDiffTbl);
    free(lowerIdxList);
    free(thInfoList);
    free(inflMaxList);
    free(inflMinList);
    free(inflList);
    return 0;


    /* 고점(극대)에서 저점(극소)으로 떨어지는 값들을 계산한다. */
    // inflDiffTbl: inflList와 같은 인덱스 체계를 공유한다.
    // diffList: inflDiffTbl로부터 극대에서 극소로 떨어지는 경우의 vnorm delta 값을 추출하여 저장한다. 값을 저장한 후 오름차순으로 정렬하여 사용한다.
    
    // TODO: inflDiffTbl을 inflList로 통합; InflectPoint 구조체에 diff 멤버를 추가한다.
    // (1). 통합할 경우 리스트의 크기는 inflCnt가 아니라 listSize, 즉 모든 점의 개수만큼 할당되므로 메모리가 낭비된다.
    // (2). 통합한다고 해서 프로그램 코드가 간략해질 것 같지는 않다.
    // listCnt, inflCnt, diffCnt의 차이점을 잘 구분해둘 것
    // listCnt: bufList의 길이 (파일로부터 읽어들인 신호의 개수에 해당한다. pass-1에서 결정)
    // inflCnt: 극값의 개수, 즉 inflList의 길이 (inflList는 listCnt를 기반으로 여유잡아 할당. inflCnt는 이후에 극값을 카운트하여 결정)
    // diffCnt: diffList의 원소 개수

    // inflCnt: 시간순으로 정렬
    // diffCnt: 신호 크기순으로 정렬
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

#ifdef _DEBUG
    puts("## diffList:");
    for (i = 0; i < diffCnt; i++)
        printf("diffList[%d]: %f\n", i, diffList[i]);
#endif

    const int DIFF_MEAN_LIST_SIZE = diffCnt / 3;    // diffList의 1/3개를 각각의 평균의 기준으로 잡는다.
    float sdiffMean = 0.0f, ldiffMean = 0.0f;       // 격차가 작은 경우의 평균과 큰 경우의 평균 (그런 두 가지 상태로 나뉘는 경향이 있다)
    for (i = 0; i < DIFF_MEAN_LIST_SIZE; i++) {
        sdiffMean += diffList[i];
        ldiffMean += diffList[diffCnt - 1 - i];
    }
    sdiffMean /= DIFF_MEAN_LIST_SIZE;
    ldiffMean /= DIFF_MEAN_LIST_SIZE;

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