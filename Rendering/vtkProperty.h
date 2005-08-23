/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProperty - represent surface properties of a geometric object
// .SECTION Description
// vtkProperty is an object that represents lighting and other surface
// properties of a geometric object. The primary properties that can be 
// set are colors (overall, ambient, diffuse, specular, and edge color);
// specular power; opacity of the object; the representation of the
// object (points, wireframe, or surface); and the shading method to be 
// used (flat, Gouraud, and Phong). Also, some special graphics features
// like backface properties can be set and manipulated with this object.

// .SECTION See Also
// vtkActor vtkPropertyDevice

#ifndef __vtkProperty_h
#define __vtkProperty_h

#include "vtkObject.h"

// shading models
#define VTK_FLAT    0
#define VTK_GOURAUD 1
#define VTK_PHONG   2

// representation models
#define VTK_POINTS    0
#define VTK_WIREFRAME 1
#define VTK_SURFACE   2

class vtkRenderer;
class vtkActor;
class vtkShaderProgram;
class vtkXMLMaterial;

class VTK_RENDERING_EXPORT vtkProperty : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with object color, ambient color, diffuse color,
  // specular color, and edge color white; ambient coefficient=0; diffuse 
  // coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
  // and surface representation. Backface and frontface culling are off.
  static vtkProperty *New();

  // Description:
  // Assign one property to another. 
  void DeepCopy(vtkProperty *p);

  // Description:
  // This method causes the property to set up whatever is required for
  // its instance variables. This is actually handled by a subclass of
  // vtkProperty, which is created automatically. This
  // method includes the invoking actor as an argument which can
  // be used by property devices that require the actor.
  virtual void Render(vtkActor *,vtkRenderer *);

  // Description:
  // This method renders the property as a backface property. TwoSidedLighting
  // must be turned off to see any backface properties. Note that only
  // colors and opacity are used for backface properties. Other properties
  // such as Representation, Culling are specified by the Property.
  virtual void BackfaceRender(vtkActor *,vtkRenderer *) {};

  // BTX
  // Description:
  // This method is called after the actor has been rendered.
  // Don't call this directly. This method cleans up 
  // any shaders allocated.
  virtual void PostRender(vtkActor*, vtkRenderer*);
  // ETX
  
  // Description:
  // Set the shading interpolation method for an object.
  vtkSetClampMacro(Interpolation,int,VTK_FLAT,VTK_PHONG);
  vtkGetMacro(Interpolation,int);
  void SetInterpolationToFlat() {this->SetInterpolation(VTK_FLAT);};
  void SetInterpolationToGouraud() {this->SetInterpolation(VTK_GOURAUD);};
  void SetInterpolationToPhong() {this->SetInterpolation(VTK_PHONG);};
  char *GetInterpolationAsString();

  // Description:
  // Control the surface geometry representation for the object.
  vtkSetClampMacro(Representation,int,VTK_POINTS,VTK_SURFACE);
  vtkGetMacro(Representation,int);
  void SetRepresentationToPoints() {this->SetRepresentation(VTK_POINTS);};
  void SetRepresentationToWireframe() {
    this->SetRepresentation(VTK_WIREFRAME);};
  void SetRepresentationToSurface() {this->SetRepresentation(VTK_SURFACE);};
  char *GetRepresentationAsString();

  // Description:
  // Set the color of the object. Has the side effect of setting the
  // ambient diffuse and specular colors as well. This is basically
  // a quick overall color setting method.
  void SetColor(double r,double g,double b);
  void SetColor(double a[3]) { this->SetColor(a[0], a[1], a[2]); };
  double *GetColor();
  void GetColor(double rgb[3]);

  // Description:
  // Set/Get the ambient lighting coefficient.
  vtkSetClampMacro(Ambient,double,0.0,1.0);
  vtkGetMacro(Ambient,double);

  // Description:
  // Set/Get the diffuse lighting coefficient.
  vtkSetClampMacro(Diffuse,double,0.0,1.0);
  vtkGetMacro(Diffuse,double);

  // Description:
  // Set/Get the specular lighting coefficient.
  vtkSetClampMacro(Specular,double,0.0,1.0);
  vtkGetMacro(Specular,double);

  // Description:
  // Set/Get the specular power.
  vtkSetClampMacro(SpecularPower,double,0.0,100.0);
  vtkGetMacro(SpecularPower,double);

  // Description:
  // Set/Get the object's opacity. 1.0 is totally opaque and 0.0 is completely
  // transparent.
  vtkSetClampMacro(Opacity,double,0.0,1.0);
  vtkGetMacro(Opacity,double);

  // Description:
  // Set/Get the ambient surface color. Not all renderers support separate
  // ambient and diffuse colors. From a physical standpoint it really
  // doesn't make too much sense to have both. For the rendering
  // libraries that don't support both, the diffuse color is used.
  vtkSetVector3Macro(AmbientColor,double);
  vtkGetVectorMacro(AmbientColor,double,3);

  // Description:
  // Set/Get the diffuse surface color.
  vtkSetVector3Macro(DiffuseColor,double);
  vtkGetVectorMacro(DiffuseColor,double,3);

  // Description:
  // Set/Get the specular surface color.
  vtkSetVector3Macro(SpecularColor,double);
  vtkGetVectorMacro(SpecularColor,double,3);

  // Description:
  // Turn on/off the visibility of edges. On some renderers it is
  // possible to render the edges of geometric primitives separately
  // from the interior.
  vtkGetMacro(EdgeVisibility,int);
  vtkSetMacro(EdgeVisibility,int);
  vtkBooleanMacro(EdgeVisibility,int);

  // Description:
  // Set/Get the color of primitive edges (if edge visibility is enabled).
  vtkSetVector3Macro(EdgeColor,double);
  vtkGetVectorMacro(EdgeColor,double,3);

  // Description:
  // Set/Get the width of a Line. The width is expressed in screen units.
  // This is only implemented for OpenGL. The default is 1.0.
  vtkSetClampMacro(LineWidth,float,0,VTK_LARGE_FLOAT);
  vtkGetMacro(LineWidth,float);

  // Description:
  // Set/Get the stippling pattern of a Line, as a 16-bit binary pattern 
  // (1 = pixel on, 0 = pixel off).
  // This is only implemented for OpenGL. The default is 0xFFFF.
  vtkSetMacro(LineStipplePattern,int);
  vtkGetMacro(LineStipplePattern,int);

  // Description:
  // Set/Get the stippling repeat factor of a Line, which specifies how
  // many times each bit in the pattern is to be repeated.
  // This is only implemented for OpenGL. The default is 1.
  vtkSetClampMacro(LineStippleRepeatFactor,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(LineStippleRepeatFactor,int);

  // Description:
  // Set/Get the diameter of a point. The size is expressed in screen units.
  // This is only implemented for OpenGL. The default is 1.0.
  vtkSetClampMacro(PointSize,float,0,VTK_LARGE_FLOAT);
  vtkGetMacro(PointSize,float);

  // Description:
  // Turn on/off fast culling of polygons based on orientation of normal 
  // with respect to camera. If backface culling is on, polygons facing
  // away from camera are not drawn.
  vtkGetMacro(BackfaceCulling,int);
  vtkSetMacro(BackfaceCulling,int);
  vtkBooleanMacro(BackfaceCulling,int);

  // Description:
  // Turn on/off fast culling of polygons based on orientation of normal 
  // with respect to camera. If frontface culling is on, polygons facing
  // towards camera are not drawn.
  vtkGetMacro(FrontfaceCulling,int);
  vtkSetMacro(FrontfaceCulling,int);
  vtkBooleanMacro(FrontfaceCulling,int);

  // Description:
  // Get the material representation used for shading. The material will be used
  // only when shading is enabled. 
  vtkGetObjectMacro(Material, vtkXMLMaterial);

  // TODO: GetMaterialName().
  
  // Description:
  // Load the material. The material can be the name of a
  // built-on material or the filename for a VTK material XML description.
  void LoadMaterial(const char* name);

  // Description:
  // Load the material given the material representation.
  void LoadMaterial(vtkXMLMaterial*);
  
  // Description:
  // Enable/Disable shading. When shading is enabled, the
  // Material must be set.
  vtkSetMacro(Shading, int);
  vtkGetMacro(Shading, int);
  vtkBooleanMacro(Shading, int);

  // Description:
  // Get the Shader program. If Material is not set/or not loaded properly,
  // this will return null.
  vtkGetObjectMacro(ShaderProgram, vtkShaderProgram);
 
  // TODO: I am not adding the AddShaderVariable() methods to the
  // property. One must get the shader program and then set the variables.
  // This will keep the user from adding shader variables before
  // setting the material.
protected:
  vtkProperty();
  ~vtkProperty();

  // Description:
  // Load property iVar values from the Material XML.
  void LoadProperty();

  double Color[3];
  double AmbientColor[3];
  double DiffuseColor[3];
  double SpecularColor[3];
  double EdgeColor[3];
  double Ambient;
  double Diffuse;
  double Specular;
  double SpecularPower;
  double Opacity;
  float PointSize;
  float LineWidth;
  int   LineStipplePattern;
  int   LineStippleRepeatFactor;
  int   Interpolation; 
  int   Representation;
  int   EdgeVisibility;
  int   BackfaceCulling;
  int   FrontfaceCulling;

  int Shading;
  
  char* MaterialName;
  vtkSetStringMacro(MaterialName);

  vtkShaderProgram* ShaderProgram;
  void SetShaderProgram(vtkShaderProgram*);

  vtkXMLMaterial* Material; // TODO: I wonder if this reference needs to be maintained.
  
private:
  vtkProperty(const vtkProperty&);  // Not implemented.
  void operator=(const vtkProperty&);  // Not implemented.
};

// Description:
// Return the method of shading as a descriptive character string.
inline char *vtkProperty::GetInterpolationAsString(void)
{
  if ( this->Interpolation == VTK_FLAT )
    {
    return (char *)"Flat";
    }
  else if ( this->Interpolation == VTK_GOURAUD ) 
    {
    return (char *)"Gouraud";
    }
  else 
    {
    return (char *)"Phong";
    }
}


// Description:
// Return the method of shading as a descriptive character string.
inline char *vtkProperty::GetRepresentationAsString(void)
{
  if ( this->Representation == VTK_POINTS )
    {
    return (char *)"Points";
    }
  else if ( this->Representation == VTK_WIREFRAME ) 
    {
    return (char *)"Wireframe";
    }
  else 
    {
    return (char *)"Surface";
    }
}



#endif
