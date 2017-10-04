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
/**
 * @class   vtkImageMapToRGBA
 * @brief   map the input image through a lookup table
 *
 * This filter has been replaced by vtkImageMapToColors, which provided
 * additional features.  Use vtkImageMapToColors instead.
 *
 * @sa
 * vtkLookupTable
*/

#ifndef vtkImageMapToRGBA_h
#define vtkImageMapToRGBA_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkImageMapToColors.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageMapToRGBA : public vtkImageMapToColors
{
public:
  static vtkImageMapToRGBA *New();
  vtkTypeMacro(vtkImageMapToRGBA,vtkImageMapToColors);

protected:
  vtkImageMapToRGBA() {}
  ~vtkImageMapToRGBA() override {}
private:
  vtkImageMapToRGBA(const vtkImageMapToRGBA&) = delete;
  void operator=(const vtkImageMapToRGBA&) = delete;
};

#endif







// VTK-HeaderTest-Exclude: vtkImageMapToRGBA.h
