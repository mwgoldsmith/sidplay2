/***************************************************************************
                          xsid.cpp  -  Support for Playsids Extended
                                       Registers
                             -------------------
    begin                : Tue Jun 20 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: not supported by cvs2svn $
 *  Revision 1.8  2001/02/07 21:02:30  s_a_white
 *  Supported for delaying samples for frame simulation.  New alogarithm to
 *  better guess original tunes volume when playing samples.
 *
 *  Revision 1.7  2000/12/12 22:51:01  s_a_white
 *  Bug Fix #122033.
 *
 ***************************************************************************/

#include <string.h>
#include <stdio.h>
#include "xsid.h"


#ifndef XSID_USE_SID_VOLUME
// Convert from 4 bit resolution to 8 bits
/* Rev 2.0.5 (saw) - Removed for a more non-linear equivalent
   which better models the SIDS master volume register
const int8_t channel::sampleConvertTable[16] =
{
    '\x80', '\x91', '\xa2', '\xb3', '\xc4', '\xd5', '\xe6', '\xf7',
    '\x08', '\x19', '\x2a', '\x3b', '\x4c', '\x5d', '\x6e', '\x7f'
};
*/
const int8_t channel::sampleConvertTable[16] =
{
    '\x80', '\x94', '\xa9', '\xbc', '\xce', '\xe1', '\xf2', '\x03',
    '\x1b', '\x2a', '\x3b', '\x49', '\x58', '\x66', '\x73', '\x7f'
};
#endif

channel::channel ()
{
    memset (reg, 0, sizeof (reg));
    reset  ();
}

void channel::reset ()
{
    galVolume  = 0; // This is left to free run until reset
    mode       = FM_NONE;
    lowerLimit = 8;
    // Set XSID to stopped state
    reg[convertAddr (0x1d)] = 0xfd;
    free ();
}

inline void channel::clock (uint_least16_t delta_t)
{   // Emulate a CIA timer
    if (!cycleCount) return;
    if (delta_t == 1)
    {   // Rev 1.4 (saw) - Optimisation to prevent _clock being
        // called un-necessarily.
#ifdef XSID_DEBUG
        cycles++;
#endif
        if (!--cycleCount)
            (this->*_clock) ();
        return;
    }

    // Slightly optimised clocking routine
    // for clock periods > than 1
    while (cycleCount <= delta_t)
    {   // Rev 1.4 (saw) - Changed to reflect change above
#ifdef XSID_DEBUG
        cycles  += cycleCount;
#endif
        delta_t -= cycleCount;
        (this->*_clock) ();
        // Check to see if the sequence finished
        if (!cycleCount) return;
    }
    
    // Reduce the cycleCount by whats left
#ifdef XSID_DEBUG
    cycles     += delta_t;
#endif
    cycleCount -= delta_t;
}

#ifndef XSID_USE_SID_VOLUME
inline int_least32_t channel::output ()
#else
inline uint8_t channel::output ()
#endif
{
#ifdef XSID_DEBUG
    outputs++;
#endif
    return sample;
}

void channel::free ()
{
    active     = false;
    cycleCount = 0;
    _clock     = &channel::silence;
    silence ();
}

void channel::checkForInit ()
{   // Check to see mode of operation
    // See xsid documentation
    uint8_t state = reg[convertAddr (0x1d)];

    if (state == 0xfd)
        free (); // Stop
    else if (state < 0xfc)
        galwayInit ();
    else // if (state >= 0xfc)
        sampleInit ();
}

