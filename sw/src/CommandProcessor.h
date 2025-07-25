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
#ifndef _CommandProcessor_h
#define _CommandProcessor_h

#include <cstdint>
#include <cstring>
#include <functional>

#include "kc1fsz-tools/Runnable.h"

namespace kc1fsz {

class Clock;

class CommandProcessor : public Runnable {
public:

    CommandProcessor(Clock& clock);

    virtual void run();

    /**
     * @brief The main entry point.  Handles a single input symbol.
     * In this is the final symbol in a valid command then the 
     * relevant trigger(s) will be fired before this function returns.
     */
    void processSymbol(char symbol);

    /**
     * @brief A utility function, mostly for testing.
    */
    void processSymbols(const char* s) {
        for (unsigned i = 0; i < std::strlen(s); i++)
            processSymbol(s[i]);
    }

    // ----- Command Triggers ---------------------------------------------
    // To make things easier to integrate, valid commands will pull
    // various "triggers" that are installed here. These triggers would
    // typically be lambda functions to mimimze boiler-plate code.

    void setAccessTrigger(std::function<void()> t) { _accessTrigger = t; }
    void setShutdownTrigger(std::function<void()> t) { _shutdownTrigger = t; }

private:

    Clock& _clock;

    static const unsigned MAX_QUEUE_LEN = 32;
    unsigned _queuePtr = 0;
    char _queue[MAX_QUEUE_LEN];

    bool _access = false;

    std::function<void()> _accessTrigger = 0;
    std::function<void()> _shutdownTrigger = 0;
};

}

#endif
