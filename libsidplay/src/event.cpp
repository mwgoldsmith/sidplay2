/***************************************************************************
                          event.cpp  -  Event schdeduler (based on alarm
                                        from Vice)
                             -------------------
    begin                : Wed May 9 2001
    copyright            : (C) 2001 by Simon White
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


#include "event.h"

// Usefull to prevent clock overflowing
void EventContext::timeWarp (int cycles)
{
    Event *e = m_pendingEvents;
    while (e != NULL)
    {
        e->m_clk += cycles;
        e = e->m_next;
    }
    m_pendingEventClk += cycles;
    m_eventClk        += cycles;
    // Re-schedule the next timeWarp
    schedule (&m_timeWarp, 0xfffff);
}

void EventContext::reset (void)
{    // Remove all events
    Event *e = m_pendingEvents;
    while (e != NULL)
    {
        e->m_pending = false;
        e = e->m_next;
    }
    m_pendingEvents   = NULL;
    m_pendingEventClk = m_eventClk = m_schedClk = 0;
}

// Add event to ordered pending queue
void EventContext::schedule (Event *event, event_clock_t cycles)
{
    uint clk = m_eventClk + cycles;
    if (event->m_pending)
        cancel (event);
    event->m_pending = true;
    event->m_clk     = clk;

    {   // Now put in the correct place so we don't need to keep
        // searching the list later.
        Event *e     = m_pendingEvents;
        Event *ePrev = NULL;
        for (;e != NULL; ePrev = e, e = e->m_next)
        {
            if (e->m_clk > clk)
                break;
        }
        event->m_next = e;
        event->m_prev = NULL;

        if (e != NULL)
            e->m_prev = event;
        if (ePrev != NULL)
        {
            ePrev->m_next = event;
            event->m_prev = ePrev;
        }
        else
        {   // At front
            m_pendingEventClk = clk;
            m_pendingEvents   = event;
        }
    }
}

// Cancel a pending event
void EventContext::cancel (Event *event)
{
    if (event == m_pendingEvents)
    {
        event->m_pending = false;
        m_pendingEvents  = event->m_next;
        if (m_pendingEvents != NULL)
        {
            m_pendingEvents->m_prev = NULL;
            m_pendingEventClk = m_pendingEvents->m_clk;
        }
    }
    else if (event->m_pending)
    {
        event->m_pending = false;
        // Remove event from pending list
        if (event->m_prev)
            event->m_prev->m_next = event->m_next;
        if (event->m_next)
            event->m_next->m_prev = event->m_prev;
    }
}
