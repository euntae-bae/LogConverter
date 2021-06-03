#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#define BUF_SIZE 256
#define DEFAULT_WINDOW_SIZE	5
double getNorm(double x, double y, double z) {
	return sqrt((x * x) + (y * y) + (z * z));
}

int main(int argc, char *argv[])
{
	char finName[3][BUF_SIZE];
	const char *foutName[3] = { "sensor-norm.txt", "sensor-vel.txt", "sensor-vnorm.txt" };
	int winSize = DEFAULT_WINDOW_SIZE;

	FILE *fin[3] = { NULL, };
	FILE *fout[3] = { NULL, };

	int i;
	double timeCnt = 0.0;
	double ax, ay, az;
	double norm;

	const double dt = 1 / 50.0;
	const double constTerm = 9.8 * dt / 2;
	double vx, vy, vz, vnorm;

	printf("## log converter version 3 ##\n");
	if (argc == 1) {
		strcpy(finName[0], "sensor-ax-out.txt");
		strcpy(finName[1], "sensor-ay-out.txt");
		strcpy(finName[2], "sensor-az-out.txt");
	}
	else if (argc == 2) {
		winSize = atoi(argv[1]);		vx = 0.0;
		vy = 0.0;
		vz = 0.0;
	}

	for (i = 0; i < 3; i++) {
		fin[i] = fopen(finName[i], "rt");
		if(!fin[i]) {
			fprintf(stderr, "could not open file: %s\n", finName[i]);
			return -1;
		}
	}
	for (i = 0; i < 3; i++) {
		fout[i] = fopen(foutName[i], "wt");
		if (!fout[i]) {
			fprintf(stderr, "could not open file: %s\n", foutName[i]);
			return -1;
		}
	}
	
	printf("input files: %s %s %s\n", finName[0], finName[1], finName[2]);
	printf("output files: %s %s %s\n", foutName[0], foutName[1], foutName[2]);
	printf("window size: %d\n", winSize);

	double blankTime;
	vx = 0.0, vy = 0.0, vz = 0.0;

	while (!feof(fin[0])) {
		//fgets(buf[0], BUF_SIZE, fin[0]);
		fscanf(fin[0], "%lf\t%lf\n", &blankTime, &ax);
		fscanf(fin[1], "%lf\t%lf\n", &blankTime, &ay);
		fscanf(fin[2], "%lf\t%lf\n", &blankTime, &az);
		norm = getNorm(ax, ay, ax);
		//printf("%.2lf\t%lf %lf %lf %lf\n", timeCnt, ax, ay, az, norm);

		fprintf(fout[0], "%.2lf\t%lf\n", timeCnt, norm); // 가속도 norm
	}

	for (i = 0; i < 3; i++) 
		rewind(fin[i]);
	double deltaTime = 0.02 * winSize;

	while (!feof(fin[0])) {
		vx = 0.0;
		vy = 0.0;
		vz = 0.0;
		timeCnt += 0.02 * winSize;
		for (i = 0; i < winSize; i++) {
			fscanf(fin[0], "%lf\t%lf\n", &blankTime, &ax);
			fscanf(fin[1], "%lf\t%lf\n", &blankTime, &ay);
			fscanf(fin[2], "%lf\t%lf\n", &blankTime, &az);
			vx += ax * constTerm;
			vy += ay * constTerm;
			vz += az * constTerm;
		}
		vnorm = getNorm(vx, vy, vz);
		fprintf(fout[1], "%.2lf\t%lf\t%lf\t%lf\t%lf\n", timeCnt, vz / deltaTime, vy / deltaTime, vz / deltaTime, vnorm / deltaTime); // 축별 속도 및 속도의 norm
		fprintf(fout[2], "%.2lf\t%lf\n", timeCnt, vnorm / deltaTime); // 속도의 norm
	}

	for (i = 0; i < 3; i++) {
		fclose(fin[i]);
		fclose(fout[i]);
	}
	return 0;
}
