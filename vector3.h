#ifndef __VECTOR3_H__
#define __VECTOR3_H__

typedef struct Vec3 {
    double x, y, z;
} Vec3;

typedef struct Vec3Buffer {
    Vec3 *buf;
    Vec3 sum;
    int curIdx;
    int capacity;
} Vec3Buffer;

void buf_create(Vec3Buffer *vec, int size);
void buf_destroy(Vec3Buffer *vec);
void buf_clear(Vec3Buffer *vec);
int buf_number_of_entry(const Vec3Buffer *vec);
Vec3 buf_sum_of_entry(const Vec3Buffer *vec);
Vec3 buf_mean_of_entry(const Vec3Buffer *vec);
Vec3 buf_integral(const Vec3Buffer *vec);
void buf_append(Vec3Buffer *vec, Vec3 data);
void buf_print(const Vec3Buffer *vec);

#endif
