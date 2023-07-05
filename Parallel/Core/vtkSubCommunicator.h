// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkSubCommunicator
 * @brief   Provides communication on a process group.
 *
 *
 *
 * This class provides an implementation for communicating on process groups.
 * In general, you should never use this class directly.  Instead, use the
 * vtkMultiProcessController::CreateSubController method.
 *
 *
 * @bug
 * Because all communication is delegated to the original communicator,
 * any error will report process ids with respect to the original
 * communicator, not this communicator that was actually used.
 *
 * @sa
 * vtkCommunicator, vtkMultiProcessController
 *
 * @par Thanks:
 * This class was originally written by Kenneth Moreland (kmorel@sandia.gov)
 * from Sandia National Laboratories.
 *
 */

#ifndef vtkSubCommunicator_h
#define vtkSubCommunicator_h

#include "vtkCommunicator.h"
#include "vtkParallelCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkProcessGroup;

class VTKPARALLELCORE_EXPORT vtkSubCommunicator : public vtkCommunicator
{
public:
  vtkTypeMacro(vtkSubCommunicator, vtkCommunicator);
  static vtkSubCommunicator* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the group on which communication will happen.
   */
  vtkGetObjectMacro(Group, vtkProcessGroup);
  virtual void SetGroup(vtkProcessGroup* group);
  ///@}

  ///@{
  /**
   * Implementation for abstract supercalss.
   */
  int SendVoidArray(
    const void* data, vtkIdType length, int type, int remoteHandle, int tag) override;
  int ReceiveVoidArray(void* data, vtkIdType length, int type, int remoteHandle, int tag) override;
  ///@}

protected:
  vtkSubCommunicator();
  ~vtkSubCommunicator() override;

  vtkProcessGroup* Group;

private:
  vtkSubCommunicator(const vtkSubCommunicator&) = delete;
  void operator=(const vtkSubCommunicator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkSubCommunicator_h
