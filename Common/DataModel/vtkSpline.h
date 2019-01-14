/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpline
 * @brief   spline abstract class for interpolating splines
 *
 * vtkSpline interpolates a set of data points (i.e., interpolation means
 * that the spline passes through the points).  vtkSpline is an abstract
 * class: its subclasses vtkCardinalSpline and vtkKochanekSpline do the
 * interpolation. Note that this spline maps the 1D parametric coordinate
 * t into a single value x. Thus if you want to use the spline to
 * interpolate points (i.e. x[3]), you have to create three splines for
 * each of the x-y-z coordinates. Fortunately, the vtkParametricSpline
 * class does this for you.
 *
 * Typically a spline is used by adding a sequence of parametric coordinate /
 * data (t,x) values followed by use of an evaluation function (e.g.,
 * vtkCardinalSpline::Evaluate()).  Since these splines are 1D, a point in
 * this context is an independent / dependent variable pair.
 *
 * Splines can also be set up to be closed or open. Closed splines continue
 * from the last point to the first point with continuous function and
 * derivative values. (You don't need to duplicate the first point to close
 * the spline, just set ClosedOn.)
 *
 * This implementation of splines does not use a normalized parametric
 * coordinate. If the spline is open, then the parameter space is (tMin <= t
 * <= tMax) where tMin and tMax are the minimum and maximum parametric values
 * seen when performing AddPoint(). If the spline is closed, then the
 * parameter space is (tMin <= t <= (tMax+1)) where tMin and tMax are the
 * minimum and maximum parametric values seen when performing AddPoint().
 * Note, however, that this behavior can be changed by explicitly setting
 * the ParametricRange(tMin,tMax). If set, the parameter space remains
 * (tMin <= t <= tMax), except that additions of data with parametric
 * values outside this range are clamped within this range.
 *
 * @sa
 * vtkCardinalSpline vtkKochanekSpline vtkParametricSpline
 * vtkParametricFunctionSource
*/

#ifndef vtkSpline_h
#define vtkSpline_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkPiecewiseFunction;

class VTKCOMMONDATAMODEL_EXPORT vtkSpline : public vtkObject
{
public:
  vtkTypeMacro(vtkSpline,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the parametric range. If not set, the range is determined
   * implicitly by keeping track of the (min,max) parameter values for
   * t. If set, the AddPoint() method will clamp the t value to lie
   * within the specified range.
   */
  void SetParametricRange(double tMin, double tMax);
  void SetParametricRange(double tRange[2])
    {this->SetParametricRange(tRange[0],tRange[1]);}
  void GetParametricRange(double tRange[2]) const;
  //@}

  //@{
  /**
   * Set/Get ClampValue. If On, results of the interpolation will be
   * clamped to the min/max of the input data.
   */
  vtkSetMacro(ClampValue,vtkTypeBool);
  vtkGetMacro(ClampValue,vtkTypeBool);
  vtkBooleanMacro(ClampValue,vtkTypeBool);
  //@}

  /**
   * Compute the coefficients for the spline.
   */
  virtual void Compute () = 0;

  /**
   * Interpolate the value of the spline at parametric location of t.
   */
  virtual double Evaluate (double t) = 0;

  /**
   * Return the number of points inserted thus far.
   */
  int GetNumberOfPoints();

  /**
   * Add a pair of points to be fit with the spline.
   */
  void AddPoint (double t, double x);

  /**
   * Remove a point from the data to be fit with the spline.
   */
  void RemovePoint (double t);

  /**
   * Remove all points from the data.
   */
  void RemoveAllPoints ();

  //@{
  /**
   * Control whether the spline is open or closed. A closed spline forms
   * a continuous loop: the first and last points are the same, and
   * derivatives are continuous.
   */
  vtkSetMacro(Closed,vtkTypeBool);
  vtkGetMacro(Closed,vtkTypeBool);
  vtkBooleanMacro(Closed,vtkTypeBool);
  //@}

  //@{
  /**
   * Set the type of constraint of the left(right) end points. Four
   * constraints are available:

   * 0: the first derivative at left(right) most point is determined
   * from the line defined from the first(last) two points.

   * 1: the first derivative at left(right) most point is set to
   * Left(Right)Value.

   * 2: the second derivative at left(right) most point is set to
   * Left(Right)Value.

   * 3: the second derivative at left(right)most points is Left(Right)Value
   * times second derivative at first interior point.
   */
  vtkSetClampMacro(LeftConstraint,int,0,3);
  vtkGetMacro(LeftConstraint,int);
  vtkSetClampMacro(RightConstraint,int,0,3);
  vtkGetMacro(RightConstraint,int);
  //@}

  //@{
  /**
   * The values of the derivative on the left and right sides. The value
   * is used only if the left(right) constraint is type 1-3.
   */
  vtkSetMacro(LeftValue,double);
  vtkGetMacro(LeftValue,double);
  vtkSetMacro(RightValue,double);
  vtkGetMacro(RightValue,double);
  //@}

  /**
   * Return the MTime also considering the Piecewise function.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Deep copy of spline data.
   */
  virtual void DeepCopy(vtkSpline *s);

protected:
  vtkSpline();
  ~vtkSpline() override;

  vtkMTimeType ComputeTime;
  vtkTypeBool ClampValue;
  double *Intervals;
  double *Coefficients;
  int LeftConstraint;
  double LeftValue;
  int RightConstraint;
  double RightValue;
  vtkPiecewiseFunction *PiecewiseFunction;
  vtkTypeBool Closed;

  // Explicitly specify the parametric range.
  double ParametricRange[2];

  // Helper methods
  double ComputeLeftDerivative();
  double ComputeRightDerivative();
  int FindIndex(int size, double t);

private:
  vtkSpline(const vtkSpline&) = delete;
  void operator=(const vtkSpline&) = delete;
};

#endif

