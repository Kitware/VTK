/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitFunction
 * @brief   abstract interface for implicit functions
 *
 * vtkImplicitFunction specifies an abstract interface for implicit
 * functions. Implicit functions are real valued functions defined in 3D
 * space, w = F(x,y,z). Two primitive operations are required: the ability to
 * evaluate the function, and the function gradient at a given point. The
 * implicit function divides space into three regions: on the surface
 * (F(x,y,z)=w), outside of the surface (F(x,y,z)>c), and inside the
 * surface (F(x,y,z)<c). (When c is zero, positive values are outside,
 * negative values are inside, and zero is on the surface. Note also
 * that the function gradient points from inside to outside.)
 *
 * Implicit functions are very powerful. It is possible to represent almost
 * any type of geometry with the level sets w = const, especially if you use
 * boolean combinations of implicit functions (see vtkImplicitBoolean).
 *
 * vtkImplicitFunction provides a mechanism to transform the implicit
 * function(s) via a vtkAbstractTransform.  This capability can be used to
 * translate, orient, scale, or warp implicit functions.  For example,
 * a sphere implicit function can be transformed into an oriented ellipse.
 *
 * @warning
 * The transformation transforms a point into the space of the implicit
 * function (i.e., the model space). Typically we want to transform the
 * implicit model into world coordinates. In this case the inverse of the
 * transformation is required.
 *
 * @sa
 * vtkAbstractTransform vtkSphere vtkCylinder vtkImplicitBoolean vtkPlane
 * vtkPlanes vtkQuadric vtkImplicitVolume vtkSampleFunction vtkCutter
 * vtkClipPolyData
*/

#ifndef vtkImplicitFunction_h
#define vtkImplicitFunction_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkDataArray;

class vtkAbstractTransform;

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkImplicitFunction,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overload standard modified time function. If Transform is modified,
   * then this object is modified as well.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Evaluate function at position x-y-z and return value. Point x[3] is
   * transformed through transform (if provided).
   */
  virtual void FunctionValue(vtkDataArray* input, vtkDataArray* output);
  double FunctionValue(const double x[3]);
  double FunctionValue(double x, double y, double z) {
    double xyz[3] = {x, y, z}; return this->FunctionValue(xyz); };
  //@}

  //@{
  /**
   * Evaluate function gradient at position x-y-z and pass back vector. Point
   * x[3] is transformed through transform (if provided).
   */
  void FunctionGradient(const double x[3], double g[3]);
  double *FunctionGradient(const double x[3]) VTK_SIZEHINT(3) {
    this->FunctionGradient(x,this->ReturnValue);
    return this->ReturnValue; };
  double *FunctionGradient(double x, double y, double z) VTK_SIZEHINT(3) {
    double xyz[3] = {x, y, z}; return this->FunctionGradient(xyz); };
  //@}

  //@{
  /**
   * Set/Get a transformation to apply to input points before
   * executing the implicit function.
   */
  virtual void SetTransform(vtkAbstractTransform*);
  virtual void SetTransform(const double elements[16]);
  vtkGetObjectMacro(Transform,vtkAbstractTransform);
  //@}

  //@{
  /**
   * Evaluate function at position x-y-z and return value.  You should
   * generally not call this method directly, you should use
   * FunctionValue() instead.  This method must be implemented by
   * any derived class.
   */
  virtual double EvaluateFunction(double x[3]) = 0;
  virtual void EvaluateFunction(vtkDataArray* input, vtkDataArray* output);
  virtual double EvaluateFunction(double x, double y, double z) {
    double xyz[3] = {x, y, z}; return this->EvaluateFunction(xyz); };
  //@}

  /**
   * Evaluate function gradient at position x-y-z and pass back vector.
   * You should generally not call this method directly, you should use
   * FunctionGradient() instead.  This method must be implemented by
   * any derived class.
   */
  virtual void EvaluateGradient(double x[3], double g[3]) = 0;

protected:
  vtkImplicitFunction();
  ~vtkImplicitFunction() override;

  vtkAbstractTransform *Transform;
  double ReturnValue[3];
private:
  vtkImplicitFunction(const vtkImplicitFunction&) = delete;
  void operator=(const vtkImplicitFunction&) = delete;
};

#endif
