/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkCamera - a virtual camera for 3D rendering
// .SECTION Description
// vtkCamera is a virtual camera for 3D rendering. It provides methods
// to position and orient the view point and focal point. Convenience 
// methods for moving about the focal point also are provided. More 
// complex methods allow the manipulation of the computer graphics
// model including view up vector, clipping planes, and 
// camera perspective.

#ifndef __vtkCamera_h
#define __vtkCamera_h

#include "vtkObject.h"
#include "vtkTransform.h"

class vtkRenderer;

class VTK_EXPORT vtkCamera : public vtkObject
{
 public:
  void PrintSelf(ostream& os, vtkIndent indent);
  const char *GetClassName() {return "vtkCamera";};

  // Description:
  // Construct camera instance with its focal point at the origin, 
  // and position=(0,0,1). The view up is along the y-axis, 
  // view angle is 30 degrees, and the clipping range is (.1,1000).
  static vtkCamera *New();

  // Description:
  // Set/Get the position of the camera in world coordinates.
  void SetPosition(float a[3]) {this->SetPosition(a[0], a[1], a[2]);};
  void SetPosition(double x, double y, double z);
  void SetPosition(double a[3]);
  vtkGetVectorMacro(Position,double,3);
  void GetPosition(float a[3]) { a[0] = this->Position[0];
      a[1] = this->Position[1]; a[2] = this->Position[2];};

  // Description:
  // Set/Get the focal point of the camera in world coordinates
  void SetFocalPoint(float a[3]) {
    this->SetFocalPoint(a[0], a[1], a[2]);};
  void SetFocalPoint(double x, double y, double z);
  void SetFocalPoint(double a[3]);
  vtkGetVectorMacro(FocalPoint,double,3);
  void GetFocalPoint(float a[3]) { a[0] = this->FocalPoint[0];
      a[1] = this->FocalPoint[1]; a[2] = this->FocalPoint[2];};
  
  // Description:
  // Set/Get the view up direction for the camera.
  void SetViewUp(double vx, double vy, double vz);
  void SetViewUp(double a[3]);
  vtkGetVectorMacro(ViewUp,double,3);
  void GetViewUp( float a[3] ) { a[0] = this->ViewUp[0];
      a[1] = this->ViewUp[1]; a[2] = this->ViewUp[2]; };

  // Description:
  // Set/Get the location of the front and back clipping planes along the
  // direction of projection. These are positive distances along the 
  // direction of projection. How these values are set can have a large
  // impact on how well z-buffering works. In particular the front clipping
  // plane can make a very big difference. Setting it to 0.01 when it
  // really could be 1.0 can have a big impact on your z-buffer resolution
  // farther away.
  void SetClippingRange(double front, double back);
  void SetClippingRange(double a[2]);
  vtkGetVectorMacro(ClippingRange,double,2);
  void GetClippingRange(float a[2]) { a[0] = this->ClippingRange[0];
      a[1] = this->ClippingRange[1];};

  // Description:
  // This method causes the camera to set up whatever is required for
  // viewing the scene. This is actually handled by an subclass of
  // vtkCamera, which is created through New()
  virtual void Render(vtkRenderer *) {};

  // Description:
  // Set/Get the camera view angle (i.e., the width of view in degrees). 
  // Larger values yield greater perspective distortion.
  //  vtkSetClampMacro(ViewAngle,double,1.0,179.0);
  void SetViewAngle( double angle );
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
  // Set/Get the center of the window.
  //  vtkSetVector2Macro(WindowCenter,double);
  void SetWindowCenter( double x, double y );
  vtkGetVectorMacro(WindowCenter,double,2);

  // Description:
  // Set/Get the scaling used for a parallel projection.
  //  vtkSetMacro(ParallelScale,double);
  void SetParallelScale( double scale );
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
  // Set the distance between clipping planes. A side effect of this method is
  // to adjust the back clipping plane to be equal to the front clipping plane 
  // plus the thickness.
  void SetThickness(double);
  vtkGetMacro(Thickness,double);

  // Description:
  // Set the distance of the focal point from the camera. The focal point is 
  // modified accordingly. This should be positive.
  void SetDistance(double);
  vtkGetMacro(Distance,double);

