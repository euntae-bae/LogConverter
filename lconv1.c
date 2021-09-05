/* lconv1: 축별 가속도 로그를 시간과 가속도 형식으로 변환하는 프로그램 
 * 입력: 축별 가속도 로그(sensor-ax.txt, sensor-ay.txt, sensor-az.txt 등)
 * 출력: 해당 축의 시간별 가속도로 변환된 로그 파일(sensor-ax-out.txt, sensor-ay-out.txt, sensor-az.out.txt 등) */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define BUF_SIZE 128

int main(int argc, char *argv[])
{
	printf("## log converter version 1 ##\n");
	if (argc != 2) {
		printf("usage: %s input-file\n", argv[0]);
		return -1;
	}

	char foutName[BUF_SIZE];
	char buf[BUF_SIZE];
	char *extp;

	FILE *fin = fopen(argv[1], "rt");
	if (!fin) {
		printf("could not open file: %s\n", argv[1]);
		return -1;
	}

	strcpy(foutName, argv[1]);
	extp = strchr(foutName, '.');
	*extp = '\0';
	strcat(foutName, "-out.txt");

	FILE *fout = fopen(foutName, "wt");
	double timeCnt = 0.0;

	while (!feof(fin)) {
		fgets(buf, BUF_SIZE, fin);
		if (isspace(*buf) || *buf == '[')
			continue;
		timeCnt += 0.02;
		fprintf(fout, "%.2lf\t%s", timeCnt, buf);
	}
	fclose(fin);
	fclose(fout);
	return 0;
}
