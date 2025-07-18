#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include "arm_math.h"

void arm_fir_init_f32(arm_fir_instance_f32*	s,
    uint16_t numTaps,
    const float32_t* pCoeffs,
    float32_t* pState,
    uint32_t blockSize) {
    s->numTaps = numTaps;
    s->pState = pState;
    s->pCoeffs = pCoeffs;
    // EXTRA
    s->blockSize = blockSize;
    for (unsigned i = 0; i < numTaps - 1; i++)
        s->pState[i] = 0;
}

arm_status arm_fir_decimate_init_f32(arm_fir_decimate_instance_f32* s,
    uint16_t numTaps,
    uint8_t M,
    const float32_t* pCoeffs,
    float32_t* pState,
    uint32_t blockSize) {
    assert(M == 2);
    assert(blockSize % 2 == 0);
    s->numTaps = numTaps;
    s->M = M;
    s->pState = pState;
    s->pCoeffs = pCoeffs;
    // EXTRA
    s->blockSize = blockSize;
    for (unsigned i = 0; i < numTaps - 1; i++)
        s->pState[i] = 0;
    return arm_status::ARM_MATH_SUCCESS;
}

arm_status arm_fir_interpolate_init_f32(arm_fir_interpolate_instance_f32* s,
    uint8_t	L,
    uint16_t numTaps,
    const float32_t* pCoeffs,
    float32_t* pState,
    uint32_t blockSize) {
    assert(numTaps % L == 0);
    s->L = L;
    s->phaseLength = numTaps / L;
    s->pState = pState;
    s->pCoeffs = pCoeffs;
    // EXTRA
    s->blockSize = blockSize;
    for (unsigned i = 0; i < numTaps / L - 1; i++)
        s->pState[i] = 0;
    return arm_status::ARM_MATH_SUCCESS;
}

void arm_fir_f32(const arm_fir_instance_f32* s,
    const float32_t* pSrc,
    float32_t* pDst,
    uint32_t blockSize) {
    assert(blockSize == s->blockSize);
    // Shift left to free space for new data   
    memmove((void*)(s->pState), (const void*)&(s->pState[blockSize]), 
        (s->numTaps - 1) * sizeof(float32_t));
    // Fill in new data on far right
    memcpy((void*)&(s->pState[s->numTaps - 1]), (const void*)pSrc, 
        blockSize * sizeof(float32_t));
    // Do the MA
    const float32_t* dataHistory = s->pState;
    for (unsigned k = 0; k < blockSize; k++) {
        float a = 0;
        for (unsigned i = 0; i < s->numTaps; i++)
            a += dataHistory[i] * s->pCoeffs[i];
        pDst[k] = a;
        dataHistory++;
    }
}

// TODO: CONSOLIDATE
static q31_t mult_q31(q31_t a, q31_t b) {
    int64_t c = a * b;
    return c >> 31;
}

void arm_fir_q31(const arm_fir_instance_q31* s,
    const q31_t* pSrc,
    q31_t* pDst,
    uint32_t blockSize) {
    assert(blockSize == s->blockSize);
    // Shift left to free space for new data   
    memmove((void*)(s->pState), (const void*)&(s->pState[blockSize]), 
        (s->numTaps - 1) * sizeof(q31_t));
    // Fill in new data on far right
    memcpy((void*)&(s->pState[s->numTaps - 1]), (const void*)pSrc, 
        blockSize * sizeof(q31_t));
    // Do the MA
    const q31_t* dataHistory = s->pState;
    for (unsigned k = 0; k < blockSize; k++) {
        q31_t a = 0;
        for (unsigned i = 0; i < s->numTaps; i++)
            //a += dataHistory[i] * s->pCoeffs[i];
            a += mult_q31(dataHistory[i], s->pCoeffs[i]);
        pDst[k] = a;
        dataHistory++;
    }
}

