


#include "vtkTimeLog.h"
//#include "sdCTimerMacros.h"

//----------------------------------------------------------------------------
vtkTimeLog::vtkTimeLog()
{
}


//----------------------------------------------------------------------------
vtkTimeLog::~vtkTimeLog()
{
}

//----------------------------------------------------------------------------
void vtkTimeLog::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
// Description:
void vtkTimeLog::Initialize(char *name, long numberOfEntries)
{
  name = name;
  SDCTIMER_INIT_MACRO(name, numberOfEntries);
}



//----------------------------------------------------------------------------
// Description:
void vtkTimeLog::MarkEvent(char *description)
{
  description = description;
  SDCTIMER_MARKEVENT_MACRO(description);
}




//----------------------------------------------------------------------------
// Description:
void vtkTimeLog::Write()
{
  SDCTIMER_WRITETIMERDATA_MACRO();
}







// Here's the code and some guidelines on the timers stuff that we
// talked about earlier.  Rick, we plan on using this (in some form)
// for perf benchmarking of the the IP code, but we might want to take
// a look at it for other areas of vtk as well.  I realized that there
// are several perf measuring utilities available, but this is a very
// reasonable and fairly painless way of instrumenting our code directly.
// We would probably make timing table entries in the Execute methods
// of many of the IP filters.

// To use the timers stuff,
// an  app (and in classes where we want to take timings) needs to
// include

// "SdCTimerMacros.h"    (Yes, we can and should change the name!)
// which also includes "timersInterface.h".

// The timer code itself, timers.c,  (Yes it's old, ugly code.  but it works!)
// also needs "timersInternal.h" to build the .o.


// The macros that should be used in the app are,

// SDCTIMER_INIT_MACRO(),
// SDCTIMER_MARKEVENT_MACRO(), and
// SDCTIMER_WRITETIMERDATA_MACRO().

// These create a timing table of a certain size, log entries to the
// table, and dump out the table to a file respectively.  There's a
// separate executable to format this output into a more readable ascii
// representation for analysis.  This formatted output includes a string
// associated with each event, a delta wall time from the last logged
// event, an accumulated wall time since when the table was initialized,
// and an accumulated CPU time.  Ex:  its been 320 ms from the last event,
// we're 7240 ms into the running of the app, and we've used a total of
// 2200 ms of the CPU during this time.  One of the attachments gives you
// an excerpt of a formatted table.


// Optional macros include

// SDCTIMER_MARKEVENTEND_MACRO(),
// and all the macros that set the logging level.


// example usage:


// -------------------------

// #include "SdCTimerMacros.h"

// main()
// {
// SDCTIMER_INIT_MACRO( argv[0], 100 );   // create table w/ 100

// SDCTIMER_MARKEVENT_MACRO("convolution");
// convolve();
// SDCTIMER_MARKEVENT_MACRO();  // explicity record when conv ends

// SDCTIMER_MARKEVENT_MACRO("doing this");
// this();

// SDCTIMER_MARKEVENT_MACRO("doing that");
// that();

// SDCTIMER_MARKEVENT_MACRO("all done");

// SDCTIMER_WRITETIMERDATA_MACRO();  // dump out the timing table

// exit(0);

// }

/*
 * GEMSBG Class Definition File Copyright (C) 1989 The General Electric
 * Company
 * 
 * Class Name: timers Developer:  Glenn Norton, Roy Nilsen, Bill Lensmire
 * 
 * 
 * Source $Revision: 1.1 $  $Date: 1996-10-09 15:16:57 $
 */

static char sccsid[] = "@(#)timers_fixedlen.c	1.7 10/29/93 08:23:24";

