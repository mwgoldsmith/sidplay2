/***************************************************************************
                          player.cpp  -  Frontend Player
                             -------------------
    begin                : Sun Oct 7 2001
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
 *  Revision 1.9  2002/01/16 19:28:55  s_a_white
 *  Now now wraps at 100th minute.
 *
 *  Revision 1.8  2002/01/10 19:39:46  s_a_white
 *  Fixed default to switch to please solaris compiler.
 *
 *  Revision 1.7  2001/12/11 19:38:13  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.6  2001/12/07 18:22:33  s_a_white
 *  Player quit fixes.
 *
 *  Revision 1.5  2001/12/05 22:22:48  s_a_white
 *  Added playerFast flag.
 *
 *  Revision 1.4  2001/12/03 20:00:24  s_a_white
 *  sidSamples no longer forced for hardsid.
 *
 *  Revision 1.3  2001/12/03 19:17:34  s_a_white
 *  Corrected spelling of BUILDER.
 *
 *  Revision 1.2  2001/12/01 20:16:23  s_a_white
 *  Player changed to ConsolePlayer.
 *
 *  Revision 1.1  2001/11/27 19:10:44  s_a_white
 *  Initial Release.
 *
 ***************************************************************************/

#include "config.h"

#ifdef HAVE_EXCEPTIONS
#   include <new.h>
#endif

#include "player.h"
#include "keyboard.h"

#ifdef HAVE_RESID_BUILDER
#   include <sidplay/builders/resid.h>
const char ConsolePlayer::RESID_ID[]   = "ReSID";
#endif
#ifdef HAVE_HARDSID_BUILDER
#   include <sidplay/builders/hardsid.h>
const char ConsolePlayer::HARDSID_ID[] = "HardSID";
#endif


ConsolePlayer::ConsolePlayer (const char * const name)
:Event("External Timer\n"),
 m_name(name),
 m_tune(0),
 m_state(playerStopped),
 m_outfile(NULL),
 m_context(NULL),
 m_quietLevel(0),
 m_verboseLevel(0)
{   // Other defaults
    m_filter.enabled = true;
    m_driver.device  = NULL;
    m_timer.start    = 0;
    m_timer.length   = 0; // FOREVER
    m_timer.valid    = false;
    m_track.first    = 0;
    m_track.selected = 0;
    m_track.loop     = false;
    m_track.single   = false;
    m_speed.current  = 1;
    m_speed.max      = 32;

    // Read default configuration
    m_iniCfg.read ();
    m_engCfg = m_engine.config ();

    {   // Load ini settings
        IniConfig::audio_section     audio     = m_iniCfg.audio();
        IniConfig::emulation_section emulation = m_iniCfg.emulation();

        // INI Configuration Settings
        m_engCfg.clockForced  = emulation.clockForced;
        m_engCfg.clockSpeed   = emulation.clockSpeed;
        m_engCfg.frequency    = audio.frequency;
        m_engCfg.optimisation = emulation.optimiseLevel;
        m_engCfg.playback     = audio.playback;
        m_engCfg.precision    = audio.precision;
        m_engCfg.sidModel     = emulation.sidModel;
        m_engCfg.sidSamples   = emulation.sidSamples;
        m_filter.enabled      = emulation.filter;
    }

    // Copy default setting to audio configuration
    m_driver.cfg.channels = 1; // Mono
    if (m_engCfg.playback == sid2_stereo)
        m_driver.cfg.channels = 2;
    m_driver.cfg.frequency = m_engCfg.frequency;
    m_driver.cfg.precision = m_engCfg.precision;

    createOutput (OUT_NULL, NULL);
    createSidEmu (EMU_NONE);
}


