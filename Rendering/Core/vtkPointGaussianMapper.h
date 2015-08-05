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

class vtkPiecewiseFunction;

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

  // Description:
  // Set/Get the optional opacity transfer function. This is only
  // used when an OpacityArray is also specified.
  void SetScalarOpacityFunction(vtkPiecewiseFunction *);
  vtkGetObjectMacro(ScalarOpacityFunction,vtkPiecewiseFunction);

  // Description:
  // The size of the table used in computing opacities, used when
  // converting a vtkPiecewiseFunction to a table
  vtkSetMacro(OpacityTableSize, int);
  vtkGetMacro(OpacityTableSize, int);

  // Description:
  // Method to set the optional opacity array.  If specified this
  // array will be used to generate the opacity values.
  vtkSetStringMacro(OpacityArray);
  vtkGetStringMacro(OpacityArray);

  // Description:
  // Method to override the fragment shader code for the splat.  You can
  // set this to draw other shapes. For the OPenGL2 backend some of
  // the variables you can use and/or modify include,
  //   opacity - 0.0 to 1.0
  //   diffuseColor - vec3
  //   ambientColor - vec3
  //   offsetVCVSOutput - vec2 offset in view coordinates from the splat center
  vtkSetStringMacro(SplatShaderCode);
  vtkGetStringMacro(SplatShaderCode);

protected:
  vtkPointGaussianMapper();
  ~vtkPointGaussianMapper();

  char *ScaleArray;
  char *OpacityArray;
  char *SplatShaderCode;

  vtkPiecewiseFunction *ScalarOpacityFunction;

  double DefaultRadius;
  int Emissive;
  int OpacityTableSize;

private:
  vtkPointGaussianMapper(const vtkPointGaussianMapper&); // Not implemented.
  void operator=(const vtkPointGaussianMapper&); // Not implemented.
};

#endif
