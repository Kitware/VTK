/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKeyVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationKeyVectorKey - Key for vector-of-keys values.
// .SECTION Description
// vtkInformationKeyVectorKey is used to represent keys for
// vector-of-keys values in vtkInformation.

#ifndef __vtkInformationKeyVectorKey_h
#define __vtkInformationKeyVectorKey_h

#include "vtkInformationKey.h"

class VTK_COMMON_EXPORT vtkInformationKeyVectorKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationKeyVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationKeyVectorKey();
  ~vtkInformationKeyVectorKey();

private:
  vtkInformationKeyVectorKey(const vtkInformationKeyVectorKey&);  // Not implemented.
  void operator=(const vtkInformationKeyVectorKey&);  // Not implemented.
};

#endif
