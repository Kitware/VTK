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

#include "Timings.h"
#include <iostream>
#include <cstring>


#include <mpi.h>

using namespace std;

// .NAME Timings - create timer for program execution
//
// .SECTION Description
// The Timer class allows for easy timing of the program.  The timer
// tracks real (clock) time elapsed, user time, and system time.

namespace cosmologytools {

// static data members of Timings class
Timings::TimerList_t Timings::TimerList;
Timings::TimerMap_t  Timings::TimerMap;


//////////////////////////////////////////////////////////////////////
// default constructor
Timings::Timings() { }


//////////////////////////////////////////////////////////////////////
// destructor
Timings::~Timings() { }


//////////////////////////////////////////////////////////////////////
// create a timer, or get one that already exists
Timings::TimerRef Timings::getTimer(const char *nm) {
  string s(nm);
  TimerInfo *tptr = 0;
  TimerMap_t::iterator loc = TimerMap.find(s);
  if (loc == TimerMap.end()) {
    tptr = new TimerInfo;
    tptr->indx = TimerList.size();
    tptr->name = s;
    TimerMap.insert(TimerMap_t::value_type(s,tptr));
    TimerList.push_back(my_auto_ptr<TimerInfo>(tptr));
  } else {
    tptr = (*loc).second;
  }
  return tptr->indx;
}


//////////////////////////////////////////////////////////////////////
// start a timer
void Timings::startTimer(TimerRef t) {
  if (t < 0 || t >= (int) TimerList.size())
    return;
  TimerList[t]->start();
}


//////////////////////////////////////////////////////////////////////
// stop a timer, and accumulate it's values
void Timings::stopTimer(TimerRef t) {
  if (t < 0 || t >= (int) TimerList.size())
    return;
  TimerList[t]->stop();
}


//////////////////////////////////////////////////////////////////////
// clear a timer, by turning it off and throwing away its time
void Timings::clearTimer(TimerRef t) {
  if (t < 0 || t >= (int) TimerList.size())
    return;
  TimerList[t]->clear();
}


//////////////////////////////////////////////////////////////////////
// print out the timing results
void Timings::print() {
  int i,j;
  if (TimerList.size() < 1)
    return;

  int nodes, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &nodes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // report the average time for each timer
  if (rank == 0) {
    cout << "-----------------------------------------------------------------";
    cout << endl;
    cout << "     Timing results for " << nodes << " nodes:" << endl;
    cout << "-----------------------------------------------------------------";
    cout << endl;
  }
  for (i=0; i<1; ++i){
    TimerInfo *tptr = TimerList[i].get();
    double walltotal = 0.0, cputotal = 0.0;

    MPI_Reduce(&tptr->wallTime, &walltotal, 1, MPI_DOUBLE, MPI_MAX, 0,
        MPI_COMM_WORLD);
    MPI_Reduce(&tptr->cpuTime, &cputotal, 1, MPI_DOUBLE, MPI_MAX, 0,
        MPI_COMM_WORLD);

    if (rank == 0) {
      cout << tptr->name.c_str() << " ";
      for (j=strlen(tptr->name.c_str()); j < 20; ++j)
        cout << ".";
      cout << " Wall tot = ";
      cout.width(10);
      cout << walltotal << ", CPU tot = ";
      cout.width(10);
      cout << cputotal << endl << endl;
    }
  }

  for (i=1; i < (int) TimerList.size(); ++i) {
    TimerInfo *tptr = TimerList[i].get();
    double wallmax = 0.0, cpumax = 0.0, wallmin = 0.0, cpumin = 0.0;
    double wallavg = 0.0, cpuavg = 0.0;

    MPI_Reduce(&tptr->wallTime, &wallmax, 1, MPI_DOUBLE, MPI_MAX, 0,
        MPI_COMM_WORLD);
    MPI_Reduce(&tptr->cpuTime, &cpumax, 1, MPI_DOUBLE, MPI_MAX, 0,
        MPI_COMM_WORLD);
    MPI_Reduce(&tptr->wallTime, &wallmin, 1, MPI_DOUBLE, MPI_MIN, 0,
        MPI_COMM_WORLD);
    MPI_Reduce(&tptr->cpuTime, &cpumin, 1, MPI_DOUBLE, MPI_MIN, 0,
        MPI_COMM_WORLD);
    MPI_Reduce(&tptr->wallTime, &wallavg, 1, MPI_DOUBLE, MPI_SUM, 0,
        MPI_COMM_WORLD);
    MPI_Reduce(&tptr->cpuTime, &cpuavg, 1, MPI_DOUBLE, MPI_SUM, 0,
        MPI_COMM_WORLD);

    if (rank == 0) {
      cout << tptr->name.c_str() << " ";
      for (j=strlen(tptr->name.c_str()); j < 20; ++j)
        cout << ".";
      cout << " Wall max = ";
      cout.width(10);
      cout << wallmax << ", CPU max = ";
      cout.width(10);
      cout << cpumax << endl;
      for (j = 0; j < 21; ++j)
        cout << " ";
      cout << " Wall avg = ";
      cout.width(10);
      cout << wallavg / nodes << ", CPU avg = ";
      cout.width(10);
      cout << cpuavg / nodes << endl;
      for (j = 0; j < 21; ++j)
        cout << " ";
      cout << " Wall min = ";
      cout.width(10);
      cout << wallmin << ", CPU min = ";
      cout.width(10);
      cout << cpumin << endl << endl;
    }
  }
  if (rank == 0) {
    cout << "-----------------------------------------------------------------";
    cout << endl;
  }
}

}
