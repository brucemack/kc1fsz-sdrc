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
#include "IDToneGenerator.h"
#include "AudioCoreOutputPort.h"

namespace kc1fsz {

static const int MorseSymbolStart = 33;
static const int MorseSymbolEnd = 90;

static const char* MorseSymbols[] = {
  "-.-.--", // 33 !
  ".-..-.", // 34 "
  "",       // 35 #
  "...-..-",// 36 $
  "",       // 37 %
  ".-...",  // 38 &
  ".----.", // 39 '
  "-.--.",  // 40 (
  "-.--.-", // 41 )
  "",       // 42 *
  ".-.-.",  // 43 +
  "--..--", // 44 ,
  "-....-", // 45 -
  ".-.-.-", // 46 .
  "-..-.",  // 47 /
  "-----",  // 48 0
  ".----",  // 49 1
  "..---",  // 50 2
  "...--",  // 51 3
  "....-",  // 52 4
  ".....",  // 53 5
  "-....",  // 54 6
  "--...",  // 55 7
  "---..",  // 56 8
  "----.",  // 57 9
  "---...", // 58 :
  "-.-.-.", // 59 ;
  "",       // 60 <
  "-...-",  // 61 =
  "",       // 62 >
  "..--..", // 63 ?
  ".--.-.", // 64 @
  ".-",     // 65 A
  "-...",   // 66 B
  "-.-.",   // 67 C
  "-..",    // 68 D
  ".",      // 69 E
  "..-.",   // 70 F
  "--.",    // 71 G
  "....",   // 72 H
  "..",     // 73 I
  ".---",   // 74 J
  "-.-",    // 75 K
  ".-..",   // 76 L
  "--",     // 77 M
  "-.",     // 78 N
  "---",    // 79 O
  ".--.",   // 80 P
  "--.-",   // 81 Q
  ".-.",    // 82 R
  "...",    // 83 S
  "-",      // 84 T
  "..-",    // 85 U
  "...-",   // 86 V
  ".--",    // 87 W
  "-..-",   // 88 X
  "-.--",   // 89 Y
  "--..",   // 90 Z
};

const float freq = 600;
const unsigned int dotMs = 50;

IDToneGenerator::IDToneGenerator(Log& log, Clock& clock, AudioCoreOutputPort& core) 
:   _log(log),
    _clock(clock),
    _core(core)
{
    _callSign[0] = 0;
}

void IDToneGenerator::run() {
    if (_running) {
        if (_clock.isPast(_endTime)) {
            // Start of call letter
            if (_state == 0) {
                if (_callSign[_callPtr] == 0) {
                    _running = false;
                    _log.info("CWID end");
                    _core.setToneEnabled(false);
                }
                else {
                    char c = toupper(_callSign[_callPtr]);
                    //_log.info("Working on %c", c);
                    if (c == ' ') {
                        // Schedule a seven dot pause between letters
                        _endTime = _clock.time() + dotMs * 7;
                        _callPtr++;
                    }
                    else {
                       // Look to see if this is a valid Morse
                       if (c >= MorseSymbolStart && c <= MorseSymbolEnd) {
                            _symPtr = 0;
                            _state = 1;
                        }
                        else {
                            // Skip the letter entirely
                            _callPtr++;
                        }
                    }
                }
            }
            // Start of symbol
            else if (_state == 1) {
                char c = toupper(_callSign[_callPtr]);
                int i = c - MorseSymbolStart;
                // Look for the end of the call sign letter
                if (MorseSymbols[i][_symPtr] == 0) {
                    // Increment to the next call sign letter
                    _callPtr++;
                    _state = 0;
                    // Schedule a three dot pause between letters
                    _endTime = _clock.time() + dotMs * 3;
                }
                else if (MorseSymbols[i][_symPtr] == '.') {
                    //_log.info("Start .");
                    _endTime = _clock.time() + dotMs;
                    _core.setToneFreq(freq);
                    _core.setToneLevel(_level);
                    _state = 2;
                }
                else if (MorseSymbols[i][_symPtr] == '-') {
                    _endTime = _clock.time() + dotMs * 3;
                    _core.setToneFreq(freq);
                    _core.setToneLevel(_level);
                    _state = 2;
                }
                else {
                    // A space or other invaid character
                    _endTime = _clock.time() + dotMs * 3;
                    // Stay quiet
                     _state = 2;
                }
            }
            // Symbol is finished transmitting
            else if (_state == 2) {
                // Set the level to silent
                _core.setToneLevel(-96);
                // Go quiet for a dot pause
                _endTime = _clock.time() + dotMs;
                _state = 3;
            }
            // Post-symbol pause is finished
            else if (_state == 3) {
                _symPtr++;
                _state = 1;
            }
        }
    }
}

void IDToneGenerator::start() {
    _running = true;
    _state = 0;
    _callPtr = 0;
    _symPtr = 0;
    // This is set in the past so that we immediately start working on the first
    // symbol of the ID.
    _endTime = 0;
    _core.setToneEnabled(true);
    _log.info("CWID start %s", _callSign);
}

bool IDToneGenerator::isFinished() {
    return !_running;
}

void IDToneGenerator::setCall(const char* callSign) {
    strcpyLimited(_callSign, callSign, _maxCallSignLen);
}

}
