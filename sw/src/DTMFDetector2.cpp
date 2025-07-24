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
    // in order to normalize the entire block of samples.
    int16_t samples[N];
    int16_t maxVal = 0;
    for (unsigned i = 0; i < N; i++) {
        samples[i] = block[i] >> 16;
        maxVal = std::max(s_abs(samples[i]), maxVal);
    }   
    // Apply the normalization
    for (unsigned i = 0; i < N; i++) 
        samples[i] = div2(samples[i], maxVal);

    char vscSymbol = _detectVSC(samples, N);
    //cout << "VSC Symbol " << (int)vscSymbol << " " << vscSymbol << endl;

    // The VSC->DSC transition requires some history.
    //
    // Look at the recent VSC history and decide on the detection status.
    // (See ETSI ES 201 235-3 V1.1.1 (2002-03) section 4.2.2)
    //
    // * Timing requirements are as follows.
    //   - A symbol must be transmitted for at least 40ms. Symbols shorter 
    //     than 23ms must be rejected.
    //   - The gap between symbols must be at least 40ms.
    //
    // If we're not in a valid symbol then look for 
    
}

/**
 * A classic implementation of the Goertzel algorithm in fixed point.
 * 
 * @param coeff This is what controls the frequency that we are filtering
 * for. Notice that the coefficient is 32-bits and starts off 32,767 
 * higher in magnitude than the samples.
 */
static int16_t computePower(int16_t* samples, uint32_t N, int32_t coeff) {

    int32_t vk_1 = 0, vk_2 = 0;

    // This is the amount that we down-shift the sample in order to 
    // preserve precision as we go through the Goertzel iterations.
    // This number very much depends on N, so pay close attention 
    // if N changes.
    const int sampleShift = 7;

    for (unsigned i = 0; i < N; i++) {        
        int16_t sample = samples[i];
        // Take out a factor to avoid overflow later
        sample >>= sampleShift;
        // This has an extra factor of 32767 in it
        int32_t c = coeff;
        // Remove the extra shift introduced by the multiplication, but we
        // are still high by 32767.
        int32_t r = (c * vk_1) >> 15;
        r -= vk_2;
        r += sample;
        vk_2 = vk_1;
        vk_1 = r;
    }

    // At this point all numbers have an extra factor of 32767 because of the 
    // initial coefficient scaling.

    // This has an extra factor of 32767 in it
    int32_t c = coeff;
    // This will be shifted 32767 * 32767 high 
    int32_t r = (vk_1 * vk_1);
    // This will be shifted 32767 * 32767 high
    r = r + (vk_2 * vk_2);
    // This will be shifted 32767 * 32767 high 
    int32_t m = (c * vk_1);
    // This will be shifted (32767 / 32767) * 32767 * 32767 high
    r = r - (((m >> 15) * vk_2));
    // Remove the extra 32767 (squared, because this is power)
    // Re-introduce the factor (squared, because this is power)
    r >>= (15 + 15 - (sampleShift + sampleShift));
    return (int16_t)r;
}

char DTMFDetector2::_detectVSC(int16_t* samples, uint32_t N) {

    // Compute the power on the fundamental frequencies across rows
    // and columns.
    bool nonZeroFound = false;
    int16_t powerRow[4], powerCol[4];
    for (unsigned k = 0; k < 4; k++) {
        powerRow[k] = computePower(samples, N, coeffRow[k]);
        powerCol[k] = computePower(samples, N, coeffCol[k]);
        if (powerRow[k] > 0 || powerCol[k] > 0)
            nonZeroFound = true;
    }

    // This could happen in the case where a DC signal is sent in
    if (!nonZeroFound)
        return 0;

    // Find the maximum of the **combined** powers
    unsigned maxRow = 0, maxCol = 0;
    int32_t maxRowPower = 0, maxColPower = 0, maxCombPower = 0;
    for (unsigned r = 0; r < 4; r++) {
        int16_t rowPower = powerRow[r];
        for (unsigned c = 0; c < 4; c++) {
            int16_t colPower = powerCol[c];
            int32_t combPower = rowPower + colPower;
            if (combPower > maxCombPower) {
                maxCombPower = combPower;
                maxRow = r;
                maxRowPower = rowPower;
                maxCol = c;
                maxColPower = colPower;
            }
        }
    }

    // Now see if any pair comes close to the maximum
    for (unsigned r = 0; r < 4; r++) {
        int16_t rowPower = powerRow[r];
        for (unsigned c = 0; c < 4; c++) {
            int16_t colPower = powerRow[c];
            int32_t combPower = rowPower + colPower;
            // If the power advantage of the first place is less than 10x
            // of the second place then the symbol is not valid.
            if (r != maxRow && 
                c != maxCol && 
                combPower != 0 && 
                (maxCombPower / combPower) < 10) {
                return 0;
            }
        }
    }

    // Now check the "twist" between the two.  

    // Make sure that the column (high group) power is not >4dB the row (low 
    // group) power. We are shifting the numerator by 4 to allow more accurate 
    // comparison in fixed point.
    //
    // The +4dB level is equivalent to x ~1.58 linear. Testing a/b > ~1.58 is 
    // the same as * testing 4a/b > ~6.
    if ((4 * maxColPower) / maxRowPower > 6) {
        return 0;
    }
    
    // Make sure that the row (low group) power is not >8dB the column (high
    // group) power. We are shifting the numerator by 4 to allow more accurate 
    // comparison in fixed point.
    // TEMP: EXPANDING THE RANGE TO 12dB BECAUSE OF PROBLEMS ON THE HIGH END WITH
    // THE FT-65
    if ((4 * maxRowPower) / maxColPower > 10) {
        //cout << "Twist failed: row too high " << maxRowPower << " " << maxColPower << endl;
        return 0;
    }

    // If we're still alive here then compute the harmonic for the row and column
    int16_t maxRowHarmonicPower = computePower(samples, N, harmonicCoeffRow[maxRow]);
    int16_t maxColHarmonicPower = computePower(samples, N, harmonicCoeffCol[ maxCol]);

    // Make sure the harmonics are 20dB down from the fundamentals
    if (maxRowHarmonicPower != 0 && maxRowPower / maxRowHarmonicPower < 10) {
        return 0;
    }
    if (maxColHarmonicPower != 0 && maxColPower / maxColHarmonicPower < 10) {
        return 0;
    }

    // Made it to a valid symbol!
    return symbolGrid[4 * maxRow + maxCol];
}

}
