/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationExecutivePortVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationExecutivePortVectorKey - Key for vtkExecutive/Port value pair vectors.
// .SECTION Description
// vtkInformationExecutivePortVectorKey is used to represent keys in
// vtkInformation for values that are vectors of vtkExecutive
// instances paired with port numbers.

#ifndef __vtkInformationExecutivePortVectorKey_h
#define __vtkInformationExecutivePortVectorKey_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkFilteringInformationKeyManager.h" // Manage instances of this type.

class vtkExecutive;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkInformationExecutivePortVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationExecutivePortVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationExecutivePortVectorKey(const char* name, const char* location);
  ~vtkInformationExecutivePortVectorKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Append(vtkInformation* info, vtkExecutive* executive, int port);
  void Remove(vtkInformation* info, vtkExecutive* executive, int port);
  void Set(vtkInformation* info, vtkExecutive** executives, int* ports, int length);
  vtkExecutive** GetExecutives(vtkInformation* info);
  int* GetPorts(vtkInformation* info);
  void Get(vtkInformation* info, vtkExecutive** executives, int* ports);
  int Length(vtkInformation* info);

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Remove this key from the given information object.
  virtual void Remove(vtkInformation* info);

  // Description:
  // Report a reference this key has in the given information object.
  virtual void Report(vtkInformation* info, vtkGarbageCollector* collector);

  // Description:
  // Print the key's value in an information object to a stream.
  virtual void Print(ostream& os, vtkInformation* info);

protected:

  // Description:
  // Get the address at which the actual value is stored.  This is
  // meant for use from a debugger to add watches and is therefore not
  // a public method.
  vtkExecutive** GetExecutivesWatchAddress(vtkInformation* info);
  int* GetPortsWatchAddress(vtkInformation* info);

private:
  vtkInformationExecutivePortVectorKey(const vtkInformationExecutivePortVectorKey&);  // Not implemented.
  void operator=(const vtkInformationExecutivePortVectorKey&);  // Not implemented.
};

#endif
