/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationExecutiveKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationExecutiveKey - Key for vtkExecutive values.
// .SECTION Description
// vtkInformationExecutiveKey is used to represent keys in
// vtkInformation for values that are vtkExecutive instances.

#ifndef __vtkInformationExecutiveKey_h
#define __vtkInformationExecutiveKey_h

#include "vtkInformationKey.h"

#include "vtkFilteringInformationKeyManager.h" // Manage instances of this type.

class vtkExecutive;

class VTK_FILTERING_EXPORT vtkInformationExecutiveKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationExecutiveKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationExecutiveKey(const char* name, const char* location);
  ~vtkInformationExecutiveKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, vtkExecutive*);
  vtkExecutive* Get(vtkInformation* info);
  int Has(vtkInformation* info);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void Copy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Report a reference this key has in the given information object.
  virtual void Report(vtkInformation* info, vtkGarbageCollector* collector);

private:
  vtkInformationExecutiveKey(const vtkInformationExecutiveKey&);  // Not implemented.
  void operator=(const vtkInformationExecutiveKey&);  // Not implemented.
};

#endif
