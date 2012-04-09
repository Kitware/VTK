/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToRGBA.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMapToRGBA - map the input image through a lookup table
// .SECTION Description
// This filter has been replaced by vtkImageMapToColors, which provided
// additional features.  Use vtkImageMapToColors instead.

// .SECTION See Also
// vtkLookupTable

#ifndef __vtkImageMapToRGBA_h
#define __vtkImageMapToRGBA_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkImageMapToColors.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageMapToRGBA : public vtkImageMapToColors
{
public:
  static vtkImageMapToRGBA *New();
  vtkTypeMacro(vtkImageMapToRGBA,vtkImageMapToColors);

protected:
  vtkImageMapToRGBA() {};
  ~vtkImageMapToRGBA() {};
private:
  vtkImageMapToRGBA(const vtkImageMapToRGBA&);  // Not implemented.
  void operator=(const vtkImageMapToRGBA&);  // Not implemented.
};

#endif







// VTK-HeaderTest-Exclude: vtkImageMapToRGBA.h
