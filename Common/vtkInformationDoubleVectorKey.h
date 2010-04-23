/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDoubleVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationDoubleVectorKey - Key for double vector values.
// .SECTION Description
// vtkInformationDoubleVectorKey is used to represent keys for double
// vector values in vtkInformation.h

#ifndef __vtkInformationDoubleVectorKey_h
#define __vtkInformationDoubleVectorKey_h

#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTK_COMMON_EXPORT vtkInformationDoubleVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationDoubleVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationDoubleVectorKey(const char* name, const char* location,
                                 int length=-1);
  ~vtkInformationDoubleVectorKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Append(vtkInformation* info, double value);
  void Set(vtkInformation* info, double* value, int length);
  double* Get(vtkInformation* info);
  double  Get(vtkInformation* info, int idx);
  void Get(vtkInformation* info, double* value);
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

private:
  vtkInformationDoubleVectorKey(const vtkInformationDoubleVectorKey&);  // Not implemented.
  void operator=(const vtkInformationDoubleVectorKey&);  // Not implemented.
};

#endif
