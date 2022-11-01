#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "MT19937.h"

static inline void MT19937_twist(MT19937 *object)
{
    for (int i = 0; i < 624; ++i) {
        u32 y = (object->mt[i] & 0x80000000) + 
                (object->mt[(i + 1) % 624] & 0x7fffffff);
        object->mt[i] = y >> 1 ^ object->mt[(i + 397) % 624];

        if (y % 2) {
            object->mt[i] ^= 0x9908b0df;
        }
    }
}

MT19937 *MT19937_init(void)
{
    MT19937 *object;

    object = malloc(sizeof(MT19937));

    object->mt[0] = time(NULL);
    object->mti = 0;

    for (int i = 1; i < 624; ++i) {
        object->mt[i] = 
            0x6c078965 * (object->mt[i - 1] ^ object->mt[i - 1] >> 30) + i;
    }

    return object;
}

u32 MT19937_rand(MT19937 *object)
{
    u32 ret;

    if (!object->mti) {
        MT19937_twist(object);
    }

    ret = object->mt[object->mti];
    ret = ret ^ ret >> 11;
    ret = ret ^ (ret << 7 & 0x9d2c5680);
    ret = ret ^ (ret << 15 & 0xefc60000);
    ret = ret ^ ret >> 18;
    object->mti = (object->mti + 1) % 624;

    return ret;
}

void MT19937_test(int round)
{
    MT19937 *object = MT19937_init();
    double min = -1;
    double max = 1;
    int array[20] = { 0 };

    for (int i = 0; i < round; ++i) {
        double n = min + ((double)MT19937_rand(object) / 0xffffffff) * (max - min);

        array[(int)((n + 1) * 10)] += 1;
    }

    for (int i = 0; i < 20; ++i) {
        printf("array[%d]: %d\n", i, array[i]);
    }

    exit(0);
}