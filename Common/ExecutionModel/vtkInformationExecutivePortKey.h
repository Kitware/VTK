/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationExecutivePortKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationExecutivePortKey - Key for vtkExecutive/Port value pairs.
// .SECTION Description
// vtkInformationExecutivePortKey is used to represent keys in
// vtkInformation for values that are vtkExecutive instances paired
// with port numbers.

#ifndef __vtkInformationExecutivePortKey_h
#define __vtkInformationExecutivePortKey_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkFilteringInformationKeyManager.h" // Manage instances of this type.

class vtkExecutive;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkInformationExecutivePortKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationExecutivePortKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationExecutivePortKey(const char* name, const char* location);
  ~vtkInformationExecutivePortKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, vtkExecutive*, int);
  vtkExecutive* GetExecutive(vtkInformation* info);
  int GetPort(vtkInformation* info);
  void Get(vtkInformation *info, vtkExecutive*& executive, int &port);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Report a reference this key has in the given information object.
  virtual void Report(vtkInformation* info, vtkGarbageCollector* collector);

  // Description:
  // Print the key's value in an information object to a stream.
  virtual void Print(ostream& os, vtkInformation* info);

private:
  vtkInformationExecutivePortKey(const vtkInformationExecutivePortKey&);  // Not implemented.
  void operator=(const vtkInformationExecutivePortKey&);  // Not implemented.
};

#endif
