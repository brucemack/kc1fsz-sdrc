#include <iostream>
#include <cstring>
#include <cassert>

#include "TestClock.h"
#include "CommandProcessor.h"

using namespace kc1fsz;
using namespace std;

struct TestData {
    bool a;
    bool b;
};

int main(int, const char**) {

    TestClock clock;
    TestData data;

    CommandProcessor proc(clock);

    proc.setAccessTrigger([&data]() {
        cout << "Access!" << endl;
        data.a = true;
    });
    proc.setDisableTrigger([&data]() {
        cout << "Disable" << endl;
        data.b = true;
    });
    proc.setReenableTrigger([&data]() {
        cout << "Reendable" << endl;
    });
    
    {
        cout << "----- Test 1 -----" << endl;
        clock.setTime(0);
        proc.run();
        proc.processSymbols("*781 C002");
        proc.processSymbols("*781 C003");
        proc.run();
        assert(data.a);
    }

    // This test we show an access timeout
    {
        cout << "----- Test 2 -----" << endl;
        clock.setTime(0);
        data.b = false;

        proc.run();
        proc.processSymbols("*781");
        assert(proc.isAccess());
        proc.processSymbols("C002");
        assert(data.b);

        // Show show a case where the access times out
        clock.setTime(0);
        data.a = false;
        data.b = false;
        proc.run();
        proc.processSymbols("*781");
        assert(proc.isAccess());
        clock.setTime(11 * 1000);
        proc.run();
        assert(!proc.isAccess());
        proc.processSymbols("C002");
        assert(!data.b);
    }

    return 0;
}