void arm_fir_decimate_f32(const arm_fir_decimate_instance_f32* s,
    const float32_t* pSrc,
    float32_t* pDst,
    uint32_t blockSize) {
    assert(blockSize == s->blockSize);
    // Shift left to free space for new data. At the end of this 
    // operation the oldest sample will be at the lowest memory
    // location and the highest locations will be availlable.  
    memmove((void*)(s->pState), (const void*)&(s->pState[blockSize]), 
        (s->numTaps - 1) * sizeof(float32_t));
    // Fill in new data on far right (highest). At the end of 
    // this operation the newest sample will be at the highest 
    // memory location.
    memcpy((void*)&(s->pState[s->numTaps - 1]), (const void*)pSrc, 
        blockSize * sizeof(float32_t));
    // Do the MA
    const float32_t* dataHistory = s->pState;
    // Here we are only processing every other output point
    for (unsigned k = 0; k < blockSize / 2; k++) {
        float a = 0;
        for (unsigned i = 0; i < s->numTaps; i++)
            a += dataHistory[i] * s->pCoeffs[i];
        pDst[k] = a;
        dataHistory += 2;
    }
}

void arm_fir_interpolate_f32(const arm_fir_interpolate_instance_f32* s,
    const float32_t* pSrc,
    float32_t* pDst,
    uint32_t blockSize)	{
    assert(blockSize == s->blockSize);
    unsigned numTaps = s->phaseLength * s->L;
    // Shift left to free space for new data. At the end of this 
    // operation the oldest sample will be at the lowest memory
    // location and the highest locations will be availlable.  
    memmove((void*)(s->pState), (const void*)&(s->pState[blockSize]), 
        (numTaps - 1) * sizeof(float32_t));
    // Fill in new data on far right (highest). At the end of 
    // this operation the newest sample will be at the highest 
    // memory location.
    memcpy((void*)&(s->pState[numTaps - 1]), (const void*)pSrc, 
        blockSize * sizeof(float32_t));
    // Do the multipy-add
    const float32_t* dataHistory = s->pState;
    unsigned phaseStart = 0;
    // k iterates through the output positions
    for (unsigned k = 0; k < blockSize * s->L; k++) {
        float a = 0;
        // The starting point for the iteration across the filter coefficients
        // rotates through L possible starting points. The increment is always
        // L.  THIS IS ESSENTIALLY A POLYPHASE FILTER.
        // 
        // i is iterating across the input history
        // j is iterating accross the coefficients
        for (unsigned i = 0, j = phaseStart; i < s->phaseLength; i++, j += s->L) {
            //assert( j < numTaps);
            a += dataHistory[i] * s->pCoeffs[j];
        }
        // WE NEED TO SCALE UP EACH OUTPUT BECAUSE WE ARE ONLY USING 1/s->L OF 
        // THE SAMPLES.
        //assert(k < blockSize * s->L);
        pDst[k] = a * (float)s->L;
        // Advance on the data history and filter coefficient starting point.
        if (phaseStart == 0) {
            // At the completion of phase 0 we shift to the next input sample
            ++dataHistory;
            // Wrap around to the next phase
            phaseStart = s->L - 1;
        }
        else {
            // Loop through the L phases
            phaseStart--;
        }
    }
}

void arm_rms_f32(const float32_t* pSrc,
    uint32_t blockSize,
    float32_t* pResult) {
    float a = 0;
    for (unsigned i = 0; i < blockSize; i++)
        a += pow(pSrc[i], 2.0);
    a /= (float)blockSize;
    *pResult = sqrt(a);
}

void arm_sqrt_f32(float32_t a, float32_t* result) {
    *result = sqrt(a);
}

float32_t arm_cos_f32(float32_t a) {
    return cos(a);
}

float32_t arm_sin_f32(float32_t a) {
    return sin(a);
}

void arm_q31_to_float(const q31_t* pSrc, float32_t* pDst, uint32_t blockSize) {
    for (unsigned i = 0; i < blockSize; i++)
        pDst[i] = (float)pSrc[i] / 2147483648.0;
}

void arm_float_to_q31(const float32_t* pSrc, q31_t* pDst, uint32_t blockSize) {
    for (unsigned i = 0; i < blockSize; i++)
        pDst[i] = pSrc[i] * 2147483648.0;
}