void channel::sampleInit ()
{
    if (active && (mode == FM_GALWAY))
        return;

#ifdef XSID_DEBUG
    printf ("XSID [%lu]: Sample Init\n", (unsigned long) this);

    if (active && (mode == FM_HUELS))
        printf ("XSID [%lu]: Stopping Playing Sample\n", (unsigned long) this);
#endif

    // Check all important parameters are legal
    _clock        = &channel::silence;
    volShift      = (uint_least8_t) (0 - (int8_t) reg[convertAddr (0x1d)]) >> 1;
    reg[convertAddr (0x1d)] = 0xfd;
    address       = ((uint_least16_t) reg[convertAddr (0x1f)] << 8) | reg[convertAddr (0x1e)];
    samEndAddr    = ((uint_least16_t) reg[convertAddr (0x3e)] << 8) | reg[convertAddr (0x3d)];
    if (samEndAddr <= address) return;
    samScale      = reg[convertAddr (0x5f)];
    samPeriod     = (((uint_least16_t) reg[convertAddr (0x5e)] << 8) | reg[convertAddr (0x5d)]) >> samScale;
    if (!samPeriod) return;

    // Load the other parameters
    samNibble     = 0;
    samRepeat     = reg[convertAddr (0x3f)];
    samOrder      = reg[convertAddr (0x7d)];
    samRepeatAddr = ((uint_least16_t) reg[convertAddr (0x7f)] << 8) | reg[convertAddr (0x7e)];
    cycleCount    = samPeriod;

    // Support Galway Samples, but that
    // mode it setup only when as Galway
    // Noise sequence begins
    if (mode == FM_NONE)
        mode  = FM_HUELS;
    active = true;
    _clock = &channel::sampleClock;
    sample = sampleCalculate ();

#ifdef XSID_USE_SID_VOLUME
    lowerLimit = 8 >> volShift;
    changed    = true;
#endif

#if XSID_DEBUG > 1
    printf ("XSID [%lu]: Sample Start Address:  0x%04x\n", (unsigned long) this, address);
    printf ("XSID [%lu]: Sample End Address:    0x%04x\n", (unsigned long) this, samEndAddr);
    printf ("XSID [%lu]: Sample Repeat Address: 0x%04x\n", (unsigned long) this, samRepeatAddr);
    printf ("XSID [%lu]: Sample Period: %u\n", (unsigned long) this, samPeriod);
    printf ("XSID [%lu]: Sample Repeat: %u\n", (unsigned long) this, samRepeat);
    printf ("XSID [%lu]: Sample Order:  %u\n", (unsigned long) this, samOrder);
#endif

#ifdef XSID_DEBUG
    cycles  = 0;
    outputs = 0;
    printf ("XSID [%lu]: Sample Start\n", (unsigned long) this);
#endif
}

void channel::sampleClock ()
{
    cycleCount   = samPeriod;
    if (address >= samEndAddr)
    {
        if (samRepeat != 0xFF)
        {
            if (samRepeat)
                samRepeat--;
            else
                samRepeatAddr = address;
        }

        address      = samRepeatAddr;
        if (address >= samEndAddr)
        {
#ifdef XSID_DEBUG
            printf ("XSID [%lu]: Sample Stop (%lu Cycles, %lu Outputs)\n",
                (unsigned long) this, cycles, outputs);
#endif
#ifdef XSID_USE_SID_VOLUME
            changed = true;
#endif
            free ();
            checkForInit ();
            return;
        }
    }

    // We have reached the required sample
    // So now we need to extract the right nibble
    sample  = sampleCalculate ();

#ifdef XSID_USE_SID_VOLUME
    changed = true;
#endif
}

int8_t channel::sampleCalculate ()
{
    uint_least8_t tempSample = envReadMemByte (address, true);
    if (samOrder == SO_LOWHIGH)
    {
        if (samScale == 0)
        {
            if (samNibble != 0)
                tempSample >>= 4;
        }
        // AND 15 further below.
    }
    else // if (samOrder == SO_HIGHLOW)
    {
        if (samScale == 0)
        {
            if (samNibble == 0)
                tempSample >>= 4;
        }
        else // if (samScale != 0)
            tempSample >>= 4;
        // AND 15 further below.
    }

    // Move to next address
    address   += samNibble;
    samNibble ^= 1;
    samNibble &= 1;
#ifndef XSID_USE_SID_VOLUME
    return sampleConvertTable[(tempSample & 0x0f) >> volShift];
#else
    return (int8_t) ((tempSample & 0x0f) - 0x08) >> volShift;
#endif
}

