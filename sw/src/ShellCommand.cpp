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
#include <cstdio>
#include <cstring>

#include "kc1fsz-tools/Common.h"
#include "Config.h"
#include "ShellCommand.h"

namespace kc1fsz {

static const char* INVALID_COMMAND = "Invalid Command\n";

void ShellCommand::process(const char* cmd) {
    // Tokenize
    const unsigned int maxTokenCount = 4;
    const unsigned int maxTokenLen = 32;
    char tokens[maxTokenCount][maxTokenLen];
    int tokenCount = 0;
    int tokenPtr = 0;
    for (int i = 0; i <= strlen(cmd) && tokenCount < maxTokenCount; i++) {
        // Space is the inter-token delimiter
        if (cmd[i] == ' ' || cmd[i] == 0) {
            // Leading spaces are ignored
            if (tokenPtr > 0)
                tokenCount++;
            tokenPtr = 0;
        } else {
            if (tokenPtr < maxTokenLen - 1) {
                tokens[tokenCount][tokenPtr++] = cmd[i];
                tokens[tokenCount][tokenPtr] = 0;
            }
        }
    }

    bool configChanged = false;

    if (tokenCount == 1) {
        if (strcmp(tokens[0], "reset") == 0) {
            printf("Reboot requested");
            // The watchdog will take over from here
            while (true);            
        }
        else if (strcmp(tokens[0], "factoryreset") == 0) {
            Config::setFactoryDefaults(&_config);
            Config::saveConfig(&_config);
            configChanged = true;
        }
        else if (strcmp(tokens[0], "save") == 0) {
            Config::saveConfig(&_config);
        }
        else if (strcmp(tokens[0], "ping") == 0) {
            printf("pong\n");
        }
        else if (strcmp(tokens[0], "show") == 0) {
            Config::show(&_config);
        }
        else if (strcmp(tokens[0], "log") == 0) {
            _logTrigger();
        }
        else if (strcmp(tokens[0], "status") == 0) {
            _statusTrigger();
        }
        else if (strcmp(tokens[0], "id") == 0) {
            _idTrigger();
        }
        else
            printf(INVALID_COMMAND);
    }
    else if (tokenCount == 2) {
        printf(INVALID_COMMAND);
    }
    else if (tokenCount == 3) {
        if (strcmp(tokens[0], "set") == 0) {
            configChanged = true;
            if (strcmp(tokens[1], "call") == 0) {
                strcpyLimited(_config.general.callSign, tokens[2], Config::callSignMaxLen);
            } else if (strcmp(tokens[1], "pass") == 0) {
                strcpyLimited(_config.general.pass, tokens[2], Config::passMaxLen);
            } else if (strcmp(tokens[1], "repeatmode") == 0) {
                _config.general.repeatMode = atoi(tokens[2]);
            } else if (strcmp(tokens[1], "testmode") == 0) {
                _config.general.diagMode = atoi(tokens[2]);
            } else if (strcmp(tokens[1], "testtonefreq") == 0) {
                _config.general.diagFreq = atof(tokens[2]);
            } else if (strcmp(tokens[1], "testtonelevel") == 0) {
                _config.general.diagLevel = atof(tokens[2]);
            } else {
                printf(INVALID_COMMAND);
            }
        }
        else {
            printf(INVALID_COMMAND);
        }
    }
    else if (tokenCount == 4) {
        if (strcmp(tokens[0], "set") == 0)
            configChanged = true;
            if (strcmp(tokens[1], "cosmode") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.cosMode = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.cosMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);
            else if (strcmp(tokens[1], "cosactivetime") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.cosActiveTime = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.cosActiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);
            else if (strcmp(tokens[1], "cosinactivetime") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.cosInactiveTime = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.cosInactiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "coslevel") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.cosLevel = atof(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.cosLevel = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "rxtonemode") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.toneMode = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.toneMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "rxtonectivetime") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.toneActiveTime = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.toneActiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);
            else if (strcmp(tokens[1], "rxtoneinactivetime") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.toneInactiveTime = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.toneInactiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "rxtonelevel") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.toneLevel = atof(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.toneLevel = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "rxtonefreq") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.toneFreq =  atof(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.toneFreq = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "rxgain") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.rx0.gain = atof(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.rx1.gain = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "txtonemode") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.tx0.toneMode = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.tx1.toneMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "txtonelevel") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.tx0.toneLevel = atof(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.tx1.toneLevel = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "txtonefreq") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.tx0.toneFreq =  atof(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.tx1.toneFreq = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "timeouttime") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.txc0.timeoutTime = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.txc1.timeoutTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (strcmp(tokens[1], "lockouttime") == 0)
                if (strcmp(tokens[2], "0") == 0)
                    _config.txc0.lockoutTime = atoi(tokens[3]);
                else if (strcmp(tokens[2], "1") == 0)
                    _config.txc1.lockoutTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                


        else
            printf(INVALID_COMMAND);
    }

    // Notify the client of a change to the configuration structure
    if (configChanged)
        _configChangedTrigger();
}
}
