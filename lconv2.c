#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#define BUF_SIZE 256
#define FOUT1_NAME	"sensor-norm.txt"
#define FOUT2_NAME	"sensor-vel.txt"
#define FOUT3_NAME	"sensor-vnorm.txt"

double getNorm(double x, double y, double z) {
	return sqrt((x * x) + (y * y) + (z * z));
}

int main(int argc, char *argv[])
{
	char finName[3][BUF_SIZE];
	FILE *fin[3] = { NULL, };
	FILE *fout1 = NULL;
	FILE *fout2 = NULL;
	FILE *fout3 = NULL;

	int i;
	double timeCnt = 0.0;
	double ax, ay, az;
	double norm;

	const double dt = 1 / 50.0;
	const double constTerm = 9.8 * dt / 2;
	double vx, vy, vz, vnorm;

	printf("## log converter version 2b ##\n");
	if (argc == 1) {
		strcpy(finName[0], "sensor-ax-out.txt");
		strcpy(finName[1], "sensor-ay-out.txt");
		strcpy(finName[2], "sensor-az-out.txt");
	}
	else if (argc == 4) {
		for (i = 0; i < 3; i++)
			strcpy(finName[i], argv[i + 1]);
	}
	else {
		printf("usage: %s [input-x input-y input-z]\n", argv[0]);
		return -1;
	}

	for (i = 0; i < 3; i++) {
		fin[i] = fopen(finName[i], "rt");
		if(!fin[i]) {
			fprintf(stderr, "could not open file: %s\n",finName[i]);
			return -1;
		}
	}
	fout1 = fopen(FOUT1_NAME, "wt");
	fout2 = fopen(FOUT2_NAME, "wt");
	fout3 = fopen(FOUT3_NAME, "wt");
	if (!fout1) {
		fprintf(stderr, "could not open file: %s\n", FOUT1_NAME);
		return -1;
	}
	if (!fout2) {
		fprintf(stderr, "could not open file: %s\n", FOUT2_NAME);
		return -1;
	}
	if (!fout3) {
		fprintf(stderr, "could not open file: %s\n", FOUT3_NAME);
		return -1;
	}
	printf("input files: %s %s %s\n", finName[0], finName[1], finName[2]);
	printf("output file: %s %s\n", FOUT1_NAME, FOUT2_NAME);

	double blankTime;
	vx = 0.0, vy = 0.0, vz = 0.0;

	while (!feof(fin[0])) {
		//fgets(buf[0], BUF_SIZE, fin[0]);
		fscanf(fin[0], "%lf\t%lf\n", &blankTime, &ax);
		fscanf(fin[1], "%lf\t%lf\n", &blankTime, &ay);
		fscanf(fin[2], "%lf\t%lf\n", &blankTime, &az);
		norm = getNorm(ax, ay, ax);
		//printf("%.2lf\t%lf %lf %lf %lf\n", timeCnt, ax, ay, az, norm);

		fprintf(fout1, "%.2lf\t%lf\n", timeCnt, norm);

		timeCnt += 0.02;
		vx += ax * constTerm;
		vy += ay * constTerm;
		vz += az * constTerm;
		vnorm = getNorm(vx, vy, vz);
		fprintf(fout2, "%.2lf\t%lf\t%lf\t%lf\t%lf\n", timeCnt, vx / timeCnt, vy / timeCnt, vz / timeCnt, vnorm / timeCnt);
		fprintf(fout3, "%.2lf\t%lf\n", timeCnt, vnorm / timeCnt);
	}

	for (i = 0; i < 3; i++)
		fclose(fin[i]);
	fclose(fout1);
	fclose(fout2);
	fclose(fout3);
	return 0;
}
