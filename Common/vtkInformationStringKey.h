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

class VTK_EXPORT vtkInformationStringKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationStringKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationStringKey();
  ~vtkInformationStringKey();

private:
  vtkInformationStringKey(const vtkInformationStringKey&);  // Not implemented.
  void operator=(const vtkInformationStringKey&);  // Not implemented.
};

#endif
