#include "SidTuneMod.h"
#include "libini.h"

class SID_EXPORT SidDatabase
{
private:
    static const char *ERR_DATABASE_CORRUPT;
    static const char *ERR_NO_DATABASE_LOADED;
    static const char *ERR_NO_SELECTED_SONG;
    static const char *ERR_MEM_ALLOC;
    static const char *ERR_UNABLE_TO_LOAD_DATABASE;

    ini_fd_t    database;
    const char *errorString;

    int_least32_t parseTimeStamp (const char* arg);
    uint_least8_t timesFound     (char *str);

public:
    SidDatabase  () : database (0) {;}
    ~SidDatabase ();

    int           open  (const char *filename);
    void          close ();
    int_least32_t getSongLength (SidTuneMod &tune);

    const char *  getErrorString (void)
    {   return errorString; }
};
