/***************************************************************************
               hardsid.h  -  Hardsid support interface.
	                     Created from Jarno's original
		             Sidplay2 patch
                             -------------------
    begin                : Fri Dec 15 2000
    copyright            : (C) 2000-2002 by Simon White
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
 *  Revision 1.5  2005/03/22 19:10:48  s_a_white
 *  Converted windows hardsid code to work with new linux streaming changes.
 *  Windows itself does not yet support streaming in the drivers for synchronous
 *  playback to multiple sids (so cannot use MK4 to full potential).
 *
 *  Revision 1.4  2004/05/05 23:47:50  s_a_white
 *  Detect available sid devices on Unix system.
 *
 *  Revision 1.3  2003/01/23 17:48:17  s_a_white
 *  Added missed return parameter for init function prototype.
 *
 *  Revision 1.2  2002/01/30 01:42:08  jpaana
 *  Don't include config.h as it isn't always available and is included elsewhere already
 *
 *  Revision 1.1  2002/01/28 22:35:20  s_a_white
 *  Initial Release.
 *
 *
 ***************************************************************************/

#ifndef  _hardsid_builder_h_
#define  _hardsid_builder_h_

#include <vector>
#include <sidplay/imp/sidbuilder.h>
#include "hardsid.h"

class HardSIDStream;

class HardSIDBuilder: public SidBuilder<IHardSIDBuilder>
{
private:
    static bool   m_initialised;
    static uint   m_count;
    char   m_errorBuffer[100];
    std::vector<HardSIDStream *> m_streams;

    int init (void);

public:
    HardSIDBuilder  (const char * const name);
    ~HardSIDBuilder (void);

    // IInterface
    void ifquery (const InterfaceID &iid, void **implementation);

    // true will give you the number of used devices.
    //    return values: 0 none, positive is used sids
    // false will give you all available sids.
    //    return values: 0 endless, positive is available sids.
    // use bool operator to determine error
    uint           devices (bool used);
    ISidEmulation *lock    (c64env *env, sid2_model_t model);
    void           unlock  (ISidEmulation *device);
    void           remove  (void);
    const char    *error   (void) const { return m_errorBuffer; }
    const char    *credits (void);
    void           flush   (void);
    void           filter  (bool enable);

    uint           create  (uint sids);
};

#endif // _hardsid_builder_h_
