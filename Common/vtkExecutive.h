/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExecutive - Superclass for all pipeline executives in VTK.
// .SECTION Description
// vtkExecutive is the superclass for all pipeline executives in VTK.
// A VTK executive is responsible for controlling one or more
// instances of vtkAlgorithm.  A pipeline consists of one or more
// executives that control data flow.  Every reader, source, writer,
// or data processing algorithm in the pipeline is implemented in an
// instance of vtkAlgorithm.

#ifndef __vtkExecutive_h
#define __vtkExecutive_h

#include "vtkObject.h"

class vtkAlgorithm;
class vtkAlgorithmToExecutiveFriendship;

class VTK_COMMON_EXPORT vtkExecutive : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkExecutive,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring the given algorithm's outputs up-to-date.  The algorithm
  // must already be managed by this executive.  Returns 1 for success
  // and 0 for failure.
  virtual int Update(vtkAlgorithm* algorithm)=0;

  // Description:
  // Decrement the count of references to this object and participate
  // in garbage collection.
  virtual void UnRegister(vtkObjectBase* o);
protected:
  vtkExecutive();
  ~vtkExecutive();

  // Description:
  // Add/Remove a algorithm from the control of this executive.  Some
  // executives support more than one algorithm while others do not.
  // These methods are called by vtkAlgorithm::SetExecutive and should
  // not be called from elsewhere.
  virtual void AddAlgorithm(vtkAlgorithm* algorithm)=0;
  virtual void RemoveAlgorithm(vtkAlgorithm* algorithm)=0;

  // Garbage collection support.
  virtual void GarbageCollectionStarting();
  int GarbageCollecting;

  //BTX
  friend class vtkAlgorithmToExecutiveFriendship;
  //ETX
private:
  vtkExecutive(const vtkExecutive&);  // Not implemented.
  void operator=(const vtkExecutive&);  // Not implemented.
};

#endif
