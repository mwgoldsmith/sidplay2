/***************************************************************************
                          mos6526.cpp  -  CIA Timer
                             -------------------
    begin                : Wed Jun 7 2000
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
 *  Revision 1.14  2004/01/06 21:28:27  s_a_white
 *  Initial TOD support (code taken from vice)
 *
 *  Revision 1.13  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.12  2003/02/24 19:44:30  s_a_white
 *  Make sure events are canceled on reset.
 *
 *  Revision 1.11  2003/01/17 08:39:04  s_a_white
 *  Event scheduler phase support.  Better default handling of keyboard lines.
 *
 *  Revision 1.10  2002/12/16 22:12:24  s_a_white
 *  Simulate serial input from data port A to prevent kernel lockups.
 *
 *  Revision 1.9  2002/11/20 22:50:27  s_a_white
 *  Reload count when timers are stopped
 *
 *  Revision 1.8  2002/10/02 19:49:21  s_a_white
 *  Revert previous change as was incorrect.
 *
 *  Revision 1.7  2002/09/11 22:30:47  s_a_white
 *  Counter interval writes now go to a new register call prescaler.  This is
 *  copied to the timer latch/counter as appropriate.
 *
 *  Revision 1.6  2002/09/09 22:49:06  s_a_white
 *  Proper idr clear if interrupt was only internally pending.
 *
 *  Revision 1.5  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.4  2002/03/03 22:04:08  s_a_white
 *  Tidy.
 *
 *  Revision 1.3  2001/07/14 13:03:33  s_a_white
 *  Now uses new component classes and event generation.
 *
 *  Revision 1.2  2001/03/23 23:21:38  s_a_white
 *  Removed redundant reset funtion.  Timer b now gets initialised properly.
 *  Switch case now allows write/read from timer b.
 *
 *  Revision 1.1  2001/03/21 22:41:45  s_a_white
 *  Non faked CIA emulation with NMI support.  Removal of Hacked VIC support
 *  off CIA timer.
 *
 *  Revision 1.8  2001/03/09 23:44:30  s_a_white
 *  Integrated more 6526 features.  All timer modes and interrupts correctly
 *  supported.
 *
 *  Revision 1.7  2001/02/21 22:07:10  s_a_white
 *  Prevent re-triggering of interrupt if it's already active.
 *
 *  Revision 1.6  2001/02/13 21:00:01  s_a_white
 *  Support for real interrupts.
 *
 *  Revision 1.4  2000/12/11 18:52:12  s_a_white
 *  Conversion to AC99
 *
 ***************************************************************************/

#include <string.h>
#include "sidendian.h"
#include "mos6526.h"

enum
{
    INTERRUPT_TA      = 1 << 0,
    INTERRUPT_TB      = 1 << 1,
    INTERRUPT_ALARM   = 1 << 2,
    INTERRUPT_SP      = 1 << 3,
    INTERRUPT_FLAG    = 1 << 4,
    INTERRUPT_REQUEST = 1 << 7
};

enum
{
    TOD_TEN    = 8,
    TOD_SEC = 9,
    TOD_MIN = 10,
    TOD_HR  = 11
};

const char *MOS6526::credit =
{   // Optional information
    "*MOS6526 (CIA) Emulation:\0"
    "\tCopyright (C) 2001 Simon White <sidplay2@email.com>\0"
};


MOS6526::MOS6526 (EventContext *context)
:idr(0),
 event_context(*context),
 m_phase(EVENT_CLOCK_PHI1),
 m_todPeriod(~0), // Dummy
 event_ta(this),
 event_tb(this),
 event_tod(this)
{
    reset ();
}

void MOS6526::clock (float64_t clock)
{    // Fixed point 25.7
    m_todPeriod = (event_clock_t) (clock * (float64_t) (1 << 7));
}

void MOS6526::reset (void)
{
    ta  = ta_latch = 0xffff;
    tb  = tb_latch = 0xffff;
    cra = crb = 0;
    // Clear off any IRQs
    trigger (0);
    cnt_high  = true;
    icr = idr = 0;
    m_accessClk = 0;
    dpa = 0xf0;

    // Reset tod
    memset(m_todclock, 0, sizeof(m_todclock));
    memset(m_todalarm, 0, sizeof(m_todalarm));
    memset(m_todlatch, 0, sizeof(m_todlatch));
    m_todlatched = false;
    m_todstopped = true;
    m_todclock[TOD_HR-TOD_TEN] = 1; // the most common value
    m_todCycles = 0;

    // Remove outstanding events
    event_context.cancel   (&event_ta);
    event_context.cancel   (&event_tb);
    event_context.schedule (&event_tod, 0, m_phase);
}

