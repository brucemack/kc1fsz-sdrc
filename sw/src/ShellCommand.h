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
#ifndef _ShellCommand_h
#define _ShellCommand_h

#include <functional>
#include "kc1fsz-tools/CommandShell.h"

namespace kc1fsz {

class Config;

class ShellCommand : public CommandSink {
public:
 
    ShellCommand(Config& config, 
        std::function<void()> logTrigger, std::function<void()> statusTrigger,
        std::function<void()> configChangedTrigger) 
    :   _config(config),
        _logTrigger(logTrigger), 
        _statusTrigger(statusTrigger),
        _configChangedTrigger(configChangedTrigger) { }

    void process(const char* cmd);

private:

    Config& _config;
    std::function<void()> _logTrigger;
    std::function<void()> _statusTrigger;
    std::function<void()> _configChangedTrigger;
};

}

#endif
