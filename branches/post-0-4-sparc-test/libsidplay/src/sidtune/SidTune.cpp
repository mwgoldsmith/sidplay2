/*
 * /home/ms/files/source/libsidtune/RCS/SidTune.cpp,v
 *
 * Copyright (C) Michael Schwendt <mschwendt@yahoo.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SidTune.h"
#include "SidTuneTools.h"
#include "PP20.h"

#ifdef SID_HAVE_EXCEPTIONS
#include <new>
#endif
#include <iostream.h>
#include <iomanip.h>
#include <string.h>
#include <limits.h>

const char _sidtune_txt_songNumberExceed[] = "SIDTUNE WARNING: Selected song number was too high";
const char _sidtune_txt_wrappedSid[] = "SIDTUNE WARNING: End of data wrapped to beginning of memory";
const char _sidtune_txt_empty[] = "SIDTUNE ERROR: No data to load";
const char _sidtune_txt_unrecognizedFormat[] = "SIDTUNE ERROR: Could not determine file format";
const char _sidtune_txt_noDataFile[] = "SIDTUNE ERROR: Did not find the corresponding data file";
const char _sidtune_txt_notEnoughMemory[] = "SIDTUNE ERROR: Not enough free memory";
const char _sidtune_txt_cantLoadFile[] = "SIDTUNE ERROR: Could not load input file";
const char _sidtune_txt_cantOpenFile[] = "SIDTUNE ERROR: Could not open file for binary input";
const char _sidtune_txt_fileTooLong[] = "SIDTUNE ERROR: Input data too long";
const char _sidtune_txt_dataTooLong[] = "SIDTUNE ERROR: Music data size exceeds C64 memory";
const char _sidtune_txt_cantCreateFile[] = "SIDTUNE ERROR: Could not create output file";
const char _sidtune_txt_fileIoError[] = "SIDTUNE ERROR: File I/O error";
const char _sidtune_txt_PAL_VBI[] = "50 Hz VBI (PAL)";
const char _sidtune_txt_PAL_CIA[] = "CIA 1 Timer A (PAL)";
const char _sidtune_txt_NTSC_VBI[] = "60 Hz VBI (NTSC)";
const char _sidtune_txt_NTSC_CIA[] = "CIA 1 Timer A (NTSC)";
const char _sidtune_txt_noErrors[] = "No errors";
const char _sidtune_txt_na[] = "N/A";

// Default sidtune file name extensions. This selection can be overriden
// by specifying a custom list in the constructor.
const char* defaultFileNameExt[] =
{
	// Preferred default file extension for single-file sidtunes
	// or sidtune description files in SIDPLAY INFOFILE format.
	".sid",
	// Common file extension for single-file sidtunes due to SIDPLAY/DOS
	// displaying files *.DAT in its file selector by default.
	// Originally this was intended to be the extension of the raw data file
	// of two-file sidtunes in SIDPLAY INFOFILE format.
	".dat",
	// Extension of Amiga Workbench tooltype icon info files, which
	// have been cut to MS-DOS file name length (8.3).
	".inf",
	// No extension for the raw data file of two-file sidtunes in
	// PlaySID Amiga Workbench tooltype icon info format.
	"",
	// Common upper-case file extensions from MS-DOS (unconverted).
	".DAT", ".SID", ".INF",
	// File extensions used (and created) by various C64 emulators and
	// related utilities. These extensions are recommended to be used as
	// a replacement for ".dat" in conjunction with two-file sidtunes.
	".c64", ".prg", ".C64", ".PRG",
	// Uncut extensions from Amiga.
	".info", ".INFO", ".data", ".DATA",
	// Stereo Sidplayer (.mus/.MUS ought not be included because
	// these must be loaded first; it sometimes contains the first
	// credit lines of a MUS/STR pair).
	".str", ".STR", ".mus", ".MUS",
	// End.
	0
};

const char** SidTune::fileNameExtensions = defaultFileNameExt;

inline void SidTune::setFileNameExtensions(const char **fileNameExt)
{
	fileNameExtensions = ((fileNameExt!=0)?fileNameExt:defaultFileNameExt);
}

SidTune::SidTune(const char* fileName, const char **fileNameExt,
				 const bool separatorIsSlash)
{
	init();
	isSlashedFileName = separatorIsSlash;
	setFileNameExtensions(fileNameExt);
#if !defined(SIDTUNE_NO_STDIN_LOADER)
	// Filename ``-'' is used as a synonym for standard input.
	if ( fileName!=0 && (strcmp(fileName,"-")==0) )
	{
		getFromStdIn();
	}
	else
#endif
	if (fileName != 0)
	{
		getFromFiles(fileName);
	}
}

SidTune::SidTune(const ubyte_sidt* data, const udword_sidt dataLen)
{
	init();
	getFromBuffer(data,dataLen);
}

SidTune::~SidTune()
{
	cleanup();
}

bool SidTune::load(const char* fileName, const bool separatorIsSlash)
{
	cleanup();
	init();
	isSlashedFileName = separatorIsSlash;
	getFromFiles(fileName);
	return status;
}

bool SidTune::read(const ubyte_sidt* data, udword_sidt dataLen)
{
	cleanup();
	init();
	getFromBuffer(data,dataLen);
	return status;
}

const SidTuneInfo& SidTune::operator[](const uword_sidt songNum)
{
	selectSong(songNum);
	return info;
}

void SidTune::getInfo(SidTuneInfo& outInfo)
{
	outInfo = info;  // copy
}

const SidTuneInfo& SidTune::getInfo()
{
	return info;
}

// First check, whether a song is valid. Then copy any song-specific
// variable information such a speed/clock setting to the info structure.
uword_sidt SidTune::selectSong(const uword_sidt selectedSong)
{
	if ( !status )
		return 0;
	else
		info.statusString = _sidtune_txt_noErrors;
		
	uword_sidt song = selectedSong;
	// Determine and set starting song number.
	if (selectedSong == 0)
		song = info.startSong;
	if (selectedSong>info.songs || selectedSong>SIDTUNE_MAX_SONGS)
	{
		song = info.startSong;
		info.statusString = _sidtune_txt_songNumberExceed;
	}
	info.currentSong = song;
	info.songLength = songLength[song-1];
	// Retrieve song speed definition.
	info.songSpeed = songSpeed[song-1];
	info.clockSpeed = clockSpeed[song-1];
	// Assign song speed description string depending on clock speed.
	if (info.clockSpeed == SIDTUNE_CLOCK_PAL)
	{
		if (info.songSpeed == SIDTUNE_SPEED_VBI)
			info.speedString = _sidtune_txt_PAL_VBI;
		else  //if info.songSpeed == SIDTUNE_SPEED_CIA
			info.speedString = _sidtune_txt_PAL_CIA;
	}
	else  //if (info.clockSpeed == SIDTUNE_CLOCK_NTSC)
	{
		if (info.songSpeed == SIDTUNE_SPEED_VBI)
			info.speedString = _sidtune_txt_NTSC_VBI;
		else  //if info.songSpeed == SIDTUNE_SPEED_CIA
			info.speedString = _sidtune_txt_NTSC_CIA;
	}
	return info.currentSong;
}

void SidTune::fixLoadAddress(bool force, uword_sidt init, uword_sidt play)
{
	if (info.fixLoad || force)
	{
		info.fixLoad = false;
		info.loadAddr += 2;
		fileOffset += 2;

		if (force)
		{
			info.initAddr = init;
			info.playAddr = play;
		}
	}
}

// ------------------------------------------------- private member functions

bool SidTune::placeSidTuneInC64mem(ubyte_sidt* c64buf)
{
	if ( status && c64buf!=0 )
	{
		udword_sidt endPos = info.loadAddr + info.c64dataLen;
		if (endPos <= SIDTUNE_MAX_MEMORY)
		{
			// Copy data from cache to the correct destination.
			memcpy(c64buf+info.loadAddr,cache.get()+fileOffset,info.c64dataLen);
			info.statusString = _sidtune_txt_noErrors;
		}
		else
		{
			// Security - split data which would exceed the end of the C64 memory.
			// Memcpy could not detect this.
			memcpy(c64buf+info.loadAddr,cache.get()+fileOffset,info.c64dataLen-(endPos-SIDTUNE_MAX_MEMORY));
			// Wrap the remaining data to the start address of the C64 memory.
			memcpy(c64buf,cache.get()+fileOffset+info.c64dataLen-(endPos-SIDTUNE_MAX_MEMORY),(endPos-SIDTUNE_MAX_MEMORY));
			info.statusString = _sidtune_txt_wrappedSid;
		}
		if (info.musPlayer)
		{
			MUS_installPlayer(c64buf);
		}
	}
	return ( status && c64buf!=0 );
}

bool SidTune::loadFile(const char* fileName, Buffer_sidtt<const ubyte_sidt>& bufferRef)
{
	Buffer_sidtt<ubyte_sidt> fileBuf;
	udword_sidt fileLen = 0;
	
	// Open binary input file stream at end of file.
#if defined(SID_HAVE_IOS_BIN)
	ifstream myIn(fileName,ios::in|ios::bin|ios::ate|ios::nocreate);
#else
	ifstream myIn(fileName,ios::in|ios::binary|ios::ate|ios::nocreate);
#endif
	// As a replacement for !is_open(), bad() and the NOT-operator don't seem
	// to work on all systems.
#if defined(SID_DONT_HAVE_IS_OPEN)
	if ( !myIn )
#else
	if ( !myIn.is_open() )
#endif
	{
		info.statusString = _sidtune_txt_cantOpenFile;
		return false;
	}
	else
	{
#if defined(SID_HAVE_SEEKG_OFFSET)
		fileLen = (myIn.seekg(0,ios::end)).offset();
#else
		myIn.seekg(0,ios::end);
		fileLen = (udword_sidt)myIn.tellg();
#endif
#ifdef SID_HAVE_EXCEPTIONS
		if ( !fileBuf.assign(new(nothrow) ubyte_sidt[fileLen],fileLen) )
#else
		if ( !fileBuf.assign(new ubyte_sidt[fileLen],fileLen) )
#endif
		{
			info.statusString = _sidtune_txt_notEnoughMemory;
			return false;
		}
		myIn.seekg(0,ios::beg);
		udword_sidt restFileLen = fileLen;
		// 16-bit compatible loading. Is this really necessary?
		while ( restFileLen > INT_MAX )
		{
			myIn.read((char*)fileBuf.get()+(fileLen-restFileLen),INT_MAX);  // !cast!
			restFileLen -= INT_MAX;
		}
		if ( restFileLen > 0 )
		{
			myIn.read((char*)fileBuf.get()+(fileLen-restFileLen),restFileLen);  // !cast!
		}
		if ( myIn.bad() )
		{
			info.statusString = _sidtune_txt_cantLoadFile;
			return false;
		}
		else
		{
			info.statusString = _sidtune_txt_noErrors;
		}
	}
	myIn.close();
	if ( fileLen==0 )
	{
		info.statusString = _sidtune_txt_empty;
		return false;
	}
	
	// Check for PowerPacker compression: load and decompress, if PP20 file.
	PP20 myPP;
	if ( myPP.isCompressed(fileBuf.get(),fileBuf.len()) )
	{
		ubyte_sidt* destBufRef = 0;
		if ( 0 == (fileLen = myPP.decompress(fileBuf.get(),fileBuf.len(),
											 &destBufRef)) )
		{
			info.statusString = myPP.getStatusString();
			return false;
		}
		else
		{
			info.statusString = myPP.getStatusString();
			// Replace compressed buffer with uncompressed buffer.
			fileBuf.assign(destBufRef,fileLen);
		}
	}

	bufferRef.assign(fileBuf.xferPtr(),fileBuf.xferLen());

	return true;
}

void SidTune::deleteFileNameCopies()
{
	// When will it be fully safe to call delete[](0) on every system?
	if ( info.dataFileName != 0 )
		delete[] info.dataFileName;
	if ( info.infoFileName != 0 )
		delete[] info.infoFileName;
	if ( info.path != 0 )
		delete[] info.path;
	info.dataFileName = 0;
	info.infoFileName = 0;
	info.path = 0;
}

void SidTune::init()
{
	// Initialize the object with some safe defaults.
	status = false;

	info.statusString = _sidtune_txt_na;
	info.path = info.infoFileName = info.dataFileName = 0;
	info.dataFileLen = info.c64dataLen = 0;
	info.formatString = _sidtune_txt_na;
	info.speedString = _sidtune_txt_na;
	info.loadAddr = ( info.initAddr = ( info.playAddr = 0 ));
	info.songs = ( info.startSong = ( info.currentSong = 0 ));
	info.sidChipBase1 = 0xd400;
	info.sidChipBase2 = 0;
	info.musPlayer = false;
	info.fixLoad = false;
	info.songSpeed = SIDTUNE_SPEED_VBI;
	info.clockSpeed = SIDTUNE_CLOCK_PAL;
	info.songLength = 0;
	
	for ( uword_sidt si = 0; si < SIDTUNE_MAX_SONGS; si++ )
	{
		songSpeed[si] = SIDTUNE_SPEED_VBI;
		clockSpeed[si] = SIDTUNE_CLOCK_PAL;
		songLength[si] = 0;
	}

	fileOffset = 0;
	musDataLen = 0;
	
	for ( uword_sidt sNum = 0; sNum < SIDTUNE_MAX_CREDIT_STRINGS; sNum++ )
	{
		for ( uword_sidt sPos = 0; sPos < SIDTUNE_MAX_CREDIT_STRLEN; sPos++ )
		{
			infoString[sNum][sPos] = 0;
		}
	}
	info.numberOfInfoStrings = 0;

	// Not used!!!
	info.numberOfCommentStrings = 1;
#ifdef SID_HAVE_EXCEPTIONS
	info.commentString = new(nothrow) char* [info.numberOfCommentStrings];
#else
	info.commentString = new char* [info.numberOfCommentStrings];
#endif
	if (info.commentString != 0)
		info.commentString[0] = SidTuneTools::myStrDup("--- SAVED WITH SIDPLAY ---");
	else
		info.commentString[0] = 0;
}

void SidTune::cleanup()
{
	// Remove copy of comment field.
	udword_sidt strNum = 0;
	// Check and remove every available line.
	while (info.numberOfCommentStrings-- > 0)
	{
		if (info.commentString[strNum] != 0)
		{
			delete[] info.commentString[strNum];
			info.commentString[strNum] = 0;
		}
		strNum++;  // next string
	};
	delete[] info.commentString;  // free the array pointer

	deleteFileNameCopies();

	status = false;
}

#if !defined(SIDTUNE_NO_STDIN_LOADER)

void SidTune::getFromStdIn()
{
	// Assume a failure, so we can simply return.
	status = false;
	// Assume the memory allocation to fail.
	info.statusString = _sidtune_txt_notEnoughMemory;
	ubyte_sidt* fileBuf;
#ifdef SID_HAVE_EXCEPTIONS
	if ( 0 == (fileBuf = new(nothrow) ubyte_sidt[SIDTUNE_MAX_FILELEN]) )
#else
	if ( 0 == (fileBuf = new ubyte_sidt[SIDTUNE_MAX_FILELEN]) )
#endif
	{
		return;
	}
	// We only read as much as fits in the buffer.
	// This way we avoid choking on huge data.
	udword_sidt i = 0;
	ubyte_sidt datb;
	while (cin.get(datb) && i<SIDTUNE_MAX_FILELEN)
		fileBuf[i++] = datb;
	info.dataFileLen = i;
	getFromBuffer(fileBuf,info.dataFileLen);
	delete[] fileBuf;
}

#endif

void SidTune::getFromBuffer(const ubyte_sidt* const buffer, const udword_sidt bufferLen)
{
	// Assume a failure, so we can simply return.
	status = false;

	if (buffer==0 || bufferLen==0)
	{
		info.statusString = _sidtune_txt_empty;
		return;
	}
	else if (bufferLen > SIDTUNE_MAX_FILELEN)
	{
		info.statusString = _sidtune_txt_fileTooLong;
		return;
	}

	ubyte_sidt* tmpBuf;
#ifdef SID_HAVE_EXCEPTIONS
	if ( 0 == (tmpBuf = new(nothrow) ubyte_sidt[bufferLen]) )
#else
	if ( 0 == (tmpBuf = new ubyte_sidt[bufferLen]) )
#endif
	{
		info.statusString = _sidtune_txt_notEnoughMemory;
		return;
	}
	memcpy(tmpBuf,buffer,bufferLen);

	Buffer_sidtt<const ubyte_sidt> buf1(tmpBuf,bufferLen);
	Buffer_sidtt<const ubyte_sidt> buf2;  // empty

	bool foundFormat = false;
	// Here test for the possible single file formats. --------------
	if ( PSID_fileSupport( buffer, bufferLen ))
	{
		foundFormat = true;
	}
	else if ( MUS_fileSupport(buf1,buf2) )
	{
		foundFormat = MUS_mergeParts(buf1,buf2);
	}
	else
	{
		// No further single-file-formats available.
		info.formatString = _sidtune_txt_na;
		info.statusString = _sidtune_txt_unrecognizedFormat;
	}
	if ( foundFormat )
	{
		status = acceptSidTune("-","-",buf1);
	}
}

bool SidTune::acceptSidTune(const char* dataFileName, const char* infoFileName,
							Buffer_sidtt<const ubyte_sidt>& buf)
{
	deleteFileNameCopies();
	// Make a copy of the data file name and path, if available.
	if ( dataFileName != 0 )
	{
		info.path = SidTuneTools::myStrDup(dataFileName);
		if (isSlashedFileName)
		{
			info.dataFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(info.path));
			*SidTuneTools::slashedFileNameWithoutPath(info.path) = 0;  // path only
		}
		else
		{
			info.dataFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(info.path));
			*SidTuneTools::fileNameWithoutPath(info.path) = 0;  // path only
		}
		if ((info.path==0) || (info.dataFileName==0))
		{
			info.statusString = _sidtune_txt_notEnoughMemory;
			return false;
		}
	}
	else
	{
		// Provide empty strings.
		info.path = SidTuneTools::myStrDup("");
		info.dataFileName = SidTuneTools::myStrDup("");
	}
	// Make a copy of the info file name, if available.
	if ( infoFileName != 0 )
	{
		char* tmp = SidTuneTools::myStrDup(infoFileName);
		if (isSlashedFileName)
			info.infoFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(tmp));
		else
			info.infoFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(tmp));
		if ((tmp==0) || (info.infoFileName==0))
		{
			info.statusString = _sidtune_txt_notEnoughMemory;
			return false;
		}
		delete[] tmp;
	}
	else
	{
		// Provide empty string.
		info.infoFileName = SidTuneTools::myStrDup("");
	}
	// Fix bad sidtune set up.
	if (info.songs > SIDTUNE_MAX_SONGS)
		info.songs = SIDTUNE_MAX_SONGS;
	else if (info.songs == 0)
		info.songs++;
	if (info.startSong > info.songs)
		info.startSong = 1;
	else if (info.startSong == 0)
		info.startSong++;
	
	if ( info.musPlayer )
		MUS_setPlayerAddress();
	
	info.dataFileLen = buf.len();
	info.c64dataLen = buf.len() - fileOffset;
	
	if (info.dataFileLen >= 2)
	{
		// We only detect an offset of two. Some position independent
		// sidtunes contain a load address of 0xE000, but are loaded
		// to 0x0FFE and call player at 0x1000.
		info.fixLoad = (readLEword(buf.get()+fileOffset)==(info.loadAddr+2));
	}
	
	// Check the size of the data.
	if ( info.c64dataLen > SIDTUNE_MAX_MEMORY )
	{
		info.statusString = _sidtune_txt_dataTooLong;
		return false;
	}
	else if ( info.c64dataLen == 0 )
	{
		info.statusString = _sidtune_txt_empty;
		return false;
	}

	cache.assign(buf.xferPtr(),buf.xferLen());

	info.statusString = _sidtune_txt_noErrors;
	return true;
}

bool SidTune::createNewFileName(Buffer_sidtt<char>& destString,
								const char* sourceName,
								const char* sourceExt)
{
	Buffer_sidtt<char> newBuf;
	udword_sidt newLen = strlen(sourceName)+strlen(sourceExt)+1;
	// Get enough memory, so we can appended the extension.
#ifdef SID_HAVE_EXCEPTIONS
	newBuf.assign(new(nothrow) char[newLen],newLen);
#else
	newBuf.assign(new char[newLen],newLen);
#endif
	if ( newBuf.isEmpty() )
	{
		info.statusString = _sidtune_txt_notEnoughMemory;
		return (status = false);
	}
	strcpy(newBuf.get(),sourceName);
	strcpy(SidTuneTools::fileExtOfPath(newBuf.get()),sourceExt);
	destString.assign(newBuf.xferPtr(),newBuf.xferLen());
	return true;
}

// Initializing the object based upon what we find in the specified file.

void SidTune::getFromFiles(const char* fileName)
{
	// Assume a failure, so we can simply return.
	status = false;

	Buffer_sidtt<const ubyte_sidt> fileBuf1, fileBuf2;
	Buffer_sidtt<char> fileName2;

	// Try to load the single specified file.
	if ( loadFile(fileName,fileBuf1) )
	{
		// File loaded. Now check if it is in a valid single-file-format.
		if ( PSID_fileSupport(fileBuf1.get(),fileBuf1.len()) )
		{
			status = acceptSidTune(fileName,0,fileBuf1);
			return;
		}
		else if ( MUS_fileSupport(fileBuf1,fileBuf2) )
		{
			// Try to find second file.
			int n = 0;
			while (fileNameExtensions[n] != 0)
			{
				if ( !createNewFileName(fileName2,fileName,fileNameExtensions[n]) )
					return;
				// 1st data file was loaded into ``fileBuf1'',
				// so we load the 2nd one into ``fileBuf2''.
				// Do not load the first file again if names are equal.
				if ( stricmp(fileName,fileName2.get())!=0 &&
					 loadFile(fileName2.get(),fileBuf2) )
				{
					if ( MUS_fileSupport(fileBuf1,fileBuf2) )
					{
						if ( MUS_mergeParts(fileBuf1,fileBuf2) )
						{
							status = acceptSidTune(fileName,fileName2.get(),
												   fileBuf1);
						}
						return;  // in either case
					}
				}
				n++;
			};
			// No second file.
			status = acceptSidTune(fileName,0,fileBuf1);
			return;
		}

// -------------------------------------- Support for multiple-files formats.
		else
		{
// We cannot simply try to load additional files, if a description file was
// specified. It would work, but is error-prone. Imagine a filename mismatch
// or more than one description file (in another) format. Any other file
// with an appropriate file name can be the C64 data file.

// First we see if ``fileName'' could be a raw data file. In that case we
// have to find the corresponding description file.

			// Right now we do not have a second file. Hence the (0, 0, ...)
			// parameters are set for the data buffer. This will not hurt the
			// file support procedures.

			// Make sure that ``fileBuf1'' does not contain a description file.
			if ( !SID_fileSupport(0,0,fileBuf1.get(),fileBuf1.len()) &&
				 !INFO_fileSupport(0,0,fileBuf1.get(),fileBuf1.len()) )
			{
				// Assuming ``fileName'' to hold the name of the raw data file,
				// we now create the name of a description file (=fileName2) by
				// appending various filename extensions.

// ------------------------------------------ Looking for a description file.

				int n = 0;
				while (fileNameExtensions[n] != 0)
				{
					if ( !createNewFileName(fileName2,fileName,fileNameExtensions[n]) )
						return;
					// 1st data file was loaded into ``fileBuf1'',
					// so we load the 2nd one into ``fileBuf2''.
					// Do not load the first file again if names are equal.
					if ( stricmp(fileName,fileName2.get())!=0 &&
						 loadFile(fileName2.get(),fileBuf2) )
					{
						if ( SID_fileSupport(fileBuf1.get(),fileBuf1.len(),
											 fileBuf2.get(),fileBuf2.len())
							|| INFO_fileSupport(fileBuf1.get(),fileBuf1.len(),
											  	fileBuf2.get(),fileBuf2.len())
							)
						{
							status = acceptSidTune(fileName,fileName2.get(),
												   fileBuf1);
							return;
						}
					}
					n++;
				};

// --------------------------------------- Could not find a description file.

				info.formatString = _sidtune_txt_na;
				info.statusString = _sidtune_txt_unrecognizedFormat;
				return;
			}

// -------------------------------------------------------------------------
// Still unsuccessful ? Probably one put a description file name into
// ``fileName''. Assuming ``fileName'' to hold the name of a description
// file, we now create the name of the data file and swap both used memory
// buffers - fileBuf1 and fileBuf2 - when calling the format support.
// If it works, the second file is the data file ! If it is not, but does
// exist, we are out of luck, since we cannot detect data files.

			// Make sure ``fileBuf1'' contains a description file.
			else if ( SID_fileSupport(0,0,fileBuf1.get(),fileBuf1.len()) ||
					  INFO_fileSupport(0,0,fileBuf1.get(),fileBuf1.len()) )
			{

// --------------------- Description file found. --- Looking for a data file.

				int n = 0;
				while (fileNameExtensions[n] != 0)
				{
					if ( !createNewFileName(fileName2,fileName,fileNameExtensions[n]) )
						return;
					// 1st info file was loaded into ``fileBuf'',
					// so we load the 2nd one into ``fileBuf2''.
					// Do not load the first file again if names are equal.
					if ( stricmp(fileName,fileName2.get())!=0 &&

						loadFile(fileName2.get(),fileBuf2) )
					{
// -------------- Some data file found, now identifying the description file.

						if ( SID_fileSupport(fileBuf2.get(),fileBuf2.len(),
											 fileBuf1.get(),fileBuf1.len())
							|| INFO_fileSupport(fileBuf2.get(),fileBuf2.len(),
												fileBuf1.get(),fileBuf1.len())
							)
						{
							status = acceptSidTune(fileName2.get(),fileName,
												   fileBuf2);
							return;
						}
					}
					n++;
				};
				
// ---------------------------------------- No corresponding data file found.

				info.formatString = _sidtune_txt_na;
				info.statusString = _sidtune_txt_noDataFile;
				return;
			} // end else if ( = is description file )

// --------------------------------- Neither description nor data file found.

			else
			{
				info.formatString = _sidtune_txt_na;
				info.statusString = _sidtune_txt_unrecognizedFormat;
				return;
			}
		} // end else ( = is no singlefile )

// ---------------------------------------------------------- File I/O error.

	} // if loaddatafile
	else
	{
		// returned fileLen was 0 = error. The info.statusString is
		// already set then.
		info.formatString = _sidtune_txt_na;
		return;
	}
} 

void SidTune::convertOldStyleSpeedToTables(udword_sidt oldStyleSpeed)
{
	// Create the speed/clock setting tables.
	//
	// This does not take into account the PlaySID bug upon evaluating the
	// SPEED field. It would most likely break compatibility to lots of
	// sidtunes, which have been converted from .SID format and vice versa.
	// The .SID format does the bit-wise/song-wise evaluation of the SPEED
	// value correctly, like it is described in the PlaySID documentation.

	int toDo = ((info.songs <= SIDTUNE_MAX_SONGS) ? info.songs : SIDTUNE_MAX_SONGS);
	for (int s = 0; s < toDo; s++)
	{
		if (( (oldStyleSpeed>>(s&31)) & 1 ) == 0 )
		{
			songSpeed[s] = SIDTUNE_SPEED_VBI;
			clockSpeed[s] = SIDTUNE_CLOCK_PAL;
		}
		else
		{
			songSpeed[s] = SIDTUNE_SPEED_CIA_1A;
			clockSpeed[s] = SIDTUNE_CLOCK_PAL;
		}
	}
}

//
// File format conversion ---------------------------------------------------
//
				
bool SidTune::saveToOpenFile(ofstream& toFile, const ubyte_sidt* buffer,
							 udword_sidt bufLen )
{
	udword_sidt lenToWrite = bufLen;
	while ( lenToWrite > INT_MAX )
	{
		toFile.write((char*)buffer+(bufLen-lenToWrite),INT_MAX);
		lenToWrite -= INT_MAX;
	}
	if ( lenToWrite > 0 )
		toFile.write((char*)buffer+(bufLen-lenToWrite),lenToWrite);
	if ( toFile.bad() )
	{
		info.statusString = _sidtune_txt_fileIoError;
		return false;
	}
	else
	{
		info.statusString = _sidtune_txt_noErrors;
		return true;
	}
}

bool SidTune::saveC64dataFile( const char* fileName, bool overWriteFlag )
{
	bool success = false;  // assume error
	// This prevents saving from a bad object.
	if ( status )
	{
		// Open binary output file stream.
		long int createAttr;
#if defined(SID_HAVE_IOS_BIN)
		createAttr = ios::out | ios::bin;
#else
		createAttr = ios::out | ios::binary;
#endif
		if ( overWriteFlag )
			createAttr |= ios::trunc;
		else
			createAttr |= ios::noreplace;
		ofstream fMyOut( fileName, createAttr );
		if ( !fMyOut )
		{ 
			info.statusString = _sidtune_txt_cantCreateFile;
		}
		else
		{  
			// Save c64 lo/hi load address.
			ubyte_sidt saveAddr[2];
			saveAddr[0] = info.loadAddr & 255;
			saveAddr[1] = info.loadAddr >> 8;
			fMyOut.write((char*)saveAddr,2);
			// Data starts at: bufferaddr + fileOffset
			// Data length: info.dataFileLen - fileOffset
			if ( !saveToOpenFile( fMyOut,cache.get()+fileOffset, info.dataFileLen - fileOffset ) )
			{
				info.statusString = _sidtune_txt_fileIoError;
			}
			else
			{
				info.statusString = _sidtune_txt_noErrors;
				success = true;
			}
			fMyOut.close();
		}
	}
	return success;
}

bool SidTune::saveSIDfile( const char* fileName, bool overWriteFlag )
{
	bool success = false;  // assume error
	// This prevents saving from a bad object.
	if ( status )
	{
		// Open ASCII output file stream.
		long int createAttr;
		createAttr = ios::out;
		if ( overWriteFlag )
			createAttr |= ios::trunc;
		else
			createAttr |= ios::noreplace;
		ofstream fMyOut( fileName, createAttr );
		if ( !fMyOut )
		{ 
			info.statusString = _sidtune_txt_cantCreateFile;
		}
		else
		{  
			if ( !SID_fileSupportSave( fMyOut ) )
			{
				info.statusString = _sidtune_txt_fileIoError;
			}
			else
			{
				info.statusString = _sidtune_txt_noErrors;
				success = true;
			}
			fMyOut.close();
		}
	}
	return success;
}

bool SidTune::savePSIDfile( const char* fileName, bool overWriteFlag )
{
	bool success = false;  // assume error
	// This prevents saving from a bad object.
	if ( status )
	{
		// Open binary output file stream.
		long int createAttr;
#if defined(SID_HAVE_IOS_BIN)
		createAttr = ios::out | ios::bin;
#else
		createAttr = ios::out | ios::binary;
#endif
	  if ( overWriteFlag )
			createAttr |= ios::trunc;
		else
			createAttr |= ios::noreplace;
		ofstream fMyOut( fileName, createAttr );
		if ( !fMyOut )
		{
			info.statusString = _sidtune_txt_cantCreateFile;
		}
		else
		{  
			if ( !PSID_fileSupportSave( fMyOut,cache.get() ) )
			{
				info.statusString = _sidtune_txt_fileIoError;
			}
			else
			{
				info.statusString = _sidtune_txt_noErrors;
				success = true;
			}
			fMyOut.close();
		}
	}
	return success;
}
