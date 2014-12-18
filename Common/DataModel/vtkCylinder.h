/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCylinder - implicit function for a cylinder
// .SECTION Description
// vtkCylinder computes the implicit function and function gradient for a
// cylinder. vtkCylinder is a concrete implementation of vtkImplicitFunction.
// Cylinder is centered at Center and axes of rotation is along the
// y-axis. (Use the superclass' vtkImplicitFunction transformation matrix if
// necessary to reposition.)

// .SECTION Caveats
// The cylinder is infinite in extent. To truncate the cylinder use the
// vtkImplicitBoolean in combination with clipping planes.


#ifndef vtkCylinder_h
#define vtkCylinder_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class VTKCOMMONDATAMODEL_EXPORT vtkCylinder : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkCylinder,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct cylinder radius of 0.5.
  static vtkCylinder *New();

  // Description
  // Evaluate cylinder equation F(x,y,z) = (x-x0)^2 + (z-z0)^2 - R^2.
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate cylinder function gradient.
  void EvaluateGradient(double x[3], double g[3]);

  // Description:
  // Set/Get cylinder radius.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description:
  // Set/Get cylinder center
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);
protected:
  vtkCylinder();
  ~vtkCylinder() {}

  double Radius;
  double Center[3];

private:
  vtkCylinder(const vtkCylinder&);  // Not implemented.
  void operator=(const vtkCylinder&);  // Not implemented.
};

#endif


