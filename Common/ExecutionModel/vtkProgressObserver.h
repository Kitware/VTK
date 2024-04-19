// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProgressObserver
 * @brief   Basic class to optionally replace vtkAlgorithm progress functionality.
 *
 * When the basic functionality in vtkAlgorithm that reports progress is
 * not enough, a subclass of vtkProgressObserver can be used to provide
 * custom functionality.
 * The main use case for this is when an algorithm's RequestData() is
 * called from multiple threads in parallel - the basic functionality in
 * vtkAlgorithm is not thread safe. vtkSMPProgressObserver can
 * handle this situation by routing progress from each thread to a
 * thread local vtkProgressObserver, which will invoke events separately
 * for each thread.
 */

#ifndef vtkProgressObserver_h
#define vtkProgressObserver_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkProgressObserver : public vtkObject
{
public:
  static vtkProgressObserver* New();
  vtkTypeMacro(vtkProgressObserver, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The default behavior is to update the Progress data member
   * and invoke a ProgressEvent. This is designed to be overwritten.
   */
  virtual void UpdateProgress(double amount);

  ///@{
  /**
   * Returns the progress reported by the algorithm.
   */
  vtkGetMacro(Progress, double);
  ///@}

protected:
  vtkProgressObserver();
  ~vtkProgressObserver() override;

  double Progress;

private:
  vtkProgressObserver(const vtkProgressObserver&) = delete;
  void operator=(const vtkProgressObserver&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
