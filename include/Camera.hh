/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Camera.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// methods for moving about the focal point are also provided. More 
// complex methods allow the manipulation of the computer graphics
// graphics model including view up vector, clipping planes, and 
// camera perspective.

#ifndef __vtkCamera_hh
#define __vtkCamera_hh

#include "Object.hh"
#include "Trans.hh"

class vtkRenderer;
class vtkCameraDevice;

class vtkCamera : public vtkObject
{
 public:
  vtkCamera();
  ~vtkCamera();
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkCamera";};

  void SetPosition(float x, float y, float z);
  void SetPosition(float a[3]);
  vtkGetVectorMacro(Position,float,3);

  void SetFocalPoint(float x, float y, float z);
  void SetFocalPoint(float a[3]);
  vtkGetVectorMacro(FocalPoint,float,3);

  void SetViewUp(float vx, float vy, float vz);
  void SetViewUp(float a[3]);
  vtkGetVectorMacro(ViewUp,float,3);

  void SetClippingRange(float front, float back);
  void SetClippingRange(float a[2]);
  vtkGetVectorMacro(ClippingRange,float,2);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vtkCamera
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Render(vtkRenderer *ren);

  // Description:
  // Set the camera view angle (i.e., the width of view in degrees). Larger
  // values yield greater perspective distortion.
  vtkSetClampMacro(ViewAngle,float,1.0,179.0);
  // Description:
  // Get the camera view angle (i.e., the width of view in degrees).
  vtkGetMacro(ViewAngle,float);

  // Description:
  // Set the seperation between eyes (in degrees). Used to generate stereo
  // images.
  vtkSetMacro(EyeAngle,float);
  // Description:
  // Get the seperation between eyes (in degrees). Used to generate stereo
  // images.
  vtkGetMacro(EyeAngle,float);

  // Description:
  // Set the size of the cameras lense in world coordinates.
  vtkSetMacro(FocalDisk,float);
  // Description:
  // Get the size of the cameras lense in world coordinates.
  vtkGetMacro(FocalDisk,float);

  vtkSetMacro(LeftEye,int);
  vtkGetMacro(LeftEye,int);

  void SetThickness(float);
  vtkGetMacro(Thickness,float);

  void SetDistance(float);
  vtkGetMacro(Distance,float);

  // Description:
  // Turn the camera on/off.
  vtkSetMacro(Switch,int);
  // Description:
  // Get the value of the Switch instance variable. This indicates if the 
  // camera is on or off.
  vtkGetMacro(Switch,int);
  // Description:
  // Turn the camera on/off.
  vtkBooleanMacro(Switch,int);

  float GetTwist();
  void SetViewPlaneNormal(float a[3]);
  void SetViewPlaneNormal(float x, float y, float z);
  void CalcViewPlaneNormal();
  void CalcDistance();
  void CalcPerspectiveTransform();
  vtkMatrix4x4 &GetPerspectiveTransform();
  vtkGetVectorMacro(ViewPlaneNormal,float,3);

  void SetRoll(float);
  void Roll(float);
  float GetRoll();

  void Zoom(float);
  void Azimuth(float);
  void Yaw(float);
  void Elevation(float);
  void Pitch(float);
  void OrthogonalizeViewUp();

  float *GetOrientation();

 protected:
  float FocalPoint[3];
  float Position[3];
  float ViewUp[3];
  float ViewAngle;
  float ClippingRange[2];
  float EyeAngle;
  int   LeftEye;
  int   Switch;
  float Thickness;
  float Distance;
  float ViewPlaneNormal[3];
  vtkTransform Transform;
  vtkTransform PerspectiveTransform;
  float Orientation[3];
  float FocalDisk;
  vtkCameraDevice *Device;
};

#endif

