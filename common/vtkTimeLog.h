// .NAME vtkTimeLog - 
// .SECTION Description


#ifndef __vtkTimeLog_h
#define __vtkTimeLog_h


#include "vtkImageSetGet.h"
#include "vtkObject.h"

class vtkTimeLog : public vtkObject 
{
public:
  vtkTimeLog();
  ~vtkTimeLog();
  char *GetClassName() {return "vtkTimeLog";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Initialize(char *name, long numberOfEntries);
  void MarkEvent(char * description);
  void Write();
  
protected:
};



// ***************************************************************************
// From #include "timersInterface.h"             /* all kinds of good things */
// ***************************************************************************

/*@Start***********************************************************/
/* GEMSBG Include File
 * Copyright (C) 1993 The General Electric Company
 *
 *	Include File Name:  timersInterface   
 *	Developer: originally William Lensmire, DRO for SdC
 *
 * Source
 * $Revision$  $Date: 1996-10-09 15:17:05 $
 */

/*@Synopsis  Definition of the functions found in timers.c
*/     

/*@Description
*/

/*@End*********************************************************/

/* only do this once in any given compilation.*/
#ifndef  timersInterface_INCL
#define  timersInterface_INCL

#include <sys/time.h>
#include <time.h>
#include <sys/times.h>
#include <stdio.h>

/*
 * this define indicates the number of characters that will be held in storage
 */
#define TAG_BUFFER_SIZE 100

typedef struct               /* structure stored in array for recording */
{                            /* checkpoint times */
    unsigned long walltime;    /* wall cloack time from -infinity */
    unsigned long realtime;    /* clock time */
    unsigned long cputime;     /* cpu time */
    char tag[TAG_BUFFER_SIZE]; /* label associated with checkpoint */
} TIMERS_ENTRY;


typedef struct timers_data
{
	/* @INV  structure required by gettimeofday function call  */
	struct timezone timezn;

	/* @INV  structure for gettimeofday - used by startTime */
	struct timeval firstime;

	/* @INV  number of microseconds elapsed in clocktime */
	long aMicroseconds;

	/* @INV  number of milliseconds elapsed in clocktime */
	long aMilliseconds;

	/* @INV  number of seconds elapsed in clocktime */
	long aSeconds;

	/* @INV  pointer to array of time structures */
	TIMERS_ENTRY *timeArray;

	/* @INV  position of next available slot in array table */
	long position;

	/* @INV  structure for getting starting cpu time */
	struct tms firstcpu;

	/* @INV  number of elements */
	long numElements;

	/* @INV  wrap flag */
	long wrap;

	/* @INV  additional file to write to (e.g. stdout) */
	FILE *log_fileptr;
} TIMERS_DATA;


#ifdef __STDC__
extern long ctim_new( long number_of_slots, TIMERS_DATA *tdat  );
extern long ctim_new_log( char *appl_name, long number_of_slots, TIMERS_DATA *tdat  );
extern long ctim_write_records( TIMERS_DATA *tdat );
extern long ctim_startTime( TIMERS_DATA *tdat );
extern long ctim_checkTime( TIMERS_DATA *tdat, char *format, ... );
extern long ctim_print( TIMERS_DATA *tdat );
extern long ctim_printon( TIMERS_DATA *tdat );
extern void ctim_log_to_file( FILE *fileptr, TIMERS_DATA *tdat );
#else
extern long ctim_new();
extern long ctim_new_log();
extern long ctim_write_records();
extern long ctim_startTime();
extern long ctim_checkTime();
extern long ctim_print();
extern long ctim_printon();
extern void ctim_log_to_file();
#endif /* __STDC__ */


#ifdef __TIMERS_ALLOC__
TIMERS_DATA _sdctimer_;
#else
extern TIMERS_DATA _sdctimer_;
#endif


#endif /* timersInterface_INCL */





// ***************************************************************************
// From sdCTimerMacros.h
// ***************************************************************************

/* Copyright (c) 1993 by General Electric Medical Systems, France       */
/*
	Advantage Windows
	File	:	SdCTimerMacros.h
	Created	:	11/3/95
	By	:	Okerlund
	In	:	

#pragma ident "%Z% %M% v%I% (c) GEMS %G%"
*/


/*===========================================================================
 
Module:

Changelog:

===========================================================================*/
 
/*=============================================================================

Description:

C Macro wrapper for timer lib functions

===========================================================================*/

#ifndef _SDCTIMERMACROS_H
#define _SDCTIMERMACROS_H

/*============== Include ==================================================*/



//#include "timersInterface.h"
#ifdef SDC_HAS_OPENGL
#include <GL/gl.h>	/* for glFinish() */
#endif

 
#ifdef SDC_TIMER

#define SDCTIMER_INIT_MACRO( app, numEntries ) \
{ \
  if (ctim_new_log( app, numEntries, &_sdctimer_) != 0) \
    { \
      fprintf(stderr, "Unable to create SDC timing table\n" ); \
    } \
  ctim_startTime( &_sdctimer_); \
}
    
#define SDCTIMER_MARKEVENT_MACRO( eventName ) \
{ \
  ctim_checkTime( &_sdctimer_, "%s::%s", __FILE__, eventName ); \
}

#define SDCTIMER_MARKEVENTEND_MACRO( ) \
{ \
  ctim_checkTime( &_sdctimer_, "%s::done (line %d)", __FILE__, __LINE__ ); \
}

#define SDCTIMER_WRITETIMERDATA_MACRO( ) \
{ \
  ctim_write_records( &_sdctimer_ ); \
}

#define SDCTIMER_RESET_MACRO( ) \
{ \
   ctim_startTime( &_sdctimer_); \
}

/* 
 * Leveling macros 
 * 
 * The level defaults to 0.  
 * Use these to trace at generally or with more detail
 */

#ifdef SDC_TIMER_LEVEL_DEF
int _sdclevel = 0;
#else
extern int _sdclevel;
#endif

#define SDCTIMER_SET_LEVEL_MACRO( level ) \
{ \
  _sdclevel = level; \
}

#define SDCTIMER_INIT_MACRO_LEVEL( app, numEntries, level ) \
{ \
  _sdclevel = level; \
  if ( _sdclevel != 0 ) \
  { \
     if (ctim_new_log( app, numEntries, &_sdctimer_) != 0) \
	 { \
	    fprintf(stderr, "Unable to create SDC timing table\n" ); \
	 } \
	 ctim_startTime( &_sdctimer_); \
  } \
}
    
#define SDCTIMER_MARKEVENT_MACRO_LEVEL_1( eventName ) \
{ \
  if (_sdclevel >= 1 ) \
     ctim_checkTime( &_sdctimer_, "%s::%s", __FILE__, eventName ); \
}

#define SDCTIMER_MARKEVENT_MACRO_LEVEL_2( eventName ) \
{ \
  if ( _sdclevel >= 2 ) \
     ctim_checkTime( &_sdctimer_, "%s::%s", __FILE__, eventName ); \
}

#define SDCTIMER_MARKEVENT_MACRO_INTARG_LEVEL_1( eventName, arg ) \
{ \
  if (_sdclevel >= 1 ) \
     ctim_checkTime( &_sdctimer_, "%s::%s arg=%d", __FILE__, eventName, arg ); \
}

#define SDCTIMER_MARKEVENT_MACRO_INTARG_LEVEL_2( eventName, arg ) \
{ \
  if ( _sdclevel >= 2 ) \
     ctim_checkTime( &_sdctimer_, "%s::%s arg=%d", __FILE__, eventName, arg ); \
}

#define SDCTIMER_MARKEVENT_MACRO_STRARG_LEVEL_1( eventName, arg ) \
{ \
  if (_sdclevel >= 1 ) \
     ctim_checkTime( &_sdctimer_, "%s::%s %s", __FILE__, eventName, arg ); \
}

#define SDCTIMER_MARKEVENT_MACRO_STRARG_LEVEL_2( eventName, arg ) \
{ \
  if ( _sdclevel >= 2 ) \
     ctim_checkTime( &_sdctimer_, "%s::%s %s", __FILE__, eventName, arg ); \
}

#define SDCTIMER_MARKEVENTEND_MACRO_LEVEL_1( ) \
{ \
  if ( _sdclevel >= 1 ) \
     ctim_checkTime( &_sdctimer_, "%s::done (line %d)", __FILE__, __LINE__ ); \
}

#define SDCTIMER_MARKEVENTEND_MACRO_LEVEL_2( ) \
{ \
  if ( _sdclevel >= 2 ) \
     ctim_checkTime( &_sdctimer_, "%s::done (line %d)", __FILE__, __LINE__ ); \
}

#define SDCTIMER_WRITETIMERDATA_MACRO_LEVEL( ) \
{ \
  if ( _sdclevel != 0 ) \
  { \
     ctim_write_records( &_sdctimer_ ); \
  } \
}

#define SDCTIMER_RESET_MACRO_LEVEL( ) \
{ \
  if ( _sdclevel != 0 ) \
  { \
     ctim_startTime( &_sdctimer_); \
  } \
}

#else

#define SDCTIMER_INIT_MACRO( app, numEntries )
#define SDCTIMER_MARKEVENT_MACRO( eventName )
#define SDCTIMER_MARKEVENTEND_MACRO( )
#define SDCTIMER_WRITETIMERDATA_MACRO( )
#define SDCTIMER_RESET_MACRO()

#define SDCTIMER_SET_LEVEL_MACRO( level ) 
#define SDCTIMER_INIT_MACRO_LEVEL( app, numEntries, level )
#define SDCTIMER_MARKEVENT_MACRO_LEVEL_1( eventName ) 
#define SDCTIMER_MARKEVENT_MACRO_LEVEL_2( eventName ) 
#define SDCTIMER_MARKEVENT_MACRO_INTARG_LEVEL_1( eventName, arg ) 
#define SDCTIMER_MARKEVENT_MACRO_INTARG_LEVEL_2( eventName, arg ) 
#define SDCTIMER_MARKEVENT_MACRO_STRARG_LEVEL_1( eventName, arg ) 
#define SDCTIMER_MARKEVENT_MACRO_STRARG_LEVEL_2( eventName, arg ) 
#define SDCTIMER_MARKEVENTEND_MACRO_LEVEL_1( ) 
#define SDCTIMER_MARKEVENTEND_MACRO_LEVEL_2( ) 
#define SDCTIMER_WRITETIMERDATA_MACRO_LEVEL( )
#define SDCTIMER_RESET_MACRO_LEVEL( )


#endif // SDC_TIMER


#endif // _SDCTIMERMACROS_H


// ***************************************************************************
// From timersInternal.h
// ***************************************************************************

/*@Start***********************************************************/
/* GEMSBG Include File
 * Copyright (C) 1993 The General Electric Company
 *
 *	Include File Name:  timers   
 *	Developer:			William Lensmire
 *
 * Source
 * $Revision$  $Date: 1996-10-09 15:17:05 $
 */

/*@Synopsis  header definition for a circular file of timers on disk.
*/     

/*@Description
     
*/

/*@End*********************************************************/

/* only do this once in any given compilation.*/
#ifndef  timers_INCL
#define  timers_INCL

/* for code readability */
#define FUNCTION
#define PRIVATE

/* Number of entries to put in a new log by default */
/* 40960 will have a times log of 5MEG in size */
#define TIMERS_LOG_ENTRIES       40960

/* usageLog magic number */
#define TIMERS_LOG_MAGIC         "TIL"
#define TIMERS_LOG_VERSION       "1.0"
 
/* length of file name */
#define TIMERS_FILE_NAME_SIZE      63
 
/*
    --- directories definitions
*/
#ifdef NOGENESIS
#define TIMERS_LOG_LOGDIR        "./"
#define TIMERS_LOG_CONFIGDIR     "."
#else
#define TIMERS_LOG_LOGDIR        "/usr/g/service/log/"
#define TIMERS_LOG_CONFIGDIR     "/w/config"
#endif /* NOGENESIS */

/* usage log filename */
/* the prefix to this name will be the name of the process, argv[0] */
#define TIMERS_LOG_NAME          ".timers.log"


/*
 * Header for the usage metrics file
 */
#define MAGIC_LEN           4
#define VERSION_LEN         4
 
struct diskLogHeader
{
    char    magic           [MAGIC_LEN];    /* magic number "SUL"   */
    char    version         [VERSION_LEN];  /* release number   */
    long    creation_time;                  /* log creation time    */
 
    long    header_size;                    /* size of this header in bytes */
    long    record_size;                    /* size of one rec in bytes */
    long    max_records;                    /* max # of records in file */
    long    num_records;                    /* # of records in this file */
    long    wrapped;                        /* flag to indicate we wrapped*/
    long    next_record;                    /* index of next record */
};

#endif /* timers_INCL */


// ***************************************************************************
// Here's the macro definition (for a product sw header file that
// you don't want to include).  You could just add it in one of the
// timer include files.
// ***************************************************************************

/*
 **************************************************************
 gettimeofday()
 **************************************************************
 */
#ifdef sun
#ifdef  __cplusplus
extern "C"
#endif
int gettimeofday(struct timeval *tp);
#define GETTIMEOFDAY(x) gettimeofday((struct  timeval *)x)
#endif

#if defined(hpux) || defined(__hpux)
#include <time.h>
#define GETTIMEOFDAY(x) gettimeofday((struct timeval *)x, (struct timezone
*)NULL)
#endif

#ifdef sgi
#include <sys/time.h>
#define GETTIMEOFDAY(x) gettimeofday(x)
#endif /* sgi */




#endif





