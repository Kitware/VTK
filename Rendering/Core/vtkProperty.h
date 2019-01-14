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
/**
 * @class   vtkProperty
 * @brief   represent surface properties of a geometric object
 *
 * vtkProperty is an object that represents lighting and other surface
 * properties of a geometric object. The primary properties that can be
 * set are colors (overall, ambient, diffuse, specular, and edge color);
 * specular power; opacity of the object; the representation of the
 * object (points, wireframe, or surface); and the shading method to be
 * used (flat, Gouraud, and Phong). Also, some special graphics features
 * like backface properties can be set and manipulated with this object.
 *
 * @sa
 * vtkActor vtkPropertyDevice
*/

#ifndef vtkProperty_h
#define vtkProperty_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"
#include <map> // used for ivar

// shading models
#define VTK_FLAT    0
#define VTK_GOURAUD 1
#define VTK_PHONG   2

// representation models
#define VTK_POINTS    0
#define VTK_WIREFRAME 1
#define VTK_SURFACE   2

class vtkActor;
class vtkInformation;
class vtkRenderer;
class vtkShaderProgram;
class vtkShaderDeviceAdapter2;
class vtkTexture;
class vtkWindow;
class vtkXMLDataElement;
class vtkXMLMaterial;

class vtkPropertyInternals;

