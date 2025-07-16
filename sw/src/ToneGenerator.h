#ifndef _ToneGenerator_h
#define _ToneGenerator_h

#include "kc1fsz-tools/Runnable.h"

namespace kc1fsz {

class ToneGenerator : public Runnable {
public:

    virtual void run() = 0;
    virtual void start() = 0;
    virtual bool isFinished() = 0;
};

}

#endif
