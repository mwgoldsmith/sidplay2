/***************************************************************************
                          sid6526.h  -  fake CIA timer for sidplay1
                                        environment modes
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
 *  Revision 1.1  2001/09/01 11:11:19  s_a_white
 *  This is the old fake6526 code required for sidplay1 environment modes.
 *
 ***************************************************************************/

#ifndef _sid6526_h_
#define _sid6526_h_

#include "component.h"
#include "event.h"
#include "c64env.h"

class SID6526: public component
{
private:

    static const char * const credit;

    c64env       &m_env;
    EventContext &m_eventContext;
    event_clock_t m_accessClk;

    uint8_t regs[0x10];
    uint8_t cra;             // Timer A Control Register
    uint_least16_t ta_latch;
    uint_least16_t ta;       // Current count (reduces to zero)
    uint_least32_t rnd;
    uint_least16_t m_count;
    bool locked; // Prevent code changing CIA.

    class TaEvent: public Event
    {
    private:
        SID6526 &m_cia;
        void event (void) {m_cia.event ();}

    public:
        TaEvent (SID6526 &cia)
            :Event("CIA Timer A"),
             m_cia(cia) {}
    } m_taEvent;

public:
    SID6526 (c64env *env);

    //Common:
    void    reset (void);
    uint8_t read  (const uint_least8_t addr);
    void    write (const uint_least8_t addr, const uint8_t data);
    const   char *credits (void) {return credit;}
    const   char *error   (void) {return "";}

    // Specific:
    void event (void);
    void clock (uint_least16_t count) { m_count = count; }
    void lock  () { locked = true; }
};

#endif // _sid6526_h_
