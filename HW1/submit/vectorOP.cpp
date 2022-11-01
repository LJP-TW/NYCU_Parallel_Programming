#include "PPintrin.h"

// implementation of absSerial(), but it is vectorized using PP intrinsics
void absVector(float *values, float *output, int N)
{
  __pp_vec_float x;
  __pp_vec_float result;
  __pp_vec_float zero = _pp_vset_float(0.f);
  __pp_mask maskAll, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {

    // All ones
    maskAll = _pp_init_ones();

    // All zeros
    maskIsNegative = _pp_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _pp_vload_float(x, values + i, maskAll); // x = values[i];

    // Set mask according to predicate
    _pp_vlt_float(maskIsNegative, x, zero, maskAll); // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _pp_vsub_float(result, zero, x, maskIsNegative); //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _pp_mask_not(maskIsNegative); // } else {

    // Execute instruction ("else" clause)
    _pp_vload_float(result, values + i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _pp_vstore_float(output + i, result, maskAll);
  }
}

void clampedExpVector(float *values, int *exponents, float *output, int N)
{
  //
  // PP STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //
  __pp_vec_int zero, one;
  __pp_vec_float clampedValue;

  zero = _pp_vset_int(0);
  one = _pp_vset_int(1);
  clampedValue = _pp_vset_float(9.999999f);

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    __pp_vec_float x, result;
    __pp_vec_int y;
    int maskWidth;
    __pp_mask mask, maskEq0, maskNeq0, maskGtCV;

    maskWidth = i + VECTOR_WIDTH <= N ? VECTOR_WIDTH : N % VECTOR_WIDTH;
    mask = _pp_init_ones(maskWidth);

    maskEq0 = _pp_init_ones(0);
    maskNeq0 = _pp_init_ones(0);
    maskGtCV = _pp_init_ones(0);

    _pp_vload_float(x, values + i, mask);
    _pp_vload_int(y, exponents + i, mask);

    _pp_veq_int(maskEq0, y, zero, mask);
    _pp_vset_float(result, 1.f, maskEq0);

    maskNeq0 = _pp_mask_not(maskEq0);

    _pp_vmove_float(result, x, maskNeq0);
    
    _pp_vsub_int(y, y, one, maskNeq0);
    
    while (1)
    {
      int cnt;
      __pp_mask maskGt0;

      maskGt0 = _pp_init_ones(0);

      _pp_vgt_int(maskGt0, y, zero, mask);
      
      cnt = _pp_cntbits(maskGt0);

      if (!cnt) {
        break;
      }

      _pp_vmult_float(result, result, x, maskGt0);
      _pp_vsub_int(y, y, one, maskGt0);
    }

    _pp_vgt_float(maskGtCV, result, clampedValue, mask);
    _pp_vmove_float(result, clampedValue, maskGtCV);

    _pp_vstore_float(output + i, result, mask);
  }
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float *values, int N)
{

  //
  // PP STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //
  __pp_vec_float sum;
  __pp_mask mask;

  sum = _pp_vset_float(0);
  mask = _pp_init_ones();

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    __pp_vec_float vec;

    _pp_vload_float(vec, values + i, mask);
    _pp_vadd_float(sum, sum, vec, mask);
  }

  for (int i = VECTOR_WIDTH; i != 1; i /= 2) {
    _pp_hadd_float(sum, sum);
    _pp_interleave_float(sum, sum);
  }

  return sum.value[0];
}