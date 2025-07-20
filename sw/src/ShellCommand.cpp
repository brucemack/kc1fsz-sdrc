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

static bool eq(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

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
        if (eq(tokens[0], "reset")) {
            printf("Reboot requested");
            // The watchdog will take over from here
            while (true);            
        }
        else if (eq(tokens[0], "factoryreset")) {
            Config::setFactoryDefaults(&_config);
            Config::saveConfig(&_config);
            configChanged = true;
        }
        else if (eq(tokens[0], "save")) {
            Config::saveConfig(&_config);
        }
        else if (eq(tokens[0], "ping")) {
            printf("pong\n");
        }
        else if (eq(tokens[0], "show")) {
            Config::show(&_config);
        }
        else if (eq(tokens[0], "log")) {
            _logTrigger();
        }
        else if (eq(tokens[0], "status")) {
            _statusTrigger();
        }
        else if (eq(tokens[0], "id")) {
            _idTrigger();
        }
        else
            printf(INVALID_COMMAND);
    }
    else if (tokenCount == 2) {
        if (eq(tokens[0], "test")) {
            _testTrigger(atoi(tokens[1]));
        }
        else 
            printf(INVALID_COMMAND);
    }
    else if (tokenCount == 3) {
        if (eq(tokens[0], "set")) {
            configChanged = true;
            if (eq(tokens[1], "call")) {
                strcpyLimited(_config.general.callSign, tokens[2], Config::callSignMaxLen);
            } else if (eq(tokens[1], "pass")) {
                strcpyLimited(_config.general.pass, tokens[2], Config::passMaxLen);
            } else if (eq(tokens[1], "repeatmode")) {
                _config.general.repeatMode = atoi(tokens[2]);
            } else if (eq(tokens[1], "testtonefreq")) {
                _config.general.diagFreq = atof(tokens[2]);
            } else if (eq(tokens[1], "testtonelevel")) {
                _config.general.diagLevel = atof(tokens[2]);
            } else if (eq(tokens[1], "idrequiredint")) {
                _config.general.idRequiredInt = atoi(tokens[2]);
            } else {
                printf(INVALID_COMMAND);
            }
        }
        else {
            printf(INVALID_COMMAND);
        }
    }
    else if (tokenCount == 4) {
        if (eq(tokens[0], "set"))
            configChanged = true;
            if (eq(tokens[1], "cosmode"))
                if (eq(tokens[2], "0"))
                    _config.rx0.cosMode = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.cosMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);
            else if (eq(tokens[1], "cosactivetime"))
                if (eq(tokens[2], "0"))
                    _config.rx0.cosActiveTime = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.cosActiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);
            else if (eq(tokens[1], "cosinactivetime"))
                if (eq(tokens[2], "0"))
                    _config.rx0.cosInactiveTime = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.cosInactiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "coslevel"))
                if (eq(tokens[2], "0"))
                    _config.rx0.cosLevel = atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.cosLevel = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "rxtonemode"))
                if (eq(tokens[2], "0"))
                    _config.rx0.toneMode = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.toneMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "rxtonectivetime"))
                if (eq(tokens[2], "0"))
                    _config.rx0.toneActiveTime = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.toneActiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);
            else if (eq(tokens[1], "rxtoneinactivetime"))
                if (eq(tokens[2], "0"))
                    _config.rx0.toneInactiveTime = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.toneInactiveTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "rxtonelevel"))
                if (eq(tokens[2], "0"))
                    _config.rx0.toneLevel = atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.toneLevel = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "rxtonefreq"))
                if (eq(tokens[2], "0"))
                    _config.rx0.toneFreq =  atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.toneFreq = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "rxgain"))
                if (eq(tokens[2], "0"))
                    _config.rx0.gain = atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.gain = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "delaytime"))
                if (eq(tokens[2], "0"))
                    _config.rx0.delayTime = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.delayTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "txenable"))
                if (eq(tokens[2], "0"))
                    _config.tx0.enabled = atoi(tokens[3]) == 1;
                else if (eq(tokens[2], "1"))
                    _config.tx1.enabled = atoi(tokens[3]) == 1;
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "txtonemode"))
                if (eq(tokens[2], "0"))
                    _config.tx0.toneMode = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.tx1.toneMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "txtonelevel"))
                if (eq(tokens[2], "0"))
                    _config.tx0.toneLevel = atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.tx1.toneLevel = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "txtonefreq"))
                if (eq(tokens[2], "0"))
                    _config.tx0.toneFreq =  atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.tx1.toneFreq = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "rxrepeat"))
                if (eq(tokens[2], "0")) 
                    for (unsigned i = 0; i < strlen(tokens[3]) && i < Config::maxReceivers; i++)
                        _config.txc0.rxEligible[i] = (tokens[3][i] == '1');
                else if (eq(tokens[2], "1"))
                    for (unsigned i = 0; i < strlen(tokens[3]) && i < Config::maxReceivers; i++)
                        _config.txc1.rxEligible[i] = (tokens[3][i] == '1');
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "timeouttime"))
                if (eq(tokens[2], "0"))
                    _config.txc0.timeoutTime = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.txc1.timeoutTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "lockouttime"))
                if (eq(tokens[2], "0"))
                    _config.txc0.lockoutTime = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.txc1.lockoutTime = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "ctmode"))
                if (eq(tokens[2], "0"))
                    _config.rx0.ctMode = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.rx1.ctMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "ctlevel"))
                if (eq(tokens[2], "0"))
                    _config.txc0.ctLevel = atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.txc1.ctLevel = atof(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "idmode"))
                if (eq(tokens[2], "0"))
                    _config.txc0.idMode = atoi(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.txc1.idMode = atoi(tokens[3]);
                else 
                    printf(INVALID_COMMAND);                
            else if (eq(tokens[1], "idlevel"))
                if (eq(tokens[2], "0"))
                    _config.txc0.idLevel = atof(tokens[3]);
                else if (eq(tokens[2], "1"))
                    _config.txc1.idLevel = atof(tokens[3]);
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