// Create the output object to process sound buffer
bool ConsolePlayer::createOutput (OUTPUTS driver, const SidTuneInfo *tuneInfo)
{
    char *name = NULL;
    const char *title = m_outfile;

    // Remove old audio driver
    m_driver.null.close ();
    m_driver.selected = &m_driver.null;
    if (m_driver.device != NULL)
    {
        if (m_driver.device != &m_driver.null)
            delete m_driver.device;
        m_driver.device = NULL;         
    }

    // Create audio driver
    switch (driver)
    {
    case OUT_NULL:
        m_driver.device = &m_driver.null;
        title = "";
    break;

    case OUT_SOUNDCARD:
#ifdef HAVE_EXCEPTIONS
        m_driver.device = new(nothrow) AudioDriver;
#else
        m_driver.device = new AudioDriver;
#endif
    break;

    case OUT_WAV:
#ifdef HAVE_EXCEPTIONS
        m_driver.device = new(nothrow) WavFile;
#else
        m_driver.device = new WavFile;
#endif
    break;

    default:
        break;
    }

    // Audio driver failed
    if (!m_driver.device)
    {
        m_driver.device = &m_driver.null;
        displayError (ERR_NOT_ENOUGH_MEMORY);
        return false;
    }

    // Generate a name for the wav file
    if (title == NULL)
    {
        size_t length, i;
        title  = tuneInfo->dataFileName;
        length = strlen (title);
        i      = length;
        while (i > 0)
        {
            if (title[--i] == '.')
                break;
        }
        if (!i) i = length;
    
#ifdef HAVE_EXCEPTIONS
        name = new(nothrow) char[i + 10];
#else
        name = new char[i + 10];
#endif
        if (!name)
        {
            displayError (ERR_NOT_ENOUGH_MEMORY);
            return false;
        }

        strcpy (name, title);
        // Prevent extension ".sid.wav"
        name[i] = '\0';

        // Change name based on subtune
        if (tuneInfo->songs > 1)
            sprintf (&name[i], "[%u]", tuneInfo->currentSong);
        strcat (&name[i], m_driver.device->extension ());
        title = name;
    }

    // Configure with user settings
    m_driver.cfg.frequency = m_engCfg.frequency;
    m_driver.cfg.precision = m_engCfg.precision;
    m_driver.cfg.channels  = 1; // Mono
    if (m_engCfg.playback == sid2_stereo)
        m_driver.cfg.channels = 2;

    {   // Open the hardware
        bool err = false;
        if (m_driver.device->open (m_driver.cfg, title) == NULL)
            err = true;
        // Can't open the same driver twice
        if (driver != OUT_NULL)
        {
            if (m_driver.null.open (m_driver.cfg, title) == NULL)
                err = true;;
        }
        if (name != NULL)
            delete [] name;
        if (err)
            return false;
    }

    // See what we got
    m_engCfg.frequency = m_driver.cfg.frequency;
    m_engCfg.precision = m_driver.cfg.precision;
    switch (m_driver.cfg.channels)
    {
    case 1:
        if (m_engCfg.playback == sid2_stereo)
            m_engCfg.playback  = sid2_mono;
        break;
    case 2:
        if (m_engCfg.playback != sid2_stereo)
            m_engCfg.playback  = sid2_stereo;
        break;
    default:
        cerr << m_name << "\n" << "ERROR: " << m_driver.cfg.channels
             << " audio channels not supported" << endl;
        return false;
    }
    return true;
}


// Create the sid emulation
bool ConsolePlayer::createSidEmu (SIDEMUS emu)
{
    // Remove old driver and emulation
    if (m_engCfg.sidEmulation)
    {
        sidbuilder *builder   = m_engCfg.sidEmulation;
        m_engCfg.sidEmulation = NULL;
        m_engine.config (m_engCfg);
        delete builder;
    }

    // Now setup the sid emulation
    switch (emu)
    {
#ifdef HAVE_RESID_BUILDER
    case EMU_RESID:
    {
#ifdef HAVE_EXCEPTIONS
        ReSIDBuilder *rs = new(nothrow) ReSIDBuilder( RESID_ID );
#else
        ReSIDBuilder *rs = new ReSIDBuilder( RESID_ID );
#endif
        if (rs && *rs)
        {
            m_engCfg.sidEmulation = rs;
            // Setup the emulation
            rs->create ((m_engine.info ()).maxsids);
            if (!*rs) goto createSidEmu_error;
            rs->filter (m_filter.enabled);
            if (!*rs) goto createSidEmu_error;
            rs->sampling (m_driver.cfg.frequency);
            if (!*rs) goto createSidEmu_error;
            if (m_filter.enabled && m_filter.definition)
            {   // Setup filter
                rs->filter (m_filter.definition);
                if (!*rs) goto createSidEmu_error;
            }
        }
        break;
    }
#endif // HAVE_RESID_BUILDER

#ifdef HAVE_HARDSID_BUILDER
    case EMU_HARDSID:
    {
#ifdef HAVE_EXCEPTIONS
        HardSIDBuilder *hs = new(nothrow) HardSIDBuilder( HARDSID_ID );
#else
        HardSIDBuilder *hs = new HardSIDBuilder( HARDSID_ID );
#endif
        if (hs && *hs)
        {
            m_engCfg.sidEmulation = hs;
            // Setup the emulation
            hs->create ((m_engine.info ()).maxsids);
            if (!*hs) goto createSidEmu_error;
            hs->filter (m_filter.enabled);
            if (!*hs) goto createSidEmu_error;
        }
        break;
    }
#endif // HAVE_HARDSID_BUILDER

    default:
        // Emulation Not yet handled
        // This default case results in the default
        // emulation
        break;
    }

    if (!m_engCfg.sidEmulation)
    {
        if (emu > EMU_DEFAULT)
        {   // No sid emulation?
            displayError (ERR_NOT_ENOUGH_MEMORY);
            return false;
        }
    }
    return true;

createSidEmu_error:
    displayError (m_engCfg.sidEmulation->error ());
    delete m_engCfg.sidEmulation;
    m_engCfg.sidEmulation = NULL;
    return false;
}


