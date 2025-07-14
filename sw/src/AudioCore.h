/**
 * Software Defined Repeater Controller
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#ifndef _AudioCore_h
#define _AudioCore_h

#include <cstdint>
#include <cmath>

#include <arm_math.h>

namespace kc1fsz {

/**
 * @brief The master application configuration structure.
 */
class AudioCore {
public:

    static const unsigned FS_ADC = 32000;
    static const unsigned BLOCK_SIZE_ADC = 256;
    static const unsigned FS = FS_ADC / 4;
    static const unsigned BLOCK_SIZE = BLOCK_SIZE_ADC / 4;

    AudioCore(unsigned id);

    void cycle0(const float* adc_in, float* cross_out);
    void cycle1(unsigned cross_count, 
        const float** cross_ins, const float* cross_gains, float* dac_out);

    float getSignalRms() const { return _signalRms; }
    float getNoiseRms() const { return _noiseRms; }
    float getOutRms() const { return _outRms; }

    void setCtcssDecodeFreq(float hz);
    float getCtcssDecodeMag() const { return _ctcssMag; }

    void setCtcssEncodeEnabled(bool b);
    void setCtcssEncodeFreq(float hz);
    void setCtcssEncodeLevel(float db);

    void setDelayMs(unsigned ms);
    void resetDelay() { _delayCountdown = _delaySamples; }

private:

    const unsigned _id;

    // Noise HPF, runs at 32k
    static const unsigned FILTER_B_LEN = 41;
    static const float FILTER_B[FILTER_B_LEN];
    arm_fir_instance_f32 _filtB;
    float _filtBState[FILTER_B_LEN + BLOCK_SIZE_ADC - 1];
  
    // Decimation LPF
    static const unsigned FILTER_C_LEN = 41;
    static const float FILTER_C[FILTER_C_LEN];
    // For the 16k filter, but still runs on 32k audio
    arm_fir_decimate_instance_f32 _filtC;
    float _filtCState[FILTER_C_LEN + BLOCK_SIZE_ADC - 1];
    // For the 8k filter, but still runs on 16k audio
    arm_fir_decimate_instance_f32 _filtD;
    float _filtDState[FILTER_C_LEN + (BLOCK_SIZE_ADC / 2) - 1];

    // Band pass filter (CTCSS removal), runs at 8k
    static const unsigned FILTER_F_LEN = 127;
    static const float FILTER_F[FILTER_F_LEN];
    arm_fir_instance_f32 _filtF;
    float _filtFState[FILTER_F_LEN + BLOCK_SIZE - 1];

    // Low-pass filter for interpolation 8K->32K, runs at 32k
    static const unsigned FILTER_N_LEN = 127;
    static const float FILTER_N[FILTER_N_LEN];
    arm_fir_instance_f32 _filtN;
    float _filtNState[FILTER_N_LEN + BLOCK_SIZE_ADC - 1];

    // For capturing various measures of energy
    float _noiseRms;
    float _signalRms;
    float _outRms;

    // Used for CTCSS encoding
    bool _ctcssEncodeEnabled = false;
    float _ctcssEncodeFreq = 123;
    float _ctcssEncodeOmega = 0;
    float _ctcssEncodePhi = 0;
    float _ctcssEncodeLevel = 0;

    // Used for CTCSS decoding
    float _ctcssDecodeFreq = 123;
    float _gz1 = 0, _gz2 = 0;
    float _gcw, _gsw, _gc;
    float _ctcssMag = 0;
    unsigned _ctcssBlock = 0;
    unsigned _ctcssBlocks = 0;

    // Audio delay (250ms)
    static const unsigned _delayAreaLen = 2000;
    unsigned _delayAreaReadPtr = 0;
    unsigned _delayAreaWritePtr = 0;
    float _delayArea[_delayAreaLen];
    unsigned _delaySamples = 0;
    unsigned _delayCountdown = 0;
};

}

#endif
