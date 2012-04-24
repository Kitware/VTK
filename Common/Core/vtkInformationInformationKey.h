/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInformationKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationInformationKey - Key for vtkInformation values.
// .SECTION Description
// vtkInformationInformationKey is used to represent keys in vtkInformation
// for other information objects.

#ifndef __vtkInformationInformationKey_h
#define __vtkInformationInformationKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationInformationKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationInformationKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationInformationKey(const char* name, const char* location);
  ~vtkInformationInformationKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, vtkInformation*);
  vtkInformation* Get(vtkInformation* info);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Duplicate (new instance created) the entry associated with this key from
  // one information object to another (new instances of any contained
  // vtkInformation and vtkInformationVector objects are created).
  virtual void DeepCopy(vtkInformation* from, vtkInformation* to);

private:
  vtkInformationInformationKey(const vtkInformationInformationKey&);  // Not implemented.
  void operator=(const vtkInformationInformationKey&);  // Not implemented.
};

#endif
