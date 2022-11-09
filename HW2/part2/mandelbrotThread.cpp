/*

  Note: This code was modified from example code
  originally provided by Intel.  To comply with Intel's open source
  licensing agreement, their copyright is retained below.

  -----------------------------------------------------------------

  Copyright (c) 2010-2011, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <thread>

#include "CycleTimer.h"

typedef int v4si __attribute__ ((vector_size (16)));
typedef union {
    v4si v;
    int e[4];
    __int128 bits;
} ve4si;

typedef float v4sf __attribute__ ((vector_size (16)));
typedef union {
    v4sf v;
    float e[4];
} ve4sf;

typedef struct
{
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int *output;
    int startRow;
    int totalRows;
} WorkerArgs;

static inline void vmandel(ve4sf *vc_re, ve4sf *vc_im, int count, ve4si *vout)
{
  ve4sf vz_re, vz_im;
  vz_re.v = vc_re->v;
  vz_im.v = vc_im->v;
  ve4si v0 = { 0, 0, 0, 0 };
  ve4si v1 = { 1, 1, 1, 1 };
  ve4sf v2f = { 2, 2, 2, 2 };
  ve4sf v4f = { 4, 4, 4, 4 };
  ve4si vmask = { 0, 0, 0, 0 };
  
  vout->v = v0.v;

  for (int i = 0; i < count; ++i)
  {
    ve4si vcmpmask;
    ve4sf vnew_re;
    ve4sf vnew_im;
    
    vcmpmask.v = vz_re.v * vz_re.v + vz_im.v * vz_im.v > v4f.v;

    vmask.v = vmask.v | vcmpmask.v;

    if (!(~vmask.bits)) {
      break;
    }

    vnew_re.v = vz_re.v * vz_re.v - vz_im.v * vz_im.v;
    vnew_im.v = v2f.v * vz_re.v * vz_im.v;

    vz_re.v = vc_re->v + vnew_re.v;
    vz_im.v = vc_im->v + vnew_im.v;

    vout->v = vout->v + (v1.v & (!vmask.v));
  }
}

static inline int mandel(float c_re, float c_im, int count)
{
  float z_re = c_re, z_im = c_im;
  int i;
  for (i = 0; i < count; ++i)
  {
    if (z_re * z_re + z_im * z_im > 4.f)
      break;

    float new_re = z_re * z_re - z_im * z_im;
    float new_im = 2.f * z_re * z_im;
    z_re = c_re + new_re;
    z_im = c_im + new_im;
  }

  return i;
}

//
// MandelbrotSerial --
//
// Compute an image visualizing the mandelbrot set.  The resulting
// array contains the number of iterations required before the complex
// number corresponding to a pixel could be rejected from the set.
//
// * x0, y0, x1, y1 describe the complex coordinates mapping
//   into the image viewport.
// * width, height describe the size of the output image
// * startRow, totalRows describe how much of the image to compute
static void mandelbrotSerialOptimize(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int totalRows,
    int maxIterations,
    int output[])
{
  float dx = (x1 - x0) / width;
  float dy = (y1 - y0) / height;

  ve4sf vy0 = { y0, y0, y0, y0 };
  ve4sf vx0 = { x0, x0, x0, x0 };
  ve4sf vdy = { dy, dy, dy, dy };
  ve4sf vdx = { dx, dx, dx, dx };
  ve4sf v03 = { 0, 1, 2, 3 };
  ve4sf vwidth = { (float)width, (float)width, (float)width, (float)width };

  int endRow = startRow + totalRows;

  for (int j = startRow; j < endRow; ++j)
  {
    ve4sf vj = { (float)j, (float)j, (float)j, (float)j };

    float _y0 = y0 + j * dy;
    ve4sf v_y0;
    v_y0.v = vy0.v + vj.v * vdy.v;

    int i = 0;

    for (; i < width;)
    {
      ve4sf vi = { (float)i, (float)i, (float)i, (float)i };

      // float _x0 = x0 + (i + 0) * dx;
      // float _x1 = x0 + (i + 1) * dx;
      // float _x2 = x0 + (i + 2) * dx;
      // float _x3 = x0 + (i + 3) * dx;
      ve4sf v_x03;
      v_x03.v = vx0.v + (vi.v + v03.v) * vdx.v;

      // int index0 = (j * width + (i + 0));
      // int index1 = (j * width + (i + 1));
      // int index2 = (j * width + (i + 2));
      // int index3 = (j * width + (i + 3));
      ve4si vindex0;
      vindex0.v = __builtin_convertvector(vj.v * vwidth.v + (vi.v + v03.v), v4si);

      // output[index0] = mandel(_x0, _y0, maxIterations);
      // output[index1] = mandel(_x1, _y0, maxIterations);
      // output[index2] = mandel(_x2, _y0, maxIterations);
      // output[index3] = mandel(_x3, _y0, maxIterations);
      ve4si vret;
      vmandel(&v_x03, &v_y0, maxIterations, &vret);
      for (int idx = 0; idx < 4; ++idx) {
        output[vindex0.e[idx]] = vret.e[idx];
      }

      if (i + 4 < width) {
        i += 4;
      } else {
        break;
      }
    }

    for (; i < width; ++i) 
    {
      float _x0 = x0 + i * dx;

      int index0 = (j * width + i);

      output[index0] = mandel(_x0, _y0, maxIterations);
    }
  }
}

//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs *const args)
{

    // TODO FOR PP STUDENTS: Implement the body of the worker
    // thread here. Each thread could make a call to mandelbrotSerial()
    // to compute a part of the output image. For example, in a
    // program that uses two threads, thread 0 could compute the top
    // half of the image and thread 1 could compute the bottom half.
    // Of course, you can copy mandelbrotSerial() to this file and 
    // modify it to pursue a better performance.

    mandelbrotSerialOptimize(
        args->x0,
        args->y0,
        args->x1,
        args->y1,
        args->width,
        args->height,
        args->startRow,
        args->totalRows,
        args->maxIterations,
        args->output
    );
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    static constexpr int MAX_THREADS = 32;
    int remain;
    int loading;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    remain = height;
    loading = height / numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        // TODO FOR PP STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].output = output;
        args[i].startRow = i * loading;
        args[i].totalRows = (i != numThreads - 1) ? loading : remain;

        remain -= loading;
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i = 1; i < numThreads; i++)
    {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }

    workerThreadStart(&args[0]);

    // join worker threads
    for (int i = 1; i < numThreads; i++)
    {
        workers[i].join();
    }
}
