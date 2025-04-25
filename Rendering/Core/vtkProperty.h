// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkDeprecation.h" // For deprecation
#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkSetGet.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <map>    // used for ivar
#include <string> // used for ivar

// shading models
#define VTK_FLAT 0
#define VTK_GOURAUD 1
#define VTK_PHONG 2
#define VTK_PBR 3

// representation models
#define VTK_POINTS 0
#define VTK_WIREFRAME 1
#define VTK_SURFACE 2

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkInformation;
class vtkRenderer;
class vtkShaderProgram;
class vtkTexture;
class vtkWindow;
class vtkXMLDataElement;
class vtkXMLMaterial;

class vtkPropertyInternals;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with object color, ambient color, diffuse color,
   * specular color, and edge color white; ambient coefficient=0; diffuse
   * coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
   * and surface representation. Backface and frontface culling are off.
   */
  static vtkProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(vtkProperty* p);

  /**
   * This method causes the property to set up whatever is required for
   * its instance variables. This is actually handled by a subclass of
   * vtkProperty, which is created automatically. This
   * method includes the invoking actor as an argument which can
   * be used by property devices that require the actor.
   */
  virtual void Render(vtkActor*, vtkRenderer*);

  /**
   * This method renders the property as a backface property. TwoSidedLighting
   * must be turned off to see any backface properties. Note that only
   * colors and opacity are used for backface properties. Other properties
   * such as Representation, Culling are specified by the Property.
   */
  virtual void BackfaceRender(vtkActor*, vtkRenderer*) {}

  /**
   * This method is called after the actor has been rendered.
   * Don't call this directly. This method cleans up
   * any shaders allocated.
   */
  virtual void PostRender(vtkActor*, vtkRenderer*);

  ///@{
  /**
   * Set/Get lighting flag for an object. Initial value is true.
   */
  vtkGetMacro(Lighting, bool);
  vtkSetMacro(Lighting, bool);
  vtkBooleanMacro(Lighting, bool);
  ///@}

  enum class Point2DShapeType
  {
    Round,
    Square,
  };

  /**
   * Set/Get the 2D shape of points to use when RenderPointsAsSpheres=false.
   * Some graphics implementations may ignore this setting.
   */
  vtkSetEnumMacro(Point2DShape, Point2DShapeType);
  vtkGetEnumMacro(Point2DShape, Point2DShapeType);

  ///@{
  /**
   * Set/Get rendering of points as spheres. The size of the
   * sphere in pixels is controlled by the PointSize
   * attribute. Note that half spheres may be rendered
   * instead of spheres.
   */
  vtkGetMacro(RenderPointsAsSpheres, bool);
  vtkSetMacro(RenderPointsAsSpheres, bool);
  vtkBooleanMacro(RenderPointsAsSpheres, bool);
  ///@}

  ///@{
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
  ///@}

  ///@{
  /**
   * Set the shading interpolation method for an object.
   */
  vtkSetClampMacro(Interpolation, int, VTK_FLAT, VTK_PBR);
  vtkGetMacro(Interpolation, int);
  void SetInterpolationToFlat() { this->SetInterpolation(VTK_FLAT); }
  void SetInterpolationToGouraud() { this->SetInterpolation(VTK_GOURAUD); }
  void SetInterpolationToPhong() { this->SetInterpolation(VTK_PHONG); }
  void SetInterpolationToPBR() { this->SetInterpolation(VTK_PBR); }
  const char* GetInterpolationAsString();
  ///@}

  ///@{
  /**
   * Control the surface geometry representation for the object.
   */
  vtkSetClampMacro(Representation, int, VTK_POINTS, VTK_SURFACE);
  vtkGetMacro(Representation, int);
  void SetRepresentationToPoints() { this->SetRepresentation(VTK_POINTS); }
  void SetRepresentationToWireframe() { this->SetRepresentation(VTK_WIREFRAME); }
  void SetRepresentationToSurface() { this->SetRepresentation(VTK_SURFACE); }
  const char* GetRepresentationAsString();
  ///@}

  ///@{
  /**
   * Set the color of the object. Has the side effect of setting the
   * ambient diffuse and specular colors as well. This is basically
   * a quick overall color setting method.
   */
  virtual void SetColor(double r, double g, double b);
  virtual void SetColor(double a[3]);
  double* GetColor() VTK_SIZEHINT(3);
  void GetColor(double rgb[3]);
  void GetColor(double& r, double& g, double& b);
  ///@}

  ///@{
  /**
   * Set/Get the Index Of Refraction of the base layer.
   * It controls the amount of light reflected at normal incidence (the reflectance F0),
   * depending on the IOR of the upper layer (eg. coat layer, or environment).
   * For example, with a base IOR of 1.5 and an IOR of 1.0 outside (IOR of the air),
   * 4% of the amount of the light is reflected at normal incidence.
   * Notice that modifying this value is only useful for dielectrics materials, as
   * the reflectance for metallic is the albedo.
   * This parameter is only used by PBR Interpolation.
   * Default value is 1.5
   */
  vtkSetClampMacro(BaseIOR, double, 1.0, VTK_FLOAT_MAX);
  vtkGetMacro(BaseIOR, double);
  ///@}

  ///@{
  /**
   * Set/Get the metallic coefficient.
   * Usually this value is either 0 or 1 for real material but any value in between is valid.
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.0
   */
  vtkSetClampMacro(Metallic, double, 0.0, 1.0);
  vtkGetMacro(Metallic, double);
  ///@}

  ///@{
  /**
   * Set/Get the roughness coefficient.
   * This value has to be between 0 (glossy) and 1 (rough).
   * A glossy material has reflections and a high specular part.
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.5
   */
  vtkSetClampMacro(Roughness, double, 0.0, 1.0);
  vtkGetMacro(Roughness, double);
  ///@}

  ///@{
  /**
   * Set/Get the anisotropy coefficient.
   * This value controls the anisotropy of the material (0.0 means isotropic)
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.0
   */
  vtkSetClampMacro(Anisotropy, double, 0.0, 1.0);
  vtkGetMacro(Anisotropy, double);
  ///@}

  ///@{
  /**
   * Set/Get the anisotropy rotation coefficient.
   * This value controls the rotation of the direction of the anisotropy.
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.0
   */
  vtkSetClampMacro(AnisotropyRotation, double, 0.0, 1.0);
  vtkGetMacro(AnisotropyRotation, double);
  ///@}

  ///@{
  /**
   * Set/Get the coat layer Index Of Refraction.
   * This parameter is only used by PBR Interpolation.
   * Default value is 2.0
   */
  vtkSetClampMacro(CoatIOR, double, 1.0, VTK_FLOAT_MAX);
  vtkGetMacro(CoatIOR, double);
  ///@}

  ///@{
  /**
   * Set/Get the coat layer roughness coefficient.
   * This value has to be between 0 (glossy) and 1 (rough).
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.0
   */
  vtkSetClampMacro(CoatRoughness, double, 0.0, 1.0);
  vtkGetMacro(CoatRoughness, double);
  ///@}

  ///@{
  /**
   * Set/Get the coat layer strength coefficient.
   * This value affects the strength of the coat layer reflection.
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.0
   */
  vtkSetClampMacro(CoatStrength, double, 0.0, 1.0);
  vtkGetMacro(CoatStrength, double);
  ///@}

  ///@{
  /**
   * Set/Get the color of the coat layer.
   * This value is only used by PBR Interpolation.
   * Default value is white [1.0, 1.0, 1.0]
   */
  vtkSetVector3Macro(CoatColor, double);
  vtkGetVector3Macro(CoatColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the coat layer normal scale coefficient.
   * This value affects the strength of the normal deviation from the coat normal texture.
   * This parameter is only used by PBR Interpolation.
   * Default value is 1.0
   */
  vtkSetClampMacro(CoatNormalScale, double, 0.0, 1.0);
  vtkGetMacro(CoatNormalScale, double);
  ///@}

  ///@{
  /**
   * Set/Get the normal scale coefficient.
   * This value affects the strength of the normal deviation from the texture.
   * Default value is 1.0
   */
  vtkSetMacro(NormalScale, double);
  vtkGetMacro(NormalScale, double);
  ///@}

  ///@{
  /**
   * Set/Get the occlusion strength coefficient.
   * This value affects the strength of the occlusion if a material texture is present.
   * This parameter is only used by PBR Interpolation.
   * Default value is 1.0
   */
  vtkSetClampMacro(OcclusionStrength, double, 0.0, 1.0);
  vtkGetMacro(OcclusionStrength, double);
  ///@}

  ///@{
  /**
   * Set/Get the emissive factor.
   * This value is multiplied with the emissive color when an emissive texture is present.
   * This parameter is only used by PBR Interpolation.
   * Default value is [1.0, 1.0, 1.0]
   */
  vtkSetVector3Macro(EmissiveFactor, double);
  vtkGetVector3Macro(EmissiveFactor, double);
  ///@}

  ///@{
  /**
   * Set/Get the edge tint (for metals only).
   * Set the color at grazing angle (fresnel reflectance).
   * This parameter is only used by PBR Interpolation.
   * Default value is [1.0, 1.0, 1.0]
   */
  vtkSetVector3Macro(EdgeTint, double);
  vtkGetVector3Macro(EdgeTint, double);
  ///@}

  ///@{
  /**
   * Set/Get the ambient lighting coefficient.
   */
  vtkSetClampMacro(Ambient, double, 0.0, 1.0);
  vtkGetMacro(Ambient, double);
  ///@}

  ///@{
  /**
   * Set/Get the diffuse lighting coefficient.
   */
  vtkSetClampMacro(Diffuse, double, 0.0, 1.0);
  vtkGetMacro(Diffuse, double);
  ///@}

  ///@{
  /**
   * Set/Get the specular lighting coefficient.
   */
  vtkSetClampMacro(Specular, double, 0.0, 1.0);
  vtkGetMacro(Specular, double);
  ///@}

  ///@{
  /**
   * Set/Get the specular power.
   */
  vtkSetClampMacro(SpecularPower, double, 0.0, 128.0);
  vtkGetMacro(SpecularPower, double);
  ///@}

  ///@{
  /**
   * Set/Get the object's opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  vtkSetClampMacro(Opacity, double, 0.0, 1.0);
  vtkGetMacro(Opacity, double);
  ///@}

  ///@{
  /**
   * Set/Get the line opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  vtkSetClampMacro(EdgeOpacity, double, 0.0, 1.0);
  vtkGetMacro(EdgeOpacity, double);
  ///@}

  ///@{
  /**
   * Set/Get the ambient surface color. Not all renderers support separate
   * ambient and diffuse colors. From a physical standpoint it really
   * doesn't make too much sense to have both. For the rendering
   * libraries that don't support both, the diffuse color is used.
   */
  vtkSetVector3Macro(AmbientColor, double);
  vtkGetVector3Macro(AmbientColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the diffuse surface color.
   * For PBR Interpolation, DiffuseColor is used as the base color
   */
  vtkSetVector3Macro(DiffuseColor, double);
  vtkGetVector3Macro(DiffuseColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the specular surface color.
   */
  vtkSetVector3Macro(SpecularColor, double);
  vtkGetVector3Macro(SpecularColor, double);
  ///@}

  ///@{
  /**
   * Turn on/off the visibility of edges. On some renderers it is
   * possible to render the edges of geometric primitives separately
   * from the interior.
   */
  vtkGetMacro(EdgeVisibility, vtkTypeBool);
  vtkSetMacro(EdgeVisibility, vtkTypeBool);
  vtkBooleanMacro(EdgeVisibility, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the color of primitive edges (if edge visibility is enabled).
   */
  vtkSetVector3Macro(EdgeColor, double);
  vtkGetVector3Macro(EdgeColor, double);
  ///@}

  ///@{
  /**
   * Turn on/off the visibility of vertices. On some renderers it is
   * possible to render the vertices of geometric primitives separately
   * from the interior.
   */
  vtkGetMacro(VertexVisibility, vtkTypeBool);
  vtkSetMacro(VertexVisibility, vtkTypeBool);
  vtkBooleanMacro(VertexVisibility, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the color of primitive vertices (if vertex visibility is enabled).
   */
  vtkSetVector3Macro(VertexColor, double);
  vtkGetVector3Macro(VertexColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the RGBA color of selection primitives (if a selection is active on the mapper).
   * Default is red and opaque.
   */
  vtkSetVector4Macro(SelectionColor, double);
  vtkGetVector4Macro(SelectionColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the selection line width.
   * Default is 2.
   */
  vtkSetMacro(SelectionLineWidth, float);
  vtkGetMacro(SelectionLineWidth, float);
  ///@}

  ///@{
  /**
   * Set/Get the selection point size.
   * Default is 2.
   */
  vtkSetMacro(SelectionPointSize, float);
  vtkGetMacro(SelectionPointSize, float);
  ///@}

  ///@{
  /**
   * Set/Get the width of a Line. The width is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  vtkSetClampMacro(LineWidth, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(LineWidth, float);
  ///@}

  ///@{
  /**
   * Set/Get the edge width.
   * Default is 1.
   * @note When UseLineWidthForEdgeThickness is false, this property
   * controls the thickness of edges of cells.
   */
  vtkSetClampMacro(EdgeWidth, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(EdgeWidth, float);
  ///@}

  ///@{
  /**
   * When UseLineWidthForEdgeThickness is true, the thickness of edges in a cell
   * is controlled by `LineWidth` property.
   * When UseLineWidthForEdgeThickness is false, the thickness of edges in a cell
   * is controlled by `EdgeWidth` property.
   * @note Default value is true. Edge width is determined by the value of `LineWidth`
   */
  vtkBooleanMacro(UseLineWidthForEdgeThickness, bool);
  vtkSetMacro(UseLineWidthForEdgeThickness, bool);
  vtkGetMacro(UseLineWidthForEdgeThickness, bool);
  ///@}

  ///@{
  /**
   * Set/Get the stippling pattern of a Line, as a 16-bit binary pattern
   * (1 = pixel on, 0 = pixel off).
   * This is only implemented for OpenGL, not OpenGL2. The default is 0xFFFF.
   */
  vtkSetMacro(LineStipplePattern, int);
  vtkGetMacro(LineStipplePattern, int);
  ///@}

  ///@{
  /**
   * Set/Get the stippling repeat factor of a Line, which specifies how
   * many times each bit in the pattern is to be repeated.
   * This is only implemented for OpenGL, not OpenGL2. The default is 1.
   */
  vtkSetClampMacro(LineStippleRepeatFactor, int, 1, VTK_INT_MAX);
  vtkGetMacro(LineStippleRepeatFactor, int);
  ///@}

  ///@{
  /**
   * Set/Get the diameter of a point. The size is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  vtkSetClampMacro(PointSize, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(PointSize, float);
  ///@}

  ///@{
  /**
   * Turn on/off fast culling of polygons based on orientation of normal
   * with respect to camera. If backface culling is on, polygons facing
   * away from camera are not drawn.
   */
  vtkGetMacro(BackfaceCulling, vtkTypeBool);
  vtkSetMacro(BackfaceCulling, vtkTypeBool);
  vtkBooleanMacro(BackfaceCulling, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off fast culling of polygons based on orientation of normal
   * with respect to camera. If frontface culling is on, polygons facing
   * towards camera are not drawn.
   */
  vtkGetMacro(FrontfaceCulling, vtkTypeBool);
  vtkSetMacro(FrontfaceCulling, vtkTypeBool);
  vtkBooleanMacro(FrontfaceCulling, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Returns the name of the material currently loaded, if any.
   */
  vtkSetStringMacro(MaterialName);
  vtkGetStringMacro(MaterialName);
  ///@}

  ///@{
  /**
   * Enable/Disable shading. When shading is enabled, the
   * Material must be set.
   */
  vtkSetMacro(Shading, vtkTypeBool);
  vtkGetMacro(Shading, vtkTypeBool);
  vtkBooleanMacro(Shading, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Provide values to initialize shader variables.
   * Useful to initialize shader variables that change over time
   * (animation, GUI widgets inputs, etc. )
   * - \p name - hardware name of the uniform variable
   * - \p numVars - number of variables being set
   * - \p x - values
   */
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  virtual void AddShaderVariable(const char*, int, int*)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  virtual void AddShaderVariable(const char*, int, float*)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  virtual void AddShaderVariable(const char*, int, double*)
  { /* noop */
  }
  ///@}

  ///@{
  /**
   * Methods to provide to add shader variables from wrappers.
   */
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, int)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, float)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, double)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, int, int)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, float, float)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, double, double)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, int, int, int)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, float, float, float)
  { /* noop */
  }
  VTK_DEPRECATED_IN_9_4_0("AddShaderVariable is a no-op and will be removed")
  void AddShaderVariable(const char*, double, double, double)
  { /* noop */
  }
  ///@}

  ///@{
  /**
   * Show texture maps when the geometry is backfacing. Texture maps are
   * always shown when frontfacing. By default this is true.
   */
  vtkSetMacro(ShowTexturesOnBackface, bool);
  vtkGetMacro(ShowTexturesOnBackface, bool);
  vtkBooleanMacro(ShowTexturesOnBackface, bool);
  ///@}

  ///@{
  /**
   * Set/Get the texture object to control rendering texture maps. This will
   * be a vtkTexture object. A property does not need to have an associated
   * texture map and multiple properties can share one texture. Textures
   * must be assigned unique names. Note that for texture blending the
   * textures will be rendering is alphabetical order and after any texture
   * defined in the actor.
   * There exists 6 special textures with reserved names: "albedoTex", "materialTex", "normalTex",
   * "emissiveTex", "anisotropyTex" and "coatNormalTex". While these textures can be added with the
   * regular SetTexture method, it is preferred to use the methods SetBaseColorTexture,
   * SetORMTexture, SetNormalTexture, SetEmissiveTexture, SetAnisotropyTexture and SetCoatNormalTex
   * respectively.
   */
  void SetTexture(const char* name, vtkTexture* texture);
  vtkTexture* GetTexture(const char* name);
  ///@}

  /**
   * Set the base color texture. Also called albedo, this texture is only used while rendering
   * with PBR interpolation. This is the color of the object.
   * This texture must be in sRGB color space.
   * @sa SetInterpolationToPBR vtkTexture::UseSRGBColorSpaceOn
   */
  void SetBaseColorTexture(vtkTexture* texture) { this->SetTexture("albedoTex", texture); }

  /**
   * Set the ORM texture. This texture contains three RGB independent components corresponding to
   * the Occlusion value, Roughness value and Metallic value respectively.
   * Each texture value is scaled by the Occlusion strength, roughness coefficient and metallic
   * coefficient.
   * This texture must be in linear color space.
   * This is only used by the PBR shading model.
   * @sa SetInterpolationToPBR SetOcclusionStrength SetMetallic SetRoughness
   */
  void SetORMTexture(vtkTexture* texture) { this->SetTexture("materialTex", texture); }

  /**
   * Set the anisotropy texture. This texture contains two independent components corresponding to
   * the anisotropy value and anisotropy rotation. The last component (blue channel) is discarded.
   * The anisotropy value is scaled by the anisotropy coefficient of the material. The anisotropy
   * rotation rotates the direction of the anisotropy (ie. the tangent) around the normal and is not
   * scaled by the anisotropy rotation coefficient.
   * This texture must be in linear color space.
   * This is only used by the PBR shading model.
   * @sa SetInterpolationToPBR SetAnisotropy
   */
  void SetAnisotropyTexture(vtkTexture* texture) { this->SetTexture("anisotropyTex", texture); }

  /**
   * Set the normal texture. This texture is required for normal mapping. It is valid for both PBR
   * and Phong interpolation.
   * The normal mapping is enabled if this texture is present and both normals and tangents are
   * presents in the vtkPolyData.
   * This texture must be in linear color space.
   * @sa vtkPolyDataTangents SetNormalScale
   */
  void SetNormalTexture(vtkTexture* texture) { this->SetTexture("normalTex", texture); }

  /**
   * Set the emissive texture. When present, this RGB texture provides location and color to the
   * shader where the vtkPolyData should emit light. Emitted light is scaled by EmissiveFactor.
   * This is only supported by PBR interpolation model.
   * This texture must be in sRGB color space.
   * @sa SetInterpolationToPBR SetEmissiveFactor vtkTexture::UseSRGBColorSpaceOn
   */
  void SetEmissiveTexture(vtkTexture* texture) { this->SetTexture("emissiveTex", texture); }

  /**
   * Set the coat normal texture. This texture is required for coat normal mapping.
   * It is valid only for PBR interpolation.
   * The coat normal mapping is enabled if this texture is present and both normals and tangents are
   * presents in the vtkPolyData.
   * This texture must be in linear color space.
   * @sa vtkPolyDataTangents SetCoatNormalScale
   */
  void SetCoatNormalTexture(vtkTexture* texture) { this->SetTexture("coatNormalTex", texture); }

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

  ///@{
  /**
   * Set/get all the textures in this property and their names
   */
  void SetAllTextures(std::map<std::string, vtkTexture*>& textures);
  std::map<std::string, vtkTexture*>& GetAllTextures() { return this->Textures; }
  ///@}

  /**
   * Release any graphics resources that are being consumed by this
   * property. The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow* win);

  ///@{
  /**
   * Set/Get the information object associated with the Property.
   */
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);
  ///@}

  ///@{
  /**
   * For PBR, calculate the reflectance from the refractive index of
   * ingoing and outgoing interfaces.
   */
  static double ComputeReflectanceFromIOR(double IORTo, double IORFrom);
  ///@}

  ///@{
  /**
   * For PBR, calculate the refractive index from the reflectance of the interface
   * and the refractive index of one of both medium.
   */
  static double ComputeIORFromReflectance(double reflectance, double ior);
  ///@}

  ///@{
  /**
   * For PBR, calculate the reflectance of the base layer depending on the presence
   * of a coat layer. If there is no coat layer, the reflectance is the one at the
   * interface environment - base layer. If a coat layer is present, the reflectance
   * is the one at the interface between the base and the coat layer.
   */
  double ComputeReflectanceOfBaseLayer();
  ///@}

protected:
  vtkProperty();
  ~vtkProperty() override;

  /**
   * Computes composite color. Used by GetColor().
   */
  static void ComputeCompositeColor(double result[3], double ambient, const double ambient_color[3],
    double diffuse, const double diffuse_color[3], double specular, const double specular_color[3]);

  double Color[3];
  double AmbientColor[3];
  double DiffuseColor[3];
  double SpecularColor[3];
  double EdgeColor[3];
  double VertexColor[3];
  double SelectionColor[4] = { 1.0, 0.0, 0.0, 1.0 };
  double Ambient;
  double Diffuse;
  double Metallic;
  double Roughness;
  double Anisotropy;
  double AnisotropyRotation;
  double BaseIOR;
  double CoatIOR;
  double CoatColor[3];
  double CoatRoughness;
  double CoatStrength;
  double CoatNormalScale;
  double NormalScale;
  double OcclusionStrength;
  double EmissiveFactor[3];
  double Specular;
  double SpecularPower;
  double Opacity;
  double EdgeOpacity = 1.0;
  double EdgeTint[3];
  float PointSize;
  float LineWidth;
  float EdgeWidth = 1.0;
  float SelectionPointSize = 2.f;
  float SelectionLineWidth = 2.f;
  bool UseLineWidthForEdgeThickness = true;
  int LineStipplePattern;
  int LineStippleRepeatFactor;
  int Interpolation;
  int Representation;
  vtkTypeBool EdgeVisibility;
  vtkTypeBool VertexVisibility;
  vtkTypeBool BackfaceCulling;
  vtkTypeBool FrontfaceCulling;
  bool Lighting;
  Point2DShapeType Point2DShape = Point2DShapeType::Square;
  bool RenderPointsAsSpheres;
  bool RenderLinesAsTubes;
  bool ShowTexturesOnBackface;

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

/**
 * Return the method of shading as a descriptive character string.
 */
inline const char* vtkProperty::GetInterpolationAsString()
{
  if (this->Interpolation == VTK_FLAT)
  {
    return "Flat";
  }
  else if (this->Interpolation == VTK_GOURAUD)
  {
    return "Gouraud";
  }
  else if (this->Interpolation == VTK_PHONG)
  {
    return "Phong";
  }
  else // if (this->Interpolation == VTK_PBR)
  {
    return "Physically based rendering";
  }
}

/**
 * Return the method of shading as a descriptive character string.
 */
inline const char* vtkProperty::GetRepresentationAsString()
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

VTK_ABI_NAMESPACE_END
#endif
