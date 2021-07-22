/* lconv4b: 기존의 lconv에 존재하는 적분 알고리즘 버그 수정 - constTerm에 보정값 1/2 곱
 * 로그로 출력하는 항목 축소
 * lconv4: lconv3c에서 버퍼 도입, 윈도우를 움직이며 적분·합산하는 방식으로 변경 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "vector3.h"

#define BUF_SIZE 256
#define DEFAULT_WINDOW_SIZE	5 // 0.1초 단위
#define DISTANCE_METER	30 // 이동 거리

typedef struct measureData {
	double elapsedTime;	// 경과 시간
	double speed;		// 실제 평균 보행 속력
	// 측정 평균 보행 속력
	double vxMean;
	double vyMean;
	double vzMean;
	double vMeanNorm;
	double errorRatio;	// 오차율
} MeasureData;

double getNorm(double x, double y, double z) {
	return sqrt((x * x) + (y * y) + (z * z));
}

double getNormVec3(const Vec3 vec) {
    return getNorm(vec.x, vec.y, vec.z);
}

int main(int argc, char *argv[])
{
	char finName[3][BUF_SIZE];
	const char * const foutName = "sensor-out.txt";
    const char * const fwinName = "sensor-win.txt";
	int winSize = DEFAULT_WINDOW_SIZE;

	FILE *fin[3] = { NULL, };
	FILE *fout = NULL;
    FILE *fwin = NULL;

	FILE *flist = NULL;
	MeasureData flistData;

	int i;
	double timeCnt = 0.0;
	double ax, ay, az;
	double axWin, ayWin, azWin;	// window-size만큼 누적, 로그에는 이 값을 window-size로 나눠 평균을 구한 다음에 기록한다.
	double vx, vy, vz;
	double anorm, vnorm;
	double vxAcc, vyAcc, vzAcc; // 속도 누적 (전체 평균 속도 계산용)

    Vec3Buffer accBuf;

	printf("## log converter version 4c ##\n");
	strcpy(finName[0], "sensor-ax-out.txt");
	strcpy(finName[1], "sensor-ay-out.txt");
	strcpy(finName[2], "sensor-az-out.txt");
	
	// read command line parameter
	if (argc == 2) {
		winSize = atoi(argv[1]);
		if (winSize <= 0) {
			fprintf(stderr, "error: illegal time-window: %s\n", argv[1]);
			return -1;
		}
	}
	else if (argc > 2) {
		printf("usage: %s time-window-size\n", argv[0]);
		return -1;
	}

	// file open
	for (i = 0; i < 3; i++) {
		fin[i] = fopen(finName[i], "rt");
		if(!fin[i]) {
			fprintf(stderr, "could not open file: %s\n", finName[i]);
			return -1;
		}
	}

	fout = fopen(foutName, "wt");
	if (!fout) {
		fprintf(stderr, "could not open file: %s\n", foutName);
		return -1;
	}

    fwin = fopen(fwinName, "wt");
    if (!fwin) {
        fprintf(stderr, "could not open file: %s\n", foutName);
		return -1;
    }
	
	flist = fopen("result-list.txt", "at");
	if (!flist) {
		fprintf(stderr, "could not open file flist.txt\n");
		return -1;
	}

	fprintf(fout, "time\tvx\tvy\tvz\tanorm\tvnorm\n");
	fprintf(fwin, "time\tvnorm\t측정속도\t실제속도\n");
	
	printf("input files: %s %s %s\n", finName[0], finName[1], finName[2]);
	printf("output files: %s %s\n", foutName, fwinName);
	printf("window size: %d\n", winSize);

	const double dt = 1 / 50.0;
	const double constTerm = 9.8 * dt / 2;
	const double interval = 0.02 * winSize;

	double blankTime; // 입력 처리를 위한 더미 변수
    Vec3 accVec;
	Vec3 mean, vwin;
	double anormWin, vnormWin;

	// fout: 축별 속도, 순간 가속도 (가속도의 norm), 순간 속도 (축별 속도의 norm)
	vxAcc = 0.0, vyAcc = 0.0, vzAcc = 0.0;
	while (!feof(fin[0])) {
		vx = 0.0, vy = 0.0, vz = 0.0;
		axWin = 0.0, ayWin = 0.0, azWin = 0.0;
		timeCnt += 0.02 * winSize;
		for (i = 0; i < winSize; i++) { // window-size만큼 가속도 값을 적분(속도)/합산
            fscanf(fin[0], "%lf\t%lf\n", &blankTime, &ax);
			fscanf(fin[1], "%lf\t%lf\n", &blankTime, &ay);
			fscanf(fin[2], "%lf\t%lf\n", &blankTime, &az);
			axWin += ax, ayWin += ay, azWin += az;
			vx += ax * constTerm, vy += ay * constTerm, vz += az * constTerm;
		}
		vxAcc += vx, vyAcc += vy, vzAcc += vz;
		//vx /= interval, vy /= interval, vz /= interval;
		anorm = getNorm(axWin / winSize, ayWin / winSize, azWin / winSize); // 순간 가속도의 스칼라 (norm)
		vnorm = getNorm(vx, vy, vz); // 순간 속도의 스칼라 (norm)
		fprintf(fout, "%.2lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", timeCnt, vx, vy, vz, anorm, vnorm);
	}

	// 전체 평균 속도 (측정 속도)
	double vxMean = vxAcc / timeCnt;
	double vyMean = vyAcc / timeCnt;
	double vzMean = vzAcc / timeCnt;
	double vMean = getNorm(vxMean, vyMean, vzMean);
	//printf(">> vxMean: %lf\tvyMean: %lf\tvzMean: %lf\n", vxMean, vyMean, vzMean);
	//printf(">> vMean: %lf\n", vMean);
	fprintf(fout, ">> vxMean: %lf\tvyMean: %lf\tvzMean: %lf\n", vxMean, vyMean, vzMean);
	fprintf(fout, ">> vMean: %lf\n", vMean);

	// 실제 이동 속도
	const double vMeanAct = DISTANCE_METER / timeCnt;

	flistData.elapsedTime = timeCnt;
	flistData.vxMean = vxMean;
	flistData.vyMean = vyMean;
	flistData.vzMean = vzMean;
	flistData.vMeanNorm = vMean;
	flistData.speed = vMeanAct;
	double vMeanAbs = fabs(vMean);
	flistData.errorRatio = (fabs(vMeanAbs - vMeanAct) / vMeanAct) * 100.0;
	// fprintf(flist, "elapsedTime: %lf\tspeed: %lf\tvxMean: %lf\tvyMean: %lf\tvzMean: %lf\tvMeanNorm: %lf\terrorRatio: %lf\n",
	// 	flistData.elapsedTime, flistData.speed, flistData.vxMean, flistData.vyMean, flistData.vzMean, flistData.vMeanNorm, flistData.errorRatio);
	// 소요시간, 실제 속도, 측정 속도, 오차율
	fprintf(flist, "%lf\t%lf\t%lf\t%lf\n", flistData.elapsedTime, flistData.speed, flistData.vMeanNorm, flistData.errorRatio);

	for (i = 0; i < 3; i++)
		rewind(fin[i]);
	timeCnt = 0.0;
    buf_create(&accBuf, winSize);
	while (!feof(fin[0])) {
		for (i = 0; i < winSize; i++) {
			timeCnt += 0.02;
            fscanf(fin[0], "%lf\t%lf\n", &blankTime, &ax);
			fscanf(fin[1], "%lf\t%lf\n", &blankTime, &ay);
			fscanf(fin[2], "%lf\t%lf\n", &blankTime, &az);
			accVec.x = ax;
            accVec.y = ay;
            accVec.z = az;
            buf_append(&accBuf, accVec);
            if (buf_number_of_entry(&accBuf) >= accBuf.capacity) {
                mean = buf_mean_of_entry(&accBuf);
				vwin = buf_integral(&accBuf);
                anormWin = getNormVec3(mean);
				vnormWin = getNormVec3(vwin);
				fprintf(fwin, "%.2f\t%lf\t%lf\t%lf\n", timeCnt, vnormWin, vMean, vMeanAct);
            }
		}
	}

    buf_destroy(&accBuf);
	for (i = 0; i < 3; i++)
		fclose(fin[i]);
	fclose(fout);
    fclose(fwin);
	fclose(flist);
	return 0;
}
