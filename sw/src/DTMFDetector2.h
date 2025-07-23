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
#ifndef _DTMFDetector2_h
#define _DTMFDetector2_h

#include <cstdint>

namespace kc1fsz {

/**
 * An instance of this class is needed because there is some state 
 * involved in capturing and de-bouncing the DTMF detection.
 *
 * NOTE: This only works for 8K sample rates at the moment.
 */
class DTMFDetector2 {
public:

    /**
     * @param block 256 input samples in 32-bit signed PCM format.
     */
    void processBlock(const int32_t* block);

private:

// This is 2 * cos(2 * PI * fk / fs) for each of the 8 frequencies
    static int32_t coeffRow[4];
    // This is 2 * cos(2 * PI * fk / fs) for each of the 8 frequencies
    static int32_t coeffCol[4];
    // This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the 8 frequencies.
    // Used for checking second-order harmonics.
    static int32_t harmonicCoeffRow[4];
    // This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the 8 frequencies.
    // Used for checking second-order harmonics.
    static int32_t harmonicCoeffCol[4];
    static char symbolGrid[4 * 4];

    static const unsigned FS = 8000;
    static const unsigned N = 256;

    // VSC history
    static const uint32_t _vscHistSize = 8;
    char _vscHist[_vscHistSize];
    // Indicates whether we are currently in the middle of a valid
    // detection.
    bool _inDSC = false;
    char _detectedSymbol = 0;
};

}

#endif
