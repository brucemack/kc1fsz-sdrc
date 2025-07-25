#include <iostream>
#include <cstring>

#include "TestClock.h"
#include "CommandProcessor.h"

using namespace kc1fsz;
using namespace std;

int main(int, const char**) {

    TestClock clock;
    CommandProcessor proc(clock);
    proc.setAccessTrigger([]() {
        cout << "Access!" << endl;
    });
    proc.setShutdownTrigger([]() {
        cout << "Shutdown!" << endl;
    });
    
    {
        proc.processSymbols("*781 C002");
        proc.processSymbols("*781 C003");
    }

    return 0;
}