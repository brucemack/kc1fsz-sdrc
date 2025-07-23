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
     * The "noise" is basically all power above ~5kHz.
     * @returns Signal voltage in Vrms, assuming full-scale is 1.0. Note
     */
    float getNoiseRms() const { return _noiseRms; }

    /**
     * The "signal" is basically all power below 4kHz, includes the sub-audible 
     * CTCSS tones.
     *
     * @returns Signal voltage in Vrms, assuming full-scale is 1.0. Note
     */
    float getSignalRms() const { return _signalRms; }
    float getSignalPeak() const { return _signalPeak; }    

    /**
     * A version of the signal RMS smoothed over some period (around 64ms)
     */
    float getSignalRmsAvgMoving() const { return _signalRmsAvgMoving; }

    /**
     * These versions of the RMS/peak functions include smoothing that is 
     * compatible with VU meter ballistics.
     */
    float getSignalRms2() const { return _signalRmsAvg; }
    float getSignalPeak2() const { return _signalPeakAvg; }

    /**
     * Voltage of all audio being routed to the transmitter, inclusive of 
     * all sources/tones/etc.
     *
     * @returns Signal voltage in Vrms, assuming full-scale is 1.0. Note
     */
    float getOutRms() const { return _outRms; }    
    float getOutPeak() const { return _outPeak; }    

    /**
     * These versions of the RMS/peak functions include smoothing that is 
     * compatible with VU meter ballistics.
     */
    float getOutRms2() const { return _outRmsAvg; }
    float getOutPeak2() const { return _outPeakAvg; }

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

    void setCtcssEncodeLevel(float dbv) { _ctcssEncodeLevel = dbvToPeak(dbv); }

    void setRxDelayMs(unsigned ms);

    void resetDelay() { _delayCountdown = _delaySamples; }

    // TODO: SOFT GAINS ON RX AND TX
    
    void setToneEnabled(bool b);
    void setToneFreq(float hz);
    void setToneLevel(float dbv) { _toneLevel = dbvToPeak(dbv); }
    void setToneTransitionTime(unsigned ms) { _toneTransitionMs = ms; }

    void setAgcEnabled(bool e) { _agcEnabled = e; }
    void setAgcTargetDbv(float dbv) { _agcTargetRms = dbvToVrms(dbv); }
    float getAgcGain () const { return _agcGain; }

    static float db(float l) {
        if (l < 0.001)
            return -99.0;
        return 20.0 * log10(l);
    }

    /**
     * Example for sanity: 0 dBv is 1 Vpp, which is 0.5 Vp, which is 0.3535 Vrms.
     */
    static float dbvToVrms(float dbv) {
        float vpp = pow(10, (dbv / 20));
        float vp = vpp / 2.0;
        return vp * 0.707;
    }

    /**
     * Example for sanity: (0.3535 Vrms / 0.707) is 0.5 Vp. 
     * 0.5 Vp * 2.0 is 1.0 Vpp. 20 * log10(1.0) is 0 dBv.
     */
    static float vrmsToDbv(float vrms) {
        float vpp = (vrms / 0.707) * 2.0;
        // Standard conversion from Vrms to Vpp
        return db(vpp);
    }

    /**
     * Example for sanity: 0dBv is 0.5 Vp.
     */
    static float dbvToPeak(float dbv) {
        return pow(10, (dbv / 20)) * 0.5;
    }

    /**
     * Example for sanity: 0dB is 1.0
     */
    static float dbToLinear(float dbv) {
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
  
    // Decimation LPFs (two half-band filters)
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
    // Needs to be multiple of 4 for interpolation 
    static const unsigned FILTER_N_LEN = 124;
    static const float32_t FILTER_N[FILTER_N_LEN];
    arm_fir_interpolate_instance_f32 _filtN;
    float32_t _filtNState[(FILTER_N_LEN / 4) + BLOCK_SIZE_ADC - 1];

    // For capturing various measures of energy on each block
    float _noiseRms;
    float _signalRms;
    float _signalPeak;
    float _outRms;
    float _outPeak;

    float _signalRmsAvgAttackCoeff = 0.12;
    float _signalRmsAvgDecayCoeff = 0.12;
    float _signalRmsAvg = 0;    

    float _signalPeakAvgAttackCoeff = 0.50;
    float _signalPeakAvgDecayCoeff = 0.12;
    float _signalPeakAvg = 0;

    float _outRmsAvgAttackCoeff = 0.12;
    float _outRmsAvgDecayCoeff = 0.12;
    float _outRmsAvg = 0;    

    float _outPeakAvgAttackCoeff = 0.50;
    float _outPeakAvgDecayCoeff = 0.12;
    float _outPeakAvg = 0;

    // Signal RMS history used for maintaining an RMS average.
    static const unsigned SIGNAL_RMS_HISTORY_SIZE = 8;
    unsigned _signalRmsHistoryPtr = 0;
    float _signalRmsHistory[SIGNAL_RMS_HISTORY_SIZE];
    float _signalRmsAvgMoving = 0;

    // A soft gain that is applied to received audio just before it
    // it passed into the crossing network.
    float _rxGain = 1.0;

    // AGC related
    bool _agcEnabled = true;
    float _agcTargetRms = dbvToVrms(-10);
    float _agcGain = 1.0;    
    float _agcMaxGain = pow(10.0, (10.0 / 20.0));
    float _agcMinGain = pow(10.0, (-10.0 / 20.0));
    // These parameters control how quickly the AGC gain comes up or down.
    float _agcAttackCoeff = 0.05;
    float _agcDecayCoeff = 0.02;

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
    float _toneLevel =  dbvToPeak(-10);
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

    // Input injection feature for testing
    bool _injectEnabled = false;
    float _injectHz = 800;
    float _injectLevel =  dbvToPeak(-10);
    // Injection runs at CODEC speed
    float _injectOmega = 2.0 * PI * _injectHz / (float)FS_ADC;
    float _injectPhi = 0;
};

}

#endif