  // Description: 
  // Set/Get the value of the ParallelProjection instance variable. This
  // determines if the camera should do a perspective or parallel projection.
  //  vtkSetMacro(ParallelProjection,int);
  void SetParallelProjection( int flag )
  {
    if ( this->ParallelProjection != flag )
      {
      this->ParallelProjection = flag;
      this->Modified();
      this->ViewingRaysModified();
      }
  }
  vtkGetMacro(ParallelProjection,int);
  vtkBooleanMacro(ParallelProjection,int);

  // Description:
  // Set/Get the direction that the camera points.
  // Adjusts position to be consistent with the view plane normal.
  void SetViewPlaneNormal(double x, double y, double z);
  void SetViewPlaneNormal(double a[3]);
  vtkGetVectorMacro(ViewPlaneNormal,double,3);
  void GetViewPlaneNormal(float a[3]) { a[0] = this->ViewPlaneNormal[0];
      a[1] = this->ViewPlaneNormal[1]; a[2] = this->ViewPlaneNormal[2];};

  // Description:
  // Compute the view plane normal from the position and focal point.
  void ComputeViewPlaneNormal();

  // Description:
  // Compute the camera distance, which is the distance between the 
  // focal point and position.
  void ComputeDistance();

  // Description:
  // Compute the view transform matrix. This is used in converting 
  // between view and world coordinates. It does not include any 
  // perspective effects but it does include shearing and scaling.
  void ComputeViewTransform();

  // Description:
  // Compute the perspective transform matrix. This is used in converting 
  // between view and world coordinates.
  void ComputePerspectiveTransform(double aspect, double nearz, double farz);

  // Description:
  // Return the perspective transform matrix. See ComputePerspectiveTransform.
  vtkMatrix4x4 *GetViewTransformMatrix();
  
  // Description:
  // Return the perspective transform matrix. See ComputePerspectiveTransform.
  vtkMatrix4x4 *GetPerspectiveTransformMatrix(double aspect,
					      double nearz, double farz);

  // Description:
  // Return the perspective transform matrix. See ComputePerspectiveTransform.
  vtkMatrix4x4 *GetCompositePerspectiveTransformMatrix(double aspect, 
						       double nearz, 
						       double farz);

  // Description:
  // Set the roll angle of the camera about the view plane normal.
  void SetRoll(double);

  // Description:
  // Rotate the camera around the view plane normal.
  void Roll(double);

  // Description:
  // Returns the roll of the camera.
  double GetRoll();

  // Description:
  // Change the ViewAngle of the camera so that more or less of a scene 
  // occupies the viewport. A value greater than 1 is a zoom-in. 
  // A value less than 1 is a zoom-out.
  void Zoom(double);

  // Description:
  // Move the position of the camera along the view plane normal. Moving
  // towards the focal point (e.g., greater than 1) is a dolly-in, moving away 
  // from the focal point (e.g., less than 1) is a dolly-out.
  void Dolly(double);

  // Description:
  // Rotate the camera about the view up vector centered at the focal point.
  void Azimuth(double);

  // Description:
  // Rotate the focal point about the view up vector centered at the camera's 
  // position. 
  void Yaw(double);

  // Description:
  // Rotate the camera about the cross product of the view plane normal and 
  // the view up vector centered on the focal point.
  void Elevation(double);

  // Description:
  // Rotate the focal point about the cross product of the view up vector 
  // and the view plane normal, centered at the camera's position.
  void Pitch(double);

  // Description:
  // Recompute the view up vector so that it is perpendicular to the
  // view plane normal.
  void OrthogonalizeViewUp();

  // Description:
  // Returns the orientation of the camera. This is a vector of X,Y and Z 
  // rotations that when performed in the order RotateZ, RotateX, and finally
  // RotateY, will yield the same 3x3 rotation matrix for the camera.
  double *GetOrientation();

  // Description:
  // Returns the WXYZ orientation of the camera. 
  float *GetOrientationWXYZ();

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
  void GetFrustumPlanes( float planes[24] );

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
  vtkTransform *PerspectiveTransform;
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