uint8_t MOS6526::read (uint_least8_t addr)
{
    event_clock_t cycles;
    if (addr > 0x0f) return 0;

    cycles       = event_context.getTime (m_accessClk, m_phase);
    m_accessClk += cycles;

    // Sync up timers
    if ((cra & 0x21) == 0x01)
    {
        ta -= cycles;
        if (!ta)
            ta_event ();
    }
    if ((crb & 0x61) == 0x01)
    {
        tb -= cycles;
        if (!tb)
            tb_event ();
    }

    switch (addr)
    {
    case 0x0: // Simulate a serial port
        dpa = ((dpa << 1) | (dpa >> 7)) & 0xff;
        if (dpa & 0x80)
            return 0xff;
        return 0x3f;
    case 0x1:
        return 0xff;
    case 0x4: return endian_16lo8 (ta);
    case 0x5: return endian_16hi8 (ta);
    case 0x6: return endian_16lo8 (tb);
    case 0x7: return endian_16hi8 (tb);

    // TOD implementation taken from Vice
    // TOD clock is latched by reading Hours, and released
    // upon reading Tenths of Seconds. The counter itself
    // keeps ticking all the time.
    // Also note that this latching is different from the input one.
    case TOD_TEN: // Time Of Day clock 1/10 s
    case TOD_SEC: // Time Of Day clock sec
    case TOD_MIN: // Time Of Day clock min
    case TOD_HR:  // Time Of Day clock hour
        if (!m_todlatched)
            memcpy(m_todlatch, m_todclock, sizeof(m_todlatch));
        if (addr == TOD_TEN)
            m_todlatched = false;
        if (addr == TOD_HR)
            m_todlatched = true;
        return m_todlatch[addr - TOD_TEN];

    case 0xd:
    {   // Clear IRQs, and return interrupt
        // data register
        uint8_t ret = idr;
        trigger (0);
        return ret;
    }

    case 0x0e: return cra;
    case 0x0f: return crb;
    default:  return regs[addr];
    }
}