/**
 * @Synopsis     "start and stop timers for timing tests"
 *=========================================================================
 * 
 *@Description - This object provides a way to measure program performance.
 * A timer is instantiated with the number of checkpoint entries the user
 * wants to store and print. The startTime method starts a timer. Every time
 * the method checkTime is called, a delta clock and cput time is stored in
 * the array. printTime will print out the array on stdout and reset the
 * position index. printTimeOn will print out the array to the IOD specified
 * by the input IOD and reset the position index. print will print the array
 * to stdout, but will not reset the position index. printOn is used directly
 * or indirectly by the other print methods and will print the array to the
 * IOD specified.
 *
 *=========================================================================
 * methods
 *=========================================================================
 *
 * ctim_new(int, TIMERS_DATA *)  
 * - instantiate Timer and allocate number of array entries.
 * 
 * ctim_new_log(int, TIMERS_DATA *) 
 * - instantiate Timers and the disk file header.
 *
 * ctim_startTime(TIMERS_DATA *) 
 * - get starting cpu and clock times.
 *
 * ctim_checkTime(TIMERS_DATA *) 
 * - get current delta times and put in time array.
 *
 * ctim_print(TIMERS_DATA *)
 * - print the time array on stdout, but don't reset position index.
 *
 * ctim_write_records(TIMERS_DATA *) 
 * - write the in memory timers to the disk file
 *
 *=========================================================================
 *	Include Files:
 *=========================================================================
 */

#include <stdio.h>

#ifdef __STDC__
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>
#else
#include <varargs.h>
#endif /* __STDC__ */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/file.h>
#include <fcntl.h>
/*
#include <common/portability.h>
*/
//#include "timersInternal.h"
#define __TIMERS_ALLOC__
//#include "timersInterface.h"             /* all kinds of good things */
#undef __TIMERS_ALLOC__


/**========================================================================= */

extern long times();

/**========================================================================
 * defines, typedefs and externs
 *=========================================================================
 */
#ifdef __STDC__
PRIVATE FUNCTION long ctim_write_record( long record, TIMERS_DATA *tdat );
PRIVATE FUNCTION long ctim_write_header( TIMERS_DATA *tdat );
PRIVATE FUNCTION long ctim_cleanup( TIMERS_DATA *tdat );
PRIVATE FUNCTION long ctim_init_to_write( TIMERS_DATA *tdat );
PRIVATE FUNCTION long ctim_initialize( TIMERS_DATA *tdat );
#else
PRIVATE FUNCTION long ctim_write_record();
PRIVATE FUNCTION long ctim_write_header();
PRIVATE FUNCTION long ctim_cleanup();
PRIVATE FUNCTION long ctim_init_to_write();
PRIVATE FUNCTION long ctim_initialize();
#endif

/**========================================================================= */

static long maxTagSize = TAG_BUFFER_SIZE-1;

/*@INV Usage file unix file descriptor */
static long   logfd = -1;

/*@INV Usage file name, used only for error reporting */
static char   log_file_name[TIMERS_FILE_NAME_SIZE+1];
static char   application_name[TIMERS_FILE_NAME_SIZE+1];

/*@INV header of the metrics file (in memory) */
static struct diskLogHeader header;

/*@INV record size in bytes */
static long recordSize;

/* this is a control flag to indicate if we should use the log file or not */
static long use_log_file = 1;


/*==================================================
 *                  ctim_new
 *==================================================
 *
 * Syntax: ctim_new(slots, tdat)
 * Input Arguments: (long)slots - The number of entries in the time array.
 *                : (TIMERS_DATA *)tdat - pointer to timers data storage.
 * Purpose: instantiates a timer object and allocates an array of slots' size.
 * Returns: 0 or -1.
 * Error Handling: returns -1 if memory can't be allocated.
 */

long
#ifdef __STDC__
ctim_new(long slots, TIMERS_DATA *tdat )
#else
ctim_new(slots, tdat)
    long slots;			       /* number of slots to reserve for timing. */
    TIMERS_DATA *tdat;
