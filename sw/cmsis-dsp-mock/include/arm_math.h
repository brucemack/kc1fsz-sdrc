#ifndef _arm_math_h
#define _arm_math_h

//#include <stdfloat>
#include <cstdint>

// TODO: FIGURE OUT WHERE DEFINED
typedef float float32_t;
typedef int32_t q31_t;

#define PI               3.14159265358979f

enum arm_status {
    ARM_MATH_SUCCESS
};

struct arm_fir_instance_f32 {
    uint16_t numTaps; 
    float32_t* pState;
    const float32_t* pCoeffs;
    // EXTRA
    uint32_t blockSize;
};

struct arm_fir_instance_q31 {
    uint16_t numTaps; 
    q31_t* pState;
    const q31_t* pCoeffs;
    // EXTRA
    uint32_t blockSize;
};

struct arm_fir_decimate_instance_f32 {
    uint8_t	M;
    uint16_t numTaps;
    float32_t* pState;
    const float32_t* pCoeffs;
    // EXTRA
    uint32_t blockSize;
};

struct arm_fir_interpolate_instance_f32 {
    uint8_t L;
    uint16_t phaseLength;
    const float32_t* pCoeffs;
    float32_t* pState;
    // EXTRA
    uint32_t blockSize;
};

struct arm_biquad_casd_df1_inst_f32 {
    uint32_t numStages;
    float32_t* pState;
    const float32_t* pCoeffs;
};

/**
 * @param pCoeffs Filter coefficients in reverse order!
 * @param pState Must be numTaps + blockSize - 1 in length 
 * @param blockSize Number of INPUT samples to process
 */
void arm_fir_init_f32(arm_fir_instance_f32*	S,
    uint16_t numTaps,
    const float32_t* pCoeffs,
    float32_t* pState,
    uint32_t blockSize 
);

void arm_fir_init_q31(arm_fir_instance_q31* S,
    uint16_t numTaps,
    const q31_t* pCoeffs,
    q31_t* pState,
    uint32_t blockSize 
);

/**
 * @param pCoeffs Filter coefficients in reverse order!
 * @param pState Must be numTaps + blockSize - 1 in length 
 * @param blockSize Number of INPUT samples to process
 */
arm_status arm_fir_decimate_init_f32(arm_fir_decimate_instance_f32* S,
    uint16_t numTaps,
    uint8_t M,
    const float32_t* pCoeffs,
    float32_t* pState,
    uint32_t blockSize 
);

/**
 * @param numTaps The length of the filter numTaps must be a multiple of the 
 * interpolation factor L.
 * @param pState points to the array of state variables. pState is of length 
 * (numTaps/L)+blockSize-1 words where blockSize is the number of input samples 
 * processed by each call to arm_fir_interpolate_f32(). 
 * @param blockSize is the number of items on the *input* side of the filter.
*/
arm_status arm_fir_interpolate_init_f32(arm_fir_interpolate_instance_f32* s,
    uint8_t	L,
    uint16_t numTaps,
    const float32_t* pCoeffs,
    float32_t* pState,
    uint32_t blockSize);

/**
 * @param blockSize Number of INPUT samples to process
 */
void arm_fir_f32(const arm_fir_instance_f32* S,
    const float32_t* pSrc,
    float32_t* pDst,
    uint32_t blockSize 
);

/**
 * @param blockSize Number of INPUT samples to process
 */
void arm_fir_decimate_f32(const arm_fir_decimate_instance_f32* S,
    const float32_t* pSrc,
    float32_t* pDst,
    uint32_t blockSize
);

/**
 * @brief NOTE: This filter has a gain of 1/L!! That may not be expected.
 */
void arm_fir_interpolate_f32(const arm_fir_interpolate_instance_f32* s,
    const float32_t* pSrc,
    float32_t* pDst,
    uint32_t blockSize);
/**
 * The coefficients are stored in the array pCoeffs in 
 * the following order:
 * {b10, b11, b12, a11, a12, b20, b21, b22, a21, a22, ...}
 * where b1x and a1x are the coefficients for the first 
 * stage, b2x and a2x are the coefficients for the
 * second stage, and so on. The pCoeffs array contains a 
 * total of 5*numStages values.
 * 
 * The pState points to state variables array. Each 
 * Biquad stage has 4 state variables x[n-1], x[n-2],
 * y[n-1], and y[n-2]. The state variables are arranged 
 * in the pState array as:
 * {x[n-1], x[n-2], y[n-1], y[n-2]}
 * The 4 state variables for stage 1 are first, then the 
 * 4 state variables for stage 2, and so on. The state array
 * has a total length of 4*numStages values. The state variables 
 * are updated after each block of data is
 * processed, the coefficients are untouched.
 */
void arm_biquad_cascade_df1_init_f32(arm_biquad_casd_df1_inst_f32* s, 
    uint8_t numStages, const float32_t* pCoeffs, 
    float32_t *pState);

/**
 * Each Biquad stage implements a second order filter using 
 * the difference equation:
 * 
 * y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] 
 */
void arm_biquad_cascade_df1_f32(const arm_biquad_casd_df1_inst_f32* s, 
    const float32_t* pSrc, float32_t* pDst, uint32_t blockSize);

void arm_rms_f32(const float32_t* pSrc,
    uint32_t blockSize,
    float32_t* pResult);

void arm_sqrt_f32(float32_t a, float32_t* result);

float32_t arm_cos_f32(float32_t a);
float32_t arm_sin_f32(float32_t a);

void arm_q31_to_float(const q31_t* pSrc, float32_t* pDst, uint32_t blockSize);
void arm_float_to_q31(const float32_t* pSrc, q31_t* pDst, uint32_t blockSize);

void arm_absmax_f32(const float32_t* pSrc, uint32_t	blockSize, float32_t* pResult,
    uint32_t* pIndex);	

#endif
