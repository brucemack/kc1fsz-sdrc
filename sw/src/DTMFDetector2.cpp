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
#include <cmath>
#include "DTMFDetector2.h"

namespace kc1fsz {

// TODO: CONSOLIDATE
int16_t s_abs(int16_t var1);
int16_t mult(int16_t var1, int16_t var2);
int16_t add(int16_t var1, int16_t var2);
int16_t sub(int16_t var1, int16_t var2);
int32_t L_mult(int16_t var1, int16_t var2);
int32_t L_add(int32_t L_var1, int32_t L_var2);
int32_t L_sub(int32_t L_var1, int32_t L_var2);

int16_t div2(int16_t var1, int16_t var2) {
    if (var1 == var2) {
        return 1;
    }
    else if (var1 == -var2) {
        return -1;
    }
    else if ( abs(var2) > abs(var1) ) {
        return (int16_t)(((int32_t)var1 << 15) / ((int32_t)var2));
    } 
    else {
        return 0;
    }
}

// This is 2 * cos(2 * PI * fk / fs) for each of the 8 frequencies
int32_t DTMFDetector2::coeffRow[4] = {
    27980 * 2,
    26956 * 2,
    25701 * 2,
    24219 * 2
 };

 // This is 2 * cos(2 * PI * fk / fs) for each of the 8 frequencies
int32_t DTMFDetector2::coeffCol[4] = {
    19261 * 2,
    16525 * 2,
    13297 * 2,
     9537 * 2
 };

// This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the 8 frequencies.
// Used for checking second-order harmonics.
int32_t DTMFDetector2::harmonicCoeffRow[4] = {
    15014 * 2,
    11583 * 2,
     7549 * 2,
     3032 * 2
 };

 // This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the 8 frequencies.
// Used for checking second-order harmonics.
int32_t DTMFDetector2::harmonicCoeffCol[4] = {
   -10565 * 2,
   -16503 * 2,
   -22318 * 2,
   -27472 * 2
 };

char DTMFDetector2::symbolGrid[4 * 4] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

void DTMFDetector2::processBlock(const int32_t* block) {  

    // Convert to 16-bit PCM and find the largest amplitude
    // in order to normalize.
    int16_t samples[N];
    int16_t maxVal = 0;
    for (unsigned i = 0; i < N; i++) {
        samples[i] = block[i] >> 16;
        int16_t absSample = s_abs(samples[i]);
        if (absSample > maxVal)
            maxVal = absSample;
    }   
    // Apply the normalization
    for (unsigned i = 0; i < N; i++) 
        samples[i] = div2(samples[i], maxVal);

    /*
    char vscSymbol = _detectVSC(samples, N);
    //cout << "VSC Symbol " << (int)vscSymbol << " " << vscSymbol << endl;

    // The VSC->DSC transition requires some history.
    // First "push the stack" to make room for the new symbol
    for (unsigned int i = 1; i < _vscHistSize; i++)
        _vscHist[_vscHistSize - i] = _vscHist[_vscHistSize - i - 1];
    // Record the latest symbol
    _vscHist[0] = vscSymbol;

    // Look at the recent VSC history and decide on the detection status.
    // (See ETSI ES 201 235-3 V1.1.1 (2002-03) section 4.2.2)

    // A valid DSC recognition requires a consistent detection over 40ms 
    if (!_inDSC) {
        if (_vscHist[0] != 0 &&
            _vscHist[0] == _vscHist[1] &&
            _vscHist[0] == _vscHist[2] &&
            _vscHist[0] == _vscHist[3]) {
            _inDSC = true;
            _detectedSymbol = _vscHist[0];
            // Record the symbol on transition
            if (_resultLen < _resultSize) {
                _result[_resultLen++] = _detectedSymbol;
            }
        }
    }
    // A valid DSC cesation requires an interruption of at least 40ms
    else {
        if (_vscHist[0] != _detectedSymbol &&
            _vscHist[1] != _detectedSymbol &&
            _vscHist[2] != _detectedSymbol &&
            _vscHist[3] != _detectedSymbol) {
            _inDSC = false;
        }
    }
    */
}

}
