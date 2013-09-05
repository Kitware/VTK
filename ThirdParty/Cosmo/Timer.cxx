/*=========================================================================
                                                                                
Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
                                                                                
=========================================================================*/

// .NAME Timer - create timer for program execution
//
// .SECTION Description
// The Timer class allows for easy timing of the program.  The timer
// tracks real (clock) time elapsed, user time, and system time.

#include "Timer.h"

#define TIMEROFF	0
#define TIMERON		1

namespace cosmologytools {
/************************************************************************/
/*									*/
/*			FUNCTION Timer					*/
/*									*/
/*	This is the constructor for the class Timer.  It sets the timer */
/*  status to TIMEROFF and clears all the values.  It also makes a call */
/*  to sysconf to determine the number of clock ticks per second for    */
/*  use with the call times()						*/
/*  It also makes calibration calls.                                    */
/*									*/
/************************************************************************/

Timer::Timer()
{
#ifdef __MWERKS__
  // For now, stub out all Timer guts for MetroWerks
#else

#ifndef POOMA_TFLOP
  cpu_speed = sysconf(_SC_CLK_TCK);
#endif
  timer_state = TIMEROFF;
  clear();

  // Calibration:
#if defined(POOMA_T3E)
  long start_time, end_time, total_time;
  (void) rtclock();
  start_time = rtclock();
  end_time = rtclock();
  total_time = end_time - start_time;
  calibration = tick_secs(total_time, cpu_speed);
#else
  // No other machines have calibration defined yet.
  calibration = 0.0;
#endif

#endif // __MWERKS__
}
/*			END OF FUNCTION Timer				*/

Timer::~Timer()
{
#ifdef __MWERKS__
  // For now, stub out all Timer guts for MetroWerks
#else

  // Check to see if the timer is on
  if (timer_state == TIMERON)
    {
      //  Destroying a running Timer
      // ERRORMSG(level2 << "TRIED TO DELETE A RUNNING TIMER!\n");
      // ERRORMSG("STOPPING AND DELETING TIMER." << endl);
      timer_state=TIMEROFF;
    }

#endif // __MWERKS__
}

/************************************************************************/
/*									*/
/*			FUNCTION clear					*/
/*									*/
/*	clear sets all of the accumulated times for this timer to 0.	*/
/*  It is intended to only be used on a stopped timer.  If it is used	*/
/*  on a running timer, a warning message is printed, the timer is      */
/*  stopped and all of its values are cleared.				*/
/*									*/
/************************************************************************/

void Timer::clear()
{
#ifdef __MWERKS__
  // For now, stub out all Timer guts for MetroWerks
  return;
#else
  // Check to see if the timer if on
  if (timer_state == TIMERON)
    {
      //  Clearing a running Timer
      // ERRORMSG(level2 << "TRIED TO CLEAR A RUNNING TIMER!\n");
      // ERRORMSG("SETTING ALL VALUES TO 0 AND STOPPING TIMER." << endl);
      timer_state = TIMEROFF;
    }

  //  Set all of the accumulated values to 0
#ifdef POOMA_TFLOP
  current_clock = 0.0;
#else
  current_secs = 0;
  current_usecs = 0;
  current_user_time = 0;
  current_system_time = 0;
#endif // POOMA_TFLOP

#endif // __MWERKS__
}
/*			END OF FUNCTION clear				*/

/************************************************************************/
/*									*/
/*			FUNCTION start					*/
/*									*/
/*	start a Timer timing.  This will start adding time elapsed to   */
/*  the current accumulated values of the timer.  If you try to start   */
/*  a timer that is already running, a warning message is printed	*/
/*									*/
/************************************************************************/

void Timer::start()
{
#ifdef __MWERKS__
  // For now, stub out all Timer guts for MetroWerks
  return;
#else
  //  Check to see if the timer is already running
  if (timer_state == TIMERON)
    {
      // ERRORMSG(level2 << "TRIED TO START A RUNNING TIMER!\n");
      // ERRORMSG("CONTINUING UNCHANGED." << endl);
      return;
    }

  //  Get the current time values from the system
#if defined(POOMA_T3E)
  last_secs = rtclock();
  // Omit non-real times on T3E:
  last_usecs = 0;
  last_user_time = 0;
  last_system_time = 0;
#elif defined(POOMA_TFLOP)
  last_clock = dclock();
#else
  gettimeofday(&tvbuf, &tzbuf);
  times(&tmsbuf);
  //  Set the starting values to the current time
  last_secs = tvbuf.tv_sec;
  last_usecs = tvbuf.tv_usec;
  last_user_time = tmsbuf.tms_utime;
  last_system_time = tmsbuf.tms_stime;
#endif

  //  Set the state of the Timer
  timer_state = TIMERON;
  return;
#endif // __MWERKS__
}
/*			END OF FUNCTION start				*/

/************************************************************************/
/*									*/
/*				FUNCITON stop				*/
/*									*/
/*	stop stops a Timer from accumulating time.  If you try to stop */
/*  a stopped Timer, a warning message is printed			*/
/*									*/
/************************************************************************/

void Timer::stop()
{
#ifdef __MWERKS__
  // For now, stub out all Timer guts for MetroWerks
  return;
#else
  //  Check to see if the timer is already stopped
  if (timer_state == TIMEROFF)
    {
      // ERRORMSG(level2 << "TRIED TO STOP A STOPPED TIMER!\n");
      // ERRORMSG("CONTINUING UNCHANGED." << endl);
      return;
    }

  //  Get the current time values from the system and accumulate
#if defined(POOMA_T3E)
  long end_time = rtclock();

  current_secs +=  end_time - last_secs;
  current_usecs += 0;
  current_user_time += 0;
  current_system_time += 0;
#elif defined(POOMA_TFLOP)
  double end_clock = dclock();
  current_clock += end_clock - last_clock;
#else
  gettimeofday(&tvbuf, &tzbuf);
  times(&tmsbuf);

  current_secs += tvbuf.tv_sec - last_secs;
  current_usecs += tvbuf.tv_usec - last_usecs;
  current_user_time += tmsbuf.tms_utime - last_user_time;
  current_system_time += tmsbuf.tms_stime - last_system_time;
#endif

  //  Set the state of the Timer
  timer_state = TIMEROFF;
  return;
#endif // __MWERKS__
}
/*			END OF FUNCTION stop				*/

/************************************************************************/
/*									*/
/*			FUNCTION clock_time				*/
/*									*/
/*	clock_time returns the current amount of real (clock) time	*/
/*  accumulated by this timer.  If the timer is stopped, this is just	*/
/*  the total accumulated time.  If the timer is running, this is the	*/
/*  accumulated time plus the time since the timer was last started.	*/
/*									*/
/************************************************************************/

double Timer::clock_time()
{
#ifdef __MWERKS__
  // For now, stub out all Timer guts for MetroWerks
  return 0.0;
#else

#if !defined(POOMA_TFLOP)
  long seconds;	    // seconds elapsed
  
#if !defined(POOMA_T3E)
  long useconds;    // useconds (mirco-seconds) elapsed
#endif

#endif

  double ret_val;    // time elpased

  if (timer_state == TIMEROFF)
    {
      // Timer is currently off, so just return accumulated time
#if !defined(POOMA_TFLOP)
      seconds = current_secs;
      
#if !defined(POOMA_T3E)
      useconds = current_usecs;
#endif

#else
      ret_val = current_clock;
#endif
    }
  else
    {
      // Timer is currently running, so add the elapsed
      // time since the timer was last started to the
      // accumulated time
#if defined(POOMA_T3E)
      long end_time = rtclock();
      seconds = current_secs + end_time - last_secs;
#elif defined(POOMA_TFLOP)
      double end_clock = dclock();
      ret_val = current_clock + end_clock - last_clock;
#else
      gettimeofday(&tvbuf, &tzbuf);

      seconds = current_secs + tvbuf.tv_sec - last_secs;
      useconds = current_usecs + tvbuf.tv_usec - last_usecs;
#endif
    }

  //  Convert into floating point number of seconds
#if defined(POOMA_T3E)
  ret_val = tick_secs(seconds, cpu_speed);
#elif defined(POOMA_TFLOP)
  // no need to convert
#else
  //  Adjust for the fact that the useconds may be negative.
  //  If they are, take away 1 second and add 1 million
  //  microseconds until they are positive
  while (useconds < 0)
    {
      useconds = useconds + 1000000;
      seconds = seconds - 1;
    }

  long long_ret_val = (1000000 * seconds) + useconds;
  ret_val = ( (double) long_ret_val ) / 1000000.0;
#endif

  return ret_val;
  
#endif // __MWERKS__
}
/*			END OF FUNCTION clock_time			*/

/************************************************************************/
/*									*/
/*			FUNCTION user_time				*/
/*									*/
/*	user_time reports the current amount of user cpu time           */
/*   accumulated by this Timer.  If the timer is currently off, 	*/
/*   this is just the accumulated time.  If the Timer is running, this  */
/*   is the accumulated time plust the time since the timer was last    */
/*   started.								*/
/*									*/
/************************************************************************/

double Timer::user_time()
{
#ifdef __MWERKS__
// For now, stub out all Timer guts for MetroWerks
  return 0.0;
#else
  double ret_val;		//  Return value	

#if ( defined(POOMA_T3E) || defined(POOMA_TFLOP) )
  // Not defined yet on T3E or TFLOP.
  // ERRORMSG("user_time() not defined." << endl);
  ret_val = -9999.0;
#else
  if (timer_state == TIMEROFF)
    {
      //  Timer is off, just return accumulated time
      ret_val = current_user_time;
    }
  else
    {
      //  Timer is on, add current running time to accumulated time
      times(&tmsbuf);
      ret_val = current_user_time + tmsbuf.tms_utime - last_user_time;
    }

  //  Convert from clock ticks to seconds using the
  //  cpu_speed value obtained by the constructor
  ret_val = ret_val / cpu_speed;
#endif

  return ret_val;
#endif // __MWERKS__
}
/*			END OF FUNCTION user_time			*/

/************************************************************************/
/*									*/
/*			FUNCTION system_time				*/
/*									*/
/*	system_time reports the current amount of system cpu time       */
/*   accumulated by this Timer.  If the timer is currently off, 	*/
/*   this is just the accumulated time.  If the Timer is running, this  */
/*   is the accumulated time plus the time since the timer was last     */
/*   started.								*/
/*									*/
/************************************************************************/

double Timer::system_time()
{
#ifdef __MWERKS__
// For now, stub out all Timer guts for MetroWerks
  return 0.0;
#else
  double ret_val;		//  Return value

#if ( defined(POOMA_T3E) || (POOMA_TFLOP) )
  // Not defined yet on T3E or TFLOP.
  // ERRORMSG("system_time() not defined." << endl);
  ret_val = -9999.0;
#else
  if (timer_state == TIMEROFF)
    {
      //  Timer is off, just return accumulated time
      ret_val = current_system_time;
    }
  else
    {
      //  Timer is on, return accumulated plus current
      times(&tmsbuf);
      ret_val = current_system_time + tmsbuf.tms_stime - last_system_time;
    }

  //  Convert from clock ticks to seconds using the
  //  cpu_speed value obtained by the constructor
  ret_val = ret_val / cpu_speed;
#endif

  return ret_val;
#endif // __MWERKS__
}

}