#endif
{
    long i;

    /* start next available slot at beginning of array */
    tdat->position = 0;

    /* init numElements and wrap flag */
    tdat->numElements = slots;
    tdat->wrap = 0;

	/* AEG 15 Jan 95: init to NULL the additional file to log to */
	tdat->log_fileptr = NULL;

    /* allocate the array of structures */
    if (!(tdat->timeArray = (TIMERS_ENTRY *) malloc(sizeof(TIMERS_ENTRY) * slots)))
    {
	printf( "Malloc failure errno %d\n", errno);
	return -1;
    }

    /* Initialize tag pointers and null terminate the tag labels */
    for (i = 0; i < slots; i++)
    {
	tdat->timeArray[i].tag[maxTagSize] = '\0';
    }

    return 0;
}


/*==================================================
 *                 ctim_new_log
 *==================================================
 *
 * Syntax: ctim_new_log(slots, tdat)
 * Input Arguments: (long)slot - the number of entries in the time array.
 *                  (TIMERS_DATA *)tdat - the timers data storage. 
 * Purpose: instantiate the timers object and the disk file header.
 * Returns: 0 or -1.
 * Error Handling: returns -1 if memory can't be allocated.
 */

long
#ifdef __STDC__
ctim_new_log( char *appl_name, long slots, TIMERS_DATA *tdat )
#else
ctim_new_log( slots, tdat )
	char *appl_name;
	long slots;
	TIMERS_DATA *tdat;
#endif
{
	long status;
	long return_status = 0;

	if ( appl_name == NULL )
	{
		printf( "Unable to initialize the timers log file: null application name.\n");
		return (88);
	}
    	strcpy(application_name, appl_name);
       
	/*
	 * create the in memory log 
	 */
	status = ctim_new(slots, tdat);
	if ( status != 0 )
	{
		return_status = -1;
	}

	/*
	 * open the file to write
	 */
	status = ctim_init_to_write(tdat);
	if ( status != 0 )
	{
		printf( "Unable to initialize the timers log file.\n");
		printf( "NOT using timers log file.\n");
		use_log_file = 0;
	}

	return( return_status );
}


/*==================================================
 *                 ctim_write_records
 *==================================================
 * 
 * Syntax: ctim_write_records(tdat)
 * Input Arguments: (TIMERS_DATA *)tdat - pointer to timers data storage.
 * Purpose: writes all the records to a file on disk. 
 * Returns: 0 or -1.
 * Error Handling: returns -1 if error. 
 */

FUNCTION long
#ifdef __STDC__
ctim_write_records( TIMERS_DATA *tdat )
#else
ctim_write_records(tdat)
TIMERS_DATA *tdat;
#endif
{
    long int_status = 0;
    long record;
    long recs_to_write;
    long i;

    if ( use_log_file == 0 )
    {
        printf( "Timers log file was not turned on.\n" );
        return ( 0 );
    }
 
    /*
     * check to see if there are any records to write
     */  
    if ( (tdat->wrap == 0) && (tdat->position == 0) )
    {
        return( 0 );
    }  
 
    if ( logfd < 0 )
    {
	printf( "Timers log file not opened.\n" );
        return(-1);
    }

    /* if we wrapped, then print all the elements, else, only print what we have */
    if ( tdat->wrap == 1 )
    {
        recs_to_write = tdat->numElements;
	i = tdat->position;
    }
    else
    {
        recs_to_write = tdat->position;
	i = 0;
    }

    /*
     * loop over all of the records that are in memory
     */  
    for ( record = 0 ; record < recs_to_write ; record++ )
    {
	int_status = ctim_write_record( i, tdat );
	if ( int_status != 0 )
        {
	    use_log_file = 0;
            return( -1 );
        }

	/* increment the offset into the data */
	if (++i >= tdat->numElements)
	{
   	   i = 0;
	}
    }    

    /*
     * update the header on disk
     */  
    int_status = ctim_write_header(tdat);
    if ( int_status != 0 )
    {
	ctim_cleanup(tdat);
	use_log_file = 0;
        return( -1 );
    }

    return( 0 );
}


/*==================================================
 *                    ctim_startTime
 *==================================================
 *
 * Syntax: ctim_startTime(tdat) 
 * Input Arguments: (TIMERS_DATA *)tdat - pointer to timers data storage. 
 * Returns: 0
 * Purpose: This method starts all timers and sets up the storage array.
 * Error Handling: none.
 */

