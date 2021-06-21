#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#define BUF_SIZE 256

double getNorm(double x, double y, double z) {
	return sqrt((x * x) + (y * y) + (z * z));
}

int main(int argc, char *argv[])
{
	char finName[3][BUF_SIZE];
	const char *foutName[3] = { "sensor-norm.txt", "sensor-vel.txt", "sensor.vnorm.txt" };

	FILE *fin[3] = { NULL, };
	FILE *fout[3] = { NULL, };

	int i;
	double timeCnt = 0.0;
	double ax, ay, az;
	double norm;

	const double dt = 1 / 50.0;
	const double constTerm = 9.8 * dt / 2;
	double vx, vy, vz, vnorm;

	printf("## log converter version 2c ##\n");
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

	double blankTime;
	vx = 0.0, vy = 0.0, vz = 0.0;

	while (!feof(fin[0])) {
		//fgets(buf[0], BUF_SIZE, fin[0]);
		fscanf(fin[0], "%lf\t%lf\n", &blankTime, &ax);
		fscanf(fin[1], "%lf\t%lf\n", &blankTime, &ay);
		fscanf(fin[2], "%lf\t%lf\n", &blankTime, &az);
		norm = getNorm(ax, ay, ax);
		//printf("%.2lf\t%lf %lf %lf %lf\n", timeCnt, ax, ay, az, norm);

		fprintf(fout[0], "%.2lf\t%lf\n", timeCnt, norm);

		timeCnt += 0.02;
		vx += ax * constTerm;
		vy += ay * constTerm;
		vz += az * constTerm;
		vnorm = getNorm(vx, vy, vz);
		fprintf(fout[1], "%.2lf\t%lf\t%lf\t%lf\t%lf\n", timeCnt, vx / timeCnt, vy / timeCnt, vz / timeCnt, vnorm / timeCnt);
		fprintf(fout[2], "%.2lf\t%lf\n", timeCnt, vnorm / timeCnt);
	}

	for (i = 0; i < 3; i++) {
		fclose(fin[i]);
		fclose(fout[i]);
	}
	return 0;
}
