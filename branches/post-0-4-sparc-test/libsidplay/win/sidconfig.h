/* Setup for Microsoft Visual C++ Version 5 */

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* SID_WORDS_BIGENDIAN, SID_WORDS_LITTLEENDIAN  */
#define SID_WORDS_LITTLEENDIAN

#define SID_SIZEOF_CHAR 1
#define SID_SIZEOF_SHORT_INT 2
#define SID_SIZEOF_INT 4
#define SID_SIZEOF_LONG_INT 4

/* Define if your C++ compiler implements exception-handling.  */
/* #define SID_HAVE_EXCEPTIONS */

/* Define if your compiler supports type ``bool''.
   If not, a user-defined signed integral type will be used.  */
#define SID_HAVE_BOOL

/* Define if you support file names longer than 14 characters.  */
#define SID_HAVE_LONG_FILE_NAMES

/* Define if you have the <strstrea.h> header file.  */
#define SID_HAVE_STRSTREA_H

/* And lastly specify operating system and sound driver */
#define HAVE_MSWINDOWS
#define HAVE_DIRECTX

#ifdef _MSC_VER
    /* Microsoft specific compiler problem */
#   define SID_HAVE_BAD_COMPILER
#   define SID_HAVE_BAD_AGGREGATES
#endif

