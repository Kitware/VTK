/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyController.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDummyController - Dummy controller for single process applications
// .SECTION Description
// This is a dummy controller which can be used by applications which always
// require a controller but are also compile on systems without threads
// or mpi.
// .SECTION see also
// vtkMultiProcessController 

#ifndef __vtkDummyController_h
#define __vtkDummyController_h

#include "vtkMultiProcessController.h"

class VTK_PARALLEL_EXPORT vtkDummyController : public vtkMultiProcessController
{
public:
  static vtkDummyController *New();
  vtkTypeRevisionMacro(vtkDummyController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting up the processes.
  virtual void Initialize(int*, char***, int) {}
  virtual void Initialize(int*, char***) {}
  virtual void Finalize() {}
  virtual void Finalize(int) {}

  // Description:
  // This method always returns 0.
  int GetLocalProcessId() { return 0; }

  // Description:
  // Does nothing.
  virtual void SingleMethodExecute() {}
  
  // Description:
  // Does nothing.
  virtual void MultipleMethodExecute() {}

  // Description:
  // Does nothing.
  virtual void Barrier() {}

  // Description:
  // Does nothing.
  virtual void CreateOutputWindow() {}

protected:
  vtkDummyController() {}
  ~vtkDummyController() {}
  
private:
  vtkDummyController(const vtkDummyController&);  // Not implemented.
  void operator=(const vtkDummyController&);  // Not implemented.
};

#endif


