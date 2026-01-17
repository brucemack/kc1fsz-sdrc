/**
 * Digital Repeater Controller
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
 */
#pragma once
#include <cstdint>

#include "Activatable.h"

namespace kc1fsz {

class Clock;

/**
 * This class makes the network audio look like a normal part of the 
 * audio core.
 *
 * Network audio frames are 160 PCM16 samples (every 20ms). The SDR
 * audio frames are 64 PCM16 samples (every 8ms).
 */
class DigitalAudioPort : public Activatable {
public:

    static const unsigned FS_ADC = 32000;
    static const unsigned BLOCK_SIZE_ADC = 256;
    static const unsigned FS = FS_ADC / 4;
    static const unsigned BLOCK_SIZE = BLOCK_SIZE_ADC / 4;
    static const unsigned MAX_CROSS_COUNT = 8;
    // Size in bytes (16 bit PCM)
    static const unsigned NETWORK_FRAME_SIZE = 160 * 2;

    DigitalAudioPort(unsigned id, unsigned crossCount, Clock& clock);

    /**
     * @brief Called once per CODEC block. Expected to run quickly 
     * inside of the interrupt service routine.
     *
     * @param cross_out One 8ms block of audio data at the 8k rate 
     * ready to be shared across the repeater.
     */
    void cycleRx(float* cross_out);

    /**
     * @brief Called once per CODEC block. Expected to run quickly 
     * inside of the interrupt service routine.
     *
     * @param cross_in 8ms of 8k data from all of the sources. See 
     * setCrossGainLinear() for information about how they are mixed.
     */
    void cycleTx(const float** cross_ins);

    /**
     * Controls how much of each cross input gets included in the output
     * during calls to cycleTx.
     * 
     * @param i The contributor index, corresponds to the cross_ins array
     * index in the cycleTx() function.
     * @param gain 0->1 linear scale.
     */
    void setCrossGainLinear(unsigned i, float gain);

    /**
     * Used to stage 20ms of audio that will be pulled out in the next call to 
     * cycleRx().
     * 
     * @param len For sanity check, must be 160 * 2.
     */
    void loadNetworkAudio(const uint8_t* audio8KLE, unsigned len);

    /**
     * @returns true If there is any network audio waiting to be 
     * extracted.
     */
    bool isNetworkAudioPending() const;

    /**
     * Used to pull out the next 20ms frame of audio that was loaded using the 
     * previous cycleTx() call.
     * 
     * @param len For sanity check, must be 160 * 2.
     */
    void extractNetworkAudio(uint8_t* audio8KLE, unsigned len);

    // ------ Activatable -----------------------------------------------------

    bool isActive() const;

private:

    const unsigned _id;
    const unsigned _crossCount;
    Clock& _clock;

    float _crossGains[MAX_CROSS_COUNT];

    // Circular buffer for inbound data (from network)
    uint8_t _extAudioIn[NETWORK_FRAME_SIZE * 2];
    const unsigned _extAudioInCapacity = NETWORK_FRAME_SIZE * 2;
    unsigned _extAudioInRd = 0;
    unsigned _extAudioInWr = 0;
    unsigned _extAudioInLen = 0;
    // This is used to trigger the playout of audio received from 
    // the network. There is a bit of hysteresis built into the system
    // to avoid subtle timing issues.
    bool _extAudioInTriggered = false;
    // The last time audio was received off the network. Used to control
    // the isActive() indicator.
    uint64_t _lastInputUs = 0;

    // Circular buffer for outbound data (to network)
    uint8_t _extAudioOut[NETWORK_FRAME_SIZE * 2];
    const unsigned _extAudioOutCapacity = NETWORK_FRAME_SIZE * 2;
    unsigned _extAudioOutRd = 0;
    unsigned _extAudioOutWr = 0;
};

}