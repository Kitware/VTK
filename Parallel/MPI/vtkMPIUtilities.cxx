/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIUtilities.h"

// VTK includes
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"

// C/C++ includes
#include <cassert>
#include <cstdarg>
#include <cstdio>

namespace vtkMPIUtilities
{


void Printf(vtkMPIController* comm, const char* format, ...)
{
  // Sanity checks
  assert("pre: MPI controller is NULL!" && (comm != NULL) );
  assert("pre: format argument is NULL!" && (format != NULL) );

  if( comm->GetLocalProcessId() == 0 )
  {
    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);
  }

  comm->Barrier();
}

//------------------------------------------------------------------------------
void SynchronizedPrintf(vtkMPIController* comm, const char* format, ...)
{
  // Sanity checks
  assert("pre: MPI controller is NULL!" && (comm != NULL) );
  assert("pre: format argument is NULL!" && (format != NULL) );

  int rank     = comm->GetLocalProcessId();
  int numRanks = comm->GetNumberOfProcesses();


  vtkMPICommunicator::Request rqst;
  int* nullmsg = NULL;

  if(rank == 0)
  {
    // STEP 0: print message
    printf("[%d]: ", rank);
    fflush(stdout);

    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);

    // STEP 1: signal next process (if any) to print
    if( numRanks > 1)
    {
      comm->NoBlockSend(nullmsg,0,rank+1,0,rqst);
    } // END if
  } // END first rank
  else if( rank == numRanks-1 )
  {
    // STEP 0: Block until previous process completes
    comm->Receive(nullmsg,0,rank-1,0);

    // STEP 1: print message
    printf("[%d]: ", rank);

    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);
  } // END last rank
  else
  {
    // STEP 0: Block until previous process completes
    comm->Receive(nullmsg,0,rank-1,0);

    // STEP 1: print message
    printf("[%d]: ", rank);

    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);

    // STEP 2: signal next process to print
    comm->NoBlockSend(nullmsg,0,rank+1,0,rqst);
  }

  comm->Barrier();
}

} // END namespace vtkMPIUtilities