FUNCTION long
#ifdef __STDC__
ctim_startTime( TIMERS_DATA *tdat )
#else
ctim_startTime(tdat)
TIMERS_DATA *tdat;
#endif
{
    /* set the pointers to the beginning of the file */
    tdat->position = 0;
    tdat->wrap = 0;

    /* initialize the first label/tag */
    tdat->timeArray[tdat->position].tag[0] = 0x01;

    /* start clock time timer */
/*
    gettimeofday(&tdat->firstime, &tdat->timezn);
*/
    GETTIMEOFDAY(&tdat->firstime);

    /* initialize wall clock time  */
    tdat->timeArray[tdat->position].walltime = tdat->firstime.tv_sec;

    /* initialize first clock time stored to zero */
    tdat->timeArray[tdat->position].realtime = 0;

    /* start the cpu time timer */
    times((struct tms*)&tdat->firstcpu);

    /* initialize the first cpu time stored to zero */
    tdat->timeArray[tdat->position++].cputime = 0;

    /* check for wrap around */
    if (tdat->position == tdat->numElements)
    {
	tdat->wrap = 1;
	tdat->position = 0;
    }

    return 0;
}


/*==================================================
 *                   ctim_checkTime
 *==================================================
 *
 * Syntax: ctim_checkTime(va_alist)
 * Input Arguments: va_alist - This is list of arguments, just like
 *                           printf (see man page on varargs),
 *                           that the user wants printed.
 *                           The function call should take the
 *                           following form:
 *             ctim_checkTime("fmt_string"[, arg1, arg2, arg...]);
 *
 * Returns: (long)aMilliseconds - The number of milliseconds that have
 *                                elapsed since the last startTime method
 *                                call.
 * Purpose: This method calculates the time spent in clock time and cpu time 
 *          since the last startTime method call.
 * Error Handling: none.
 */
#ifdef __STDC__
long ctim_checkTime(TIMERS_DATA *tdat, char *format, ...)
#else
long ctim_checkTime(tdat, va_alist)
TIMERS_DATA *tdat;
va_dcl
#endif
{
    char event[256];
    struct timeval checktime;
    struct tms tbuf;
    long strsize = 0;

    /* DO NOT ADD LOCAL DECLARATIONS AFTER THIS LINE */
#ifdef __STDC__
    va_list var_args;

    va_start(var_args, format);
#else
    va_list var_args;
    char *format;

    va_start(var_args);
    format = va_arg(var_args, char *);
#endif

    /*        
     * Now using the variable argument version of sprintf (vsprintf) write
     * out the string 'event' which is then logged with a time stamp by
     * the rest of this function.
     */       
    vsprintf(event, format, var_args);
    va_end(var_args);

    /*
     * Check if event string ends in '\n'. If one is found at the end of
     * the string remove it since CTTimers automatically adds one.
     */
    if (event[strlen(event) - 1] == '\n')
    {
	/* Carrage return found at end of string... Nuke it! */
	event[strlen(event) - 1] = NULL;
    }

    /* get the clock time since last startTime */
/*
    gettimeofday(&checktime, &tdat->timezn);
*/
    GETTIMEOFDAY(&checktime);

    /* set wall clock time  */
    /* AEG: changed walltime=firstime.tv_sec to walltime=checktime.tv_sec */
    tdat->timeArray[tdat->position].walltime = checktime.tv_sec;

    /* get the cpu time since last startTime */
    times(&tbuf);

    tdat->aMicroseconds = 0;

    /* get the number of seconds elapsed */
    tdat->aSeconds = checktime.tv_sec - tdat->firstime.tv_sec;

    /* if first microseconds > than current - compensate */
    if (tdat->firstime.tv_usec > checktime.tv_usec)
    {
	/* decrement seconds by 1 */
	tdat->aSeconds--;
	/* add in to current microseconds */
	checktime.tv_usec += 1000000;
    }

    /* convert seconds to milliseconds */
    tdat->aMilliseconds = tdat->aSeconds * 1000;

    /* add differance in microseconds between first and current */
    tdat->aMicroseconds += (checktime.tv_usec - tdat->firstime.tv_usec);

    /* round microseconds < 1000 to 1 millisecond */
    if ((tdat->aMicroseconds < 1000) && (tdat->aMicroseconds >= 500))
    {
	tdat->aMilliseconds += 1;
    }
    else
    {
	if (tdat->aMicroseconds < 500)
	    tdat->aMilliseconds += 0;
	else
	    tdat->aMilliseconds += tdat->aMicroseconds / 1000;
    }

    /* set milliseconds in the storage array */
    tdat->timeArray[tdat->position].realtime = tdat->aMilliseconds;

    /* calculate cpu time in microseconds */
    tdat->timeArray[tdat->position].cputime = ((tbuf.tms_utime + tbuf.tms_stime) -
				   (tdat->firstcpu.tms_utime + tdat->firstcpu.tms_stime));

    /* copy the users string into the storage array */
    strsize = (strlen(event)) > maxTagSize ? maxTagSize : strlen(event);
    strncpy(tdat->timeArray[tdat->position].tag, event, strsize);
    tdat->timeArray[tdat->position].tag[strsize] = 0;

    /*
    ** 29 Oct 93 BDB & 15 Jan 94 AEG
    ** log to this file as well as internal buffer
    */
    if ( tdat->log_fileptr != NULL )
    {
	fprintf( (FILE *)(tdat->log_fileptr), "%s\n", event );
    }

    tdat->position++;

    /* check for wraparound */
    if (tdat->position == tdat->numElements)
    {
	tdat->wrap = 1;
	tdat->position = 0;
    }

    return (tdat->aMilliseconds);
}


