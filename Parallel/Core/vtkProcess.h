// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProcess
 * @brief   a process that can be launched by a vtkMultiProcessController
 *
 * vtkProcess is an abstract class representing a process that can be launched
 * by a vtkMultiProcessController. Concrete classes just have to implement
 * Execute() method and make sure it set the proper value in ReturnValue.
 *
 * @par Example:
 *  class MyProcess: public vtkProcess
 *  ...
 *  vtkMultiProcessController *c;
 *  MyProcess *p=new MyProcess::New();
 *  p->SetArgs(argc,argv); // some parameters specific to the process
 *  p->SetX(10.0); // ...
 *  c->SetSingleProcess(p);
 *  c->SingleMethodExecute();
 *  int returnValue=p->GetReturnValue();
 *
 * @sa
 * vtkMultiProcessController
 */

#ifndef vtkProcess_h
#define vtkProcess_h

#include "vtkObject.h"
#include "vtkParallelCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKPARALLELCORE_EXPORT vtkProcess : public vtkObject
{
public:
  vtkTypeMacro(vtkProcess, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Entry point of the process.
   * This method is expected to update ReturnValue.
   */
  virtual void Execute() = 0;

  /**
   * Give access to the controller that launched the process.
   * Initial value is nullptr.
   */
  vtkMultiProcessController* GetController();

  /**
   * This method should not be called directly but set by the controller
   * itself.
   */
  void SetController(vtkMultiProcessController* aController);

  /**
   * Value set at the end of a call to Execute.
   */
  int GetReturnValue();

protected:
  vtkProcess();
  ~vtkProcess() override;

  vtkMultiProcessController* Controller;
  int ReturnValue;

private:
  vtkProcess(const vtkProcess&) = delete;
  void operator=(const vtkProcess&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
