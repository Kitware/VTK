/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationIntegerKey - Key for integer values in vtkInformation.
// .SECTION Description
// vtkInformationIntegerKey is used to represent keys for integer values
// in vtkInformation.

#ifndef __vtkInformationIntegerKey_h
#define __vtkInformationIntegerKey_h

#include "vtkInformationKey.h"

class VTK_EXPORT vtkInformationIntegerKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationIntegerKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationIntegerKey();
  ~vtkInformationIntegerKey();

private:
  vtkInformationIntegerKey(const vtkInformationIntegerKey&);  // Not implemented.
  void operator=(const vtkInformationIntegerKey&);  // Not implemented.
};

#endif
