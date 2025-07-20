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
 * @brief Audio processing core.
 */
class AudioCore {
public:

    static const unsigned FS_ADC = 32000;
    static const unsigned BLOCK_SIZE_ADC = 256;
    static const unsigned FS = FS_ADC / 4;
    static const unsigned BLOCK_SIZE = BLOCK_SIZE_ADC / 4;
    static const unsigned MAX_CROSS_COUNT = 8;

    AudioCore(unsigned id, unsigned crossCount);

    /**
     * @brief Called once per CODEC block. Expected to run quickly 
     * inside of the interupt service routine.
     *
     * @param adc_in One block of signed 32-bit PCM audio.
     * @param cross_out One block of audio data at the 8k rate 
     * ready to be shared across the repeater.
     */
    void cycleRx(const int32_t* codec_in, float* cross_out);

    /**
     * @brief Called once per CODEC block. Expected to run quickly 
     * inside of the interupt service routine.
     *
     * @param dac_out A block of 32-bit signed PCM samples will be written here
     */
    void cycleTx(const float** cross_ins, int32_t* codec_out);

    /**
     * The "signal" is basically all power below 4kHz, includes the sub-audible 
     * CTCSS tones.
     * @returns Signal voltage in Vrms, assuming full-scale is 1.0. Note
     */
    float getSignalRms() const { return _signalRms; }

    /**
     * The "noise" is basically all power above ~5kHz.
     * @returns Signal voltage in Vrms, assuming full-scale is 1.0. Note
     */
    float getNoiseRms() const { return _noiseRms; }

    /**
     * Voltage of all audio being routed to the transmitter, inclusive of 
     * all sources/tones/etc.
     *
     * @returns Signal voltage in Vrms, assuming full-scale is 1.0. Note
     */
    float getOutRms() const { return _outRms; }

    /**
     * @brief The received audio is multiplied by this value.
     */
    void setRxGainLinear(float gain) { _rxGain = gain; }

    void setCrossGainLinear(unsigned i, float gain);

    /**
     * @brief Controls the HPF that is intended to filter out the low
     * end of the received audio, generally PL tone elimination.
     */
    void setHPFEnabled(bool b) { _hpfEnabled = b; }

    void setCtcssDecodeFreq(float hz);

    /**
     * Voltage detected at the frequency set by setCtcssDecodeFreq().
     *
     * @returns Signal voltage in Vrms, assuming full-scale is 1.0. Note
     */
    float getCtcssDecodeRms() const;

    void setCtcssEncodeEnabled(bool b);

    void setCtcssEncodeFreq(float hz);

    void setCtcssEncodeLevel(float dbv) { _ctcssEncodeLevel = dbvToLinear(dbv); }

    void setRxDelayMs(unsigned ms);

    void resetDelay() { _delayCountdown = _delaySamples; }

    // TODO: SOFT GAINS ON RX AND TX
    
    void setToneEnabled(bool b);
    void setToneFreq(float hz);
    void setToneLevel(float dbv) { _toneLevel = dbvToLinear(dbv); }
    void setToneTransitionTime(unsigned ms) { _toneTransitionMs = ms; }

    /**
     * @brief When enabled, the diagnostic tone is transmitted directly
     * without any other inputs (i.e. no CTCSS, no mixing, etc.)
     */
    void setDiagToneEnabled(bool b) { _diagToneEnabled = b; }
    void setDiagToneFreq(float hz);
    void setDiagToneLevel(float dbv) { _diagToneLevel = dbvToLinear(dbv); }

    static float db(float l) {
        if (l < 0.001)
            return -99.0;
        return 20.0 * log10(l);
    }

    static float vrmsToDbv(float vrms) {
        // Standard conversion from Vrms to Vpp
        return db(vrms * 1.4);
    }

    static float dbvToLinear(float dbv) {
        return pow(10, (dbv / 20));
    }

private:

    const unsigned _id;
    const unsigned _crossCount;

    float _crossGains[MAX_CROSS_COUNT];

    // Noise HPF, runs at 32k
    static const unsigned FILTER_B_LEN = 41;
    static const float32_t FILTER_B[FILTER_B_LEN];
    arm_fir_instance_f32 _filtB;
    float32_t _filtBState[FILTER_B_LEN + BLOCK_SIZE_ADC - 1];
  
    // Decimation LPF (half-band)
    static const unsigned FILTER_C_LEN = 41;
    static const float32_t FILTER_C[FILTER_C_LEN];
    // For the 16k filter, but still runs on 32k audio
    arm_fir_decimate_instance_f32 _filtC;
    float32_t _filtCState[FILTER_C_LEN + BLOCK_SIZE_ADC - 1];
    // For the 8k filter, but still runs on 16k audio
    arm_fir_decimate_instance_f32 _filtD;
    float32_t _filtDState[FILTER_C_LEN + (BLOCK_SIZE_ADC / 2) - 1];

    // Band pass filter (CTCSS removal), runs at 8k
    static const unsigned FILTER_F_LEN = 127;
    static const float32_t FILTER_F[FILTER_F_LEN];
    arm_fir_instance_f32 _filtF;
    float32_t _filtFState[FILTER_F_LEN + BLOCK_SIZE - 1];

    // Low-pass filter for interpolation 8K->32K, runs at 32k
    //static const unsigned FILTER_N_LEN = 127;
    // Needs to be multiple of 4 for interpolation 
    static const unsigned FILTER_N_LEN = 124;
    static const float32_t FILTER_N[FILTER_N_LEN];
    //arm_fir_instance_f32 _filtN;
    //float32_t _filtNState[FILTER_N_LEN + BLOCK_SIZE_ADC - 1];
    arm_fir_interpolate_instance_f32 _filtN;
    float32_t _filtNState[(FILTER_N_LEN / 4) + BLOCK_SIZE_ADC - 1];

    // For capturing various measures of energy on each block
    float _noiseRms;
    float _signalRms;
    float _outRms;

    float _rxGain = 1.0;
    bool _hpfEnabled = true;

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

    // Used for synthesis of tone 
    // This is a fixed level that can be used to set the overall
    // (i.e. after transition) level of the tone
    float _toneLevel =  dbvToLinear(-10);
    float _toneOmega = 0;
    float _tonePhi = 0;
    // Tones turn on/off transitions are smoothed to minimize clicks.
    // This contains the current level (i.e. a value from 0.0->1.0)
    float _toneTransitionLevel = 0;
    // This controls how much the level is increased on each sample.
    float _toneTransitionIncrement = 0;
    // This controls the final target value, generally 0.0 or 1.0 depending
    // on whether we are fading in or fading out.
    float _toneTransitionLimit = 0;
    // Controls how long the transition should last.
    unsigned _toneTransitionMs = 20;

    // Diagnostic tone stuff
    bool _diagToneEnabled = false;
    float _diagToneLevel = dbvToLinear(-10);
    float _diagToneOmega = 0;
    float _diagTonePhi = 0;
};

}

#endif