void MOS6526::write (uint_least8_t addr, uint8_t data)
{
    event_clock_t cycles;
    if (addr > 0x0f) return;

    regs[addr]   = data;
    cycles       = event_context.getTime (m_accessClk, m_phase);
    m_accessClk += cycles;

    // Sync up timers
    if ((cra & 0x21) == 0x01)
    {
        ta -= cycles;
        if (!ta)
            ta_event ();
    }
    if ((crb & 0x61) == 0x01)
    {
        tb -= cycles;
        if (!tb)
            tb_event ();
    }

    switch (addr)
    {
    case 0x4: endian_16lo8 (ta_latch, data); break;
    case 0x5:
        endian_16hi8 (ta_latch, data);
        if (!(cra & 0x01)) // Reload timer if stopped
            ta = ta_latch;
    break;

    case 0x6: endian_16lo8 (tb_latch, data); break;
    case 0x7:
        endian_16hi8 (tb_latch, data);
        if (!(crb & 0x01)) // Reload timer if stopped
            tb = tb_latch;
    break;

    // TOD implementation taken from Vice
    case TOD_HR:  // Time Of Day clock hour
        // Flip AM/PM on hour 12
        //   (Andreas Boose <viceteam@t-online.de> 1997/10/11).
        // Flip AM/PM only when writing time, not when writing alarm
        // (Alexander Bluhm <mam96ehy@studserv.uni-leipzig.de> 2000/09/17).
        data &= 0x9f;
        if ((data & 0x1f) == 0x12 && !(crb & 0x80))
            data ^= 0x80;
        // deliberate run on
    case TOD_TEN: // Time Of Day clock 1/10 s
    case TOD_SEC: // Time Of Day clock sec
    case TOD_MIN: // Time Of Day clock min
        if (crb & 0x80)
            m_todalarm[addr - TOD_TEN] = data;
        else
        {
            if (addr == TOD_TEN)
                m_todstopped = false;
            if (addr == TOD_HR)
                m_todstopped = true;
            m_todclock[addr - TOD_TEN] = data;
        }
        // check alarm
        if (!m_todstopped && !memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
            trigger (INTERRUPT_ALARM);
        break;

    case 0xd:
        if (data & 0x80)
            icr |= data & 0x1f;
        else
            icr &= ~data;
        trigger (idr);
    break;

    case 0x0e:
        // Check for forced load
        cra = data;
        if (data & 0x10)
        {
            cra &= (~0x10);
            ta   = ta_latch;
        }

        if ((data & 0x21) == 0x01)
        {   // Active
            event_context.schedule (&event_ta, (event_clock_t) ta + 1,
                                    m_phase);
        } else
        {   // Inactive
            ta = ta_latch;
            event_context.cancel (&event_ta);
        }
    break;

    case 0x0f:
        // Check for forced load
        crb = data;
        if (data & 0x10)
        {
            crb &= (~0x10);
            tb   = tb_latch;
        }

        if ((data & 0x61) == 0x01)
        {   // Active
            event_context.schedule (&event_tb, (event_clock_t) tb + 1,
                                    m_phase);
        } else
        {   // Inactive
            tb = tb_latch;
            event_context.cancel (&event_tb);
        }
    break;

    default:
    break;
    }
}

void MOS6526::trigger (int irq)
{
    if (!irq)
    {   // Clear any requested IRQs
        if (idr & INTERRUPT_REQUEST)
            interrupt (false);
        idr = 0;
        return;
    }

    idr |= irq;
    if (icr & idr)
    {
        if (!(idr & INTERRUPT_REQUEST))
        {
            idr |= INTERRUPT_REQUEST;
            interrupt (true);
        }
    }
}

void MOS6526::ta_event (void)
{   // Timer Modes
    event_clock_t cycles;
    uint8_t mode = cra & 0x21;

    if (mode == 0x21)
    {
        if (ta--)
            return;
    }

    cycles       = event_context.getTime (m_accessClk, m_phase);
    m_accessClk += cycles;

    ta = ta_latch;
    if (cra & 0x08)
    {   // one shot, stop timer A
        cra &= (~0x01);
    } else if (mode == 0x01)
    {   // Reset event
        event_context.schedule (&event_ta, (event_clock_t) ta + 1,
                                m_phase);
    }
    trigger (INTERRUPT_TA);
    
    switch (crb & 0x61)
    {
    case 0x01: tb -= cycles; break;
    case 0x41:
    case 0x61:
        tb_event ();
    break;
    }
}
    
void MOS6526::tb_event (void)
{   // Timer Modes
    uint8_t mode = crb & 0x61;
    switch (mode)
    {
    case 0x01:
        break;

    case 0x21:
    case 0x41:
        if (tb--)
            return;
    break;

    case 0x61:
        if (cnt_high)
        {
            if (tb--)
                return;
        }
    break;
    
    default:
        return;
    }

    m_accessClk = event_context.getTime (m_phase);
    tb = tb_latch;
    if (crb & 0x08)
    {   // one shot, stop timer A
        crb &= (~0x01);
    } else if (mode == 0x01)
    {   // Reset event
        event_context.schedule (&event_tb, (event_clock_t) tb + 1,
                                m_phase);
    }
    trigger (INTERRUPT_TB);
}

// TOD implementation taken from Vice
#define byte2bcd(byte) (((((byte) / 10) << 4) + ((byte) % 10)) & 0xff)
#define bcd2byte(bcd)  (((10*(((bcd) & 0xf0) >> 4)) + ((bcd) & 0xf)) & 0xff)

void MOS6526::tod_event(void)
{   // Reload divider according to 50/60 Hz flag
    // Only performed on expiry according to Frodo
    if (cra & 0x80)
        m_todCycles += (m_todPeriod * 5);
    else
        m_todCycles += (m_todPeriod * 6);    
    
    // Fixed precision 25.7
    event_context.schedule (&event_tod, m_todCycles >> 7, m_phase);
    m_todCycles &= 0x7F; // Just keep the decimal part

    if (!m_todstopped)
    {
        // inc timer
        uint8_t *tod = m_todclock;
        uint8_t t = bcd2byte(*tod) + 1;
        *tod++ = byte2bcd(t % 10);
        if (t >= 10)
        {
            t = bcd2byte(*tod) + 1;
            *tod++ = byte2bcd(t % 60);
            if (t >= 60)
            {
                t = bcd2byte(*tod) + 1;
                *tod++ = byte2bcd(t % 60);
                if (t >= 60)
                {
                    uint8_t pm = *tod & 0x80;
                    t = *tod & 0x1f;
                    if (t == 0x11)
                        pm ^= 0x80; // toggle am/pm on 0:59->1:00 hr
                    if (t == 0x12)
                        t = 1;
                    else if (++t == 10)
                        t = 0x10;   // increment, adjust bcd
                    t &= 0x1f;
                    *tod = t | pm;
                }
            }
        }
        // check alarm
        if (!memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
            trigger (INTERRUPT_ALARM);
    }
}