#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/random.h>
#include "shishua-avx2.h"

#define U32_MAX 0xffffffff

typedef long int v4di __attribute__ ((vector_size (32)));
typedef union {
    v4di v;
    long int e[4];
} ve4di;

typedef double v4df __attribute__ ((vector_size (32)));
typedef union {
    v4df v;
    double e[4];
} ve4df;

typedef unsigned long u64;
typedef unsigned int u32;
typedef long long int s64;

s64 hit;
pthread_mutex_t hit_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

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

s64 _tf_estimate_pi_4(s64 toss_cnt, prng_state *prng)
{
    s64 _hit = 0;
    ve4df fone = {1, 1, 1, 1};
    ve4di ione = {1, 1, 1, 1};
    ve4di result = {0, 0, 0, 0};
    
    for (s64 i = 0; i < toss_cnt; i+=4) {
        ve4df distance;
        ve4di cmp;
        
        ve4df x = { get_rand(prng, -1, 1),
                    get_rand(prng, -1, 1),
                    get_rand(prng, -1, 1),
                    get_rand(prng, -1, 1) };
        ve4df y = { get_rand(prng, -1, 1),
                    get_rand(prng, -1, 1),
                    get_rand(prng, -1, 1),
                    get_rand(prng, -1, 1) };
        distance.v = x.v * x.v + y.v * y.v;

        cmp.v = distance.v <= fone.v;
        result.v = result.v + (ione.v & cmp.v);

        // printf("x: %f, %f, %f, %f\n", x.e[0], x.e[1], x.e[2], x.e[3]);
        // printf("y: %f, %f, %f, %f\n", y.e[0], y.e[1], y.e[2], y.e[3]);
        // printf("d: %f, %f, %f, %f\n", distance.e[0], distance.e[1], distance.e[2], distance.e[3]);
        // printf("c: %ld, %ld, %ld, %ld\n", cmp.e[0], cmp.e[1], cmp.e[2], cmp.e[3]);
        // printf("r: %ld, %ld, %ld, %ld\n", result.e[0], result.e[1], result.e[2], result.e[3]);
    }

    for (int i = 0; i < 4; ++i) {
        _hit += result.e[i];
    }

    // printf("_hit: %lld\n", _hit);

    return _hit;
}

void *tf_estimate_pi(void *_toss_cnt)
{
    s64 toss_cnt = (s64)_toss_cnt;
    s64 _hit = 0;
    s64 remain;
    prng_state prng;

    rand_init(&prng);
    
    remain = toss_cnt % 4;

    _hit += _tf_estimate_pi_4(toss_cnt - remain, &prng);

    for (s64 i = 0; i < remain; i++) {
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