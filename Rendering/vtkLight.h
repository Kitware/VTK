/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLight.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkLight - a virtual light for 3D rendering
// .SECTION Description
// vtkLight is a virtual light for 3D rendering. It provides methods to locate
// and point the light, turn it on and off, and set its brightness and color.
// In addition to the basic infinite distance point light source attributes,
// you also can specify the light attenuation values and cone angle.
// These attributes are only used if the light is a positional light.
// The default is a directional light (e.g. infinite point light source).
//
// Lights have a type that describes how the light should move with respect
// to the camera.  A Headlight is always located at the current camera position
// and shines on the camera's focal point.  A CameraLight also moves with
// the camera, but may not be coincident to it.  CameraLights are defined
// in a normalized coordinate space where the camera is located at (0, 0, 1),
// the camera is looking at (0, 0, 0), and up is (0, 1, 0).  Finally, a 
// SceneLight is part of the scene itself and does not move with the camera.
// (Renderers are responsible for moving the light based on its type.)
//
// Lights have a transformation matrix that describes the space in which
// they are positioned.  A light's world space position and focal point
// are defined by their local position and focal point, transformed by
// their transformation matrix (if it exists).

#ifndef __vtkLight_h
#define __vtkLight_h

#include "vtkObject.h"
#include "vtkMatrix4x4.h"

/* need for virtual function */
class vtkRenderer;

#define VTK_LIGHT_TYPE_HEADLIGHT    1
#define VTK_LIGHT_TYPE_CAMERA_LIGHT 2
#define VTK_LIGHT_TYPE_SCENE_LIGHT  3

class VTK_RENDERING_EXPORT vtkLight : public vtkObject
{
public:
  vtkTypeMacro(vtkLight,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a light with the focal point at the origin and its position
  // set to (0,0,1). The light is a SceneLight, its color is white, 
  // intensity=1, and the light is turned on. 
  static vtkLight *New();

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vtkLight
  // will load its data into the graphics system in response to this method
  // invocation. The actual loading is performed by a vtkLightDevice
  // subclass, which will get created automatically.
  virtual void Render(vtkRenderer *, int) {};

  // Description:
  // Set/Get the color of the light.
  vtkSetVector3Macro(Color,float);
  vtkGetVectorMacro(Color,float,3);

  // Description:
  // Set/Get the position of the light.
  // Note: The position of the light is defined in the coordinate
  // space indicated by its transformation matrix (if it exists).
  // Thus, to get the light's world space position, use 
  // vtkGetTransformedPosition() instead of vtkGetPosition().
  vtkSetVector3Macro(Position,float);
  vtkGetVectorMacro(Position,float,3);
  void SetPosition(double *a) {this->SetPosition(a[0],a[1],a[2]);};
  
  // Description:
  // Set/Get the point at which the light is shining.
  // Note: The focal point of the light is defined in the coordinate
  // space indicated by its transformation matrix (if it exists).
  // Thus, to get the light's world space focal point, use 
  // vtkGetTransformedFocalPoint() instead of vtkGetFocalPoint().
  vtkSetVector3Macro(FocalPoint,float);
  vtkGetVectorMacro(FocalPoint,float,3);
  void SetFocalPoint(double *a) {this->SetFocalPoint(a[0],a[1],a[2]);};

  // Description:
  // Set/Get the brightness of the light (from one to zero).
  vtkSetMacro(Intensity,float);
  vtkGetMacro(Intensity,float);

  // Description:
  // Turn the light on or off.
  vtkSetMacro(Switch,int);
  vtkGetMacro(Switch,int);
  vtkBooleanMacro(Switch,int);

  // Description:
  // Turn positional lighting on or off.
  vtkSetMacro(Positional,int);
  vtkGetMacro(Positional,int);
  vtkBooleanMacro(Positional,int);

  // Description:
  // Set/Get the exponent of the cosine used in positional lighting.
  vtkSetMacro(Exponent,float);
  vtkGetMacro(Exponent,float);

  // Description:
  // Set/Get the lighting cone angle of a positional light in degrees.
  // A value of 180 indicates that you want no spot lighting effects
  // just a positional light.
  vtkSetMacro(ConeAngle,float);
  vtkGetMacro(ConeAngle,float);

  // Description:
  // Set/Get the quadratic attenuation constants. They are specified as
  // constant, linear, and quadratic, in that order.
  vtkSetVector3Macro(AttenuationValues,float);
  vtkGetVectorMacro(AttenuationValues,float,3);

  // Description:
  // Set/Get the light's transformation matrix.  If a matrix is set for
  // a light, the light's parameters (position and focal point) are 
  // transformed by the matrix before being rendered.
  vtkSetObjectMacro(TransformMatrix,vtkMatrix4x4);
  vtkGetObjectMacro(TransformMatrix,vtkMatrix4x4);

  // Description:
  // Get the position of the light, modified by the transformation matrix
  // (if it exists).
  void GetTransformedPosition(float &a0, float &a1, float &a2);
  void GetTransformedPosition(float a[3]);
  float *GetTransformedPosition();

  // Description:
  // Get the focal point of the light, modified by the transformation matrix
  // (if it exists).
  void GetTransformedFocalPoint(float &a0, float &a1, float &a2);
  void GetTransformedFocalPoint(float a[3]);
  float *GetTransformedFocalPoint();

  // Description:
  // Set the position and focal point of a light based on elevation and
  // azimuth.  The light is moved so it is shining from the given angle.
  // Angles are given in degrees.  If the light is a
  // positional light, it is made directional instead.
  void SetDirectionAngle(float elevation, float azimuth);
  void SetDirectionAngle(float ang[2]) { this->SetDirectionAngle(ang[0], ang[1]); };

  // Description:
  // Perform deep copy of this light.
  void DeepCopy(vtkLight *light);

  // Description:
  // Set/Get the type of the light.
  // A SceneLight is a light located in the world coordinate space.  A light
  // is initially created as a scene light.
  //
  // A Headlight is always located at the camera and is pointed at the 
  // camera's focal point.  The renderer is free to modify the position and
  // focal point of the camera at any time.
  //
  // A CameraLight is also attached to the camera, but is not necessarily
  // located at the camera's position.  CameraLights are defined in a 
  // coordinate space where the camera is located at (0, 0, 1), looking
  // towards (0, 0, 0) at a distance of 1, with up being (0, 1, 0).

  vtkSetMacro(LightType, int);
  vtkGetMacro(LightType, int);

  void SetLightTypeToHeadlight();
  void SetLightTypeToSceneLight();
  void SetLightTypeToCameraLight();

  // Description:
  // Query the type of the light.
  int LightTypeIsHeadlight();
  int LightTypeIsSceneLight();
  int LightTypeIsCameraLight();

  void ReadSelf(istream& is);
  void WriteSelf(ostream& os);
  
protected:
  vtkLight();
  ~vtkLight();
  vtkLight(const vtkLight&);
  void operator=(const vtkLight&);

  float FocalPoint[3];
  float Position[3];
  float Intensity;
  float Color[3];
  int   Switch;
  int   Positional;
  float Exponent;
  float ConeAngle;
  float AttenuationValues[3];
  vtkMatrix4x4 *TransformMatrix;
  float TransformedFocalPointReturn[3];
  float TransformedPositionReturn[3];
  int LightType;
};

#endif
