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
  vtkCamera();
  ~vtkCamera();
  void PrintSelf(ostream& os, vtkIndent indent);
  const char *GetClassName() {return "vtkCamera";};

  // Description:
  // Construct camera instance with its focal point at the origin, 
  // and position=(0,0,1). The view up is along the y-axis, 
  // view angle is 30 degrees, and the clipping range is (.1,1000).
  static vtkCamera *New();

  // Description:
  // Set/Get the position of the camera in world coordinates.
  void SetPosition(float x, float y, float z);
  void SetPosition(float a[3]);
  vtkGetVectorMacro(Position,float,3);

  // Description:
  // Set/Get the focal point of the camera in world coordinates
  void SetFocalPoint(float x, float y, float z);
  void SetFocalPoint(float a[3]);
  vtkGetVectorMacro(FocalPoint,float,3);

  // Description:
  // Set/Get the view up direction for the camera.
  void SetViewUp(float vx, float vy, float vz);
  void SetViewUp(float a[3]);
  vtkGetVectorMacro(ViewUp,float,3);

  // Description:
  // Set/Get the location of the front and back clipping planes along the
  // direction of projection. These are positive distances along the 
  // direction of projection. How these values are set can have a large
  // impact on how well z-buffering works. In particular the front clipping
  // plane can make a very big difference. Setting it to 0.01 when it
  // really could be 1.0 can have a big impact on your z-buffer resolution
  // farther away.
  void SetClippingRange(float front, float back);
  void SetClippingRange(float a[2]);
  vtkGetVectorMacro(ClippingRange,float,2);

  // Description:
  // This method causes the camera to set up whatever is required for
  // viewing the scene. This is actually handled by an subclass of
  // vtkCamera, which is created through New()
  virtual void Render(vtkRenderer *) {};

  // Description:
  // Set/Get the camera view angle (i.e., the width of view in degrees). 
  // Larger values yield greater perspective distortion.
  //  vtkSetClampMacro(ViewAngle,float,1.0,179.0);
  void SetViewAngle( float angle );
  vtkGetMacro(ViewAngle,float);

  // Description:
  // Set/Get the separation between eyes (in degrees). This is used
  // when generating stereo images.
  vtkSetMacro(EyeAngle,float);
  vtkGetMacro(EyeAngle,float);

  // Description:
  // Is this camera rendering in stereo?
  vtkGetMacro(Stereo,int);

  // Description:
  // Set/Get the center of the window.
  //  vtkSetVector2Macro(WindowCenter,float);
  void SetWindowCenter( float x, float y );
  vtkGetVectorMacro(WindowCenter,float,2);

  // Description:
  // Set/Get the scaling used for a parallel projection.
  //  vtkSetMacro(ParallelScale,float);
  void SetParallelScale( float scale );
  vtkGetMacro(ParallelScale,float);

  // Description:
  // Set the size of the cameras lens in world coordinates. This is only 
  // used when the renderer is doing focal depth rendering. When that is 
  // being done the size of the focal disk will effect how significant the
  // depth effects will be.
  vtkSetMacro(FocalDisk,float);
  vtkGetMacro(FocalDisk,float);

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
  void SetThickness(float);
  vtkGetMacro(Thickness,float);

  // Description:
  // Set the distance of the focal point from the camera. The focal point is 
  // modified accordingly. This should be positive.
  void SetDistance(float);
  vtkGetMacro(Distance,float);

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
  void SetViewPlaneNormal(float x, float y, float z);
  void SetViewPlaneNormal(float a[3]);
  vtkGetVectorMacro(ViewPlaneNormal,float,3);

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
  void ComputePerspectiveTransform(float aspect, float nearz, float farz);

  // Description:
  // Return the perspective transform matrix. See ComputePerspectiveTransform.
  vtkMatrix4x4 &GetViewTransform();
  
  // Description:
  // Return the perspective transform matrix. See ComputePerspectiveTransform.
  vtkMatrix4x4 &GetPerspectiveTransform(float aspect,
					float nearz, float farz);

  // Description:
  // Return the perspective transform matrix. See ComputePerspectiveTransform.
  vtkMatrix4x4 &GetCompositePerspectiveTransform(float aspect, 
						 float nearz, float farz);

  // Description:
  // Set the roll angle of the camera about the view plane normal.
  void SetRoll(float);

  // Description:
  // Rotate the camera around the view plane normal.
  void Roll(float);

  // Description:
  // Returns the roll of the camera.
  float GetRoll();

  // Description:
  // Change the ViewAngle of the camera so that more or less of a scene 
  // occupies the viewport. A value > 1 is a zoom-in. 
  // A value < 1 is a zoom-out.
  void Zoom(float);

  // Description:
  // Move the position of the camera along the view plane normal. Moving
  // towards the focal point (e.g., > 1) is a dolly-in, moving away 
  // from the focal point (e.g., < 1) is a dolly-out.
  void Dolly(float);

  // Description:
  // Rotate the camera about the view up vector centered at the focal point.
  void Azimuth(float);

  // Description:
  // Rotate the focal point about the view up vector centered at the camera's 
  // position. 
  void Yaw(float);

  // Description:
  // Rotate the camera about the cross product of the view plane normal and 
  // the view up vector centered on the focal point.
  void Elevation(float);

  // Description:
  // Rotate the focal point about the cross product of the view up vector 
  // and the view plane normal, centered at the camera's position.
  void Pitch(float);

  // Description:
  // Recompute the view up vector so that it is perpendicular to the
  // view plane normal.
  void OrthogonalizeViewUp();

  // Description:
  // Returns the orientation of the camera. This is a vector of X,Y and Z 
  // rotations that when performed in the order RotateZ, RotateX, and finally
  // RotateY, will yield the same 3x3 rotation matrix for the camera.
  float *GetOrientation();

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
  // The plane normals point inward.
  void GetFrustumPlanes( float planes[24] );

 protected:
  float WindowCenter[2];
  float FocalPoint[3];
  float Position[3];
  float ViewUp[3];
  float ViewAngle;
  float ClippingRange[2];
  float EyeAngle;
  int   LeftEye;
  int   ParallelProjection;
  float ParallelScale;
  int   Stereo;  
  float Thickness;
  float Distance;
  float ViewPlaneNormal[3];
  vtkTransform *Transform;
  vtkTransform *PerspectiveTransform;
  float Orientation[3];
  float FocalDisk;

  // ViewingRaysMtime keeps track of camera modifications which will 
  // change the calculation of viewing rays for the camera before it is 
  // transformed to the camera's location and orientation. 
  vtkTimeStamp ViewingRaysMTime;

  // VPNDotDOP stores the dot product between the view plane normal and
  // the direction of projection. If this dot product changes then the view
  // rays must be updated.
  float        VPNDotDOP;

};

inline void vtkCamera::SetWindowCenter( float x, float y )
{
  if ((this->WindowCenter[0] != x)||(this->WindowCenter[1] != y))
    {
    this->Modified();
    this->ViewingRaysModified();
    this->WindowCenter[0] = x;
    this->WindowCenter[1] = y;
    }
}

inline void vtkCamera::SetViewAngle( float angle )
{
  float min =   1.0;
  float max = 179.0;

  if ( this->ViewAngle != angle )
    {
    this->ViewAngle = (angle<min?min:(angle>max?max:angle));
    this->Modified();
    this->ViewingRaysModified();
    }
}

inline void vtkCamera::SetParallelScale( float scale )
{
  if ( this->ParallelScale != scale )
    {
    this->ParallelScale = scale;
    this->Modified();
    this->ViewingRaysModified();
    }
}

#endif

