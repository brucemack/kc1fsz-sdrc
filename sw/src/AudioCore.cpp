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
#include "AudioCore.h"

#include <iostream>
#include <cstring>
#include <fstream>
#include <cmath>

using namespace std;

static float db(float l) {
    return 20.0 * log10(l);
}

static float PI = 3.14159265;

namespace kc1fsz {

// HPF 41 taps, [0, 4000/32000]:0, [6000/32000, 0.5]:1.0
const float AudioCore::FILTER_B[] = 
{
0.009496502349662752, 0.032168266001826, -0.004020017447607337, -0.029774359071379836, -0.03025119554604127, 0.02111609845361212, 0.07216728736619965, 0.038324850322965634, -0.10997562615757675, -0.292262898960302, 0.6251660988707263, -0.292262898960302, -0.10997562615757675, 0.038324850322965634, 0.07216728736619965, 0.02111609845361212, -0.03025119554604127, -0.029774359071379836, -0.004020017447607337, 0.032168266001826, 0.009496502349662752
}
/*
{
    0.01412168806807286, -0.021791106217694617, -0.030550511146843623, -0.020518460655753096, 0.015082480274916685, 0.05305126976974765, 0.05114811519563668, -0.01919362874777388, -0.14331997832590626, -0.2633164541826926, 0.6870364118682479, -0.2633164541826926, -0.14331997832590626, -0.01919362874777388, 0.05114811519563668, 0.05305126976974765, 0.015082480274916685, -0.020518460655753096, -0.030550511146843623, -0.021791106217694617, 0.01412168806807286
}
*/
/*
{ 
    0.013698037426030683, -0.02113737303116375, -0.0296339958124383, -0.019902906836080543, 0.014630005866669179, 0.05145973167665524, 0.04961367173976759, -0.018617819885340652, -0.13902037897612904, -0.2554169605572119, 0.6964253195122007, -0.2554169605572119, -0.13902037897612904, -0.018617819885340652, 0.04961367173976759, 0.05145973167665524, 0.014630005866669179, -0.019902906836080543, -0.0296339958124383, -0.02113737303116375, 0.013698037426030683 
}
*/
;

// Half-band LPF 
const float AudioCore::FILTER_C[] =
{
0, -0.0022612636393077577, 0, 0.003657523706990156, 0, -0.006253237573582923, 0, 0.010223415066636696, 0, -0.015918543076970815, 0, 0.02405816723332713, 0, -0.03626191327686043, 0, 0.05685837837449928, 0, -0.10193071949733788, 0, 0.3169038896556724, 0.5, 0.3169038896556724, 0, -0.10193071949733788, 0, 0.05685837837449928, 0, -0.03626191327686043, 0, 0.02405816723332713, 0, -0.015918543076970815, 0, 0.010223415066636696, 0, -0.006253237573582923, 0, 0.003657523706990156, 0, -0.0022612636393077577, 0
};    

const float AudioCore::FILTER_F[] =
{
-0.00148000309963449, -0.0005269741357922974, -0.0005685671070370724, -0.0005711749829735852, -0.0005266916725728142, -0.00042913627308751234, -0.0002755717663130774, -6.669740116989601e-05, 0.00019289529930185335, 0.0004941557060915148, 0.0008232593800761903, 0.0011624032641297407, 0.001489723786596802, 0.0017811735068456627, 0.0020109214330739852, 0.0021540659927329407, 0.0021879021452668344, 0.002093326851899909, 0.0018571203462031307, 0.0014728586325032935, 0.0009427185177111338, 0.00027848441890266826, -0.0004971585694644127, -0.001351364271152696, -0.002243016335197132, -0.003123981930466243, -0.003939575261888304, -0.004629849806552187, -0.005136281334100473, -0.005410089999381532, -0.005397451726151065, -0.005066687724484985, -0.004393914824044533, -0.003374842955357053, -0.0020238573686101767, -0.0003761987668511685, 0.0015116002875365363, 0.0035626724415071653, 0.0056816769290264, 0.007757243155979987, 0.009666942303341823, 0.011280756622100796, 0.012468863095060578, 0.013105561896983975, 0.013077176347114467, 0.012287917930839233, 0.010664760904990755, 0.008163304148203593, 0.004771830172095333, 0.0005137604348397658, -0.004551985044294312, -0.01033095443097309, -0.016694548763785236, -0.023484203379860298, -0.03051728622988254, -0.037593632540821084, -0.04450143345882363, -0.05102617591830403, -0.05696183922773863, -0.06211400140976345, -0.06631327201855662, -0.06941849364373319, -0.07132493713929951, 0.9280322559969071, -0.07132493713929951, -0.06941849364373319, -0.06631327201855662, -0.06211400140976345, -0.05696183922773863, -0.05102617591830403, -0.04450143345882363, -0.037593632540821084, -0.03051728622988254, -0.023484203379860298, -0.016694548763785236, -0.01033095443097309, -0.004551985044294312, 0.0005137604348397658, 0.004771830172095333, 0.008163304148203593, 0.010664760904990755, 0.012287917930839233, 0.013077176347114467, 0.013105561896983975, 0.012468863095060578, 0.011280756622100796, 0.009666942303341823, 0.007757243155979987, 0.0056816769290264, 0.0035626724415071653, 0.0015116002875365363, -0.0003761987668511685, -0.0020238573686101767, -0.003374842955357053, -0.004393914824044533, -0.005066687724484985, -0.005397451726151065, -0.005410089999381532, -0.005136281334100473, -0.004629849806552187, -0.003939575261888304, -0.003123981930466243, -0.002243016335197132, -0.001351364271152696, -0.0004971585694644127, 0.00027848441890266826, 0.0009427185177111338, 0.0014728586325032935, 0.0018571203462031307, 0.002093326851899909, 0.0021879021452668344, 0.0021540659927329407, 0.0020109214330739852, 0.0017811735068456627, 0.001489723786596802, 0.0011624032641297407, 0.0008232593800761903, 0.0004941557060915148, 0.00019289529930185335, -6.669740116989601e-05, -0.0002755717663130774, -0.00042913627308751234, -0.0005266916725728142, -0.0005711749829735852, -0.0005685671070370724, -0.0005269741357922974, -0.00148000309963449
};

const float AudioCore::FILTER_N[] =
{
-0.001982534158040627, 0.001296379225957984, 0.0012939078662503082, 0.001376510924297798, 0.0013968186928422892, 0.0012480721545734296, 0.0008800216715366647, 0.00030426721961881813, -0.0004024394043320008, -0.0011136850206245256, -0.001675513135490227, -0.0019434746619823929, -0.001814696822883365, -0.0012610574278670535, -0.0003444215697879109, 0.0007801426854268337, 0.001892938321353142, 0.0027438855276097117, 0.003109694178350442, 0.0028443040507239907, 0.001922717579402261, 0.0004614795576284417, -0.0012878895860192869, -0.0029803070444824552, -0.004238324407913041, -0.004734140148539243, -0.004265197499592371, -0.0028150474572516504, -0.0005787159006973155, 0.0020541392435142677, 0.004561698475773093, 0.0063871495326140474, 0.007054086354219865, 0.006279682528275528, 0.004056395231688754, 0.0006896069110307657, -0.003240862978928012, -0.006951255288381282, -0.009629788140398134, -0.010565495097337133, -0.009333139035536052, -0.005925992612448506, -0.0007843498147293103, 0.005223603470014965, 0.010936156689657426, 0.01508938806936964, 0.016571876656639412, 0.014661602074320965, 0.009233638479219416, 0.000860012129656809, -0.009193261941836086, -0.01911275399706565, -0.026776781992391502, -0.030102246253995393, -0.0273941831487124, -0.017673062018678257, -0.0009078337587586618, 0.021891942126258085, 0.04874432951146835, 0.07692043188583145, 0.1033047055481802, 0.12482383586152615, 0.13889094803946472, 0.14378128745795307, 0.13889094803946472, 0.12482383586152615, 0.1033047055481802, 0.07692043188583145, 0.04874432951146835, 0.021891942126258085, -0.0009078337587586618, -0.017673062018678257, -0.0273941831487124, -0.030102246253995393, -0.026776781992391502, -0.01911275399706565, -0.009193261941836086, 0.000860012129656809, 0.009233638479219416, 0.014661602074320965, 0.016571876656639412, 0.01508938806936964, 0.010936156689657426, 0.005223603470014965, -0.0007843498147293103, -0.005925992612448506, -0.009333139035536052, -0.010565495097337133, -0.009629788140398134, -0.006951255288381282, -0.003240862978928012, 0.0006896069110307657, 0.004056395231688754, 0.006279682528275528, 0.007054086354219865, 0.0063871495326140474, 0.004561698475773093, 0.0020541392435142677, -0.0005787159006973155, -0.0028150474572516504, -0.004265197499592371, -0.004734140148539243, -0.004238324407913041, -0.0029803070444824552, -0.0012878895860192869, 0.0004614795576284417, 0.001922717579402261, 0.0028443040507239907, 0.003109694178350442, 0.0027438855276097117, 0.001892938321353142, 0.0007801426854268337, -0.0003444215697879109, -0.0012610574278670535, -0.001814696822883365, -0.0019434746619823929, -0.001675513135490227, -0.0011136850206245256, -0.0004024394043320008, 0.00030426721961881813, 0.0008800216715366647, 0.0012480721545734296, 0.0013968186928422892, 0.001376510924297798, 0.0012939078662503082, 0.001296379225957984, -0.001982534158040627    
};

static unsigned incAndWrap(unsigned i, unsigned len) {
    if (i == len - 1) 
        return 0;
    else 
        return i + 1;
}

static unsigned decAndWrap(unsigned i, unsigned len) {
    if (i == 0) 
        return len - 1;
    else 
        return i - 1;
}

AudioCore::AudioCore(unsigned id)
:   _id(id) {
    for (unsigned i = 0; i < BLOCK_SIZE_ADC; i++)
        _hist32k[i] = 0;
    for (unsigned i = 0; i < BLOCK_SIZE_ADC / 2; i++)
        _hist16k[i] = 0;
    for (unsigned i = 0; i < BLOCK_SIZE_ADC / 4; i++)
        _hist8k[i] = 0;
    for (unsigned i = 0; i < HISTB_32K_LEN; i++)
        _histB32k[i] = 0;
}

void AudioCore::cycle0(const float* adc_in, float* cross_out) {

    // Work on the 32kHz samples
    for (unsigned i = 0; i < BLOCK_SIZE_ADC; i++) {
        
        // Build the history the the FIR filters will use
        _hist32k[i] = adc_in[i];
        
        // HPF for noise detection
        {
            // Iterate backwards through history and forward
            // through the filter coefficients
            unsigned k = i;
            float a = 0;
            for (unsigned j = 0; j < FILTER_B_LEN; j++) {
                a += (_hist32k[k] * FILTER_B[j]);
                k = decAndWrap(k, BLOCK_SIZE_ADC);
            }
            _filtOutB[i] = a;
        }
        
        // Decimation LPF down to 16kHz. We only do this
        // on half of the input samples.
        if (i % 2 == 0) {
            // Iterate backwards through history and forward
            // through the filter coefficients.
            // TODO: LEVERAGE THE SYMMETRY OF THE COEFFICIENTS
            // TO ELIMINATE SOME MULTIPLICATIONS.
            unsigned k = i;
            float a = 0;
            for (unsigned j = 0; j < FILTER_C_LEN; j++) {
                // Many of the coefficients will be zero
                if (FILTER_C[j] != 0)
                    a += (_hist32k[k] * FILTER_C[j]);
                k = decAndWrap(k, BLOCK_SIZE_ADC);
            }
            // NOTE: Packing into half of the space
            _filtOutC[i / 2] = a;
        }
    }

    // Work on the 16kHz samples
    for (unsigned i = 0; i < BLOCK_SIZE_ADC / 2; i++) {
        
        // Build the history that the FIR filters will use
        _hist16k[i] = _filtOutC[i];
        
        // Decimation LPF down to 8kHz. We only do this
        // on half of the input samples.
        if (i % 2 == 0) {
            // Iterate backwards through history and forward
            // through the filter coefficients.
            // TODO: LEVERAGE THE SYMMETRY OF THE COEFFICIENTS
            // TO ELIMINATE SOME MULTIPLICATIONS.
            unsigned k = i;
            float a = 0;
            for (unsigned j = 0; j < FILTER_C_LEN; j++) {
                // Many of the coefficients will be zero
                if (FILTER_C[j] != 0)
                    a += (_hist16k[k] * FILTER_C[j]);
                k = decAndWrap(k, BLOCK_SIZE_ADC / 2);
            }
            // NOTE: Packing into half of the space
            _filtOutD[i / 2] = a;
        }
    }
    
    // Work on the 8kHz samples
    float ctcssTotal = 0;
    
    for (unsigned i = 0; i < BLOCK_SIZE; i++) {
        
        float s = _filtOutD[i];

        // Build the history that the FIR filters will use
        _hist8k[_hist8KPtr] = s;

        // BPF
        {
            // Iterate backwards through history and forward
            // through the filter coefficients
            unsigned k = _hist8KPtr;
            float a = 0;
            for (unsigned j = 0; j < FILTER_F_LEN; j++) {
                a += (_hist8k[k] * FILTER_F[j]);
                k = decAndWrap(k, HIST_8K_LEN);
            }

            // Put the final filtered sample into the 
            // delay area.
            _delayArea[_delayAreaWritePtr] = a;
            _delayAreaWritePtr = incAndWrap(_delayAreaWritePtr,
                _delayAreaLen);

            _hist8KPtr = incAndWrap(_hist8KPtr, HIST_8K_LEN);
        }

        // Move a sample from the delay area into the 
        // output
        if (_delayCountdown) {
            cross_out[i] = 0;
            _delayCountdown--;
        }
        else { 
            cross_out[i] = _delayArea[_delayAreaReadPtr];
        }

        _delayAreaReadPtr = incAndWrap(_delayAreaReadPtr,
            _delayAreaLen);

        // CTCSS decode
        float z0 = s + _gc * _gz1 - _gz2;
        _gz2 = _gz1;
        _gz1 = z0;

        // DTMF decode   
    }

    // Look to see if we can update the CTCSS estimation
    if (++_ctcssBlock == _ctcssBlocks) {
        float gi = _gcw * _gz1 - _gz2;
        float gq = _gsw * _gz1;
        _ctcssMag = sqrt(gi * gi + gq * gq);
        // Scale down by half of the sample count
        _ctcssMag /= (float)(_ctcssBlocks * BLOCK_SIZE / 2.0);
        // Reset for the next block
        _gz1 = 0;
        _gz2 = 0;
        _ctcssBlock = 0;
    }

    // Compute noise RMS
    {
        float a = 0;
        for (unsigned i = 0; i < BLOCK_SIZE_ADC; i++)
            a += (_filtOutB[i] * _filtOutB[i]);
        a /= (float)BLOCK_SIZE_ADC;
        _noiseRms = sqrt(a);
    }

    // Compute the signal RMS
    {
        float a = 0;
        for (unsigned i = 0; i < BLOCK_SIZE_ADC / 4; i++)
            a += (_filtOutD[i] * _filtOutD[i]);
        a /= (float)(BLOCK_SIZE_ADC / 4);
        _signalRms = sqrt(a);
    }
}

float AudioCore::getSnr() const {
    return db(getSignalRms() / getNoiseRms());
}

void AudioCore::cycle1(const float** cross_in, float* dac_out) {

    float mix[BLOCK_SIZE];

    // CTCSS encoder [see flow diagram reference J] 
    // Notice that all of the calculations needed to 
    // generate the CTCSS tone are performed regardless 
    // of whether the encoding is enabled.  This is to 
    // maintain a consistent CPU cost.
    float level = _ctcssEncodeEnabled ? _ctcssEncodeLevel : 0;
    for (unsigned i = 0; i < BLOCK_SIZE; 
        i++, _ctcssEncodePhi += _ctcssEncodeOmega)
        mix[i] = level * cos(_ctcssEncodePhi);

    // We do this to avoid phi growing very large and 
    // creating overflow/precision problems.
    _ctcssEncodePhi = fmod(_ctcssEncodePhi, 2.0 * PI);

    // Other tones/voice

    // Transmit Mix [float diagram reference L]
    for (unsigned i = 0; i < BLOCK_SIZE; i++)
        mix[i] += cross_in[0][i];
    
    // LPF 2.3kHz [float diagram reference M]
    // (Not used at this time)

    // Interpolation x4 [flow diagram reference N]   
    for (unsigned i = 0; i < BLOCK_SIZE_ADC; i++) {
        
        // Pad the 8K samples with zeros
        float s;
        if (i % 4 == 0) 
            s = mix[i / 4];
        else 
            s = 0;
        
        // Apply the LPF filter
        {
            _histB32k[_histB32kPtr] = s;
            unsigned k = _histB32kPtr;
            _histB32kPtr = incAndWrap(_histB32kPtr, HISTB_32K_LEN);
            float a = 0;
            for (unsigned j = 0; j < FILTER_N_LEN; j++) {
                // Remember, there are lots of zeros
                if (_histB32k[k] != 0)
                    a += (_histB32k[k] * FILTER_N[j]);
                k = decAndWrap(k, HISTB_32K_LEN);
            }
            // Keep in mind the gain of the filter
            dac_out[i] = a * 4.0;
        }
    }
}

void AudioCore::setCtcssDecodeFreq(float hz) {
    _ctcssDecodeFreq = hz;
    _ctcssBlocks = 8;
    _ctcssBlock = 0;
    float gw =  2.0 * PI * hz / (float)FS;
    _gcw = cos(gw);
    _gsw = sin(gw);
    _gc = 2.0 * _gcw;
    _gz1 = 0;
    _gz2 = 0;
}

void AudioCore::setCtcssEncodeEnabled(bool b) {
    _ctcssEncodeEnabled = b;
}

void AudioCore::setCtcssEncodeFreq(float hz) {
    _ctcssEncodeFreq = hz;
    // Convert frequency to radians/sample.  The CTCSS
    // generation happens at the FS (8k) rate.
    _ctcssEncodeOmega = 2.0 * PI * hz / (float)FS;
    _ctcssEncodePhi = 0;
}

void AudioCore::setCtcssEncodeLevel(float db) {
    // Convert DB to linear level
    _ctcssEncodeLevel = powf(10.0, (db / 20.0));
    cout << "CTCSS Level " << _ctcssEncodeLevel << endl;
}

void AudioCore::setDelayMs(unsigned ms) {
    _delaySamples = FS * ms / 1000;
    // Cap out delay length
    if (_delaySamples > _delayAreaLen) 
        _delaySamples = _delayAreaLen;
    if (_delayAreaReadPtr >= _delaySamples)
        _delayAreaReadPtr -= _delaySamples;
    else {
        unsigned extra = _delaySamples - _delayAreaReadPtr;
        _delayAreaReadPtr = _delayAreaLen - 1 - extra;
    }
}

}

