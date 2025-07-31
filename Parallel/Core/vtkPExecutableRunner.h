// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPExecutableRunner
 *
 * vtkPExecutableRunner provides a way to launch commands processes and control for multi-process
 * use cases. This class can contains a process id attribute to specifiy on which process the
 * command will be executed.
 */

#ifndef vtkPExecutableRunner_h
#define vtkPExecutableRunner_h

#include "vtkExecutableRunner.h"
#include "vtkParallelCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKPARALLELCORE_EXPORT vtkPExecutableRunner : public vtkExecutableRunner
{
public:
  static vtkPExecutableRunner* New();
  vtkTypeMacro(vtkPExecutableRunner, vtkExecutableRunner);

  ///@{
  /**
   * Process id where commands will be executed.
   * Note: Default value is 0. If its value is -1, then all processes will execute commands.
   */
  vtkGetMacro(ExecutionProcessId, int);
  vtkSetMacro(ExecutionProcessId, int);
  ///@}

  /**
   * This function will check the ExecutionProcessId and then execute using Superclass
   * implementation if it is correct.
   */
  void Execute() override;

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPExecutableRunner() = default;
  ~vtkPExecutableRunner() override = default;

private:
  int ExecutionProcessId = 0;

  vtkPExecutableRunner(const vtkPExecutableRunner&) = delete;
  void operator=(const vtkPExecutableRunner&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
