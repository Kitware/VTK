/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPProgressObserver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPProgressObserver - Progress observer that is thread safe
// .SECTION Description
// vtkSMPProgressObserver is designed to handle progress events coming
// from an algorithm in a thread safe way. It does this by using
// thread local objects that it updates. To receive the progress
// information, one has to listen to the local observer in the same
// thread. Since the execution will be somewhat load balanced,
// it may be enough to do this only on the main thread.

#ifndef __vtkSMPProgressObserver_h
#define __vtkSMPProgressObserver_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkProgressObserver.h"
#include "vtkSMPThreadLocalObject.h" // For thread local observers.

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkSMPProgressObserver : public vtkProgressObserver
{
public:
  static vtkSMPProgressObserver *New();
  vtkTypeMacro(vtkSMPProgressObserver,vtkProgressObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Passes the progress event to a thread local ProgressObserver
  // instance.
  void UpdateProgress(double amount);

  // Description:
  // Returns the progress observer local to the thread it was
  // called from.
  vtkProgressObserver* GetLocalObserver()
  {
    return this->Observers.Local();
  }

protected:
  vtkSMPProgressObserver();
  ~vtkSMPProgressObserver();

  vtkSMPThreadLocalObject<vtkProgressObserver> Observers;

private:
  vtkSMPProgressObserver(const vtkSMPProgressObserver&);  // Not implemented.
  void operator=(const vtkSMPProgressObserver&);  // Not implemented.
};

#endif
