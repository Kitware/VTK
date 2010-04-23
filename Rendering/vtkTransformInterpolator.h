/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransformInterpolator - interpolate a series of transformation matrices
// .SECTION Description
// This class is used to interpolate a series of 4x4 transformation
// matrices. Position, scale and orientation (i.e., rotations) are
// interpolated separately, and can be interpolated linearly or with a spline
// function. Note that orientation is interpolated using quaternions via
// SLERP (spherical linear interpolation) or the special vtkQuaternionSpline
// class.
//
// To use this class, specify at least two pairs of (t,transformation matrix)
// with the AddTransform() method.  Then interpolated the transforms with the
// InterpolateTransform(t,transform) method, where "t" must be in the range
// of (min,max) times specified by the AddTransform() method.
//
// By default, spline interpolation is used for the interpolation of the
// transformation matrices. The position, scale and orientation of the
// matrices are interpolated with instances of the classes
// vtkTupleInterpolator (position,scale) and vtkQuaternionInterpolator
// (rotation). The user can override the interpolation behavior by gaining
// access to these separate interpolation classes.  These interpolator
// classes (vtkTupleInterpolator and vtkQuaternionInterpolator) can be
// modified to perform linear versus spline interpolation, and/or different
// spline basis functions can be specified.
//
// .SECTION Caveats
// The interpolator classes are initialized when the InterpolateTransform()
// is called. Any changes to the interpolators, or additions to the list of
// transforms to be interpolated, causes a reinitialization of the 
// interpolators the next time InterpolateTransform() is invoked. Thus the
// best performance is obtained by 1) configuring the interpolators, 2) adding 
// all the transforms, and 3) finally performing interpolation.

#ifndef __vtkTransformInterpolator_h
#define __vtkTransformInterpolator_h

#include "vtkObject.h"


class vtkTransform;
class vtkMatrix4x4;
class vtkProp3D;
class vtkTupleInterpolator;
class vtkQuaternionInterpolator;
class vtkTransformList;


class VTK_RENDERING_EXPORT vtkTransformInterpolator : public vtkObject
{
public:
  vtkTypeMacro(vtkTransformInterpolator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the class.
  static vtkTransformInterpolator* New();

  // Description:
  // Return the number of transforms in the list of transforms.
  int GetNumberOfTransforms();

  // Description:
  // Obtain some information about the interpolation range. The numbers
  // returned (corresponding to parameter t, usually thought of as time)
  // are undefined if the list of transforms is empty.
  double GetMinimumT();
  double GetMaximumT();

  // Description:
  // Clear the list of transforms.
  void Initialize();
  
  // Description:
  // Add another transform to the list of transformations defining
  // the transform function. Note that using the same time t value
  // more than once replaces the previous transform value at t.
  // At least two transforms must be added to define a function.
  // There are variants to this method depending on whether you are
  // adding a vtkTransform, vtkMaxtirx4x4, and/or vtkProp3D.
  void AddTransform(double t, vtkTransform *xform);
  void AddTransform(double t, vtkMatrix4x4 *matrix);
  void AddTransform(double t, vtkProp3D *prop3D);

  // Description:
  // Delete the transform at a particular parameter t. If there is no
  // transform defined at location t, then the method does nothing.
  void RemoveTransform(double t);

  // Description:
  // Interpolate the list of transforms and determine a new transform (i.e.,
  // fill in the transformation provided). If t is outside the range of
  // (min,max) values, then t is clamped.
  void InterpolateTransform(double t, vtkTransform *xform);

//BTX
  // Description:
  // Enums to control the type of interpolation to use.
  enum {INTERPOLATION_TYPE_LINEAR=0,
        INTERPOLATION_TYPE_SPLINE,
        INTERPOLATION_TYPE_MANUAL
  };
//ETX

  // Description:
  // These are convenience methods to switch between linear and spline
  // interpolation. The methods simply forward the request for linear or
  // spline interpolation to the position, scale and orientation
  // interpolators. Note that if the InterpolationType is set to "Manual",
  // then the interpolators are expected to be directly manipulated and
  // this class does not forward the request for interpolation type to its
  // interpolators.
  vtkSetClampMacro(InterpolationType,int, INTERPOLATION_TYPE_LINEAR,
                   INTERPOLATION_TYPE_MANUAL);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToLinear()
    {this->SetInterpolationType(INTERPOLATION_TYPE_LINEAR);}
  void SetInterpolationTypeToSpline()
    {this->SetInterpolationType(INTERPOLATION_TYPE_SPLINE);}
  void SetInterpolationTypeToManual()
    {this->SetInterpolationType(INTERPOLATION_TYPE_MANUAL);}

  // Description:
  // Set/Get the tuple interpolator used to interpolate the position portion
  // of the transformation matrix. Note that you can modify the behavior of
  // the interpolator (linear vs spline interpolation; change spline basis)
  // by manipulating the interpolator instances.
  virtual void SetPositionInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(PositionInterpolator,vtkTupleInterpolator);

  // Description:
  // Set/Get the tuple interpolator used to interpolate the scale portion
  // of the transformation matrix. Note that you can modify the behavior of
  // the interpolator (linear vs spline interpolation; change spline basis)
  // by manipulating the interpolator instances.
  virtual void SetScaleInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(ScaleInterpolator,vtkTupleInterpolator);

  // Description:
  // Set/Get the tuple interpolator used to interpolate the orientation portion
  // of the transformation matrix. Note that you can modify the behavior of
  // the interpolator (linear vs spline interpolation; change spline basis)
  // by manipulating the interpolator instances.
  virtual void SetRotationInterpolator(vtkQuaternionInterpolator*);
  vtkGetObjectMacro(RotationInterpolator,vtkQuaternionInterpolator);

  // Description:
  // Override GetMTime() because we depend on the interpolators which may be
  // modified outside of this class.
  unsigned long GetMTime();

protected:
  vtkTransformInterpolator();
  virtual ~vtkTransformInterpolator();

  // Control the interpolation type
  int InterpolationType;

  // Interpolators
  vtkTupleInterpolator      *PositionInterpolator;
  vtkTupleInterpolator      *ScaleInterpolator;
  vtkQuaternionInterpolator *RotationInterpolator;
  
  // Initialize the interpolating splines
  int Initialized;
  vtkTimeStamp InitializeTime;
  void InitializeInterpolation();
  
  // Keep track of inserted data
  vtkTransformList *TransformList;

private:
  vtkTransformInterpolator(const vtkTransformInterpolator&);  // Not implemented.
  void operator=(const vtkTransformInterpolator&);  // Not implemented.

};

#endif
