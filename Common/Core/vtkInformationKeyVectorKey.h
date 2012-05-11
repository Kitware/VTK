/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKeyVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationKeyVectorKey - Key for vector-of-keys values.
// .SECTION Description
// vtkInformationKeyVectorKey is used to represent keys for
// vector-of-keys values in vtkInformation.

#ifndef __vtkInformationKeyVectorKey_h
#define __vtkInformationKeyVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationKeyVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationKeyVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationKeyVectorKey(const char* name, const char* location);
  ~vtkInformationKeyVectorKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Append(vtkInformation* info, vtkInformationKey* value);
  void AppendUnique(vtkInformation* info, vtkInformationKey* value);
  void Set(vtkInformation* info, vtkInformationKey** value, int length);
  void RemoveItem(vtkInformation* info, vtkInformationKey* value);
  vtkInformationKey** Get(vtkInformation* info);
  vtkInformationKey*  Get(vtkInformation* info, int idx);
  void Get(vtkInformation* info, vtkInformationKey** value);
  int Length(vtkInformation* info);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Print the key's value in an information object to a stream.
  virtual void Print(ostream& os, vtkInformation* info);

private:
  vtkInformationKeyVectorKey(const vtkInformationKeyVectorKey&);  // Not implemented.
  void operator=(const vtkInformationKeyVectorKey&);  // Not implemented.
};

#endif