bool ConsolePlayer::open (void)
{
    const SidTuneInfo *tuneInfo;

    if ((m_state & ~playerFast) == playerRestart)
    {
        cerr << endl << endl;
        if (m_state & playerFast)
            m_driver.selected->reset ();
        m_state = playerStopped;
    }
    
    // Select the required song
    m_track.selected = m_tune.selectSong (m_track.selected);
    if (m_engine.load (&m_tune) < 0)
    {
        cerr << m_name << "\n" << m_engine.error () << endl;
        return false;
    }

    // Get tune details
    tuneInfo = (m_engine.info ()).tuneInfo;
    if (!m_track.single)
        m_track.songs = tuneInfo->songs;
    if (!createOutput (m_driver.output, tuneInfo))
        return false;
    if (!createSidEmu (m_driver.sid))
        return false;

    // Configure engine with settings
    if (m_engine.config (m_engCfg) < 0)
    {   // Config failed
        displayError (m_engine.error ());
        return false;
    }

    // Start the player.  Do this by fast
    // forwarding to the start position
    m_driver.selected = &m_driver.null;
    m_speed.current   = m_speed.max;
    m_engine.fastForward (100 * m_speed.current);

    // As yet we don't have a required songlength
    // so try the songlength database
    if (!m_timer.valid)
    {
        int_least32_t length = m_database.length (m_tune);
        if (length > 0)
            m_timer.length = length;
    }

    // Set up the play timer
    m_context = (m_engine.info()).eventContext;
    m_timer.stop  = 0;
    m_timer.stop += m_timer.length;

    if (m_timer.valid)
    {   // Length relative to start
        m_timer.stop += m_timer.start;
    }
    else
    {   // Check to make start time dosen't exceed end
        if (m_timer.stop & (m_timer.start >= m_timer.stop))
        {
            cerr << m_name << "\n" << "ERROR: Start time exceeds song length!" << endl;
            return false;
        }
    }

    m_timer.current = ~0;
    m_state = playerRunning;

    // Update display
    menu  ();
    event ();
    return true;
}

void ConsolePlayer::close ()
{
    m_engine.stop   ();
    if (m_state == playerExit)
    {   // Natural finish
        emuflush ();
        if (m_driver.file)
            cerr << (char) 7; // Bell
    }
    else // Destroy buffers
        m_driver.selected->reset ();

    // Shutdown drivers, etc
    createOutput    (OUT_NULL, NULL);
    createSidEmu    (EMU_NONE);
    m_engine.load   (NULL);
    m_engine.config (m_engCfg);

    // Correctly leave ansi mode and get prompt to
    // end up in a suitable location
#ifndef HAVE_MSWINDOWS
    cerr << endl;
#endif
    if ((m_iniCfg.console ()).ansi)
        cerr << '\x1b' << "[0m";
    cerr << endl;
}

// Flush any hardware sid fifos so all music is played
void ConsolePlayer::emuflush ()
{
    switch (m_driver.sid)
    {
#ifdef HAVE_HARDSID_BUILDER
    case EMU_HARDSID:
        ((HardSIDBuilder *)m_engCfg.sidEmulation)->flush ();
        break;
#endif // HAVE_HARDSID_BUILDER
    default:
        break;
    }
}


