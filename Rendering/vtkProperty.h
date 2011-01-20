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

class vtkActor;
class vtkRenderer;
class vtkShaderProgram;
class vtkTexture;
class vtkWindow;
class vtkXMLDataElement;
class vtkXMLMaterial;

class vtkPropertyInternals;

class VTK_RENDERING_EXPORT vtkProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkProperty,vtkObject);
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

  //BTX
  // Description:
  // This method is called after the actor has been rendered.
  // Don't call this directly. This method cleans up
  // any shaders allocated.
  virtual void PostRender(vtkActor*, vtkRenderer*);
  //ETX

  // Description:
  // Set/Get lighting flag for an object. Initial value is true.
  vtkGetMacro(Lighting,bool);
  vtkSetMacro(Lighting,bool);
  vtkBooleanMacro(Lighting,bool);
  
  // Description:
  // Set the shading interpolation method for an object.
  vtkSetClampMacro(Interpolation,int,VTK_FLAT,VTK_PHONG);
  vtkGetMacro(Interpolation,int);
  void SetInterpolationToFlat() {this->SetInterpolation(VTK_FLAT);};
  void SetInterpolationToGouraud() {this->SetInterpolation(VTK_GOURAUD);};
  void SetInterpolationToPhong() {this->SetInterpolation(VTK_PHONG);};
  const char *GetInterpolationAsString();

  // Description:
  // Control the surface geometry representation for the object.
  vtkSetClampMacro(Representation,int,VTK_POINTS,VTK_SURFACE);
  vtkGetMacro(Representation,int);
  void SetRepresentationToPoints() {this->SetRepresentation(VTK_POINTS);};
  void SetRepresentationToWireframe() {
    this->SetRepresentation(VTK_WIREFRAME);};
  void SetRepresentationToSurface() {this->SetRepresentation(VTK_SURFACE);};
  const char *GetRepresentationAsString();

  // Description:
  // Set the color of the object. Has the side effect of setting the
  // ambient diffuse and specular colors as well. This is basically
  // a quick overall color setting method.
  void SetColor(double r, double g, double b);
  void SetColor(double a[3]) { this->SetColor(a[0], a[1], a[2]); };
  double *GetColor();
  void GetColor(double rgb[3]);
  void GetColor(double &r, double &g, double &b);

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
  vtkSetClampMacro(SpecularPower,double,0.0,128.0);
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
  vtkGetVector3Macro(AmbientColor,double);

  // Description:
  // Set/Get the diffuse surface color.
  vtkSetVector3Macro(DiffuseColor,double);
  vtkGetVector3Macro(DiffuseColor,double);

  // Description:
  // Set/Get the specular surface color.
  vtkSetVector3Macro(SpecularColor,double);
  vtkGetVector3Macro(SpecularColor,double);

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
  vtkGetVector3Macro(EdgeColor,double);

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

  // Description:
  // Returns the name of the material currenly loaded, if any.
  vtkGetStringMacro(MaterialName);

  // Description:
  // Load the material. The material can be the name of a
  // built-on material or the filename for a VTK material XML description.
  void LoadMaterial(const char* name);

  // Description:
  // Load the material given the contents of the material file.
  void LoadMaterialFromString(const char* materialxml);

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

  // Description:
  // Provide values to initialize shader variables.
  // Useful to initialize shader variables that change over time
  // (animation, GUI widgets inputs, etc. )
  // - \p name - hardware name of the uniform variable
  // - \p numVars - number of variables being set
  // - \p x - values
  virtual void AddShaderVariable(const char *name, int numVars, int *x);
  virtual void AddShaderVariable(const char *name, int numVars, float *x);
  virtual void AddShaderVariable(const char *name, int numVars, double *x);

  // Description:
  // Methods to provide to add shader variables from tcl.
  void AddShaderVariable(const char* name, int v)
    {
    this->AddShaderVariable(name, 1, &v);
    }
  void AddShaderVariable(const char* name, float v)
    {
    this->AddShaderVariable(name, 1, &v);
    }
  void AddShaderVariable(const char* name, double v)
    {
    this->AddShaderVariable(name, 1, &v);
    }
  void AddShaderVariable(const char* name, int v1, int v2)
    {
    int v[2];
    v[0] = v1;
    v[1] = v2;
    this->AddShaderVariable(name, 2, v);
    }
  void AddShaderVariable(const char* name, float v1, float v2)
    {
    float v[2];
    v[0] = v1;
    v[1] = v2;
    this->AddShaderVariable(name, 2, v);
    }
  void AddShaderVariable(const char* name, double v1, double v2)
    {
    double v[2];
    v[0] = v1;
    v[1] = v2;
    this->AddShaderVariable(name, 2, v);
    }
  void AddShaderVariable(const char* name, int v1, int v2, int v3)
    {
    int v[3];
    v[0] = v1;
    v[1] = v2;
    v[2] = v3;
    this->AddShaderVariable(name, 3, v);
    }
  void AddShaderVariable(const char* name, float v1, float v2, float v3)
    {
    float v[3];
    v[0] = v1;
    v[1] = v2;
    v[2] = v3;
    this->AddShaderVariable(name, 3, v);
    }
  void AddShaderVariable(const char* name, double v1, double v2, double v3)
    {
    double v[3];
    v[0] = v1;
    v[1] = v2;
    v[2] = v3;
    this->AddShaderVariable(name, 3, v);
    }

  // Description:
  // Set/Get the texture object to control rendering texture maps.  This will
  // be a vtkTexture object. A property does not need to have an associated
  // texture map and multiple properties can share one texture. Textures
  // must be assigned unique names.
  void SetTexture(const char* name, vtkTexture* texture);
  vtkTexture* GetTexture(const char* name);

  // Description:
  // Set/Get the texture object to control rendering texture maps.  This will
  // be a vtkTexture object. A property does not need to have an associated
  // texture map and multiple properties can share one texture. Textures
  // must be assigned unique names.
  void SetTexture(int unit, vtkTexture* texture);
  vtkTexture* GetTexture(int unit);
  void RemoveTexture(int unit);


  // Description:
  // Remove a texture from the collection. Note that the
  // indices of all the subsquent textures, if any, will change.
  void RemoveTexture(const char* name);

  // Description:
  // Remove all the textures.
  void RemoveAllTextures();

  // Description:
  // Returns the number of textures in this property.
  int GetNumberOfTextures();

  // Description:
  // Release any graphics resources that are being consumed by this
  // property. The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *win);

