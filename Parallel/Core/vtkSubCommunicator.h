// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkSubCommunicator - Provides communication on a process group.
//
// .SECTION Description
//
// This class provides an implementation for communicating on process groups.
// In general, you should never use this class directly.  Instead, use the
// vtkMultiProcessController::CreateSubController method.
//
// .SECTION BUGS
//
// Because all communication is delegated to the original communicator,
// any error will report process ids with respect to the original
// communicator, not this communicator that was actually used.
//
// .SECTION See Also
// vtkCommunicator, vtkMultiProcessController
//
// .SECTION Thanks
// This class was originally written by Kenneth Moreland (kmorel@sandia.gov)
// from Sandia National Laboratories.
//

#ifndef __vtkSubCommunicator_h
#define __vtkSubCommunicator_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkCommunicator.h"

class vtkProcessGroup;

class VTKPARALLELCORE_EXPORT vtkSubCommunicator : public vtkCommunicator
{
public:
  vtkTypeMacro(vtkSubCommunicator, vtkCommunicator);
  static vtkSubCommunicator *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/get the group on which communication will happen.
  vtkGetObjectMacro(Group, vtkProcessGroup);
  virtual void SetGroup(vtkProcessGroup *group);

  // Description:
  // Implementation for abstract supercalss.
  virtual int SendVoidArray(const void *data, vtkIdType length, int type,
                            int remoteHandle, int tag);
  virtual int ReceiveVoidArray(void *data, vtkIdType length, int type,
                               int remoteHandle, int tag);

protected:
  vtkSubCommunicator();
  virtual ~vtkSubCommunicator();

  vtkProcessGroup *Group;

private:
  vtkSubCommunicator(const vtkSubCommunicator &);       // Not implemented
  void operator=(const vtkSubCommunicator &);           // Not implemented
};

#endif //__vtkSubCommunicator_h
