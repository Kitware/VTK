/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerPointerKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationIntegerPointerKey - Key for pointer to integer.
// .SECTION Description
// vtkInformationIntegerPointerKey is used to represent keys for pointer
// to integer values in vtkInformation.h

#ifndef __vtkInformationIntegerPointerKey_h
#define __vtkInformationIntegerPointerKey_h

#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTK_COMMON_EXPORT vtkInformationIntegerPointerKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationIntegerPointerKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationIntegerPointerKey(const char* name, const char* location,
                                 int length=-1);
  ~vtkInformationIntegerPointerKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, int* value, int length);
  int* Get(vtkInformation* info);
  void Get(vtkInformation* info, int* value);
  int Length(vtkInformation* info);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Print the key's value in an information object to a stream.
  virtual void Print(ostream& os, vtkInformation* info);

protected:
  // The required length of the vector value (-1 is no restriction).
  int RequiredLength;

  // Description:
  // Get the address at which the actual value is stored.  This is
  // meant for use from a debugger to add watches and is therefore not
  // a public method.
  int* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationIntegerPointerKey(const vtkInformationIntegerPointerKey&);  // Not implemented.
  void operator=(const vtkInformationIntegerPointerKey&);  // Not implemented.
};

#endif
