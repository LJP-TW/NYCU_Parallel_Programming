#ifndef MT19936_H
#define MT19936_H

typedef unsigned int u32;

typedef struct {
    u32 mt[624];
    u32 mti;
} MT19937;

MT19937 *MT19937_init(void);
u32 MT19937_rand(MT19937 *object);

void MT19937_test(int round);

#endif