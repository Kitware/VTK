/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

/* need for virtual function */
class vtkRenderer;
class vtkMatrix4x4;

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
  // set to (0,0,1). The light is a SceneLight, its color is white
  // (black ambient, white diffuse, white specular), 
  // intensity=1, the light is turned on, positional lighting is off,
  // ConeAngle=30, AttenuationValues=(1,0,0), Exponent=1 and the
  // TransformMatrix is NULL.
  static vtkLight *New();

  // Description:
  // Create a new light object with the same light parameters than the current
  // object (any ivar from the superclasses (vtkObject and vtkObjectBase),
  // like reference counting, timestamp and observers are not copied).
  // This is a shallow clone (TransformMatrix is referenced)
  virtual vtkLight *ShallowClone();
  
  // Description:
  // Abstract interface to renderer. Each concrete subclass of vtkLight
  // will load its data into the graphics system in response to this method
  // invocation. The actual loading is performed by a vtkLightDevice
  // subclass, which will get created automatically.
  virtual void Render(vtkRenderer *, int) {};

  // Description:
  // Set/Get the color of the light. It is possible to set the ambient,
  // diffuse and specular colors separately. The SetColor() method sets
  // the diffuse and specular colors to the same color (this is a feature
  // to preserve backward compatbility.)
  vtkSetVector3Macro(AmbientColor,double);
  vtkGetVectorMacro(AmbientColor,double,3);
  vtkSetVector3Macro(DiffuseColor,double);
  vtkGetVectorMacro(DiffuseColor,double,3);
  vtkSetVector3Macro(SpecularColor,double);
  vtkGetVectorMacro(SpecularColor,double,3);
  void SetColor(double, double, double); 
  void SetColor(double a[3]) { this->SetColor(a[0], a[1], a[2]); }
  
  // Description:
  // @deprecated Use GetDiffuseColor instead as of VTK 5.7.
  VTK_LEGACY(void GetColor(double rgb[3]));
  VTK_LEGACY(double *GetColor());

  // Description:
  // Set/Get the position of the light.
  // Note: The position of the light is defined in the coordinate
  // space indicated by its transformation matrix (if it exists).
  // Thus, to get the light's world space position, use 
  // vtkGetTransformedPosition() instead of vtkGetPosition().
  vtkSetVector3Macro(Position,double);
  vtkGetVectorMacro(Position,double,3);
  void SetPosition(float *a) {this->SetPosition(a[0],a[1],a[2]);};
  
  // Description:
  // Set/Get the point at which the light is shining.
  // Note: The focal point of the light is defined in the coordinate
  // space indicated by its transformation matrix (if it exists).
  // Thus, to get the light's world space focal point, use 
  // vtkGetTransformedFocalPoint() instead of vtkGetFocalPoint().
  vtkSetVector3Macro(FocalPoint,double);
  vtkGetVectorMacro(FocalPoint,double,3);
  void SetFocalPoint(float *a) {this->SetFocalPoint(a[0],a[1],a[2]);};

  // Description:
  // Set/Get the brightness of the light (from one to zero).
  vtkSetMacro(Intensity,double);
  vtkGetMacro(Intensity,double);

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
  vtkSetClampMacro(Exponent,double,0.0,128.0);
  vtkGetMacro(Exponent,double);

  // Description:
  // Set/Get the lighting cone angle of a positional light in degrees.
  // This is the angle between the axis of the cone and a ray along the edge of
  // the cone.
  // A value of 180 indicates that you want no spot lighting effects
  // just a positional light.
  vtkSetMacro(ConeAngle,double);
  vtkGetMacro(ConeAngle,double);

  // Description:
  // Set/Get the quadratic attenuation constants. They are specified as
  // constant, linear, and quadratic, in that order.
  vtkSetVector3Macro(AttenuationValues,double);
  vtkGetVectorMacro(AttenuationValues,double,3);

  // Description:
  // Set/Get the light's transformation matrix.  If a matrix is set for
  // a light, the light's parameters (position and focal point) are 
  // transformed by the matrix before being rendered.
  virtual void SetTransformMatrix(vtkMatrix4x4*);
  vtkGetObjectMacro(TransformMatrix,vtkMatrix4x4);

  // Description:
  // Get the position of the light, modified by the transformation matrix
  // (if it exists).
  void GetTransformedPosition(double &a0, double &a1, double &a2);
  void GetTransformedPosition(double a[3]);
  double *GetTransformedPosition();

  // Description:
  // Get the focal point of the light, modified by the transformation matrix
  // (if it exists).
  void GetTransformedFocalPoint(double &a0, double &a1, double &a2);
  void GetTransformedFocalPoint(double a[3]);
  double *GetTransformedFocalPoint();

  // Description:
  // Set the position and focal point of a light based on elevation and
  // azimuth.  The light is moved so it is shining from the given angle.
  // Angles are given in degrees.  If the light is a
  // positional light, it is made directional instead.
  void SetDirectionAngle(double elevation, double azimuth);
  void SetDirectionAngle(double ang[2]) { 
    this->SetDirectionAngle(ang[0], ang[1]); };

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
  //
  // Note: Use SetLightTypeToSceneLight, rather than SetLightType(3), since
  // the former clears the light's transform matrix.
  vtkSetMacro(LightType, int);
  vtkGetMacro(LightType, int);
  void SetLightTypeToHeadlight()
    {this->SetLightType(VTK_LIGHT_TYPE_HEADLIGHT);}
  void SetLightTypeToSceneLight()
    {
    this->SetTransformMatrix(NULL);
    this->SetLightType(VTK_LIGHT_TYPE_SCENE_LIGHT);
    }
  void SetLightTypeToCameraLight()
    {this->SetLightType(VTK_LIGHT_TYPE_CAMERA_LIGHT);}

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

  double FocalPoint[3];
  double Position[3];
  double Intensity;
  double AmbientColor[3];
  double DiffuseColor[3];
  double SpecularColor[3];
  int    Switch;
  int    Positional;
  double Exponent;
  double ConeAngle;
  double AttenuationValues[3];
  vtkMatrix4x4 *TransformMatrix;
  double TransformedFocalPointReturn[3];
  double TransformedPositionReturn[3];
  int    LightType;

private:
  vtkLight(const vtkLight&);  // Not implemented.
  void operator=(const vtkLight&);  // Not implemented.
};

#endif
