/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointGaussianMapper - draw PointGaussians using imposters
// .SECTION Description
// An  mapper that uses imposters to draw PointGaussians. Supports
// transparency and picking as well.

#ifndef __vtkPointGaussianMapper_h
#define __vtkPointGaussianMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPolyDataMapper.h"

class VTKRENDERINGCORE_EXPORT vtkPointGaussianMapper : public vtkPolyDataMapper
{
public:
  static vtkPointGaussianMapper* New();
  vtkTypeMacro(vtkPointGaussianMapper, vtkPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to set the array to scale with.
  vtkSetStringMacro(ScaleArray);
  vtkGetStringMacro(ScaleArray);

  // Description:
  // Set the default radius of the point gaussians.  This is used if the
  // array to scale with has not been set or is set to NULL.
  vtkSetMacro(DefaultRadius,double);
  vtkGetMacro(DefaultRadius,double);

protected:
  vtkPointGaussianMapper();
  ~vtkPointGaussianMapper();

  char *ScaleArray;
  double DefaultRadius;

private:
  vtkPointGaussianMapper(const vtkPointGaussianMapper&); // Not implemented.
  void operator=(const vtkPointGaussianMapper&); // Not implemented.
};

#endif
