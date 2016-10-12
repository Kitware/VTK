/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCone.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCone
 * @brief   implicit function for a cone
 *
 * vtkCone computes the implicit function and function gradient for a cone.
 * vtkCone is a concrete implementation of vtkImplicitFunction. The cone vertex
 * is located at the origin with axis of rotation coincident with x-axis. (Use
 * the superclass' vtkImplicitFunction transformation matrix if necessary to
 * reposition.) The angle specifies the angle between the axis of rotation
 * and the side of the cone.
 *
 * @warning
 * The cone is infinite in extent. To truncate the cone use the
 * vtkImplicitBoolean in combination with clipping planes.
*/

#ifndef vtkCone_h
#define vtkCone_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class VTKCOMMONDATAMODEL_EXPORT vtkCone : public vtkImplicitFunction
{
public:
  /**
   * Construct cone with angle of 45 degrees.
   */
  static vtkCone *New();

  vtkTypeMacro(vtkCone,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Evaluate cone equation.
   */
  double EvaluateFunction(double x[3]) VTK_OVERRIDE;
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;
  //@}

  /**
   * Evaluate cone normal.
   */
  void EvaluateGradient(double x[3], double g[3]) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the cone angle (expressed in degrees).
   */
  vtkSetClampMacro(Angle,double,0.0,89.0);
  vtkGetMacro(Angle,double);
  //@}

protected:
  vtkCone();
  ~vtkCone() VTK_OVERRIDE {}

  double Angle;

private:
  vtkCone(const vtkCone&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCone&) VTK_DELETE_FUNCTION;
};

#endif


