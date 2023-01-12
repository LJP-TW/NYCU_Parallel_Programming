// Ref: https://www.eriksmistad.no/getting-started-with-opencl-and-gpu-computing/
#include <stdio.h>
#include <stdlib.h>
#include "hostFE.h"
#include "helper.h"

void hostFE(int filter_width, float *filter, int image_height, int image_width,
            float *inputImage, float *outputImage, cl_device_id *device,
            cl_context *context, cl_program *program)
{
    cl_int status;
    int image_size = image_height * image_width;
    int filter_size = filter_width * filter_width;

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(*context, *device, 0, &status);
    CHECK(status, "clCreateCommandQueue");
 
    // Create memory buffers on the device
    cl_mem input_img_mem_obj = clCreateBuffer(*context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
            sizeof(float) * image_size, inputImage, &status);
    CHECK(status, "clCreateBuffer");

    cl_mem filter_mem_obj = clCreateBuffer(*context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
            sizeof(float) * filter_size, filter, &status);
    CHECK(status, "clCreateBuffer");

    cl_mem output_img_mem_obj = clCreateBuffer(*context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 
            sizeof(float) * image_size, outputImage, &status);
    CHECK(status, "clCreateBuffer");

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(*program, "convolution", &status);
    CHECK(status, "clCreateKernel");

    // Set the arguments of the kernel
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&input_img_mem_obj);
    CHECK(status, "clSetKernelArg");

    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&filter_mem_obj);
    CHECK(status, "clSetKernelArg");

    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&output_img_mem_obj);
    CHECK(status, "clSetKernelArg");

    status = clSetKernelArg(kernel, 3, sizeof(int), (void *)&image_height);
    CHECK(status, "clSetKernelArg");

    status = clSetKernelArg(kernel, 4, sizeof(int), (void *)&image_width);
    CHECK(status, "clSetKernelArg");

    status = clSetKernelArg(kernel, 5, sizeof(int), (void *)&filter_width);
    CHECK(status, "clSetKernelArg");
 
    // Execute the OpenCL kernel on the list
    size_t global_item_size[2] = { image_width, image_height };
    size_t local_item_size[2] = { 40, 25 };
    status = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, 
            global_item_size, local_item_size, 0, NULL, NULL);
    CHECK(status, "clEnqueueNDRangeKernel");

    // After map call, host-memory area for outputImage is
    // automatically updated with the latest bits from the device
    clEnqueueMapBuffer(
        command_queue,
        output_img_mem_obj,
        CL_TRUE,
        CL_MAP_READ,
        0,
        sizeof(float) * image_size,
        0, 0, 0,
        &status
    );
    CHECK(status, "clEnqueueMapBuffer");

    // All resources are deallocated automatically.
}