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

class VTK_COMMON_EXPORT vtkInformationInformationVectorKey : public vtkInformationKey
{
public:
  vtkTypeRevisionMacro(vtkInformationInformationVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationInformationVectorKey();
  ~vtkInformationInformationVectorKey();

private:
  vtkInformationInformationVectorKey(const vtkInformationInformationVectorKey&);  // Not implemented.
  void operator=(const vtkInformationInformationVectorKey&);  // Not implemented.
};

#endif
