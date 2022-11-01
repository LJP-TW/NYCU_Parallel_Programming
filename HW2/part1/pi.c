#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define U64_MAX 0xffffffffffffffff

typedef unsigned long long int u64;
typedef long long int s64;

int rand_fd;

s64 hit;
pthread_mutex_t hit_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

void rand_init()
{
    rand_fd = open("/dev/urandom", O_RDONLY);

    if (rand_fd < 0) {
        fprintf(stderr, "Cannot open /dev/urandom!\n");
        exit(0);
    }
}

double get_rand(double min, double max)
{
    u64 num;
    ssize_t ret;
    
    ret = read(rand_fd, &num, sizeof(num));

    if (ret < 0) {
        fprintf(stderr, "Cannot read from /dev/urandom!\n");
        exit(0);
    }

    return min + ((double)num / U64_MAX) * (max - min);
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
    s64 _hit;
    
    for (s64 i = 0; i < toss_cnt; i++) {
        double x = get_rand(-1, 1);
        double y = get_rand(-1, 1);
        double distance = x * x + y * y;
        if (distance <= 1)
            _hit++;
    }

    add_hit(_hit);
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

    rand_init();

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