void channel::galwayInit()
{
    // If mode was HUELS, then it's
    // gonna become GALWAY and we should
    // not interrupt current sample
    mode = FM_GALWAY;
    if (active)
        return;

#ifdef XSID_DEBUG
    printf ("XSID [%lu]: Galway Init\n", (unsigned long) this);
#endif

    // Check all important parameters are legal
    _clock        = &channel::silence;
    galTones      = reg[convertAddr (0x1d)];
    reg[convertAddr (0x1d)] = 0xfd;
    galInitLength = reg[convertAddr (0x3d)];
    if (!galInitLength) return;
    galLoopWait   = reg[convertAddr (0x3f)];
    if (!galLoopWait)   return;
    galNullWait   = reg[convertAddr (0x5d)];
    if (!galNullWait)   return;

    // Load the other parameters
    address  = ((uint_least16_t) reg[convertAddr (0x1f)] << 8) | reg[convertAddr(0x1e)];
    volShift = reg[convertAddr (0x3e)] & 0x0f;
    mode     = FM_GALWAY;
    active   = true;

#ifndef XSID_USE_SID_VOLUME
    sample     = sampleConvertTable[galVolume];
#else
    sample     = (int8_t) galVolume;
    lowerLimit = 8;
    changed    = true;
#endif

    galwayTonePeriod ();
    _clock  = &channel::galwayClock;

#ifdef XSID_DEBUG
    cycles  = 0;
    outputs = 0;
    printf ("XSID [%lu]: Galway Start\n", (unsigned long) this);
#endif
}

void channel::galwayClock ()
{
    if (--galLength)
        cycleCount = samPeriod;
    else if (galTones == 0xff)
    {   // The counter has completed
#ifdef XSID_DEBUG
        printf ("XSID [%lu]: Galway Stop (%lu Cycles, %lu Outputs)\n",
            (unsigned long) this, cycles, outputs);
        if (reg[convertAddr (0x1d)] != 0xfd)
            printf ("XSID [%lu]: Starting Delayed Sequence\n",
                (unsigned long) this);
#endif
        free ();
        checkForInit ();
        return;
    }
    else
        galwayTonePeriod ();

    // See Galway Example...
    galVolume += volShift;
    galVolume &= 0x0f;
#ifndef XSID_USE_SID_VOLUME
    sample  = sampleConvertTable[galVolume];
#else
    sample  = (int8_t) galVolume;
    changed = true;
#endif
}

void channel::galwayTonePeriod ()
{   // Calculate the number of cycles over which sample should last
    galLength  = galInitLength;
    samPeriod  = envReadMemByte (address + galTones, true);
    samPeriod *= galLoopWait;
    samPeriod += galNullWait;
    cycleCount = samPeriod;
#if XSID_DEBUG > 2
    printf ("XSID [%lu]: Galway Settings\n", (unsigned long) this);
    printf ("XSID [%lu]: Length %u, LoopWait %u, NullWait %u\n",
        (unsigned long) this, galLength, galLoopWait, galNullWait);
    printf ("XSID [%lu]: Tones %u, Data %u\n",
        (unsigned long) this, galTones, envReadMemByte (address + galTones), true);
#endif
    galTones--;
}

void channel::silence ()
{
    sample = 0;
}

// Use Suppress to delay the samples and start them later
// Effectivly allows running samples in a frame based mode.
void XSID::suppress (bool enable)
{
    // @FIXME@: Mute Temporary Hack
    suppressed = enable | muted;
    if (!suppressed)
    {   // Get the channels running
        if (ch4.read (0x1d) != 0xfd)
            ch4.checkForInit ();
        if (ch5.read (0x1d) != 0xfd)
            ch5.checkForInit ();
        calcSampleOffset ();
    }
}

// By muting samples they will start and play the at the
// appropriate time but no sound is produced.  Un-muting
// will cause sound output from the current play position.
void XSID::mute (bool enable)
{
    // @FIXME@: Mute not properly implemented
    muted = suppressed = enable;
}

void XSID::write (uint_least16_t addr, uint8_t data)
{
    channel *ch;
    uint8_t tempAddr;

    // Make sure address is legal
    if ((addr & 0xfe8c) ^ 0x000c)
        return;

    ch = &ch4;
    if (addr & 0x0100)
        ch = &ch5;

    tempAddr = (uint8_t) addr;
    ch->write (tempAddr, data);
#if XSID_DEBUG > 1
    printf ("XSID: Addr 0x%02x, Data 0x%02x\n", tempAddr, data);
#endif

    if (addr == 0x1d)
    {
        if (data == 0xfd)
        {
            ch->checkForInit ();
            return;
        }
        
        if (suppressed)
        {
#if XSID_DEBUG
            printf ("XSID: Initialise Suppressed\n");
#endif
            return;
        }
#ifdef XSID_USE_SID_VOLUME
        calcSampleOffset ();
#endif
        ch->checkForInit ();
    }
}

