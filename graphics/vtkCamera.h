/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCamera *New();
  char *GetClassName() {return "vtkCamera";};

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

  vtkSetMacro(LeftEye,int);
  vtkGetMacro(LeftEye,int);

  void SetThickness(float);
  vtkGetMacro(Thickness,float);

  void SetDistance(float);
  vtkGetMacro(Distance,float);

  // Description: 
  // Set/Get the value of the ParallelProjection instance variable. This
  // determines if the camera should do a perspective or parallel projection.
//  vtkSetMacro(ParallelProjection,int);
  void SetParallelProjection( int flag )
  {
    if ( ParallelProjection != flag )
      {
      ParallelProjection = flag;
      this->Modified();
      this->ViewingRaysModified();
      }
  }

  vtkGetMacro(ParallelProjection,int);
  vtkBooleanMacro(ParallelProjection,int);

  void SetViewPlaneNormal(float a[3]);
  void SetViewPlaneNormal(float x, float y, float z);
  void CalcViewPlaneNormal();
  void CalcDistance();
  void CalcViewTransform();
  void CalcPerspectiveTransform(float aspect, float nearz, float farz);
  vtkMatrix4x4 &GetViewTransform();
  vtkMatrix4x4 &GetPerspectiveTransform(float aspect,
					float nearz, float farz);
  vtkMatrix4x4 &GetCompositePerspectiveTransform(float aspect, 
						 float nearz, float farz);
  vtkGetVectorMacro(ViewPlaneNormal,float,3);

  void SetRoll(float);
  void Roll(float);
  float GetRoll();

  void Zoom(float);
  void Dolly(float);
  void Azimuth(float);
  void Yaw(float);
  void Elevation(float);
  void Pitch(float);
  void OrthogonalizeViewUp();

  float *GetOrientation();
  float *GetOrientationWXYZ();

  unsigned long int GetViewingRaysMTime();
  void              ViewingRaysModified();

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
  vtkTransform Transform;
  vtkTransform PerspectiveTransform;
  float Orientation[3];
  float FocalDisk;

  // ViewingRaysMtime keeps track of camera modifications which will 
  // change the calculation of viewing rays for the camera before it is 
  // transformed to the camera's location and orientation. 
  vtkTimeStamp ViewingRaysMTime;

  // VPN_dot_DOP stores the dot product between the view plane normal and
  // the direction of projection. If this dot product changes then the view
  // rays must be updated.
  float        VPN_dot_DOP;

};

inline void vtkCamera::SetWindowCenter( float x, float y )
{
  if ((WindowCenter[0] != x)||(WindowCenter[1] != y))
    {
    this->Modified();
    this->ViewingRaysModified();
    WindowCenter[0] = x;
    WindowCenter[1] = y;
    }
}

inline void vtkCamera::SetViewAngle( float angle )
{
  float min =   1.0;
  float max = 179.0;

  if ( ViewAngle != angle )
    {
    ViewAngle = (angle<min?min:(angle>max?max:angle));
    this->Modified();
    this->ViewingRaysModified();
    }
}

inline void vtkCamera::SetParallelScale( float scale )
{
  if ( ParallelScale != scale )
    {
    ParallelScale = scale;
    this->Modified();
    this->ViewingRaysModified();
    }
}

#endif