//BTX
  // Description:
  // Used to specify which texture unit a texture will use.
  // Only relevant when multitexturing.
  enum VTKTextureUnit
  {
    VTK_TEXTURE_UNIT_0 = 0,
    VTK_TEXTURE_UNIT_1,
    VTK_TEXTURE_UNIT_2,
    VTK_TEXTURE_UNIT_3,
    VTK_TEXTURE_UNIT_4,
    VTK_TEXTURE_UNIT_5,
    VTK_TEXTURE_UNIT_6,
    VTK_TEXTURE_UNIT_7
  };
//ETX

protected:
  vtkProperty();
  ~vtkProperty();

  // Description:
  // Load property iVar values from the Material XML.
  void LoadProperty();
  void LoadTextures();
  void LoadTexture(vtkXMLDataElement* elem);
  void LoadPerlineNoise(vtkXMLDataElement* );
  void LoadMember(vtkXMLDataElement* elem);

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
  bool Lighting;

  int Shading;

  char* MaterialName;
  vtkSetStringMacro(MaterialName);

  vtkShaderProgram* ShaderProgram;
  void SetShaderProgram(vtkShaderProgram*);

  vtkXMLMaterial* Material; // TODO: I wonder if this reference needs to be maintained.

  // Description:
  // Read this->Material from new style shaders.
  // Default implementation is empty.
  virtual void ReadFrameworkMaterial();
  
//BTX
  // These friends are provided only for the time being
  // till we device a graceful way of loading texturing for GLSL.
  friend class vtkGLSLShaderProgram;
  friend class vtkShader;
  // FIXME:
  // Don't use these methods. They will be removed. They are provided only
  // for the time-being.
  vtkTexture* GetTextureAtIndex(int index);
  int GetTextureUnitAtIndex(int index);
  int GetTextureUnit(const char* name);
//ETX

private:
  vtkProperty(const vtkProperty&);  // Not implemented.
  void operator=(const vtkProperty&);  // Not implemented.

  vtkPropertyInternals* Internals;
};

// Description:
// Return the method of shading as a descriptive character string.
inline const char *vtkProperty::GetInterpolationAsString(void)
{
  if ( this->Interpolation == VTK_FLAT )
    {
    return "Flat";
    }
  else if ( this->Interpolation == VTK_GOURAUD )
    {
    return "Gouraud";
    }
  else
    {
    return "Phong";
    }
}


// Description:
// Return the method of shading as a descriptive character string.
inline const char *vtkProperty::GetRepresentationAsString(void)
{
  if ( this->Representation == VTK_POINTS )
    {
    return "Points";
    }
  else if ( this->Representation == VTK_WIREFRAME )
    {
    return "Wireframe";
    }
  else
    {
    return "Surface";
    }
}



#endif
