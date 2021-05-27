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
		fprintf(fout, "%.2lf\t%s", timeCnt, buf);
		timeCnt += 0.02;
	}
	fclose(fin);
	fclose(fout);
	return 0;
}
