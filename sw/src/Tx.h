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

#include "kc1fsz-tools/Runnable.h"

#include "CourtesyToneGenerator.h"

namespace kc1fsz {

class Tx : public Runnable {
public:

    virtual void run() = 0;

    virtual int getId() const = 0;
    virtual void setEnabled(bool en) = 0;
    virtual bool getEnabled() const = 0;
    virtual void setPtt(bool ptt) = 0;
    virtual bool getPtt() const = 0;
    
    // ----- CONFIGURATION ---------------------------------------------------

    enum PLToneMode { NONE, SOFT };

    virtual void setPLToneMode(PLToneMode mode) = 0;
    virtual void setPLToneFreq(float hz) = 0;
    virtual void setPLToneLevel(float db) = 0;
    virtual CourtesyToneGenerator::Type getCourtesyType() const = 0;
    virtual void setCtMode(CourtesyToneGenerator::Type ctType) = 0;
};

}
