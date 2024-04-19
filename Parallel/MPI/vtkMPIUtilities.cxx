// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
VTK_ABI_NAMESPACE_BEGIN

void Printf(vtkMPIController* comm, const char* format, ...)
{
  // Sanity checks
  assert("pre: MPI controller is nullptr!" && (comm != nullptr));
  assert("pre: format argument is nullptr!" && (format != nullptr));

  if (comm->GetLocalProcessId() == 0)
  {
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    fflush(stdout);
    va_end(argptr);
  }

  comm->Barrier();
}

//------------------------------------------------------------------------------
void SynchronizedPrintf(vtkMPIController* comm, const char* format, ...)
{
  // Sanity checks
  assert("pre: MPI controller is nullptr!" && (comm != nullptr));
  assert("pre: format argument is nullptr!" && (format != nullptr));

  int rank = comm->GetLocalProcessId();
  int numRanks = comm->GetNumberOfProcesses();

  vtkMPICommunicator::Request rqst;
  int* nullmsg = nullptr;

  if (rank == 0)
  {
    // STEP 0: print message
    printf("[%d]: ", rank);
    fflush(stdout);

    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    fflush(stdout);
    va_end(argptr);

    // STEP 1: signal next process (if any) to print
    if (numRanks > 1)
    {
      comm->NoBlockSend(nullmsg, 0, rank + 1, 0, rqst);
    } // END if
  }   // END first rank
  else if (rank == numRanks - 1)
  {
    // STEP 0: Block until previous process completes
    comm->Receive(nullmsg, 0, rank - 1, 0);

    // STEP 1: print message
    printf("[%d]: ", rank);

    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    fflush(stdout);
    va_end(argptr);
  } // END last rank
  else
  {
    // STEP 0: Block until previous process completes
    comm->Receive(nullmsg, 0, rank - 1, 0);

    // STEP 1: print message
    printf("[%d]: ", rank);

    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    fflush(stdout);
    va_end(argptr);

    // STEP 2: signal next process to print
    comm->NoBlockSend(nullmsg, 0, rank + 1, 0, rqst);
  }

  comm->Barrier();
}

VTK_ABI_NAMESPACE_END
} // END namespace vtkMPIUtilities
