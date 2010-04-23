/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDoubleKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationDoubleKey - Key for double values in vtkInformation.
// .SECTION Description
// vtkInformationDoubleKey is used to represent keys for double values
// in vtkInformation.

#ifndef __vtkInformationDoubleKey_h
#define __vtkInformationDoubleKey_h

#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTK_COMMON_EXPORT vtkInformationDoubleKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationDoubleKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationDoubleKey(const char* name, const char* location);
  ~vtkInformationDoubleKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, double);
  double Get(vtkInformation* info);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Print the key's value in an information object to a stream.
  virtual void Print(ostream& os, vtkInformation* info);

protected:
  // Description:
  // Get the address at which the actual value is stored.  This is
  // meant for use from a debugger to add watches and is therefore not
  // a public method.
  double* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationDoubleKey(const vtkInformationDoubleKey&);  // Not implemented.
  void operator=(const vtkInformationDoubleKey&);  // Not implemented.
};

#endif
