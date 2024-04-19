// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkDummyCommunicator
 * @brief   Dummy controller for single process applications.
 *
 *
 *
 * This is a dummy communicator, which can be used by applications that always
 * require a controller but are also compiled on systems without threads or MPI.
 * Because there is always only one process, no real communication takes place.
 *
 */

#ifndef vtkDummyCommunicator_h
#define vtkDummyCommunicator_h

#include "vtkCommunicator.h"
#include "vtkParallelCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKPARALLELCORE_EXPORT vtkDummyCommunicator : public vtkCommunicator
{
public:
  vtkTypeMacro(vtkDummyCommunicator, vtkCommunicator);
  static vtkDummyCommunicator* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Since there is no one to communicate with, these methods just report an
   * error.
   */
  int SendVoidArray(const void*, vtkIdType, int, int, int) override
  {
    vtkWarningMacro("There is no one to send to.");
    return 0;
  }
  int ReceiveVoidArray(void*, vtkIdType, int, int, int) override
  {
    vtkWarningMacro("There is no one to receive from.");
    return 0;
  }
  ///@}

protected:
  vtkDummyCommunicator();
  ~vtkDummyCommunicator() override;

private:
  vtkDummyCommunicator(const vtkDummyCommunicator&) = delete;
  void operator=(const vtkDummyCommunicator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDummyCommunicator_h
