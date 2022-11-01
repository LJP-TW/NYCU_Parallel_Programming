#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/random.h>
#include "shishua-avx2.h"

#define U32_MAX 0xffffffff

typedef unsigned long u64;
typedef unsigned int u32;
typedef long long int s64;

s64 hit;
pthread_mutex_t hit_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

// Mersenne Twister PRNG

static inline void rand_init(prng_state *prng)
{
    u64 seed[4];
    ssize_t ret;
    
    ret = getrandom(seed, sizeof(seed), 0);

    if (ret < 0) {
        fprintf(stderr, "getrandom failed\n");
        exit(0);
    }

    prng_init(prng, seed);
}

double get_rand(prng_state *prng, double min, double max)
{
    uint8_t buf[0x80] __attribute__ ((aligned (64)));
    
    prng_gen(prng, buf, sizeof(buf));

    return min + ((double)(*((u32 *)buf)) / U32_MAX) * (max - min);
}

void add_hit(int value)
{
    pthread_mutex_lock(&hit_mutex);
    
    hit += value;

    pthread_mutex_unlock(&hit_mutex);
}

void *tf_estimate_pi(void *_toss_cnt)
{
    s64 toss_cnt = (s64)_toss_cnt;
    s64 _hit = 0;
    prng_state prng;

    rand_init(&prng);
    
    for (s64 i = 0; i < toss_cnt; i++) {
        double x = get_rand(&prng, -1, 1);
        double y = get_rand(&prng, -1, 1);
        double distance = x * x + y * y;
        if (distance <= 1)
            _hit++;
    }

    add_hit(_hit);

    return NULL;
}

/*
 * pi.out takes two command-line arguments, which indicate the number of 
 * threads and the number of tosses, respectively. The value of the first
 * and second arguments will not exceed the range of int and long long int,
 * respectively. 
 */
int main(int argc, char **argv)
{
    int thread_cnt;
    s64 toss_cnt, remain, loading;
    double pi;
    pthread_t *tid;
    int i;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s thread_num tosses_num\n", argv[0]);
        return 0;
    }

    thread_cnt = atoi(argv[1]);
    toss_cnt = atoll(argv[2]);

    tid = (pthread_t *)malloc(sizeof(pthread_t) * thread_cnt);
    remain = toss_cnt;
    loading = toss_cnt / thread_cnt;

    for (i = 0; i < thread_cnt - 1; i++) {
        pthread_create(&tid[i], NULL, tf_estimate_pi, (void *)loading);
        remain -= loading;
    }
    pthread_create(&tid[i], NULL, tf_estimate_pi, (void *)remain);

    for (int i = 0; i < thread_cnt; i++) {
        pthread_join(tid[i], NULL);
    }

    pi = hit * (4 / ((double)toss_cnt));

    printf("%f\n", pi);

    return 0;
}