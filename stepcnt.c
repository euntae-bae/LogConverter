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
// #define START_TIME_CNT          0.5
// #define UNIT_TIME               0.02
#define DEFAULT_LIST_SIZE       2000

#define TH_ADAP_PT_SIZE  4

#define DIFF_MAP_MAX_SIZE   5

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

typedef struct Trough {
    int startIdx;
    int endIdx;
} Trough;

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

typedef struct DiffMapEntry {
        int idx;
        int cnt;
} DiffMapEntry;

int sumi(int *arr, int len) {
    int i;
    int sum = 0;
    for (i = 0; i < len; i++)
        sum += arr[i];
    return sum;
}

float avgi(int *arr, int len) {
    int sum = sumi(arr, len);
    return (float)sum / len;
}

float sumf(float *arr, int len) {
    int i;
    float sum = 0;
    for (i = 0; i < len; i++)
        sum += arr[i];
    return sum;
}

float avgf(float *arr, int len) {
    float sum = sumf(arr, len);
    return sum / (float)len;
}

void displayDiffMap(int *diffMap, int len) {
    int i, j;
    for (i = 0; i <len; i++) {
        printf("[%03d]: ", i);
        for (j = 0; j < diffMap[i]; j++)
            printf("#");
        printf("\n");
    }
}

void th_dump_mpt(float *mpt, int mIdx) {
    int i;
    printf("mPt: ");
    if (mIdx == 0) {
        printf("(NULL)\n");
        return;
    }
    for (i = 0; i < mIdx; i++)
        printf("%f ", mpt[i]);
    if (mIdx >= TH_ADAP_PT_SIZE)
        printf("$");
    printf("\n");
}

