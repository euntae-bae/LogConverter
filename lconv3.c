/* lconv3b: 입력된 time-window 크기에 따라 
 * 입력: time-window
 * 입력 파일: lconv1의 x, y, z축 입력에 대한 각각의 출력파일, 즉 sensor-ax-out.txt, sensor-ay-out.txt, sensor-az.out.txt
 * 출력: 전체 평균 속도 (m/s)
 * 출력 파일: 1개 (sensor-out.txt)
 * (1). time-window 크기에 따른 vx, vy, vz 산출 
 * (2). time-window 크기에 따른 순간 가속도 스칼라
 * (3). time-window 크기에 따른 순간 속도의 norm
 * 기타: time-window 크기가 1인 경우에는 lconv2와 출력 같음
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define BUF_SIZE 256
#define DEFAULT_WINDOW_SIZE	5 // 0.1초 단위

double getNorm(double x, double y, double z) {
	return sqrt((x * x) + (y * y) + (z * z));
}

int main(int argc, char *argv[])
{
	char finName[3][BUF_SIZE];
	const char *foutName = "sensor-out.txt";
	int winSize = DEFAULT_WINDOW_SIZE;

	FILE *fin[3] = { NULL, };
	FILE *fout = NULL;

	int i;
	double timeCnt = 0.0;
	double ax, ay, az;
	double axWin, ayWin, azWin;	// window-size만큼 누적, 로그에는 이 값을 window-size로 나눠 평균을 구한 다음에 기록한다.
	double vx, vy, vz;
	double anorm, vnorm;
	double vxAcc, vyAcc, vzAcc; // 속도 누적 (전체 평균 속도 계산용)

	printf("## log converter version 3c ##\n");
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
	
	printf("input files: %s %s %s\n", finName[0], finName[1], finName[2]);
	printf("output files: %s\n", foutName);
	printf("window size: %d\n", winSize);

	const double dt = 1 / 50.0;
	//const double constTerm = 9.8 * dt / 2;
	const double constTerm = 9.8 * dt; // dcsmp2 수정을 반영함
	const double interval = 0.02 * winSize;

	double blankTime; // 입력 처리를 위한 더미 변수

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

	// 전체 평균 속도
	double vxMean = vxAcc / timeCnt;
	double vyMean = vyAcc / timeCnt;
	double vzMean = vzAcc / timeCnt;
	double vMean = getNorm(vxMean, vyMean, vzMean);
	printf("vxMean: %lf\tvyMean: %lf\tvzMean: %lf\n", vxMean, vyMean, vzMean);
	printf("vMean: %lf\n", vMean);

	for (i = 0; i < 3; i++)
		fclose(fin[i]);
	fclose(fout);
	return 0;
}
