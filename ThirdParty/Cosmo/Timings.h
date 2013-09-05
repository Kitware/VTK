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

#ifndef TIMINGS_H
#define TIMINGS_H

/*************************************************************************
 * Timings - a simple singleton class which lets the user create and
 *   timers that can be printed out at the end of the program.
 *
 * General usage
 *  1) create a timer:
 *     Timings::TimerRef val = Timings::getTimer("timer name");
 *  This will either create a new one, or return a ref to an existing one
 *
 *  2) start a timer:
 *     Timings::startTimer(val);
 *  This will start the referenced timer running.  If it is already running,
 *  it will not change anything.
 *
 *  3) stop a timer:
 *     Timings::stopTimer(val);
 *  This will stop the timer, assuming it was running, and add in the
 *  time to the accumulating time for that timer.
 *
 *  4) print out the results:
 *     Timings::print();
 *
 *************************************************************************/

#include "Timer.h"
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

namespace cosmologytools {

//////////////////////////////////////////////////////////////////////
/*
  A simple compliant implementation of auto_ptr.
  This is from Greg Colvin's implementation posted to comp.std.c++.

  Instead of using mutable this casts away const in release.

  We have to do this because we can't build containers of these
  things otherwise.
  */
//////////////////////////////////////////////////////////////////////

template<class X>
class my_auto_ptr
{
  X* px;
public:
  my_auto_ptr() : px(0) {}
  my_auto_ptr(X* p) : px(p) {}
  my_auto_ptr(const my_auto_ptr<X>& r) : px(r.release()) {}
  my_auto_ptr& operator=(const my_auto_ptr<X>& r)
  {
    if (&r != this)
      {
	delete px;
	px = r.release();
      }
    return *this;
  }
  ~my_auto_ptr() { delete px; }
  X& operator*()  const { return *px; }
  X* operator->() const { return px; }
  X* get()        const { return px; }
  X* release()    const { X *p=px; ((my_auto_ptr<X>*)(this))->px=0; return p; }
};

// a simple class used to store timer values
class TimerInfo
{
public:
  // typedef for reference to a timer
  typedef int TimerRef;

  // constructor
  TimerInfo() : name(""), cpuTime(0.0), wallTime(0.0), indx(-1) {
    clear();
  }

  // destructor
  ~TimerInfo() { }

  // timer operations
  void start() {
    if (!running) {
      running = true;
      t.stop();
      t.clear();
      t.start();
    }
  }

  void stop() {
    if (running) {
      t.stop();
      running = false;
      cpuTime += t.cpu_time();
      wallTime += t.clock_time();
    }
  }

  void clear() {
    t.stop();
    t.clear();
    running = false;
  }

  // the POOMA timer that this object manages
  Timer t;

  // the name of this timer
  string name;

  // the accumulated time
  double cpuTime;
  double wallTime;

  // is the timer turned on right now?
  bool running;

  // an index value for this timer
  TimerRef indx;
};



class Timings
{
public:
  // typedef for reference to a timer
  typedef int TimerRef;

  // a typedef for the timer information object
  typedef TimerInfo TimerInfo_t;

public:
  // Default constructor
  Timings();

  // Destructor - clear out the existing timers
  ~Timings();

  //
  // timer manipulation methods
  //

  // create a timer, or get one that already exists
  static TimerRef getTimer(const char *);

  // start a timer
  static void startTimer(TimerRef);

  // stop a timer, and accumulate it's values
  static void stopTimer(TimerRef);

  // clear a timer, by turning it off and throwing away its time
  static void clearTimer(TimerRef);

  // return a TimerInfo struct by asking for the name
  static TimerInfo_t *infoTimer(const char *nm) {
    return TimerMap[string(nm)];
  }

  //
  // I/O methods
  //

  // print the results to standard out
  static void print();

private:
  // type of storage for list of TimerInfo
  typedef vector<my_auto_ptr<TimerInfo> > TimerList_t;
  typedef map<string, TimerInfo *> TimerMap_t;

  // a list of timer info structs
  static TimerList_t TimerList;

  // a map of timers, keyed by string
  static TimerMap_t TimerMap;
};

}
#endif
