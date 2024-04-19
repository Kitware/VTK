// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDummyController
 * @brief   Dummy controller for single process applications
 *
 * This is a dummy controller which can be used by applications which always
 * require a controller but are also compile on systems without threads
 * or mpi.
 * @sa
 * vtkMultiProcessController
 */

#ifndef vtkDummyController_h
#define vtkDummyController_h

#include "vtkMultiProcessController.h"
#include "vtkParallelCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKPARALLELCORE_EXPORT vtkDummyController : public vtkMultiProcessController
{
public:
  static vtkDummyController* New();
  vtkTypeMacro(vtkDummyController, vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method is for setting up the processes.
   */
  void Initialize(int*, char***, int) override {}
  void Initialize(int*, char***) override {}
  void Finalize() override {}
  void Finalize(int) override {}

  /**
   * This method always returns 0.
   */
  int GetLocalProcessId() { return 0; }

  /**
   * Directly calls the single method.
   */
  void SingleMethodExecute() override;

  /**
   * Directly calls multiple method 0.
   */
  void MultipleMethodExecute() override;

  /**
   * Does nothing.
   */
  void CreateOutputWindow() override {}

  ///@{
  /**
   * If you don't need any special functionality from the controller, you
   * can swap out the dummy communicator for another one.
   */
  vtkGetObjectMacro(RMICommunicator, vtkCommunicator);
  virtual void SetCommunicator(vtkCommunicator*);
  virtual void SetRMICommunicator(vtkCommunicator*);
  ///@}

protected:
  vtkDummyController();
  ~vtkDummyController() override;

private:
  vtkDummyController(const vtkDummyController&) = delete;
  void operator=(const vtkDummyController&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
