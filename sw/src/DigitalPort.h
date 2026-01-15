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

class DigitalPort : public Activatable {
public:

    static const unsigned FS_ADC = 32000;
    static const unsigned BLOCK_SIZE_ADC = 256;
    static const unsigned FS = FS_ADC / 4;
    static const unsigned BLOCK_SIZE = BLOCK_SIZE_ADC / 4;
    static const unsigned MAX_CROSS_COUNT = 8;

    DigitalPort(unsigned id, unsigned crossCount, Clock& clock);

    /**
     * @brief Called once per CODEC block. Expected to run quickly 
     * inside of the interrupt service routine.
     *
     * @param cross_out One block of audio data at the 8k rate 
     * ready to be shared across the repeater.
     */
    void cycleRx(float* cross_out);

    /**
     * @brief Called once per CODEC block. Expected to run quickly 
     * inside of the interrupt service routine.
     *
     * @param cross_in 8k data from all of the sources. See setCrossGainLinear()
     * for information about how they are mixed.
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
     * Used to stage some audio that will be pulled out in the next call to 
     * cycleRx().
     */
    void setAudio(const uint8_t* audio8KLE, unsigned len);

    /**
     * Used to pull out the last frame of audio that was loaded using the previous
     * cycleTx() call.
     */
    void getAudio(uint8_t* audio8KLE, unsigned len);

    // ------ Activatable -----------------------------------------------------

    bool isActive() const;

private:

    const unsigned _id;
    const unsigned _crossCount;
    Clock& _clock;

    float _crossGains[MAX_CROSS_COUNT];

    bool _extAudioInValid = false;
    uint8_t _extAudioIn[BLOCK_SIZE * 2];
    uint64_t _lastInputUs = 0;

    bool _extAudioOutValid = false;
    uint8_t _extAudioOut[BLOCK_SIZE * 2];
};

}