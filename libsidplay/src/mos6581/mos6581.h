/***************************************************************************
                          mos6581.h  -  Just redirects to the current SID
                                        emulation.  Currently this is reSID
                             -------------------
    begin                : Thu Jul 20 2000
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

#include "sidconfig.h"

// Rev 1.2 (saw) - Changed to allow resid to be in more than one location
#ifdef SID_HAVE_LOCAL_RESID
#   include "resid/sid.h"
#else
#   ifdef SID_HAVE_USER_RESID
#       include "sid.h"
#   else
#       include <resid/sid.h>
#   endif
#endif
