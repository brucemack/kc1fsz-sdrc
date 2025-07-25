#include "CommandProcessor.h"

namespace kc1fsz {

CommandProcessor::CommandProcessor(Clock& clock)
:   _clock(clock) { }

void CommandProcessor::processSymbol(char symbol) {

    if (symbol == ' ')
        return;

    // Kick the run function to address timeouts, etc.
    run();

    if (_access) {
        if (symbol == '*')
            return;
        if (_queuePtr < MAX_QUEUE_LEN) {
            _queue[_queuePtr++] = symbol;
        }
    }
    else if (symbol == '*') {
        if (_accessTrigger) _accessTrigger();
        _access = true;
    }
}

void CommandProcessor::run() {
}

}