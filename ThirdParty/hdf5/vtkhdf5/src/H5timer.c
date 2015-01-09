/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5timer.c
 *			Aug 21 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Internal 'timer' routines & support routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/

/* We need this for the struct rusage declaration */
#if defined(H5_HAVE_GETRUSAGE) && defined(H5_HAVE_SYS_RESOURCE_H)
#   include <sys/resource.h>
#endif

#if defined(H5_HAVE_GETTIMEOFDAY) && defined(H5_HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/


/*-------------------------------------------------------------------------
 * Function:	H5_timer_reset
 *
 * Purpose:	Resets the timer struct to zero.  Use this to reset a timer
 *		that's being used as an accumulator for summing times.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_timer_reset (H5_timer_t *timer)
{
    HDassert(timer);
    HDmemset(timer, 0, sizeof *timer);
} /* end H5_timer_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5_timer_begin
 *
 * Purpose:	Initialize a timer to time something.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_timer_begin (H5_timer_t *timer)
{
#ifdef H5_HAVE_GETRUSAGE
    struct rusage	rusage;
#endif
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval	etime;
#endif

    HDassert(timer);

#ifdef H5_HAVE_GETRUSAGE
    HDgetrusage (RUSAGE_SELF, &rusage);
    timer->utime = (double)rusage.ru_utime.tv_sec +
                   ((double)rusage.ru_utime.tv_usec / 1e6);
    timer->stime = (double)rusage.ru_stime.tv_sec +
                   ((double)rusage.ru_stime.tv_usec / 1e6);
#else
    timer->utime = 0.0;
    timer->stime = 0.0;
#endif
#ifdef H5_HAVE_GETTIMEOFDAY
    HDgettimeofday (&etime, NULL);
    timer->etime = (double)etime.tv_sec + ((double)etime.tv_usec / 1e6);
#else
    timer->etime = 0.0;
#endif
} /* end H5_timer_begin() */


/*-------------------------------------------------------------------------
 * Function:	H5_timer_end
 *
 * Purpose:	This function should be called at the end of a timed region.
 *		The SUM is an optional pointer which will accumulate times.
 *		TMS is the same struct that was passed to H5_timer_start().
 *		On return, TMS will contain total times for the timed region.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_timer_end (H5_timer_t *sum/*in,out*/, H5_timer_t *timer/*in,out*/)
{
    H5_timer_t		now;

    HDassert(timer);
    H5_timer_begin (&now);

    timer->utime = MAX(0.0, now.utime - timer->utime);
    timer->stime = MAX(0.0, now.stime - timer->stime);
    timer->etime = MAX(0.0, now.etime - timer->etime);

    if (sum) {
	sum->utime += timer->utime;
	sum->stime += timer->stime;
	sum->etime += timer->etime;
    }
} /* end H5_timer_end() */


/*-------------------------------------------------------------------------
 * Function:	H5_bandwidth
 *
 * Purpose:	Prints the bandwidth (bytes per second) in a field 10
 *		characters wide widh four digits of precision like this:
 *
 * 			       NaN	If <=0 seconds
 *			1234. TB/s
 * 			123.4 TB/s
 *			12.34 GB/s
 *			1.234 MB/s
 *			4.000 kB/s
 *			1.000  B/s
 *			0.000  B/s	If NBYTES==0
 *			1.2345e-10	For bandwidth less than 1
 *			6.7893e+94	For exceptionally large values
 *			6.678e+106	For really big values
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_bandwidth(char *buf/*out*/, double nbytes, double nseconds)
{
    double	bw;

    if(nseconds <= 0.0)
	HDstrcpy(buf, "       NaN");
    else {
	bw = nbytes/nseconds;
        if(HDfabs(bw) < 0.0000000001)
            /* That is == 0.0, but direct comparison between floats is bad */
	    HDstrcpy(buf, "0.000  B/s");
	else if(bw < 1.0)
	    sprintf(buf, "%10.4e", bw);
	else if(bw < 1024.0) {
	    sprintf(buf, "%05.4f", bw);
	    HDstrcpy(buf+5, "  B/s");
	} else if(bw < (1024.0 * 1024.0)) {
	    sprintf(buf, "%05.4f", bw / 1024.0);
	    HDstrcpy(buf+5, " kB/s");
	} else if(bw < (1024.0 * 1024.0 * 1024.0)) {
	    sprintf(buf, "%05.4f", bw / (1024.0 * 1024.0));
	    HDstrcpy(buf+5, " MB/s");
	} else if(bw < (1024.0 * 1024.0 * 1024.0 * 1024.0)) {
	    sprintf(buf, "%05.4f", bw / (1024.0 * 1024.0 * 1024.0));
	    HDstrcpy(buf+5, " GB/s");
	} else if(bw < (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0)) {
	    sprintf(buf, "%05.4f", bw / (1024.0 * 1024.0 * 1024.0 * 1024.0));
	    HDstrcpy(buf+5, " TB/s");
	} else {
	    sprintf(buf, "%10.4e", bw);
	    if(HDstrlen(buf) > 10)
		sprintf(buf, "%10.3e", bw);
	}
    }
} /* end H5_bandwidth() */


/*-------------------------------------------------------------------------
 * Function:	H5_now
 *
 * Purpose:	Retrieves the current time, as seconds after the UNIX epoch.
 *
 * Return:	# of seconds from the epoch (can't fail)
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, November 28, 2006
 *
 *-------------------------------------------------------------------------
 */
time_t
H5_now(void)
{
    time_t	now;                    /* Current time */

#ifdef H5_HAVE_GETTIMEOFDAY
    {
        struct timeval now_tv;

        HDgettimeofday(&now_tv, NULL);
        now = now_tv.tv_sec;
    }
#else /* H5_HAVE_GETTIMEOFDAY */
    now = HDtime(NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

    return(now);
} /* end H5_now() */

