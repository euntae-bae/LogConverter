#include <stdio.h>
#include <stdlib.h>
#include "vector3.h"

void buf_create(Vec3Buffer *vec, int size) {
    vec->capacity = size;
    vec->buf = (Vec3*)malloc(sizeof(Vec3) * size);
    vec->curIdx = -1;
    vec->sum.x = 0.0;
    vec->sum.y = 0.0;
    vec->sum.z = 0.0;
}

void buf_destroy(Vec3Buffer *vec) {
    if (vec->buf) {
        free(vec->buf);
    }
}

void buf_clear(Vec3Buffer *vec) {
    vec->curIdx = -1;
    vec->sum.x = 0.0;
    vec->sum.y = 0.0;
    vec->sum.z = 0.0;
}

int buf_number_of_entry(const Vec3Buffer *vec) {
    return vec->curIdx + 1;
}

Vec3 buf_sum_of_entry(const Vec3Buffer *vec) {
    return vec->sum;
}

Vec3 buf_mean_of_entry(const Vec3Buffer *vec) {
    Vec3 sum = vec->sum;
    int nEntry = vec->curIdx + 1;
    sum.x /= (double)nEntry;
    sum.y /= (double)nEntry;
    sum.z /= (double)nEntry;
    return sum;
}

Vec3 buf_integral(const Vec3Buffer *vec) {
    const double dt = 1 / 50.0;
	const double constTerm = 9.8 * dt / 2;
	const double interval = 0.02 * vec->capacity;

    int i;
    Vec3 v = { 0.0, 0.0, 0.0 };

    for (i = 0; i < buf_number_of_entry(vec); i++) {
        v.x += vec->buf[i].x * constTerm;
        v.y += vec->buf[i].x * constTerm;
        v.z += vec->buf[i].x * constTerm;
    }
    // v.x /= interval;
    // v.y /= interval;
    // v.z /= interval;
    

    Vec3 sum = buf_sum_of_entry(vec);
    sum.x = sum.x * constTerm / interval, sum.y = sum.y * constTerm / interval, sum.z = sum.z * constTerm / interval;
    // sum.x = sum.x * constTerm, sum.y = sum.y * constTerm, sum.z = sum.z * constTerm;

    //printf("v: (%lf, %lf, %lf)\n", v.x, v.y, v.z);
    //printf("s: (%lf, %lf, %lf)\n", sum.x, sum.y, sum.z);
    return v;
    
    //return sum;
}

void buf_append(Vec3Buffer *vec, Vec3 data) {
    int i;
    if (vec->curIdx >= (vec->capacity - 1)) { // 버퍼가 가득찬 경우
        vec->sum.x -= vec->buf[0].x;
        vec->sum.y -= vec->buf[0].y;
        vec->sum.z -= vec->buf[0].z;

        // 두 번째부터 마지막 원소까지 앞으로 한 칸씩 밀어낸다.
        for (i = 1; i < vec->curIdx; i++) {
            vec->buf[i] = vec->buf[i + 1];
        }
    }
    else {
        ++vec->curIdx;
    }
    // 새로운 원소 삽입
    vec->buf[vec->curIdx] = data;
    vec->sum.x += data.x;
    vec->sum.y += data.y;
    vec->sum.z += data.z;
}

void buf_print(const Vec3Buffer *vec) {
    int i;
    Vec3 sum = buf_sum_of_entry(vec);
    Vec3 mean = buf_mean_of_entry(vec);
    for (i = 0; i < buf_number_of_entry(vec); i++) {
        printf("[%02d]: (%lf, %lf, %lf)\n", i, vec->buf[i].x, vec->buf[i].y, vec->buf[i].z);
    }
    printf(">> total %d entries\n", i);
    printf(">> sum: (%lf, %lf, %lf)\n", sum.x, sum.y, sum.z);
    printf(">> mean: (%lf, %lf, %lf)\n", mean.x, mean.y, mean.z);
}