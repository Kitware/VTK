/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationIntegerVectorKey - Key for integer vector values.
// .SECTION Description
// vtkInformationIntegerVectorKey is used to represent keys for integer
// vector values in vtkInformation.h

#ifndef __vtkInformationIntegerVectorKey_h
#define __vtkInformationIntegerVectorKey_h

#include "vtkInformationKey.h"

class VTK_COMMON_EXPORT vtkInformationIntegerVectorKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationIntegerVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationIntegerVectorKey();
  ~vtkInformationIntegerVectorKey();

private:
  vtkInformationIntegerVectorKey(const vtkInformationIntegerVectorKey&);  // Not implemented.
  void operator=(const vtkInformationIntegerVectorKey&);  // Not implemented.
};

#endif