/*==================================================
 *                   ctim_print
 *==================================================
 *
 * Syntax: ctim_print( tdat )
 * Input Arguments: (TIMERS_DATA *)tdat - pointer to timers data storage. 
 * Returns: 0
 * Purpose: This method prints the time array to stdout and
 *          does not reset the position pointer. This method
 *          would be called when the user wants to print the
 *          array before the timing process is done.
 * Error Handling: none.
 */

FUNCTION long
#ifdef __STDC__
ctim_print( TIMERS_DATA *tdat )
#else
ctim_print(tdat)
TIMERS_DATA *tdat;
#endif
{
    long i, j, lcv, counter;

    FILE *fout;
    time_t locChecktime;
    char fileName[64];
    char usingSTDOUT;
    long time_delta;

    /*
     * Get the time of day.
     */
    localtime(&locChecktime);

    /*
     * create the filename for the timing output.
     */


    strcpy(fileName, application_name);
    strcat(fileName, ".timers");

/*
    sprintf(fileName, "./%s.%d.timers", application_name, getpid());
*/

    fout = fopen(fileName, "a");

    usingSTDOUT = 0;
    if (fout == NULL)
    {
	printf("\nUnable to open %s in append mode.\n", fileName);
	printf("USING stdout.....\n");
	usingSTDOUT = 1;
	fout = stdout;
    }
    fprintf(fout, "\n\nThe following timings were collected on: \n");
    fprintf(fout, "\t%s\n\n", ctime(&locChecktime));

    /*
     * assign the number of elements to print
     */

    if (tdat->wrap)                 /* check for a wrapped table */
    {
	counter = tdat->numElements;    /* we wrapped, print out the whole table */
	i = tdat->position;             /* start the current element */

	if (tdat->timeArray[i].tag[0] != 0x01)    /* if special tag, print header */
	{
	   fprintf(fout, "    START   Real (milliseconds)    |    CPU (milliseconds)\n");
	   fprintf(fout, "    Wall       Total     Delta     |  Total     Process      Tag -----------\n");
	}
    }
    else
    {
	/* we did not wrap */
	counter = tdat->position;                 /* this many entries... */
	i = 0;                                    /* start with the first element */
    }

    for (lcv = 1; lcv <= counter; lcv++)      /* loop this many times...  */
    {
	if (tdat->timeArray[i].tag[0] == 0x01)    /* if special tag, print header */
	{
	   fprintf(fout, "    START   Real (milliseconds)     |    CPU (milliseconds)\n");
	   fprintf(fout, "    Wall       Total     Delta      |  Total     Process      Tag -----------\n");
	}
	else
	{
    	   /* print data */
    	   if (i == 0)				   /* if first element, */
		j = tdat->numElements - 1;	   /* make j last element (wrap) */
	   else
		j = i - 1;			   /* j = i - 1 */

	   /*
	    * because the numbers are printed unsigned, it does not make
	    * sense to print out negative numbers.  We get a negative
	    * number when the table wraps.
	    */
	   time_delta = tdat->timeArray[i].realtime - tdat->timeArray[j].realtime;
	   if ( time_delta < 0 )
	   {
		time_delta = 0;
	   }

    	   fprintf(fout, "%010.10u  %010.10u  %010.10u   %010.10u  %s  %s\n",
		    	tdat->timeArray[i].walltime,
		    	tdat->timeArray[i].realtime,
			time_delta,
		    	(long) (tdat->timeArray[i].cputime * 16.667),
		    	application_name,
		    	tdat->timeArray[i].tag);
	}

	i++;	   /* increment the offset into the data */
	if (i >= tdat->numElements)
    	   i = 0;
    }

    /*
     * flush and close the file.
     */
    if (usingSTDOUT == 0)
    {
	fflush(fout);
	fclose(fout);
    }

    return 0;
}


