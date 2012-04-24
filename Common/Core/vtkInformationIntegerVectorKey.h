/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationIntegerVectorKey - Key for integer vector values.
// .SECTION Description
// vtkInformationIntegerVectorKey is used to represent keys for integer
// vector values in vtkInformation.h

#ifndef __vtkInformationIntegerVectorKey_h
#define __vtkInformationIntegerVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationIntegerVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationIntegerVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationIntegerVectorKey(const char* name, const char* location,
                                 int length=-1);
  ~vtkInformationIntegerVectorKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Append(vtkInformation* info, int value);
  void Set(vtkInformation* info, int* value, int length);
  void Set(vtkInformation* info);
  int* Get(vtkInformation* info);
  int  Get(vtkInformation* info, int idx);
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
  vtkInformationIntegerVectorKey(const vtkInformationIntegerVectorKey&);  // Not implemented.
  void operator=(const vtkInformationIntegerVectorKey&);  // Not implemented.
};

#endif
