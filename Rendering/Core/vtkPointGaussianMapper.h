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
// transparency and picking as well. It draws all the points and
// does not require or look at any cell arrays

#ifndef vtkPointGaussianMapper_h
#define vtkPointGaussianMapper_h

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
  // array to scale with has not been set or is set to NULL. If there
  // is no scale array and the default radius is set to zero then
  // the splats wil be rendered as simple points requiring less memory.
  vtkSetMacro(DefaultRadius,double);
  vtkGetMacro(DefaultRadius,double);

  // Description:
  // Treat the points/splats as emissive light sources. The default is true.
  vtkSetMacro(Emissive, int);
  vtkGetMacro(Emissive, int);
  vtkBooleanMacro(Emissive, int);

protected:
  vtkPointGaussianMapper();
  ~vtkPointGaussianMapper();

  char *ScaleArray;
  double DefaultRadius;
  int Emissive;

private:
  vtkPointGaussianMapper(const vtkPointGaussianMapper&); // Not implemented.
  void operator=(const vtkPointGaussianMapper&); // Not implemented.
};

#endif
