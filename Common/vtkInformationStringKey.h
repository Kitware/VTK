/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationStringKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationStringKey - Key for string values in vtkInformation.
// .SECTION Description
// vtkInformationStringKey is used to represent keys for string values
// in vtkInformation.

#ifndef __vtkInformationStringKey_h
#define __vtkInformationStringKey_h

#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTK_COMMON_EXPORT vtkInformationStringKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationStringKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationStringKey(const char* name, const char* location);
  ~vtkInformationStringKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, const char*);
  const char* Get(vtkInformation* info);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Print the key's value in an information object to a stream.
  virtual void Print(ostream& os, vtkInformation* info);

private:
  vtkInformationStringKey(const vtkInformationStringKey&);  // Not implemented.
  void operator=(const vtkInformationStringKey&);  // Not implemented.
};

#endif
