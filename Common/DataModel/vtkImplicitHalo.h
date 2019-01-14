/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitHalo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitHalo
 * @brief   implicit function for an halo
 *
 * vtkImplicitHalo evaluates to 1.0 for each position in the sphere of a
 * given center and radius Radius*(1-FadeOut). It evaluates to 0.0 for each
 * position out the sphere of a given Center and radius Radius. It fades out
 * linearly from 1.0 to 0.0 for points in a radius from Radius*(1-FadeOut) to
 * Radius.
 * vtkImplicitHalo is a concrete implementation of vtkImplicitFunction.
 * It is useful as an input to
 * vtkSampleFunction to generate an 2D image of an halo. It is used this way by
 * vtkShadowMapPass.
 * @warning
 * It does not implement the gradient.
*/

#ifndef vtkImplicitHalo_h
#define vtkImplicitHalo_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitHalo : public vtkImplicitFunction
{
public:
  /**
   * Center=(0.0,0.0,0.0), Radius=1.0, FadeOut=0.01
   */
  static vtkImplicitHalo *New();

  vtkTypeMacro(vtkImplicitHalo,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Evaluate the equation.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  //@}

  /**
   * Evaluate normal. Not implemented.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  //@{
  /**
   * Radius of the sphere.
   */
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Center of the sphere.
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);
  //@}

  //@{
  /**
   * FadeOut ratio. Valid values are between 0.0 and 1.0.
   */
  vtkSetMacro(FadeOut,double);
  vtkGetMacro(FadeOut,double);
  //@}

protected:
  vtkImplicitHalo();
  ~vtkImplicitHalo() override;

  double Radius;
  double Center[3];
  double FadeOut;

private:
  vtkImplicitHalo(const vtkImplicitHalo&) = delete;
  void operator=(const vtkImplicitHalo&) = delete;
};

#endif
