/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCamera - a virtual camera for 3D rendering
// .SECTION Description
// vtkCamera is a virtual camera for 3D rendering. It provides methods
// to position and orient the view point and focal point. Convenience 
// methods for moving about the focal point also are provided. More 
// complex methods allow the manipulation of the computer graphics
// model including view up vector, clipping planes, and 
// camera perspective.
// .SECTION See Also
// vtkProjectionTransform vtkPerspectiveTransform 

#ifndef __vtkCamera_h
#define __vtkCamera_h

#include "vtkObject.h"
#include "vtkTransform.h"
#include "vtkProjectionTransform.h"

class vtkRenderer;

class VTK_EXPORT vtkCamera : public vtkObject
{
 public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkCamera,vtkObject);

  // Description:
  // Construct camera instance with its focal point at the origin, 
  // and position=(0,0,1). The view up is along the y-axis, 
  // view angle is 30 degrees, and the clipping range is (.1,1000).
  static vtkCamera *New();

  // Description:
  // Set/Get the position of the camera in world coordinates.
  // After setting the position and focal point, you must also 
  // set the ViewPlaneNormal.
  void SetPosition(double x, double y, double z);
  void SetPosition(const double a[3]) {
    this->SetPosition(a[0], a[1], a[2]); };
  void SetPosition(const float a[3]) {
    this->SetPosition(a[0], a[1], a[2]); };
  vtkGetVector3Macro(Position,double);
  void GetPosition(float a[3]) {
    double tmp[3]; this->GetPosition(tmp); 
    a[0] = tmp[0]; a[1] = tmp[1]; a[2] = tmp[2]; };     

  // Description:
  // Set/Get the focal of the camera in world coordinates.
  // After setting the position and focal point, you must also 
  // set the ViewPlaneNormal.
  void SetFocalPoint(double x, double y, double z);
  void SetFocalPoint(const double a[3]) {
    this->SetFocalPoint(a[0], a[1], a[2]);};
  void SetFocalPoint(const float a[3]) {
    this->SetFocalPoint(a[0], a[1], a[2]);};
  vtkGetVector3Macro(FocalPoint,double);
  void GetFocalPoint(float a[3]) {
    double tmp[3]; this->GetFocalPoint(tmp); 
    a[0] = tmp[0]; a[1] = tmp[1]; a[2] = tmp[2]; }; 
  
  // Description:
  // Set/Get the view up direction for the camera.
  void SetViewUp(double vx, double vy, double vz);
  void SetViewUp(const double a[3]) {
    this->SetViewUp(a[0], a[1], a[2]); }
  vtkGetVector3Macro(ViewUp,double);
  void GetViewUp(float a[3]) {
    double tmp[3]; this->GetViewUp(tmp); 
    a[0] = tmp[0]; a[1] = tmp[1]; a[2] = tmp[2]; }; 

  // Description:
  // Set/Get the location of the front and back clipping planes along the
  // direction of projection. These are positive distances along the 
  // direction of projection. How these values are set can have a large
  // impact on how well z-buffering works. In particular the front clipping
  // plane can make a very big difference. Setting it to 0.01 when it
  // really could be 1.0 can have a big impact on your z-buffer resolution
  // farther away.
  void SetClippingRange(double front, double back);
  void SetClippingRange(const double a[2]) {
    this->SetClippingRange(a[0], a[1]); };
  void SetClippingRange(const float a[2]) {
    this->SetClippingRange(a[0], a[1]); };
  vtkGetVector2Macro(ClippingRange,double);
  void GetClippingRange(float a[2]) {
    double tmp[2]; this->GetClippingRange(tmp); 
    a[0] = tmp[0]; a[1] = tmp[1]; }; 

  // Description:
  // This method causes the camera to set up whatever is required for
  // viewing the scene. This is actually handled by an subclass of
  // vtkCamera, which is created through New()
  virtual void Render(vtkRenderer *) {};

  // Description:
  // Set/Get the camera view angle (i.e., the width of view in degrees). 
  // Larger values yield greater perspective distortion.
  void SetViewAngle(double angle);
  vtkGetMacro(ViewAngle,double);

  // Description:
  // Set/Get the separation between eyes (in degrees). This is used
  // when generating stereo images.
  vtkSetMacro(EyeAngle,double);
  vtkGetMacro(EyeAngle,double);

  // Description:
  // Is this camera rendering in stereo?
  vtkGetMacro(Stereo,int);

  // Description:
  // Set/Get the center of the window in viewport coordinates.
  // The viewport coordinate range is ([-1,+1],[-1,+1]).
  void SetWindowCenter(double x, double y);
  vtkGetVector2Macro(WindowCenter,double);

  // Description:
  // Set/Get the scaling used for a parallel projection, i.e. the height
  // of the viewport in world-coordinate distances.
  void SetParallelScale(double scale);
  vtkGetMacro(ParallelScale,double);

  // Description:
  // Set the size of the cameras lens in world coordinates. This is only 
  // used when the renderer is doing focal depth rendering. When that is 
  // being done the size of the focal disk will effect how significant the
  // depth effects will be.
  vtkSetMacro(FocalDisk,double);
  vtkGetMacro(FocalDisk,double);

  // Description:
  // Set/Get whether this is the left eye's render or the right eye's.
  // This is normally only used in stereo rendering and is handled
  // automatically.
  vtkSetMacro(LeftEye,int);
  vtkGetMacro(LeftEye,int);

  // Description:
  // Set the distance between clipping planes.  This method adjusts the 
  // far clipping plane to be set a distance 'thickness' beyond the
  // near clipping plane.
  void SetThickness(double);
  vtkGetMacro(Thickness,double);

  // Description:
  // Move the focal point so that it is the specified distance from
  // the camera position.  This distance must be positive.
  void SetDistance(double);
  vtkGetMacro(Distance,double);

  // Description: 
  // Set/Get the value of the ParallelProjection instance variable. This
  // determines if the camera should do a perspective or parallel projection.
  void SetParallelProjection(int flag) {
    if ( this->ParallelProjection != flag ) {
      this->ParallelProjection = flag;
      this->Modified();
      this->ViewingRaysModified(); } };
  vtkGetMacro(ParallelProjection,int);
  vtkBooleanMacro(ParallelProjection,int);

  // Description:
  // Set/Get the direction that the camera points.  After you call
  // this you _must_ adjust either the position or the focal point
  // so that the vector between them lies along the view plane normal.
  // Consider using SetPosition/SetFocalPoint instead and using
  // ComputeViewPlaneNormal() to compute the view plane normal.
  void SetViewPlaneNormal(double x, double y, double z);
  void SetViewPlaneNormal(const double a[3]) {
    this->SetViewPlaneNormal(a[0], a[1], a[2]); };
  void SetViewPlaneNormal(const float a[3]) {
    this->SetViewPlaneNormal(a[0], a[1], a[2]); };
  vtkGetVector3Macro(ViewPlaneNormal,double);
  void GetViewPlaneNormal(float a[3]) {
    double tmp[3]; this->GetViewPlaneNormal(tmp); 
    a[0] = tmp[0]; a[1] = tmp[1]; a[2] = tmp[2]; }; 

  // Description:
  // Return the view transform matrix.  This matrix converts world 
  // to camera coordinates.  It contains only rotation and translation, 
  // no perspective or scale.
  vtkMatrix4x4 *GetViewTransformMatrix();
  
  // Description:
  // Return the perspective transform matrix, which converts from camera
  // coordinates to viewport coordinates.  The 'aspect' is the
  // width/height for the viewport, and the nearz and farz are the
  // Z-buffer values that map to the near and far clipping planes.
  // The viewport coordinates are in the range ([-1,+1],[-1,+1],[nearz,farz]).
  vtkMatrix4x4 *GetPerspectiveTransformMatrix(double aspect,
					      double nearz, 
					      double farz);

  // Description:
  // Return the concatenation of the ViewTransform and the 
  // PerspectiveTransform.  This transform will convert world
  // coordinates to viewport coordinates.  The 'aspect' is the
  // width/height for the viewport, and the nearz and farz are the
  // Z-buffer values that map to the near and far clipping planes.
  // The viewport coordinates are in the range ([-1,+1],[-1,+1],[nearz,farz]).
  vtkMatrix4x4 *GetCompositePerspectiveTransformMatrix(double aspect, 
						       double nearz, 
						       double farz);

  // Description:
  // Set the roll angle of the camera about the view plane normal.
  void SetRoll(double angle);

  // Description:
  // Rotate the camera around the view plane normal.
  void Roll(double angle);

  // Description:
  // Returns the roll of the camera.
  double GetRoll();

  // Description:
  // Change the ViewAngle of the camera so that more or less of a scene 
  // occupies the viewport. A value greater than 1 is a zoom-in. 
  // A value less than 1 is a zoom-out.
  void Zoom(double factor);

  // Description:
  // Move the position of the camera along the view plane normal. Moving
  // towards the focal point (e.g., greater than 1) is a dolly-in, moving away 
  // from the focal point (e.g., less than 1) is a dolly-out.
  void Dolly(double distance);

  // Description:
  // Rotate the camera about the view up vector centered at the focal point.
  void Azimuth(double angle);

  // Description:
  // Rotate the focal point about the view up vector centered at the camera's 
  // position. 
  void Yaw(double angle);

  // Description:
  // Rotate the camera about the cross product of the view plane normal and 
  // the view up vector centered on the focal point.
  void Elevation(double angle);

  // Description:
  // Rotate the focal point about the cross product of the view up vector 
  // and the view plane normal, centered at the camera's position.
  void Pitch(double angle);

  // Description:
  // Returns the orientation of the camera. This is a vector of X,Y and Z 
  // rotations that when performed in the order RotateZ, RotateX, and finally
  // RotateY, will yield the same 3x3 rotation matrix for the camera.
  double *GetOrientation();

  // Description:
  // Returns the WXYZ orientation of the camera. 
  float *GetOrientationWXYZ();

  // Description:
  // Recompute the view up vector so that it is perpendicular to the
  // view plane normal.  You do not have to do this if you set a
  // ViewUp which is not orthogonal to your ViewPlaneNormal -- when
  // the view transformation matrix is created the ViewUp will
  // automatically be orthogonalized.
  void OrthogonalizeViewUp();

  // Description:
  // Return the MTime that concerns recomputing the view rays of the camera.
  unsigned long GetViewingRaysMTime();

  // Description:
  // Mark that something has changed which requires the view rays
  // to be recomputed.
  void ViewingRaysModified();

  // Description:
  // Get the plane equations that bound the view frustum.
  // The plane normals point inward. The planes array contains six
  // plane equations of the form (Ax+By+Cz+D=0), the first four
  // values are (A,B,C,D) which repeats for each of the planes.
  // The aspect of the viewport is needed to correctly compute the planes
  void GetFrustumPlanes(float aspect, float planes[24]);

  // Description:
  // These methods should only be used within vtkCamera.cxx.
  void ComputeViewPlaneNormal();
  void ComputeDistance();
  void ComputeViewTransform();
  void ComputePerspectiveTransform(double aspect, double nearz, double farz);

  // Description:
  // For legacy compatibility. Do not use.
  vtkMatrix4x4 &GetViewTransform(){return *this->GetViewTransformMatrix();}
  vtkMatrix4x4 &GetPerspectiveTransform(double aspect,double nearz,
					double farz)
    {return *this->GetPerspectiveTransformMatrix(aspect, nearz, farz);}
  vtkMatrix4x4 &GetCompositePerspectiveTransform(double aspect, 
						 double nearz, 
  						 double farz)
    {return *this->GetCompositePerspectiveTransformMatrix(aspect,nearz,farz);}
  