/*==================================================
 *                   ctim_printon
 *==================================================
 */
FUNCTION long
#ifdef __STDC__
ctim_printon( TIMERS_DATA *tdat )
#else
ctim_printon(tdat)
TIMERS_DATA *tdat;
#endif
{
    /*
     * print the instance variables
     */  
    printf("\n\n" );
    printf("file descriptor........... %d\n", logfd );
    printf("file name................. %s\n", log_file_name );
    printf("header address............ 0x%x\n", header );
    printf("record size............... %d\n", recordSize );


    /*
     * print the header of the file
     */  
    printf("\n\n" );
    printf("HEADER OF SCAN USAGE LOG\n\n" );
    printf("magic number........... %4s\n",  header.magic );
    printf("version number......... %4s\n",  header.version );
    printf("creation time.......... %12d\n", header.creation_time );
    printf("header size............ %12d\n", header.header_size );
    printf("record size............ %12d\n", header.record_size );
    printf("max number of records.. %12d\n", header.max_records );
    printf("number of records...... %12d\n", header.num_records );
    printf("wrapped flag........... %12d\n", header.wrapped );
    printf("next record entry...... %12d\n", header.next_record );
    printf("\n\n" );

    return( 0 );
}


/*==================================================
 *          (private) ctim_write_record
 *==================================================
 */
PRIVATE FUNCTION long
#ifdef __STDC__
ctim_write_record( long record, TIMERS_DATA *tdat )
#else
ctim_write_record( record, tdat )
	long record;
	TIMERS_DATA *tdat;
#endif
{
    long seek_dest;

/* DEBUG
    printf( "Writing record %d\n", record );
    printf( "tag %s\n", timeArray[record].tag );
*/

    /*
     * figure where the next entry goes in the file
     */  
    seek_dest = sizeof(header) + (header.next_record * recordSize);

    /*
    // Write the record
    */
    if ( lseek(logfd, seek_dest, L_SET) != seek_dest )
    {
	printf( "Lseek failed on %s, errno %d\n", log_file_name, errno );
        return( -1 );
    }

    if ( write(logfd, (char *)&tdat->timeArray[record], recordSize) != recordSize )
    {
	printf( "Write failed on %s, errno %d\n", log_file_name, errno );
        return( -1 );
    }

    /*
     * increment to the number of  entries in the disk file
     */  
    if ( ! header.wrapped )
    {
        header.num_records++;
    }
 
    /*
     * this means we will wrap, set the wrapped flag
     * and start at the beginning of the file for the next write
     */
    if ( (header.next_record + 1) >= header.max_records )
    {
        header.wrapped = 1;
        header.next_record = 0;
    }
    else
    {
        header.next_record++;
    }    
   
    return( 0 );
}


