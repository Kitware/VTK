/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationKey - Superclass for vtkInformation keys.
// .SECTION Description
// vtkInformationKey is the superclass for all keys used to access the
// map represented by vtkInformation.  The vtkInformation::Set and
// vtkInformation::Get methods of vtkInformation are accessed by
// information keys.  A key is a pointer to an instance of a subclass
// of vtkInformationKey.  The type of the subclass determines the
// overload of Set/Get that is selected.  This ensures that the type
// of value stored in a vtkInformation instance corresponding to a
// given key matches the type expected for that key.

#ifndef __vtkInformationKey_h
#define __vtkInformationKey_h

#include "vtkObjectBase.h"

class VTK_EXPORT vtkInformationKey : public vtkObjectBase
{
public:
  vtkTypeRevisionMacro(vtkInformationKey,vtkObjectBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void Register(vtkObjectBase*);

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void UnRegister(vtkObjectBase*);

  vtkInformationKey();
  ~vtkInformationKey();

private:
  vtkInformationKey(const vtkInformationKey&);  // Not implemented.
  void operator=(const vtkInformationKey&);  // Not implemented.
};

#endif
