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

#ifndef TIMER_H
#define TIMER_H

#ifdef __sgi
// make sure this is defined for BSD time routines
#define _BSD_TYPES
// fix a glitch in ANSI compatibility with SGI headers
#define _STAMP_T
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>

#ifdef __sgi
// fix a glitch in ANSI compatibility with SGI headers
#undef _STAMP_T
#endif


namespace cosmologytools {
class Timer
{
public:
  Timer();			// Constructor
  ~Timer();                     // Destructor
  void clear();			// Set all accumulated times to 0
  void start();			// Start timer
  void stop();			// Stop timer

  double clock_time();		// Report clock time accumulated in seconds
  double user_time();		// Report user time accumlated in seconds
  double system_time();		// Report system time accumulated in seconds
  double cpu_time()
  {
    // Report total cpu_time which is just user_time + system_time
    return ( user_time() + system_time() );
  }		

  double calibration;		// Calibration time: time it takes to
                                // get in and out of timer functions
private:
  short timer_state;		// State of timer, either on or off
  long cpu_speed;		  // CPU speed for times() call

  unsigned long last_secs;	  // Clock seconds value when the
				  // timer was last started
  long last_usecs;		  // Clock useconds value when the
				  // timer was last started
  unsigned long last_user_time;   // User time when timer was last started
  unsigned long last_system_time; // System time when timer was last started

  long current_secs;		// Current accumulated clock seconds
  long current_usecs;		// Current accumulated clock useconds
  long current_user_time;	// Current accumulated user time
  long current_system_time;	// Current accumulated system time

  struct tms tmsbuf;	        //  Values from call to times
  struct timeval tvbuf;	        //  Values from call to gettimeofday
  struct timezone tzbuf;        //  Timezone values from gettimeofday
	  		        //  These values aren't used for anything
};

}
#endif
