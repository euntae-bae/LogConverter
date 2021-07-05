#include <stdio.h>
#include "vector3.h"

int main(void)
{
    Vec3Buffer buf1;
    Vec3 vec1 = { 1, 1, 1 };
    Vec3 vec2;

    buf_create(&buf1, 100);
    buf_print(&buf1);

    int i;
    for (i = 0; i < 100; i++) {
        buf_append(&buf1, vec1);
    }
    buf_print(&buf1);
    buf_integral(&buf1);
    
    for (i = 0; i <= 10; i++) {
        vec2.x = vec2.y = vec2.z = i;
        buf_append(&buf1, vec2);
    }
    buf_print(&buf1);
    buf_integral(&buf1);

    return 0;
}