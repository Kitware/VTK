/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDataObjectVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationDataObjectVectorKey - Key for vector-of-data values.
// .SECTION Description
// vtkInformationDataObjectVectorKey is used to represent keys for
// values in vtkInformation that are vectors of vtkDataObject
// instances.

#ifndef __vtkInformationDataObjectVectorKey_h
#define __vtkInformationDataObjectVectorKey_h

#include "vtkInformationKey.h"

class vtkDataObject;

class VTK_COMMON_EXPORT vtkInformationDataObjectVectorKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationDataObjectVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationDataObjectVectorKey(const char* name, const char* location);
  ~vtkInformationDataObjectVectorKey();

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Set(vtkInformation* info, vtkDataObject** value, int length);
  vtkDataObject** Get(vtkInformation* info);
  void Get(vtkInformation* info, vtkDataObject** value);
  int Length(vtkInformation* info);
  int Has(vtkInformation* info);

private:
  vtkInformationDataObjectVectorKey(const vtkInformationDataObjectVectorKey&);  // Not implemented.
  void operator=(const vtkInformationDataObjectVectorKey&);  // Not implemented.
};

#endif