void th_dump_ppt(float *ppt, int pIdx) {
    int i;
    printf("pPt: ");
    if (pIdx == 0) {
        printf("(NULL)\n");
        return;
    }
    for (i = 0; i <pIdx; i++)
        printf("%f ", ppt[i]);
    if (pIdx >= TH_ADAP_PT_SIZE)
        printf("$");
    printf("\n");
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

void bubble_sort_diffmap(DiffMapEntry *list, int len) {
    int i, j;
    DiffMapEntry temp;
    for (i = 0; i < len; i++) {
        for (j = 0; j < len - i - 1; j++) {
            if (list[j].cnt > list[j + 1].cnt) {
                temp = list[j + 1];
                list[j + 1] = list[j];
                list[j] = temp;
            }
        }
    }
}

int getTrIdx(const Trough *trList, int len, int idx) {
    int i;
    for (i = 0; i < len; i++) {
        if (idx >= trList[i].startIdx && idx <= trList[i].endIdx)
            return i;
    }
    return -1;
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
    //return 0;
#endif 

    PeakBuffer pbuf;
    PeakBufferEntry pe;
    peak_buffer_init(&pbuf);
    //int preMinIdx = -1, curMinIdx = -1;
    
    char prevStatus, curStatus;
    float mPt[TH_ADAP_PT_SIZE], pPt[TH_ADAP_PT_SIZE];
    int mIdx = 0, pIdx = 0;

    bool adaptiveThMode = false;
    prevStatus = '0';

    const float ADAP_TH_RATIO = 0.7f;

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

                    if (mIdx < TH_ADAP_PT_SIZE) {
                        mPt[mIdx++] = minAvg;
                    }
                }
                else {
                    float maxAvg = peak_upper_avg(&pbuf);
                    peak_upper_clear(&pbuf);
                    peak_lower_append(&pbuf, pe);
                    if (pIdx < TH_ADAP_PT_SIZE) {
                        pPt[pIdx++] = maxAvg;
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
            if (adaptiveThMode) {
                printf("\n# %c%03d/SVM:%f ########################################\n", pe.thStatus, pe.bufIdx, pe.svm);
                printf("lowerTh: %f, upperTh: %f\n", lowerTh, upperTh);
                th_dump_mpt(mPt, mIdx);
                th_dump_ppt(pPt, pIdx);
                peak_buffer_dump(&pbuf);
            }
#endif

            // 임계값 갱신
            if (adaptiveThMode && mIdx >= TH_ADAP_PT_SIZE) {
                lowerTh = avgf(mPt, TH_ADAP_PT_SIZE) * (3.0f - ADAP_TH_RATIO);
                mIdx = 0;
#ifdef _DEBUG
                printf(">> new lowerTh: %f\n", lowerTh);
#endif
            }
            if (adaptiveThMode && pIdx >= TH_ADAP_PT_SIZE) {
                upperTh = avgf(pPt, TH_ADAP_PT_SIZE) * ADAP_TH_RATIO;
                pIdx = 0;
#ifdef _DEBUG
                printf(">> new upperTh: %f\n", upperTh);
#endif
            }

            prevStatus = curStatus;
            thInfoCnt++;
        }
    }

    // 골의 리스트를 만든다.
    Trough troughList[200];
    int trCnt = 0;

    prevStatus = curStatus = '+';
#ifdef _DEBUG
    printf("troughList:\n");
#endif
    for (i = 0; i < listSize; i++) {
        if (bufList[i].vnorm <= lowerTh) {
            curStatus = '-';
        }
        else {
            curStatus = '+';
        }

        // 골 시작
        if (prevStatus == '+' && curStatus == '-') {
            troughList[trCnt].startIdx = i;
        }

        // 골 끝
        if (prevStatus == '-' && curStatus == '+') {
            troughList[trCnt].endIdx = i;
#ifdef _DEBUG
            printf("[%03d] start: %4d, end: %4d\n", trCnt, troughList[trCnt].startIdx, troughList[trCnt].endIdx);
            //printf("%.2f\t%f\n", bufList[troughList[trCnt].startIdx].time, bufList[troughList[trCnt].startIdx].vnorm);
            //printf("%.2f\t%f\n", bufList[troughList[trCnt].endIdx].time, bufList[troughList[trCnt].endIdx].vnorm);
#endif
            trCnt++;
        }
        prevStatus = curStatus;
    }

    /* lowerTh 밑에 있는 점들의 리스트를 만든다. */
    // lowerTh의 값이 정확할수록 lowerIdxList도 정확하게 나온다.
    int *lowerIdxList = (int*)malloc(sizeof(int) * thInfoCnt);
    int *lowerIdxDiffTbl = NULL;
    int lowerCnt = 0;

    int prevTrIdx, curTrIdx;
    prevTrIdx = curTrIdx = -1;


#ifdef _DEBUG
    printf("\nthInfoList:\n");
    for (i = 0; i < thInfoCnt; i++) {
        if (thInfoList[i].thStatus == '-')
                printf("%03d: [%c/%c] %d %d %f\n", i, thInfoList[i].thStatus, thInfoList[i].inflStatus, thInfoList[i].inflIdx, thInfoList[i].bufIdx, thInfoList[i].svm);
    }
    printf("\n");
    
#endif
    for (i = 0; i < thInfoCnt; i++) {
        if (thInfoList[i].thStatus == '-') {
            // 현재 점이 포함된 골을 찾는다.
            curTrIdx = getTrIdx(troughList, trCnt, thInfoList[i].bufIdx);
            if ((curTrIdx != prevTrIdx) || curTrIdx == -1) {
                lowerIdxList[lowerCnt++] = thInfoList[i].bufIdx;
#ifdef _DEBUG
#endif
            }
            prevTrIdx = curTrIdx;
        }
    }

#ifdef _DEBUG
    printf("lowerIdxList:\ntime\tsvm\n");
    for (i = 0; i < lowerCnt; i++) {
        int idx = lowerIdxList[i];
        //printf("[%d]: %d\n", i, lowerIdxList[i]);
        printf("[%d] %f\t%f\n", i, bufList[idx].time, bufList[idx].vnorm);
    }
    printf("\n");
#endif

    /* 점들 사이의 간격을 구한다. */
    float lowerDiffAvg = 0.0f;
    const int LOWER_DIFF_CNT = lowerCnt - 1;
    lowerIdxDiffTbl = (int*)malloc(sizeof(int) * LOWER_DIFF_CNT);
    for (i = 0; i < LOWER_DIFF_CNT; i++) {
        lowerIdxDiffTbl[i] = -(lowerIdxList[i] - lowerIdxList[i + 1]);
    }

    const int TR_DIFF_CNT = trCnt - 1;
    int *trDiffTbl = (int*)malloc(sizeof(int) * TR_DIFF_CNT);
    for (i = 0; i < TR_DIFF_CNT; i++) {
        trDiffTbl[i] = troughList[i + 1].startIdx - troughList[i].startIdx;
    }

    /* 점들 사이의 간격의 평균인 lowerDiffAvg를 구한다. */
    // 간격을 오름차순으로 정렬
    bubble_sort(lowerIdxDiffTbl, LOWER_DIFF_CNT);
    bubble_sort(trDiffTbl, TR_DIFF_CNT);

    int lowerIdxDiffSum = sumi(lowerIdxDiffTbl, LOWER_DIFF_CNT);
    float lowerIdxDiffAvg = (float)lowerIdxDiffSum / (float)LOWER_DIFF_CNT;
    int trDiffSum = sumi(trDiffTbl, TR_DIFF_CNT);
    float trDiffAvg = (float)trDiffSum / (float)TR_DIFF_CNT;

#ifdef _DEBUG
    puts("");
    puts("lowerIdxDiffTbl:");
    for (i = 0; i < LOWER_DIFF_CNT; i++) {
        //printf("lowerIdxDiffTbl[%d]: %d\n", i, lowerIdxDiffTbl[i]);
        printf("%d\n", lowerIdxDiffTbl[i]);
    }
    printf("> sum: %d\n", lowerIdxDiffSum);
    printf("> avg: %d\n", (int)lowerIdxDiffAvg);
    puts("");

    puts("trDiffTbl:");
    for (i = 0; i < TR_DIFF_CNT; i++) {
        printf("%d\n", trDiffTbl[i]);
    }
    printf("> sum: %d\n", trDiffSum);
    printf("> avg: %f\n", trDiffAvg);
    puts("");
#endif

    /* diffMap: 각 간격의 빈도를 나타내는 테이블 */
    int *diffMap = NULL;
    int diffMapLen = lowerIdxDiffTbl[LOWER_DIFF_CNT - 1] + 1; // 가장 큰 원소 선택
    diffMap = (int*)malloc(sizeof(int) * diffMapLen);
    memset(diffMap, 0, sizeof(int) * diffMapLen);

    for (i = 0; i < LOWER_DIFF_CNT; i++) {
        diffMap[lowerIdxDiffTbl[i]]++;
    }

    DiffMapEntry *sortedDiffMap = (DiffMapEntry*)malloc(sizeof(DiffMapEntry) * diffMapLen);
    memset(sortedDiffMap, 0, sizeof(DiffMapEntry) * diffMapLen);
    for (i = 0; i < diffMapLen; i++) {
        sortedDiffMap[i].idx = i;
        sortedDiffMap[i].cnt = diffMap[i];
    }

    bubble_sort_diffmap(sortedDiffMap, diffMapLen);
#ifdef _DEBUG
    for (i = 0; i < diffMapLen; i++)
        printf("sortedDiffMap[%d]: %d\n", sortedDiffMap[i].idx, sortedDiffMap[i].cnt);
#endif

    int diffMapMax[DIFF_MAP_MAX_SIZE] = { 0, };
    float diffMapMaxAvg = 0.0f;
    int diffMapMaxCnt = 0;

    diffMapMaxCnt = 0;
    for (i = 0; i < diffMapLen; i++) {
        int idx = sortedDiffMap[diffMapLen - 1 - i].idx;
        if (idx < (int)lowerIdxDiffAvg || idx > (int)(lowerIdxDiffAvg * 2))
            continue;
        //printf("idx: %d\n", idx);
        diffMapMax[diffMapMaxCnt++] = idx;
        if (diffMapMaxCnt >= DIFF_MAP_MAX_SIZE)
            break;
    }
    //diffMapMaxAvg = avgi(diffMapMax, diffMapMaxCnt);
    diffMapMaxAvg = 0.0f;
    int entryCnt = 0;
    for (i = 0; i < diffMapMaxCnt; i++) {
        int cnt = diffMap[diffMapMax[i]];
        diffMapMaxAvg += (float)diffMapMax[i] * cnt;
        entryCnt += cnt;
    }
    diffMapMaxAvg /= entryCnt;

#ifdef _DEBUG
    printf("maxDiff: %d, diffMapLen: %d\n", lowerIdxDiffTbl[LOWER_DIFF_CNT - 1], diffMapLen);
    displayDiffMap(diffMap, diffMapLen);
    puts("");

    puts("diffMapMax:");
    for (i = 0; i < diffMapMaxCnt; i++) {
        printf("[%03d]: %d\n", diffMapMax[i], diffMap[diffMapMax[i]]);
    }
    puts("");
    printf("diffMapMaxAvg: %f\n", diffMapMaxAvg);
#endif

    /* lowerIdxDiffTbl로부터 간격의 평균을 취할 데이터를 선택한다. */

    // 앞뒤로 (TRUNC_RATIO * 100)% 만큼 잘라낸다.
    const float TRUNC_RATIO = 0.3f;
    int stIdx = LOWER_DIFF_CNT * TRUNC_RATIO;
    //int stIdx = 0;
    //int edIdx = LOWER_DIFF_CNT - stIdx;
    int edIdx = LOWER_DIFF_CNT * (1.0f - TRUNC_RATIO);
    int selCnt = 0;
    
    for (i = stIdx; i <= edIdx; i++) {
        int diff = lowerIdxDiffTbl[i];
        if (diff < (int)lowerIdxDiffAvg)
            continue;
#ifdef _DEBUG
        printf("lowerIdxDiffTbl[%d]: %d\n", i, lowerIdxDiffTbl[i]);
#endif
        lowerDiffAvg += (float)lowerIdxDiffTbl[i];
        selCnt++;
    }
    
    /*
    for (i = 0; i < LOWER_DIFF_CNT; i++) {
        int diff = lowerIdxDiffTbl[i];
        int diffAvg = (int)lowerIdxDiffAvg;
        if (diff > diffAvg * 2 || diff < diffAvg)
            continue;
#ifdef _DEBUG
        printf("lowerIdxDiffTbl[%d]: %d\n", i, diff);
#endif
        lowerDiffAvg += (float)diff;
        selCnt++;
    } */


    lowerDiffAvg /= (float)selCnt;

    //float stepcntTmp = (float)listSize / lowerDiffAvg;
    float stepcntTmp = (float)listSize / diffMapMaxAvg;
    stepcntTmp *= 2;
    stepcnt = (int)stepcntTmp;

    /* 걸음 수 보정 */
    float startIdx = (float)lowerIdxList[0];
    float endIdxDiff = listSize - lowerIdxList[lowerCnt - 1];
    float addSteps = (startIdx + endIdxDiff) / (lowerDiffAvg / 2);
    stepcnt += (int)addSteps;

    /* 결과 값 출력 */
    //const float TIME_UNIT = 0.1f;
    //printf("%d\t%f\t%d\t%f", stepcnt, lowerDiffAvg, listSize, ((float)listSize / lowerDiffAvg) * 2);
    //printf("%d\t%f\t%f\t%d", listSize, lowerDiffAvg, stepcntTmp, stepcnt);
    //printf("%d %f", listSize, lowerDiffAvg);
    //printf("%d\t%f\t%f\t%f\t%f", stepcnt, lowerDiffAvg, lowerDiffAvg * TIME_UNIT, listSize * TIME_UNIT, INIT_LOWER_TH);
    //printf("%d\t%f", stepcnt, addSteps);

    //printf("%d, %d sec", stepcnt, (int)bufList[listSize - 1].time);
    printf("%d", stepcnt);

    /* troughList를 기반으로 계산 */
    stIdx = TR_DIFF_CNT * TRUNC_RATIO;
    edIdx = TR_DIFF_CNT - stIdx;
    selCnt = 0;
    for (i = stIdx; i <= edIdx; i++) {
#ifdef _DEBUG
        printf("trDiffTbl[%d]: %d\n", i, trDiffTbl[i]);
#endif
        lowerDiffAvg += (float)trDiffTbl[i];
        selCnt++;
    }
    lowerDiffAvg /= (float)selCnt;
    stepcntTmp = (float)listSize / lowerDiffAvg;
    stepcntTmp *= 2;
    stepcnt = (int)stepcntTmp;

    startIdx = (float)troughList[0].startIdx;
    endIdxDiff = listSize - troughList[trCnt - 1].endIdx;
    addSteps = (startIdx + endIdxDiff) / (lowerDiffAvg / 2);
    stepcnt += (int)addSteps;

    //printf(" | %d, %d sec", stepcnt, (int)bufList[listSize - 1].time);
    

    /* 동적 할당 데이터 해제 */
    free(trDiffTbl);
    free(diffMap);
    free(lowerIdxDiffTbl);
    free(lowerIdxList);
    free(thInfoList);
    free(inflMaxList);
    free(inflMinList);
    free(inflList);
    return 0;
}