class VTKRENDERINGCORE_EXPORT vtkProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with object color, ambient color, diffuse color,
   * specular color, and edge color white; ambient coefficient=0; diffuse
   * coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
   * and surface representation. Backface and frontface culling are off.
   */
  static vtkProperty *New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(vtkProperty *p);

  /**
   * This method causes the property to set up whatever is required for
   * its instance variables. This is actually handled by a subclass of
   * vtkProperty, which is created automatically. This
   * method includes the invoking actor as an argument which can
   * be used by property devices that require the actor.
   */
  virtual void Render(vtkActor *, vtkRenderer *);

  /**
   * This method renders the property as a backface property. TwoSidedLighting
   * must be turned off to see any backface properties. Note that only
   * colors and opacity are used for backface properties. Other properties
   * such as Representation, Culling are specified by the Property.
   */
  virtual void BackfaceRender(vtkActor *, vtkRenderer *) {}

  /**
   * This method is called after the actor has been rendered.
   * Don't call this directly. This method cleans up
   * any shaders allocated.
   */
  virtual void PostRender(vtkActor*, vtkRenderer*);

  //@{
  /**
   * Set/Get lighting flag for an object. Initial value is true.
   */
  vtkGetMacro(Lighting, bool);
  vtkSetMacro(Lighting, bool);
  vtkBooleanMacro(Lighting, bool);
  //@}

  //@{
  /**
   * Set/Get rendering of points as spheres. The size of the
   * sphere in pixels is controlled by the PointSize
   * attribute. Note that half spheres may be rendered
   * instead of spheres.
   */
  vtkGetMacro(RenderPointsAsSpheres, bool);
  vtkSetMacro(RenderPointsAsSpheres, bool);
  vtkBooleanMacro(RenderPointsAsSpheres, bool);
  //@}

  //@{
  /**
   * Set/Get rendering of lines as tubes. The width of the
   * line in pixels is controlled by the LineWidth
   * attribute. May not be supported on every platform
   * and the implementation may be half tubes, or something
   * only tube like in appearance.
   */
  vtkGetMacro(RenderLinesAsTubes, bool);
  vtkSetMacro(RenderLinesAsTubes, bool);
  vtkBooleanMacro(RenderLinesAsTubes, bool);
  //@}

  //@{
  /**
   * Set the shading interpolation method for an object.
   */
  vtkSetClampMacro(Interpolation, int, VTK_FLAT, VTK_PHONG);
  vtkGetMacro(Interpolation,int);
  void SetInterpolationToFlat()
    { this->SetInterpolation(VTK_FLAT); }
  void SetInterpolationToGouraud()
    { this->SetInterpolation(VTK_GOURAUD); }
  void SetInterpolationToPhong()
    { this->SetInterpolation(VTK_PHONG); }
  const char *GetInterpolationAsString();
  //@}

  //@{
  /**
   * Control the surface geometry representation for the object.
   */
  vtkSetClampMacro(Representation,int, VTK_POINTS, VTK_SURFACE);
  vtkGetMacro(Representation,int);
  void SetRepresentationToPoints()
    { this->SetRepresentation(VTK_POINTS); }
  void SetRepresentationToWireframe()
    { this->SetRepresentation(VTK_WIREFRAME); }
  void SetRepresentationToSurface()
    { this->SetRepresentation(VTK_SURFACE); }
  const char *GetRepresentationAsString();
  //@}

  //@{
  /**
   * Set the color of the object. Has the side effect of setting the
   * ambient diffuse and specular colors as well. This is basically
   * a quick overall color setting method.
   */
  virtual void SetColor(double r, double g, double b);
  virtual void SetColor(double a[3]);
  double *GetColor() VTK_SIZEHINT(3);
  void GetColor(double rgb[3]);
  void GetColor(double &r, double &g, double &b);
  //@}

  //@{
  /**
   * Set/Get the ambient lighting coefficient.
   */
  vtkSetClampMacro(Ambient, double, 0.0, 1.0);
  vtkGetMacro(Ambient, double);
  //@}

  //@{
  /**
   * Set/Get the diffuse lighting coefficient.
   */
  vtkSetClampMacro(Diffuse, double, 0.0, 1.0);
  vtkGetMacro(Diffuse, double);
  //@}

  //@{
  /**
   * Set/Get the specular lighting coefficient.
   */
  vtkSetClampMacro(Specular, double, 0.0, 1.0);
  vtkGetMacro(Specular, double);
  //@}

  //@{
  /**
   * Set/Get the specular power.
   */
  vtkSetClampMacro(SpecularPower, double, 0.0, 128.0);
  vtkGetMacro(SpecularPower, double);
  //@}

  //@{
  /**
   * Set/Get the object's opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  vtkSetClampMacro(Opacity, double, 0.0, 1.0);
  vtkGetMacro(Opacity, double);
  //@}

  //@{
  /**
   * Set/Get the ambient surface color. Not all renderers support separate
   * ambient and diffuse colors. From a physical standpoint it really
   * doesn't make too much sense to have both. For the rendering
   * libraries that don't support both, the diffuse color is used.
   */
  vtkSetVector3Macro(AmbientColor, double);
  vtkGetVector3Macro(AmbientColor, double);
  //@}

  //@{
  /**
   * Set/Get the diffuse surface color.
   */
  vtkSetVector3Macro(DiffuseColor, double);
  vtkGetVector3Macro(DiffuseColor, double);
  //@}

  //@{
  /**
   * Set/Get the specular surface color.
   */
  vtkSetVector3Macro(SpecularColor, double);
  vtkGetVector3Macro(SpecularColor, double);
  //@}

  //@{
  /**
   * Turn on/off the visibility of edges. On some renderers it is
   * possible to render the edges of geometric primitives separately
   * from the interior.
   */
  vtkGetMacro(EdgeVisibility, vtkTypeBool);
  vtkSetMacro(EdgeVisibility, vtkTypeBool);
  vtkBooleanMacro(EdgeVisibility, vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the color of primitive edges (if edge visibility is enabled).
   */
  vtkSetVector3Macro(EdgeColor, double);
  vtkGetVector3Macro(EdgeColor, double);
  //@}

  //@{
  /**
   * Turn on/off the visibility of vertices. On some renderers it is
   * possible to render the vertices of geometric primitives separately
   * from the interior.
   */
  vtkGetMacro(VertexVisibility, vtkTypeBool);
  vtkSetMacro(VertexVisibility, vtkTypeBool);
  vtkBooleanMacro(VertexVisibility, vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the color of primitive vertices (if vertex visibility is enabled).
   */
  vtkSetVector3Macro(VertexColor, double);
  vtkGetVector3Macro(VertexColor, double);
  //@}

  //@{
  /**
   * Set/Get the width of a Line. The width is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  vtkSetClampMacro(LineWidth, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(LineWidth, float);
  //@}

  //@{
  /**
   * Set/Get the stippling pattern of a Line, as a 16-bit binary pattern
   * (1 = pixel on, 0 = pixel off).
   * This is only implemented for OpenGL, not OpenGL2. The default is 0xFFFF.
   */
  vtkSetMacro(LineStipplePattern, int);
  vtkGetMacro(LineStipplePattern, int);
  //@}

  //@{
  /**
   * Set/Get the stippling repeat factor of a Line, which specifies how
   * many times each bit in the pattern is to be repeated.
   * This is only implemented for OpenGL, not OpenGL2. The default is 1.
   */
  vtkSetClampMacro(LineStippleRepeatFactor, int, 1, VTK_INT_MAX);
  vtkGetMacro(LineStippleRepeatFactor, int);
  //@}

  //@{
  /**
   * Set/Get the diameter of a point. The size is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  vtkSetClampMacro(PointSize, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(PointSize, float);
  //@}

  //@{
  /**
   * Turn on/off fast culling of polygons based on orientation of normal
   * with respect to camera. If backface culling is on, polygons facing
   * away from camera are not drawn.
   */
  vtkGetMacro(BackfaceCulling, vtkTypeBool);
  vtkSetMacro(BackfaceCulling, vtkTypeBool);
  vtkBooleanMacro(BackfaceCulling, vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off fast culling of polygons based on orientation of normal
   * with respect to camera. If frontface culling is on, polygons facing
   * towards camera are not drawn.
   */
  vtkGetMacro(FrontfaceCulling, vtkTypeBool);
  vtkSetMacro(FrontfaceCulling, vtkTypeBool);
  vtkBooleanMacro(FrontfaceCulling, vtkTypeBool);
  //@}

  //@{
  /**
   * Returns the name of the material currently loaded, if any.
   */
  vtkSetStringMacro(MaterialName);
  vtkGetStringMacro(MaterialName);
  //@}

  //@{
  /**
   * Enable/Disable shading. When shading is enabled, the
   * Material must be set.
   */
  vtkSetMacro(Shading, vtkTypeBool);
  vtkGetMacro(Shading, vtkTypeBool);
  vtkBooleanMacro(Shading, vtkTypeBool);
  //@}

  /**
   * Get the vtkShaderDeviceAdapter2 if set, returns null otherwise.
   */
  virtual vtkShaderDeviceAdapter2* GetShaderDeviceAdapter2()
    { return nullptr; }

  //@{
  /**
   * Provide values to initialize shader variables.
   * Useful to initialize shader variables that change over time
   * (animation, GUI widgets inputs, etc. )
   * - \p name - hardware name of the uniform variable
   * - \p numVars - number of variables being set
   * - \p x - values
   */
  virtual void AddShaderVariable(const char *name, int numVars, int *x);
  virtual void AddShaderVariable(const char *name, int numVars, float *x);
  virtual void AddShaderVariable(const char *name, int numVars, double *x);
  //@}

  //@{
  /**
   * Methods to provide to add shader variables from wrappers.
   */
  void AddShaderVariable(const char* name, int v)
    { this->AddShaderVariable(name, 1, &v); }
  void AddShaderVariable(const char* name, float v)
    { this->AddShaderVariable(name, 1, &v); }
  void AddShaderVariable(const char* name, double v)
    { this->AddShaderVariable(name, 1, &v); }
  void AddShaderVariable(const char* name, int v1, int v2)
  {
    int v[2] = {v1, v2};
    this->AddShaderVariable(name, 2, v);
  }
  void AddShaderVariable(const char* name, float v1, float v2)
  {
    float v[2] = {v1, v2};
    this->AddShaderVariable(name, 2, v);
  }
  void AddShaderVariable(const char* name, double v1, double v2)
  {
    double v[2] = {v1, v2};
    this->AddShaderVariable(name, 2, v);
  }
  void AddShaderVariable(const char* name, int v1, int v2, int v3)
  {
    int v[3] = {v1, v2, v3};
    this->AddShaderVariable(name, 3, v);
  }
  void AddShaderVariable(const char* name, float v1, float v2, float v3)
  {
    float v[3] = {v1, v2, v3};
    this->AddShaderVariable(name, 3, v);
  }
  void AddShaderVariable(const char* name, double v1, double v2, double v3)
  {
    double v[3] = {v1, v2, v3};
    this->AddShaderVariable(name, 3, v);
  }
  //@}

  //@{
  /**
   * Set/Get the texture object to control rendering texture maps. This will
   * be a vtkTexture object. A property does not need to have an associated
   * texture map and multiple properties can share one texture. Textures
   * must be assigned unique names. Note that for texture blending the
   * textures will be rendering is alphabetical order and after any texture
   * defined in the actor.
   */
  void SetTexture(const char* name, vtkTexture* texture);
  vtkTexture* GetTexture(const char* name);
  //@}

  //@{
  /**
   * Set/Get the texture object to control rendering texture maps. This will
   * be a vtkTexture object. A property does not need to have an associated
   * texture map and multiple properties can share one texture. Textures
   * must be assigned unique names.
   */
  VTK_LEGACY(void SetTexture(int unit, vtkTexture* texture));
  VTK_LEGACY(vtkTexture* GetTexture(int unit));
  VTK_LEGACY(void RemoveTexture(int unit));
  //@}

  /**
   * Remove a texture from the collection.
   */
  void RemoveTexture(const char* name);

  /**
   * Remove all the textures.
   */
  void RemoveAllTextures();

  /**
   * Returns the number of textures in this property.
   */
  int GetNumberOfTextures();

  /**
   * Returns all the textures in this property and their names
   */
  std::map<std::string, vtkTexture *> &GetAllTextures() {
    return this->Textures; }

  /**
   * Release any graphics resources that are being consumed by this
   * property. The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *win);


#ifndef VTK_LEGACY_REMOVE
  // deprecated. Textures should use names not units
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
#endif

  //@{
  /**
   * Set/Get the information object associated with the Property.
   */
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);
  //@}

protected:
  vtkProperty();
  ~vtkProperty() override;

  /**
   * Computes composite color. Used by GetColor().
   */
  static void ComputeCompositeColor(double result[3],
    double ambient, const double ambient_color[3],
    double diffuse, const double diffuse_color[3],
    double specular, const double specular_color[3]);

  double Color[3];
  double AmbientColor[3];
  double DiffuseColor[3];
  double SpecularColor[3];
  double EdgeColor[3];
  double VertexColor[3];
  double Ambient;
  double Diffuse;
  double Specular;
  double SpecularPower;
  double Opacity;
  float PointSize;
  float LineWidth;
  int LineStipplePattern;
  int LineStippleRepeatFactor;
  int Interpolation;
  int Representation;
  vtkTypeBool EdgeVisibility;
  vtkTypeBool VertexVisibility;
  vtkTypeBool BackfaceCulling;
  vtkTypeBool FrontfaceCulling;
  bool Lighting;
  bool RenderPointsAsSpheres;
  bool RenderLinesAsTubes;

  vtkTypeBool Shading;

  char* MaterialName;

  typedef std::map<std::string, vtkTexture*> MapOfTextures;
  MapOfTextures Textures;

  // Arbitrary extra information associated with this Property.
  vtkInformation* Information;

private:
  vtkProperty(const vtkProperty&) = delete;
  void operator=(const vtkProperty&) = delete;
};

//@{
/**
 * Return the method of shading as a descriptive character string.
 */
inline const char *vtkProperty::GetInterpolationAsString(void)
{
  if (this->Interpolation == VTK_FLAT)
  {
    return "Flat";
  }
  else if (this->Interpolation == VTK_GOURAUD)
  {
    return "Gouraud";
  }
  else
  {
    return "Phong";
  }
}
//@}

//@{
/**
 * Return the method of shading as a descriptive character string.
 */
inline const char *vtkProperty::GetRepresentationAsString(void)
{
  if (this->Representation == VTK_POINTS)
  {
    return "Points";
  }
  else if (this->Representation == VTK_WIREFRAME)
  {
    return "Wireframe";
  }
  else
  {
    return "Surface";
  }
}
//@}

#endif
