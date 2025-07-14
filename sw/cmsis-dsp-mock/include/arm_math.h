#ifndef _arm_math_h
#define _arm_math_h

#include <stdfloat>
#include <cstdint>

// TODO: FIGURE OUT WHERE DEFINED
typedef float float32_t;

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

struct arm_fir_decimate_instance_f32 {
    uint8_t	M;
    uint16_t numTaps;
    float32_t* pState;
    const float32_t* pCoeffs;
    // EXTRA
    uint32_t blockSize;
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

#endif
