/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTupleInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTupleInterpolator - interpolate a tuple of arbitray size
// .SECTION Description
// This class is used to interpolate a tuple which may have an arbitrary
// number of components (but at least one component). The interpolation may
// be linear in form, or via a subclasses of vtkSpline.
//
// To use this class, begin by specifying the number of components of the
// tuple and the interpolation function to use. Then specify at least one
// pair of (t,tuple) with the AddTuple() method.  Next interpolate the
// tuples with the InterpolateTuple(t,tuple) method, where "t" must be in the
// range of (t_min,t_max) parameter values specified by the AddTuple() method
// (if not then t is clamped), and tuple[] is filled in by the method (make
// sure that tuple [] is long enough to hold the interpolated data).
//
// You can control the type of interpolation to use. By default, the
// interpolation is based on a Kochanek spline. However, other types of
// splines can be specified. You can also set the interpolation method
// to linear, in which case the specified spline has no effect on the
// interpolation.

// .SECTION Caveats
// Setting the number of components or changing the type of interpolation
// causes the list of tuples to be reset, so any data inserted up to that
// point is lost. Bisection methods are used to speed up the search for the
// interpolation interval.

#ifndef vtkTupleInterpolator_h
#define vtkTupleInterpolator_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkSpline;
class vtkPiecewiseFunction;


class VTKRENDERINGCORE_EXPORT vtkTupleInterpolator : public vtkObject
{
public:
  vtkTypeMacro(vtkTupleInterpolator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the class.
  static vtkTupleInterpolator* New();

  // Description:
  // Specify the number of tuple components to interpolate. Note that setting
  // this value discards any previously inserted data.
  void SetNumberOfComponents(int numComp);
  vtkGetMacro(NumberOfComponents,int);

  // Description:
  // Return the number of tuples in the list of tuples to be
  // interpolated.
  int GetNumberOfTuples();

  // Description:
  // Obtain some information about the interpolation range. The numbers
  // returned (corresponding to parameter t, usually thought of as time)
  // are undefined if the list of transforms is empty. This is a convenience
  // method for interpolation.
  double GetMinimumT();
  double GetMaximumT();

  // Description:
  // Reset the class so that it contains no (t,tuple) information.
  void Initialize();

  // Description:
  // Add another tuple to the list of tuples to be interpolated.  Note that
  // using the same time t value more than once replaces the previous tuple
  // value at t.  At least two tuples must be added to define an
  // interpolation function.
  void AddTuple(double t, double tuple[]);

  // Description:
  // Delete the tuple at a particular parameter t. If there is no
  // tuple defined at t, then the method does nothing.
  void RemoveTuple(double t);

  // Description:
  // Interpolate the list of tuples and determine a new tuple (i.e.,
  // fill in the tuple provided). If t is outside the range of
  // (min,max) values, then t is clamped. Note that each component
  // of tuple[] is interpolated independently.
  void InterpolateTuple(double t, double tuple[]);

//BTX
  // Description:
  // Enums to control the type of interpolation to use.
  enum {INTERPOLATION_TYPE_LINEAR=0,
        INTERPOLATION_TYPE_SPLINE
  };
//ETX

  // Description:
  // Specify which type of function to use for interpolation. By default
  // spline interpolation (SetInterpolationFunctionToSpline()) is used
  // (i.e., a Kochanek spline) and the InterpolatingSpline instance variable
  // is used to birth the actual interpolation splines via a combination of
  // NewInstance() and DeepCopy(). You may also choose to use linear
  // interpolation by invoking SetInterpolationFunctionToLinear(). Note that
  // changing the type of interpolation causes previously inserted data
  // to be discarded.
  void SetInterpolationType(int type);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToLinear()
    {this->SetInterpolationType(INTERPOLATION_TYPE_LINEAR);}
  void SetInterpolationTypeToSpline()
    {this->SetInterpolationType(INTERPOLATION_TYPE_SPLINE);}

  // Description:
  // If the InterpolationType is set to spline, then this method applies. By
  // default Kochanek interpolation is used, but you can specify any instance
  // of vtkSpline to use. Note that the actual interpolating splines are
  // created by invoking NewInstance() followed by DeepCopy() on the
  // interpolating spline specified here, for each tuple component to
  // interpolate.
  void SetInterpolatingSpline(vtkSpline*);
  vtkGetObjectMacro(InterpolatingSpline,vtkSpline);

protected:
  vtkTupleInterpolator();
  virtual ~vtkTupleInterpolator();

  // The number of components being interpolated
  int NumberOfComponents;

  // Specify the type of interpolation to use
  int InterpolationType;

  // This is the default 1D spline to use
  vtkSpline *InterpolatingSpline;

  // Internal variables for interpolation functions
  void InitializeInterpolation();
  vtkPiecewiseFunction    **Linear;
  vtkSpline               **Spline;


private:
  vtkTupleInterpolator(const vtkTupleInterpolator&);  // Not implemented.
  void operator=(const vtkTupleInterpolator&);  // Not implemented.

};

#endif