// Out play loop to be externally called
bool ConsolePlayer::play ()
{
    void *buffer = m_driver.selected->buffer ();
    uint_least32_t length = m_driver.cfg.bufSize;

    if (m_state == playerRunning)
    {
        // Fill buffer
        uint_least32_t ret;
        ret = m_engine.play (buffer, length);
        if (ret < length)
        {
            if (m_engine.state () != sid2_stopped)
            {
                m_state = playerError;
                return false;
            }
            return false;
        }
    }

    switch (m_state)
    {
    case playerRunning:
        m_driver.selected->write ();
        // Deliberate run on
    case playerPaused:
        // Check for a keypress (approx 250ms rate, but really depends
        // on music buffer sizes)
        if (_kbhit ())
            decodeKeys ();
        return true;
    default:
        break;
    }
    return false;
}


void ConsolePlayer::stop ()
{
    m_state = playerStopped;
    m_engine.stop ();
}


// External Timer Event
void ConsolePlayer::event (void)
{
    uint_least32_t seconds = m_engine.time() / 10;
    if ( !m_quietLevel )
    {
        cerr << "\b\b\b\b\b" << setw(2) << setfill('0')
             << ((seconds / 60) % 100) << ':' << setw(2)
             << setfill('0') << (seconds % 60) << flush;
    }

    if (seconds != m_timer.current)
    {
        if (seconds == m_timer.start)
        {   // Switch audio drivers.
            m_driver.selected = m_driver.device;
            memset (m_driver.selected->buffer (), 0, m_driver.cfg.bufSize);
            m_speed.current = 1;
            m_engine.fastForward (100);
            m_engine.debug (true);
        }
        else if (m_timer.stop && (seconds == m_timer.stop))
        {
            m_state = playerExit;
            if (!m_driver.file)
            {
                for (;;)
                {
                    if (m_track.single)
                        break;
                    // Move to next track
                    m_track.selected++;
                    if (m_track.selected > m_track.songs)
                        m_track.selected = 1;
                    if (m_track.selected == m_track.first)
                        break;
                    m_state = playerRestart;
                    break;
                }
                if (m_track.loop)
                    m_state = playerRestart;
            }
        }
        m_timer.current = seconds;
    }
    
    // Units in C64 clock cycles
    m_context->schedule (this, 900000);
}


void ConsolePlayer::displayError (const char *error)
{
    cerr << m_name << endl;
    cerr << error << endl;
}


// Keyboard handling
void ConsolePlayer::decodeKeys ()
{
    int action;

    do
    {
        action = keyboard_decode ();
        if (action == A_INVALID)
            continue;

        switch (action)
        {
        case A_RIGHT_ARROW:
            m_state = playerFastRestart;
            if (!m_track.single)
            {
                m_track.selected++;
                if (m_track.selected > m_track.songs)
                    m_track.selected = 1;
            }
        break;

        case A_LEFT_ARROW:
            m_state = playerFastRestart;
            if (!m_track.single)
            {
                m_track.selected--;
                if (m_track.selected < 1)
                    m_track.selected = m_track.songs;
            }
        break;

        case A_UP_ARROW:
            m_speed.current *= 2;
            if (m_speed.current > m_speed.max)
                m_speed.current = m_speed.max;
            m_engine.fastForward (100 * m_speed.current);
        break;

        case A_DOWN_ARROW:
            m_speed.current = 1;
            m_engine.fastForward (100);
        break;

        case A_HOME:
            m_state = playerFastRestart;
            m_track.selected = 1;
        break;

        case A_END:
            m_state = playerFastRestart;
            m_track.selected = m_track.songs;
        break;

        case A_PAUSED:
            if (m_state == playerPaused)
            {
                cerr << "\b\b\b\b\b\b\b\b\b";
                // Just to make sure PAUSED is removed from screen
                cerr << "         ";
                cerr << "\b\b\b\b\b\b\b\b\b";
                m_state  = playerRunning;
            }
            else
            {
                cerr << " [PAUSED]";
                m_state = playerPaused;
                m_driver.selected->pause ();
            }
        break;

        case A_QUIT:
            m_state = playerFastExit;
            return;
        break;
        }
    } while (_kbhit ());
}
