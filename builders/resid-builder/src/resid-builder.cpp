/***************************************************************************
         resid-builder.cpp - ReSID builder class for creating/controlling
                             resids.
                             -------------------
    begin                : Wed Sep 5 2001
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
/***************************************************************************
 *  $Log: not supported by cvs2svn $
 ***************************************************************************/

#include <stdio.h>
#include "resid.h"
#include "config.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

// Error String(s)
const char *ReSIDBuilder::ERR_FILTER_DEFINITION = "RESID ERROR: Filter definition is not valid (see docs).";

ReSIDBuilder::ReSIDBuilder (const char * const name)
:sidbuilder (name)
{
    m_error = "N/A";
}

ReSIDBuilder::~ReSIDBuilder (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.  Called by libsidplay2 only
uint ReSIDBuilder::create (uint sids)
{
	uint   count;
    ReSID *sid = NULL;
    m_status   = true;

    // Check available devices
	count = devices (false);
	if (!m_status)
		goto ReSIDBuilder_create_error;
    if (count && (count < sids))
		sids = count;

	for (count = 0; count < sids; count++)
	{
#   ifdef HAVE_EXCEPTIONS
	    sid = new(nothrow) ReSID(this);
#   else
	    sid = new ReSID(this);
#   endif

        // Memory alloc failed?
        if (!sid)
		{
			sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSID object", name ());
            m_error = m_errorBuffer;
			goto ReSIDBuilder_create_error;
		}

		// SID init failed?
		if (!*sid)
		{
			m_error = sid->error ();
			goto ReSIDBuilder_create_error;
		}
	    sidobjs.push_back (sid);
	}
    return count;

ReSIDBuilder_create_error:
    m_status = false;
    if (sid)
        delete sid;
    return count;
}

uint ReSIDBuilder::devices (bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();
    else // Available devices
		return 0;
}

void ReSIDBuilder::filter (const sid_filter_t *filter)
{
    int size = sidobjs.size ();
	m_status = true;
    for (int i = 0; i < size; i++)
	{
		ReSID *sid = (ReSID *) sidobjs[i];
        if (!sid->filter (filter))
            goto ReSIDBuilder_sidFilterDef_error;
    }
return;

ReSIDBuilder_sidFilterDef_error:
    m_error  = ERR_FILTER_DEFINITION;
    m_status = false;
}

void ReSIDBuilder::filter (bool enable)
{
    int size = sidobjs.size ();
	m_status = true;
    for (int i = 0; i < size; i++)
	{
		ReSID *sid = (ReSID *) sidobjs[i];
        sid->filter (enable);
    }
}

// Find a free SID of the required specs
sidemu *ReSIDBuilder::lock (c64env *env, sid2_model_t model)
{
    int size = sidobjs.size ();
    m_status = true;

    for (int i = 0; i < size; i++)
    {
        ReSID *sid = (ReSID *) sidobjs[i];
        if (sid->lock ())
            continue;
        sid->lock  (env);
        sid->model (model);
        return sid;
    }
    // Unable to locate free SID
    m_status = false;
    sprintf (m_errorBuffer, "%s ERROR: No available SIDs to lock", name ());
    return NULL;
}

// Allow something to use this SID
void ReSIDBuilder::unlock (sidemu *device)
{
    int size = sidobjs.size ();
    // Maek sure this is our SID
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = (ReSID *) sidobjs[i];
        if (sid != device)
            continue;
        // Unlock it
        sid->lock (NULL);
    }
}

// Remove all SID emulations.
void ReSIDBuilder::remove ()
{
    int size = sidobjs.size ();
    for (int i = 0; i < size; i++)
        delete sidobjs[i];
    sidobjs.clear();
}

void ReSIDBuilder::sampling (uint_least32_t freq)
{
    int size = sidobjs.size ();
	m_status = true;
    for (int i = 0; i < size; i++)
	{
		ReSID *sid = (ReSID *) sidobjs[i];
        sid->sampling (freq);
    }
}
