/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDataSetKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationDataSetKey - Key for vtkDataSet values.
// .SECTION Description
// vtkInformationDataSetKey is used to represent keys in
// vtkInformation for values that are vtkDataSet instances.

#ifndef __vtkInformationDataSetKey_h
#define __vtkInformationDataSetKey_h

#include "vtkInformationKey.h"

class VTK_EXPORT vtkInformationDataSetKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationDataSetKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationDataSetKey();
  ~vtkInformationDataSetKey();

private:
  vtkInformationDataSetKey(const vtkInformationDataSetKey&);  // Not implemented.
  void operator=(const vtkInformationDataSetKey&);  // Not implemented.
};

#endif
