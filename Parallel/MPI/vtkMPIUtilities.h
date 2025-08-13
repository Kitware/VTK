// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMPIUtilities_h
#define vtkMPIUtilities_h

#include "vtkDeprecation.h"     // For VTK_DEPRECATED_IN_9_5_0
#include "vtkMPIController.h"   // For vtkMPIController
#include "vtkStringFormatter.h" // For vtk::print

#include <cassert> // For assert
#include <string>  // For std::string

namespace vtkMPIUtilities
{
VTK_ABI_NAMESPACE_BEGIN

///@{
/**
 * Rank 0 prints the user-supplied formatted message to stdout.
 * This method works just like vtk::print, but, requires an additional
 * argument to specify the MPI controller for the application.
 * NOTE: This is a collective operation, all ranks in the given communicator
 * must call this method. The format should follow std::format.
 */
template <typename... T>
void Print(vtkMPIController* comm, const char* format, T&&... args)
{
  assert(comm != nullptr);
  if (comm->GetLocalProcessId() == 0)
  {
    vtk::print(format, std::forward<T>(args)...);
    std::fflush(stdout);
  }
  comm->Barrier();
}
template <typename... T>
VTK_DEPRECATED_IN_9_6_0("Use vtkMPIUtilities::Print instead")
void Printf(vtkMPIController* comm, const char* formatArg, T&&... args)
{
  std::string format = formatArg ? formatArg : "";
  if (vtk::is_printf_format(format))
  {
    format = vtk::printf_to_std_format(format);
  }
  assert(comm != nullptr);
  vtkMPIUtilities::Print(comm, format.c_str(), std::forward<T>(args)...);
}
///@}

///@{
/**
 * Each rank, r_0 to r_{N-1}, prints the formatted message to stdout in
 * rank order. That is, r_i prints the supplied message right after r_{i-1}.
 * NOTE: This is a collective operation, all ranks in the given communicator
 * must call this method. The format should follow std::format.
 */
template <typename... T>
void SynchronizedPrint(vtkMPIController* comm, const char* format, T&&... args)
{
  assert(comm != nullptr);
  int rank = comm->GetLocalProcessId();
  int numRanks = comm->GetNumberOfProcesses();

  vtkMPICommunicator::Request rqst;
  int* nullmsg = nullptr;

  if (rank == 0)
  {
    // STEP 0: print message
    vtk::print("[{:d}]: ", rank);
    std::fflush(stdout);

    vtk::print(format, std::forward<T>(args)...);
    std::fflush(stdout);

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
    vtk::print("[{:d}]: ", rank);

    vtk::print(format, std::forward<T>(args)...);
    std::fflush(stdout);
  } // END last rank
  else
  {
    // STEP 0: Block until previous process completes
    comm->Receive(nullmsg, 0, rank - 1, 0);

    // STEP 1: print message
    vtk::print("[{:d}]: ", rank);

    vtk::print(format, std::forward<T>(args)...);
    std::fflush(stdout);

    // STEP 2: signal next process to print
    comm->NoBlockSend(nullmsg, 0, rank + 1, 0, rqst);
  }

  comm->Barrier();
}
template <typename... T>
VTK_DEPRECATED_IN_9_6_0("Use vtkMPIUtilities::SynchronizedPrint instead")
void SynchronizedPrintf(vtkMPIController* comm, const char* formatArg, T&&... args)
{
  std::string format = formatArg ? formatArg : "";
  if (vtk::is_printf_format(format))
  {
    format = vtk::printf_to_std_format(format);
  }
  assert(comm != nullptr);
  vtkMPIUtilities::SynchronizedPrint(comm, format.c_str(), std::forward<T>(args)...);
}
///@}
VTK_ABI_NAMESPACE_END
} // END namespace vtkMPIUtilities

#endif // vtkMPIUtilities_h
// VTK-HeaderTest-Exclude: vtkMPIUtilities.h
