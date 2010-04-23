/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInformationVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationInformationVectorKey - Key for vtkInformation vectors.
// .SECTION Description
// vtkInformationInformationVectorKey is used to represent keys in
// vtkInformation for vectors of other vtkInformation objects.

#ifndef __vtkInformationInformationVectorKey_h
#define __vtkInformationInformationVectorKey_h

#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class vtkInformationVector;

class VTK_COMMON_EXPORT vtkInformationInformationVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationInformationVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationInformationVectorKey(const char* name, const char* location);
  ~vtkInformationInformationVectorKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, vtkInformationVector*);
  vtkInformationVector* Get(vtkInformation* info);

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

  // Description:
  // Report a reference this key has in the given information object.
  virtual void Report(vtkInformation* info, vtkGarbageCollector* collector);

private:
  vtkInformationInformationVectorKey(const vtkInformationInformationVectorKey&);  // Not implemented.
  void operator=(const vtkInformationInformationVectorKey&);  // Not implemented.
};

#endif
