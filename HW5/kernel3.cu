#ifdef _WIN32
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#else
#include <cuda.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "kernel.h"

using namespace std;

#if USE_KERNEL == 3

// Group size: 4
// 1600 * 1200 -> 1600 * 300
#define GRID_X 100
#define GRID_Y 5
#define BLOCK_X 16
#define BLOCK_Y 60

static int cudaInited;

void cudaInit()
{
    cudaError_t cudaStatus;

    if (cudaInited)
        return;

    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        cerr << "cudaSetDevice failed! Do you have a CUDA-capable GPU installed?" << endl;
        exit(EXIT_FAILURE);
    }

    cudaInited = 1;
}

__device__ int mandel(float c_re, float c_im, int count)
{
    float z_re = c_re, z_im = c_im;
    int i;

    for (i = 0; i < count; ++i) {
        if (z_re * z_re + z_im * z_im > 4.f)
            break;

        float new_re = z_re * z_re - z_im * z_im;
        float new_im = 2.f * z_re * z_im;
        z_re = c_re + new_re;
        z_im = c_im + new_im;
    }

    return i;
}

__global__ void mandelKernel(int *output, float x0, float y0, float dx, float dy, int maxIterations)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = (blockIdx.y * blockDim.y + threadIdx.y) * 4;
    float x = x0 + i * dx;

    #pragma unroll 4
    for (int loop = 0; loop < 4; ++loop) {
        float y = y0 + (j + loop) * dy;
        int index = (j + loop) * gridDim.x * blockDim.x + i;
        output[index] = mandel(x, y, maxIterations);
    }
}

// Host front-end function that allocates the memory and launches the GPU kernel
void hostFE(float upperX, float upperY, float lowerX, float lowerY, int *img, int resX, int resY, int maxIterations)
{
    cudaError_t cudaStatus;
    int *cudaResult, *result;
    size_t pitch;
    float dx, dy;

    cudaInit();

    cudaStatus = cudaMallocPitch((void **)&cudaResult, &pitch, sizeof(int) * resX, resY);
    if (cudaStatus != cudaSuccess) {
        cerr << "cudaMallocPitch failed!" << endl;
        exit(EXIT_FAILURE);
    }

    // HW required
    cudaStatus = cudaHostAlloc((void **)&result, sizeof(int) * resX * resY, cudaHostAllocDefault);
    if (cudaStatus != cudaSuccess) {
        cerr << "cudaHostAlloc failed!" << endl;
        exit(EXIT_FAILURE);
    }

    dx = (upperX - lowerX) / resX;
    dy = (upperY - lowerY) / resY;

    dim3 dimGrid(GRID_X, GRID_Y);
    dim3 dimBlock(BLOCK_X, BLOCK_Y);

    mandelKernel<<<dimGrid, dimBlock>>>(cudaResult, lowerX, lowerY, dx, dy, maxIterations);

    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        cerr << "cudaDeviceSynchronize returned error code " << cudaStatus << " after launching addKernel!" << endl;
        exit(EXIT_FAILURE);
    }

    cudaStatus = cudaMemcpy(result, cudaResult, sizeof(int) * resX * resY, cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        cerr << "cudaMemcpy failed!" << endl;
        exit(EXIT_FAILURE);
    }

    cudaFree(cudaResult);

    // Copy result to output
    memcpy(img, result, sizeof(int) * resX * resY);

    cudaFreeHost(result);
}

#endif