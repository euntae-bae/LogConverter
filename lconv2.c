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
	char foutName[BUF_SIZE];
	FILE *fin[3] = { NULL, };
	FILE *fout = NULL;

	int i;
	double timeCnt = 0.0;
	double ax, ay, az;
	double norm;

	printf("## log converter version 2 ##\n");
	if (argc == 2) {
		strcpy(finName[0], "sensor-ax-out.txt");
		strcpy(finName[1], "sensor-ay-out.txt");
		strcpy(finName[2], "sensor-az-out.txt");
		strcpy(foutName, argv[1]);
	}
	else if (argc == 5) {
		for (i = 0; i < 3; i++)
			strcpy(finName[i], argv[i + 1]);
		strcpy(foutName, argv[4]);
	}
	else {
		printf("usage: %s [input-x input-y input-z] output-file\n", argv[0]);
		return -1;
	}

	for (i = 0; i < 3; i++) {
		fin[i] = fopen(finName[i], "rt");
		if(!fin[i]) {
			fprintf(stderr, "could not open file: %s\n",finName[i]);
			return -1;
		}
	}
	fout = fopen(foutName, "wt");
	if (!fout) {
		fprintf(stderr, "could not open file: %s\n", foutName);
		return -1;
	}
	printf("input files: %s %s %s\n", finName[0], finName[1], finName[2]);
	printf("output file: %s\n", foutName);

	/*
	for (i = 0; i < 5; i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
	*/

	double dtime;
	while (!feof(fin[0])) {
		//fgets(buf[0], BUF_SIZE, fin[0]);
		fscanf(fin[0], "%lf\t%lf\n", &dtime, &ax);
		fscanf(fin[1], "%lf\t%lf\n", &dtime, &ay);
		fscanf(fin[2], "%lf\t%lf\n", &dtime, &az);
		norm = getNorm(ax, ay, ax);
		//printf("%.2lf\t%lf %lf %lf %lf\n", timeCnt, ax, ay, az, norm);

		fprintf(fout, "%.2lf\t%lf\n", timeCnt, norm);
		timeCnt += 0.02;
	}

	for (i = 0; i < 3; i++)
		fclose(fin[i]);
	fclose(fout);
	return 0;
}