/*==================================================
 *          (private) ctim_write_header
 *==================================================
 */
PRIVATE FUNCTION long
#ifdef __STDC__
ctim_write_header( TIMERS_DATA *tdat )
#else
ctim_write_header(tdat)
TIMERS_DATA *tdat;
#endif
{
    if ( logfd < 0 )
    {
	printf( "Timers log file not opened.\n" );
        return(-1);
    }  
 
    /*
    // Write contents to the metrics file
    */
    if ( lseek(logfd, 0L, L_SET) != 0L )
    {
	printf( "Lseek failed on %s, errno %d\n", log_file_name, errno );
        return( -1 );
    }  
 
    if ( write(logfd, (char *)&header, sizeof(header)) != sizeof(header) )
    {
	printf( "Write failed on %s, errno %d\n", log_file_name, errno );
        return( -1 );
    }
 
    return( 0 );
}


/*==================================================
 *           (private) ctim_cleanup
 *==================================================
 */
PRIVATE FUNCTION long
#ifdef __STDC__
ctim_cleanup( TIMERS_DATA *tdat )
#else
ctim_cleanup(tdat)
TIMERS_DATA *tdat;
#endif
{
    /*
    // if a file is open, close it
    */
    if ( logfd > 2 )  /* do not close stdin, stdout, or stderr */
    {
        close( logfd );
    }

    /*
    // set all instance variables to initial values
    */
    ctim_initialize(tdat);

    return( 0 );
}


/*==================================================
 *           (private) ctim_initialize
 *==================================================
 */
PRIVATE FUNCTION long
#ifdef __STDC__
ctim_initialize( TIMERS_DATA *tdat )
#else
ctim_initialize(tdat)
TIMERS_DATA *tdat;
#endif
{
    logfd = -1;
    log_file_name[0] = 0;
    recordSize = sizeof ( TIMERS_ENTRY );

    return( 0 );
}


/*==================================================
 *         (private) ctim_fill_header
 *==================================================
 */
PRIVATE FUNCTION long
#ifdef __STDC__
ctim_fill_header( TIMERS_DATA *tdat )
#else
ctim_fill_header(tdat)
TIMERS_DATA *tdat;
#endif
{
	long int_status = 0;
    /*
     * The file is new (empty),
     * fill in the header fields
     */  
    strncpy( header.magic,   TIMERS_LOG_MAGIC,   MAGIC_LEN );
    strncpy( header.version, TIMERS_LOG_VERSION, VERSION_LEN );

    header.creation_time = time(0);
    header.header_size   = sizeof( header );
    header.record_size   = recordSize;
    header.max_records   = TIMERS_LOG_ENTRIES;
    header.num_records   = 0;
    header.wrapped       = 0;
    header.next_record   = 0;

    /*
    // Write contents to the file
    */
    int_status = ctim_write_header(tdat);
    if ( int_status != 0 )
    {
	ctim_cleanup(tdat);
	use_log_file = 0;
        return( -1 );
    }

    return ( 0 );
}


/*==================================================
 *      (private) readHeaderUpdateIfNeeded 
 *==================================================
 */
