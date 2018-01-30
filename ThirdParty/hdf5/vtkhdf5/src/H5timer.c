/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
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
                   ((double)rusage.ru_utime.tv_usec / (double)1e6F);
    timer->stime = (double)rusage.ru_stime.tv_sec +
                   ((double)rusage.ru_stime.tv_usec / (double)1e6F);
#else
    timer->utime = 0.0F;
    timer->stime = 0.0F;
#endif
#ifdef H5_HAVE_GETTIMEOFDAY
    HDgettimeofday (&etime, NULL);
    timer->etime = (double)etime.tv_sec + ((double)etime.tv_usec / (double)1e6F);
#else
    timer->etime = 0.0F;
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
 *-------------------------------------------------------------------------
 */
void
H5_timer_end (H5_timer_t *sum/*in,out*/, H5_timer_t *timer/*in,out*/)
{
    H5_timer_t		now;

    HDassert(timer);
    H5_timer_begin(&now);

    timer->utime = MAX((double)0.0F, now.utime - timer->utime);
    timer->stime = MAX((double)0.0F, now.stime - timer->stime);
    timer->etime = MAX((double)0.0F, now.etime - timer->etime);

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
 *-------------------------------------------------------------------------
 */
void
H5_bandwidth(char *buf/*out*/, double nbytes, double nseconds)
{
    double	bw;

    if(nseconds <= (double)0.0F)
        HDstrcpy(buf, "       NaN");
    else {
        bw = nbytes/nseconds;
        if(H5_DBL_ABS_EQUAL(bw, (double)0.0F))
            HDstrcpy(buf, "0.000  B/s");
        else if(bw < (double)1.0F)
            sprintf(buf, "%10.4e", bw);
        else if(bw < (double)H5_KB) {
            sprintf(buf, "%05.4f", bw);
            HDstrcpy(buf+5, "  B/s");
        } else if(bw < (double)H5_MB) {
            sprintf(buf, "%05.4f", bw / (double)H5_KB);
            HDstrcpy(buf+5, " kB/s");
        } else if(bw < (double)H5_GB) {
            sprintf(buf, "%05.4f", bw / (double)H5_MB);
            HDstrcpy(buf+5, " MB/s");
        } else if(bw < (double)H5_TB) {
            sprintf(buf, "%05.4f", bw / (double)H5_GB);
            HDstrcpy(buf+5, " GB/s");
        } else if(bw < (double)H5_PB) {
            sprintf(buf, "%05.4f", bw / (double)H5_TB);
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

