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
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#ifndef _AudioSourceControl_h
#define _AudioSourceControl_h

#include <cstdio>
#include "kc1fsz-tools/Runnable.h"
#include "kc1fsz-tools/Clock.h"

namespace kc1fsz {

class AudioSourceControl : public Runnable {
public:

    AudioSourceControl() { }

    enum Source { SILENT, RADIO0, RADIO1 };

    void setSource(Source source) { 
        _source = source; 
    }

    Source getSource() const { return _source; }

    virtual void run() { 
    }

private:

    volatile Source _source = Source::SILENT;
};

}

#endif