PRIVATE FUNCTION long
#ifdef __STDC__
readHeaderUpdateIfNeeded( TIMERS_DATA *tdat )
#else
readHeaderUpdateIfNeeded(tdat)
TIMERS_DATA *tdat;
#endif
{
    long size;

    if ( logfd < 0 )
    {
	printf( "Log not opened.\n");
        return(-1);
    }

    /*     
    // read contents to the metrics file
    */     
    if ( lseek(logfd, 0L, L_SET) != 0L )
    {      
	printf( "Lseek failed on %s, errno %d\n", log_file_name, errno );
        return( -1 );
    }      
           
    if ( (size = read(logfd, (char *)&header, sizeof(header))) < 0 )
    {      
	printf( "Read failed on %s, errno %d\n", log_file_name, errno );
        return( -1 );
    }      
 
    if ( size != sizeof(header) )
    {      
	printf( "header for %s is not correct, %d, expecting %d\n", log_file_name, size, sizeof(header) );
           
        /* 
         * truncate the file ... continue on
         */
        if ( ftruncate( logfd, 0 ) != 0 )
        {  
	    ctim_cleanup(tdat);
            return( -1 );
        }

        /*
         * fill in the header with default values
         */
        if ( ctim_fill_header(tdat) != 0)
        {
	    ctim_cleanup(tdat);
            return( -1 );
        }
    }
    else
    {
        if ( header.record_size != sizeof(TIMERS_ENTRY) )
	{
		printf( "record size for %s is not correct, %d, expecting %d\n", log_file_name, header.record_size, sizeof(TIMERS_ENTRY) );
           
        	/* 
        	 * truncate the file ... continue on
        	 */
        	if ( ftruncate( logfd, 0 ) != 0 )
        	{
	    		ctim_cleanup(tdat);
            		return( -1 );
        	}

	        /*
        	 * fill in the header with default values
        	 */
        	if (  ctim_fill_header(tdat) != 0)
        	{
	    		ctim_cleanup(tdat);
            		return( -1 );
        	}
	}
    }

    return( 0 );
}


/*==================================================
 *      (private) ctim_init_to_write 
 *==================================================
 */
PRIVATE FUNCTION long
#ifdef __STDC__
ctim_init_to_write( TIMERS_DATA *tdat )
#else
ctim_init_to_write(tdat)
TIMERS_DATA *tdat;
#endif
{
    long int_status = 0;
    long filesize;
    struct stat statbuf;
 
    /*
    // Make sure we have no resources allocated
    */
    ctim_cleanup(tdat);
 
    /*
     *    create the filename for the timing output.
     */
/*
    strcpy ( (char *)log_file_name, getenv("SDCHOME") );
    strcat ( (char *)log_file_name, "/logfiles/" );
    strcat ( (char *)log_file_name, application_name );
    strcat ( (char *)log_file_name, ".timers.log");
*/

    sprintf((char *)log_file_name, "./%s.%d.timers.log", application_name, getpid());
 
    /*
    // Open the file
    */
    logfd = open ( (char *)log_file_name, (O_RDWR|O_CREAT), 0644 );
     
    if ( logfd < 0 )
    {
	printf( "Unable to open log %s.\n", log_file_name );
    	ctim_cleanup(tdat);
        return( -1 );
    }

    /*
    // Get the size of the file
    */
    if ( fstat(logfd, &statbuf) != 0 )
    {
	printf( "Unable to fstat log %s.\n", log_file_name );
    	ctim_cleanup(tdat);
        return( -1 );
    }
    filesize = statbuf.st_size;

    /*
    // if the size is greater than zero, get the header of the file
    */
    if ( filesize > 0 )
    {
        int_status = readHeaderUpdateIfNeeded(tdat);
        if ( int_status != 0 )
        {
    	    ctim_cleanup(tdat);
            return( -1 );
        }
    }    
    else
    {   
        /*
         * fill in the header with default values
         */
	int_status = ctim_fill_header(tdat);
        if ( int_status != 0 )
        {
    	    ctim_cleanup(tdat);
            return( -1 );
        }
    }
     
    return( 0 );
}


/*==================================================
 *           ctim_log_to_file 
 *==================================================
 * 29 Oct 93 BDB & 15 Jan 95 AEG
 *
 * Function to specify an additional file to log to (e.g. stdout)
 */
void
#ifdef __STDC__
ctim_log_to_file( FILE *fileptr, TIMERS_DATA *tdat )
#else
ctim_log_to_file( fileptr, tdat)
FILE *fileptr;
TIMERS_DATA *tdat;
#endif
{
	tdat->log_fileptr = fileptr;
}