uint8_t XSID::read (uint_least16_t addr)
{
    channel *ch;
    uint8_t  tempAddr;

    // Make sure address is legal
    if ((addr & 0xfe8c) ^ 0xd40c)
        return 0;
    if ((addr & 0x001f) < 0x001d)
    {   // Should never happen
        return 0;
    }

    ch = &ch4;
    if (addr & 0x0100)
        ch = &ch5;

    tempAddr = (uint8_t) addr;
    return ch->read (tempAddr);
}

// This function is a mess but is remapped according
// to the mode.  It's kept like this so I don't have to
// keep updating 2 bits of code
#ifndef XSID_USE_SID_VOLUME
int_least32_t XSID::output (uint_least8_t bits)
{
    int_least32_t sample = 0;
#else
uint8_t XSID::output ()
{
    int8_t sample = 0;
#endif // XSID_USE_SID_VOLUME
    sample += ch4.output ();
    sample += ch5.output ();
#ifndef XSID_USE_SID_VOLUME
    sample <<= (bits - 8);
#endif
    // Automatically compensated for by C64 code
    //return (sample >> 1);
    return sample;
}

void XSID::reset ()
{
    ch4.reset ();
    ch5.reset ();
    suppressed = false;
}

#ifdef XSID_USE_SID_VOLUME
void XSID::setSidVolume (bool cached)
{
    if (ch4.changed || ch5.changed)
    {
        uint8_t reg = sidReg0x18 | ((sampleOffset + output ()) & 0x0f);
        ch4.changed = false;
        ch5.changed = false;
#if XSID_DEBUG > 1
        printf ("XSID: Writing Sample to SID Volume [0x%02x]\n", reg);
#endif
        envWriteMemByte (sidVolAddr, reg, false);
    }
}
#endif

void XSID::clock (uint_least16_t delta_t)
{
#ifdef XSID_USE_SID_VOLUME
    bool wasRunning;
    wasRunning = ch4 || ch5;
#endif

    // Call optimised clock routine
    // Well it's better then just doing:
    // while (delta_t--)
    //     clock ();
    ch4.clock (delta_t);
    ch5.clock (delta_t);

#ifdef XSID_USE_SID_VOLUME
    if (ch4 || ch5)
        setSidVolume ();
    else if (wasRunning)
    {   // Rev 2.0.5 (saw) - Changed to restore volume different depending on mode
        // Normally after samples volume should be restored to half volume,
        // however, Galway Tunes sound horrible and seem to require setting back to
        // the original volume.  Setting back to the original volume for normal
        // samples can have nasty pulsing effects
        if (ch4.isGalway ())
        {
            uint8_t sidVolume = envReadMemDataByte (sidVolAddr, true);
            envWriteMemByte (sidVolAddr, sidVolume);
        }
        else
            setSidVolume ();
    }
#endif
}

void  XSID::setEnvironment (C64Environment *envp)
{
#ifdef XSID_USE_SID_VOLUME
    C64Environment::setEnvironment (envp);
#endif

    ch4.setEnvironment (envp);
    ch5.setEnvironment (envp);
}

#ifdef XSID_USE_SID_VOLUME
void XSID::calcSampleOffset (void)
{
    // Try to determine a sensible offset between voice
    // and sample volumes.
    uint_least8_t lower = ch4.limit () + ch5.limit ();
    uint_least8_t upper;
    sidReg0x18   = envReadMemDataByte (sidVolAddr, true);
    sampleOffset = sidReg0x18 & 0x0f;
    sidReg0x18  &= 0xf0;

    // Is possible to compensate for both channels
    // set to 4 bits here, but should never happen.
    if (lower > 8)
        lower >>= 1;
    upper = 0x0f - lower + 1;

    // Check against limits
    if (sampleOffset < lower)
        sampleOffset = lower;
    else if (sampleOffset > upper)
        sampleOffset = upper;
}
#endif // XSID_USE_SID_VOLUME