protected:
  vtkCamera();
  ~vtkCamera();
  vtkCamera(const vtkCamera&) {};
  void operator=(const vtkCamera&) {};

  double WindowCenter[2];
  double FocalPoint[3];
  double Position[3];
  double ViewUp[3];
  double ViewAngle;
  double ClippingRange[2];
  double EyeAngle;
  int   LeftEye;
  int   ParallelProjection;
  double ParallelScale;
  int   Stereo;  
  double Thickness;
  double Distance;
  double ViewPlaneNormal[3];
  vtkTransform *Transform;
  vtkProjectionTransform *PerspectiveTransform;
  double Orientation[3];
  double FocalDisk;

  // ViewingRaysMtime keeps track of camera modifications which will 
  // change the calculation of viewing rays for the camera before it is 
  // transformed to the camera's location and orientation. 
  vtkTimeStamp ViewingRaysMTime;

  // VPNDotDOP stores the dot product between the view plane normal and
  // the direction of projection. If this dot product changes then the view
  // rays must be updated.
  double        VPNDotDOP;

};

inline void vtkCamera::SetWindowCenter( double x, double y )
{
  if ((this->WindowCenter[0] != x)||(this->WindowCenter[1] != y))
    {
    this->Modified();
    this->ViewingRaysModified();
    this->WindowCenter[0] = x;
    this->WindowCenter[1] = y;
    }
}

inline void vtkCamera::SetViewAngle( double angle )
{
  double min =   1.0;
  double max = 179.0;

  if ( this->ViewAngle != angle )
    {
    this->ViewAngle = (angle<min?min:(angle>max?max:angle));
    this->Modified();
    this->ViewingRaysModified();
    }
}

inline void vtkCamera::SetParallelScale( double scale )
{
  if ( this->ParallelScale != scale )
    {
    this->ParallelScale = scale;
    this->Modified();
    this->ViewingRaysModified();
    }
}

#endif

