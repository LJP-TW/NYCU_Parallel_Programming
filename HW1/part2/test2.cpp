#include <iostream>
#include "test.h"
#include "fasttime.h"

void test2(float *__restrict a, float *__restrict b, float *__restrict c, int N)
{
  __builtin_assume(N == 1024);
  a = (float *)__builtin_assume_aligned(a, 16);
  b = (float *)__builtin_assume_aligned(b, 16);

  fasttime_t time1 = gettime();
  for (int i = 0; i < I; i++)
  {
    for (int j = 0; j < N; j++)
    {
      /* max() */
      c[j] = a[j];
      if (b[j] > a[j])
        c[j] = b[j];
      // if (b[j] > a[j]) c[j] = b[j];
      // else c[j] = a[j];
    }
  }

  fasttime_t time2 = gettime();

  double elapsedf = tdiff(time1, time2);
  std::cout << "Elapsed execution time of the loop in test2():\n"
            << elapsedf << "sec (N: " << N << ", I: " << I << ")\n";
}
