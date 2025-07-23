#ifndef _Rx_h
#define _Rx_h

#include "kc1fsz-tools/Runnable.h"
#include "CourtesyToneGenerator.h"

namespace kc1fsz {

class Rx : public Runnable {
public:

    virtual void run() = 0;

    virtual int getId() const = 0;

    /**
     * @returns true when the receiver audio is valid. This will depend
     * on some combination of COS (hard or soft) and CTCSS, depending on 
     * the configuration of the receiver. isActive() factors together 
     * everything.
     */
    virtual bool isActive() const = 0;

    /**
     * @returns true when carrier is detected.  May be hard or soft, 
     * depending on the configuration.
     */
    virtual bool isCOS() const = 0;

    /**
     * @returns true when the CTCSS tone is detected.  May be hard or soft, 
     * depending on the configuration.
     */
    virtual bool isCTCSS() const = 0;

    /**
     * @brief Used when we first start repeating audio from this receiver.
     * Clears the audio delay line with silence so that we don't hear the 
     * preceding static, etc.
     */
    virtual void resetDelay() = 0;

    // ----- CONFIGURATION ---------------------------------------------------

    enum CosMode {
        COS_IGNORE, COS_EXT_LOW, COS_EXT_HIGH, COS_SOFT
    };

    virtual void setCosMode(CosMode mode) = 0;

    /**
     * @brief Sets the minimum time that the COS signal needs
     * to be asserted for it to be considered "active." This
     * is essentially a debounce.
     */ 
    virtual void setCosActiveTime(unsigned ms) = 0;

    /**
     * @brief Sets the minimum time that the COS signal needs
     * to be unasserted for it to be considered "inactive." This
     * is essentially a debounce.
     */ 
    virtual void setCosInactiveTime(unsigned ms) = 0;

    /**
     * @brief For soft COS detection, sets the audio threshold that 
     * must be exceeded to trigger a detection.
     */
    virtual void setCosLevel(float db) = 0;

    /**
     * @brief Controls how the CTCSS tone decode works.
     */
    enum ToneMode {
        TONE_IGNORE, TONE_EXT_LOW, TONE_EXT_HIGH, TONE_SOFT
    };

    virtual void setToneMode(ToneMode mode) = 0;

    /**
     * @brief Sets the minimum time that the CTCSS signal needs
     * to be asserted for it to be considered "active." This
     * is essentially a debounce.
     */ 
    virtual void setToneActiveTime(unsigned ms) = 0;

    /**
     * @brief Sets the minimum time that the CTCSS signal needs
     * to be unasserted for it to be considered "inactive." This
     * is essentially a debounce.
     */ 
    virtual void setToneInactiveTime(unsigned ms) = 0;

    virtual void setToneLevel(float db) = 0;

    virtual void setToneFreq(float hz) = 0;

    /**
     * @brief Sets the receiver soft gain. Received
     * audio is multiplied by this value.
     */
    virtual void setGainLinear(float lvl) = 0;

    virtual CourtesyToneGenerator::Type getCourtesyType() const = 0;
    
    virtual void setCtMode(CourtesyToneGenerator::Type t) = 0;

    virtual void setDelayTime(unsigned ms) = 0;

    virtual void setAgcMode(uint32_t mode) = 0;
    virtual void setAgcLevel(float dbfs) = 0;
};

}

#endif
