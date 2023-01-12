__kernel void convolution(
    __global const float *input_image,
    __global const float *filter,
    __global float *output_image,
    const int image_height,
    const int image_width,
    const int filter_width)
{
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    int halffilter_width = filter_width / 2;
    float sum;
    int k, l;

    sum = 0;
    for (k = -halffilter_width; k <= halffilter_width; k++)
    {
        for (l = -halffilter_width; l <= halffilter_width; l++)
        {
            if (gy + k >= 0 && gy + k < image_height &&
                gx + l >= 0 && gx + l < image_width)
            {
                sum += input_image[(gy + k) * image_width + gx + l] *
                       filter[(k + halffilter_width) * filter_width +
                              l + halffilter_width];
            }
        }
    }
    output_image[gy * image_width + gx] = sum;
}
