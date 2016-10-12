/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgressObserver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkProgressObserver : public vtkObject
{
public:
  static vtkProgressObserver *New();
  vtkTypeMacro(vtkProgressObserver,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * The default behavior is to update the Progress data member
   * and invoke a ProgressEvent. This is designed to be overwritten.
   */
  virtual void UpdateProgress(double amount);

  //@{
  /**
   * Returns the progress reported by the algorithm.
   */
  vtkGetMacro(Progress, double);
  //@}

protected:
  vtkProgressObserver();
  ~vtkProgressObserver() VTK_OVERRIDE;

  double Progress;

private:
  vtkProgressObserver(const vtkProgressObserver&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProgressObserver&) VTK_DELETE_FUNCTION;
};

#endif
