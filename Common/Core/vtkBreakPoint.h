// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBreakPoint
 * @brief   Utility function to debug with gdb and MPI.
 *
 * Wherever you need to set a break point inside a piece of code run by MPI,
 *
 * Step 1: call vtkBreakPoint::Break() in the code.
 * Step 2: start MPI, each process will display its PID and sleep.
 * Step 3: start gdb with the PID: \code gdb --pid=$PID \endcode
 * Step 4: set a breakpoint at the line of interest: \code (gdb) b $option \endcode
 * Step 5: go out of the sleep: (gdb) set var i=1
 * Original instructions at the OpenMPI FAQ:
 * http://www.open-mpi.de/faq/?category=debugging#serial-debuggers
 * - 6 Can I use serial debuggers (such as gdb) to debug MPI applications?
 * - 6.1. Attach to individual MPI processes after they are running.
 *
 * @par Implementation:
 * This function is in Common, not in Parallel because it does not depend on
 * MPI and you may want to call vtkBreakPoint::Break() in any class of VTK.
 */

#ifndef vtkBreakPoint_h
#define vtkBreakPoint_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"           // to get VTKCOMMONCORE_EXPORT

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkBreakPoint
{
public:
  ///@{
  /**
   * Process fall asleep until local variable `i' is set to a value different
   * from 0 inside a debugger.
   */
  static void Break();
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif // #ifndef vtkBreakPoint_h
// VTK-HeaderTest-Exclude: vtkBreakPoint.h
