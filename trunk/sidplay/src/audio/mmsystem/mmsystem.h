/***************************************************************************
                          mmsystem.h  -  ``Waveout for Windows''
                                         specific audio driver interface.
                             -------------------
    begin                : Fri Aug 11 2000
    copyright            : (C) 2000 by Jarno Paananen
    email                : jpaana@s2.org
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

#ifndef _audio_mmsystem_h_
#define _audio_mmsystem_h_

#include "config.h"
#ifdef   HAVE_MMSYSTEM
#define  AUDIO_HAVE_DRIVER
#define  AudioDriver Audio_MMSystem

#include <windows.h>
#include <mmsystem.h>

#include "../AudioBase.h"

class Audio_MMSystem: public AudioBase
{
private:  // ------------------------------------------------------- private
    HWAVEOUT    waveHandle;

    // Rev 1.3 (saw) - Buffer sizes adjusted to get a
    // correct playtimes
    #define  MAXBUFBLOCKS 3
    BYTE    *blocks[MAXBUFBLOCKS];
    HGLOBAL  blockHandles[MAXBUFBLOCKS];
    WAVEHDR *blockHeaders[MAXBUFBLOCKS];
    HGLOBAL  blockHeaderHandles[MAXBUFBLOCKS];
    int      blockNum;
    bool     isOpen;
    int      bufSize;

public:  // --------------------------------------------------------- public
    Audio_MMSystem();
    ~Audio_MMSystem();

    void *open  (AudioConfig &cfg);
    void  close ();
    // Rev 1.2 (saw) - Changed, see AudioBase.h	
    void *reset ();
    void *write ();
};

#endif // HAVE_MMSYSTEM
#endif // _audio_mmsystem_h_
