/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCameraInterpolator - interpolate a series of cameras to update a new camera
// .SECTION Description
// This class is used to interpolate a series of cameras to update a
// specified camera. Either linear interpolation or spline interpolation may
// be used. The instance variables currently interpolated include position,
// focal point, view up, view angle, parallel scale, and clipping range.
//
// To use this class, specify the type of interpolation to use, and add a
// series of cameras at various times "t" to the list of cameras from which to
// interpolate. Then to interpolate in between cameras, simply invoke the
// function InterpolateCamera(t,camera) where "camera" is the camera to be
// updated with interpolated values. Note that "t" should be in the range
// (min,max) times specified with the AddCamera() method. If outside this
// range, the interpolation is clamped. This class copies the camera information
// (as compared to referencing the cameras) so you do not need to keep separate
// instances of the camera around for each camera added to the list of cameras
// to interpolate.
//
// .SECTION Caveats
// The interpolator classes are initialized the first time InterpolateCamera()
// is called. Any later changes to the interpolators, or additions to the list of
// cameras to be interpolated, causes a reinitialization of the
// interpolators the next time InterpolateCamera() is invoked. Thus the
// best performance is obtained by 1) configuring the interpolators, 2) adding
// all the cameras, and 3) finally performing interpolation.
//
// Currently position, focal point and view up are interpolated to define
// the orientation of the camera. Quaternion interpolation may be added in the
// future as an alternative interpolation method for camera orientation.


#ifndef __vtkCameraInterpolator_h
#define __vtkCameraInterpolator_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkCamera;
class vtkCameraList;
class vtkTupleInterpolator;
class vtkCameraList;


class VTKRENDERINGCORE_EXPORT vtkCameraInterpolator : public vtkObject
{
public:
  vtkTypeMacro(vtkCameraInterpolator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the class.
  static vtkCameraInterpolator* New();

  // Description:
  // Return the number of cameras in the list of cameras.
  int GetNumberOfCameras();

  // Description:
  // Obtain some information about the interpolation range. The numbers
  // returned are undefined if the list of cameras is empty.
  double GetMinimumT();
  double GetMaximumT();

  // Description:
  // Clear the list of cameras.
  void Initialize();

  // Description:
  // Add another camera to the list of cameras defining
  // the camera function. Note that using the same time t value
  // more than once replaces the previous camera value at t.
  // At least one camera must be added to define a function.
  void AddCamera(double t, vtkCamera *camera);

  // Description:
  // Delete the camera at a particular parameter t. If there is no
  // camera defined at location t, then the method does nothing.
  void RemoveCamera(double t);

  // Description:
  // Interpolate the list of cameras and determine a new camera (i.e.,
  // fill in the camera provided). If t is outside the range of
  // (min,max) values, then t is clamped to lie within this range.
  void InterpolateCamera(double t, vtkCamera *camera);

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
  // spline interpolation to the instance variable interpolators (i.e.,
  // position, focal point, clipping range, orientation, etc.)
  // interpolators. Note that if the InterpolationType is set to "Manual",
  // then the interpolators are expected to be directly manipulated and this
  // class does not forward the request for interpolation type to its
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
  // of the camera. Note that you can modify the behavior of the interpolator
  // (linear vs spline interpolation; change spline basis) by manipulating
  // the interpolator instances directly.
  virtual void SetPositionInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(PositionInterpolator,vtkTupleInterpolator);

  // Description:
  // Set/Get the tuple interpolator used to interpolate the focal point portion
  // of the camera. Note that you can modify the behavior of the interpolator
  // (linear vs spline interpolation; change spline basis) by manipulating
  // the interpolator instances directly.
  virtual void SetFocalPointInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(FocalPointInterpolator,vtkTupleInterpolator);

  // Description:
  // Set/Get the tuple interpolator used to interpolate the view up portion
  // of the camera. Note that you can modify the behavior of the interpolator
  // (linear vs spline interpolation; change spline basis) by manipulating
  // the interpolator instances directly.
  virtual void SetViewUpInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(ViewUpInterpolator,vtkTupleInterpolator);

  // Description:
  // Set/Get the tuple interpolator used to interpolate the view angle portion
  // of the camera. Note that you can modify the behavior of the interpolator
  // (linear vs spline interpolation; change spline basis) by manipulating
  // the interpolator instances directly.
  virtual void SetViewAngleInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(ViewAngleInterpolator,vtkTupleInterpolator);

  // Description:
  // Set/Get the tuple interpolator used to interpolate the parallel scale portion
  // of the camera. Note that you can modify the behavior of the interpolator
  // (linear vs spline interpolation; change spline basis) by manipulating
  // the interpolator instances directly.
  virtual void SetParallelScaleInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(ParallelScaleInterpolator,vtkTupleInterpolator);

  // Description:
  // Set/Get the tuple interpolator used to interpolate the clipping range portion
  // of the camera. Note that you can modify the behavior of the interpolator
  // (linear vs spline interpolation; change spline basis) by manipulating
  // the interpolator instances directly.
  virtual void SetClippingRangeInterpolator(vtkTupleInterpolator*);
  vtkGetObjectMacro(ClippingRangeInterpolator,vtkTupleInterpolator);

  // Description:
  // Override GetMTime() because we depend on the interpolators which may be
  // modified outside of this class.
  unsigned long GetMTime();

protected:
  vtkCameraInterpolator();
  virtual ~vtkCameraInterpolator();

  // Control the interpolation type
  int InterpolationType;

  // These perform the interpolation
  vtkTupleInterpolator *PositionInterpolator;
  vtkTupleInterpolator *FocalPointInterpolator;
  vtkTupleInterpolator *ViewUpInterpolator;
  vtkTupleInterpolator *ViewAngleInterpolator;
  vtkTupleInterpolator *ParallelScaleInterpolator;
  vtkTupleInterpolator *ClippingRangeInterpolator;

  // Initialize the interpolating splines
  int Initialized;
  vtkTimeStamp InitializeTime;
  void InitializeInterpolation();

  // Hold the list of cameras. PIMPL'd STL list.
  vtkCameraList *CameraList;

private:
  vtkCameraInterpolator(const vtkCameraInterpolator&);  // Not implemented.
  void operator=(const vtkCameraInterpolator&);  // Not implemented.

};

#endif
