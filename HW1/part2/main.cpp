#include <iostream>
#include <getopt.h>

using namespace std;

void usage(const char* progname);
void initValue(float* values1, float* values2, double* value3, float* output, unsigned int N);

extern void test1(float* a, float* b, float* c, int N);
extern void test2(float *__restrict a, float *__restrict b, float *__restrict c, int N);
extern double test3(double* __restrict a, int N) ;

int main(int argc, char * argv[]) {
  int N = 1024;
  int whichTestToRun = 1;

  // parse commandline options ////////////////////////////////////////////
  int opt;
  static struct option long_options[] = {
    {"size", 1, 0, 's'},
    {"test", 1, 0, 't'},
    {"help", 0, 0, '?'},
    {0 ,0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "st:?", long_options, NULL)) != EOF) {

    switch (opt) {
      case 's':
        N = atoi(optarg);
        if (N <= 0) {
          cout << "Error: Workload size is set to" << N << " (<0).\n";
          return -1;
        }
        break;
      case 't':
        whichTestToRun = atoi(optarg);
        if (whichTestToRun <= 0 || whichTestToRun >= 4) {
          cout << "Error: test" << whichTestToRun << "() is not available.\n";
          return -1;
        }
        break;
      case 'h':
      default:
        usage(argv[0]);
        return 1;
    }
  }

  float* values1 = new(std::align_val_t{ 32 }) float[N];
  float* values2 = new(std::align_val_t{ 32 }) float[N];
  double* values3 = new(std::align_val_t{ 32 }) double[N];
  float* output = new(std::align_val_t{ 32 }) float[N];
  initValue(values1, values2, values3, output, N);

  cout << "Running test" << whichTestToRun << "()...\n";
  switch (whichTestToRun) {
    case 1: test1(values1, values2, output, N); break;
    case 2: test2(values1, values2, output, N); break;
    case 3: test3(values3, N); break;
  }

  delete [] values1;
  delete [] values2;
  delete [] values3;
  delete [] output;

  return 0;
}

void usage(const char* progname) {
  printf("Usage: %s [options]\n", progname);
  printf("Program Options:\n");
  printf("  -s  --size <N>     Use workload size N (Default = 1024)\n");
  printf("  -t  --test <N>     Just run the testN function (Default = 1)\n");
  printf("  -h  --help         This message\n");
}

void initValue(float* values1, float* values2, double* values3, float* output, unsigned int N) {
  for (unsigned int i=0; i<N; i++)
  {
    // random input values
    values1[i] = -1.f + 4.f * static_cast<float>(rand()) / RAND_MAX;
    values2[i] = -1.f + 4.f * static_cast<float>(rand()) / RAND_MAX;
    values3[i] = -1.f + 4.f * static_cast<double>(rand()) / RAND_MAX;
    output[i] = 0.f;
  }
}
