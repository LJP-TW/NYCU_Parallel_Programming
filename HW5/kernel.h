#ifndef KERNEL_H_
#define KERNEL_H_

#define USE_KERNEL 4

//extern "C"
void hostFE(float uX, float uY, float lX, float lY, int *image, int resX, int resY, int maxIterations);

#endif /* KERNEL_H_ */
