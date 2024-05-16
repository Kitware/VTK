// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTexture
 * @brief   handles properties associated with a texture map
 *
 * vtkTexture is an image algorithm that handles loading and binding of
 * texture maps. It obtains its data from an input image data dataset type.
 * Thus you can create visualization pipelines to read, process, and
 * construct textures. Note that textures will only work if texture
 * coordinates are also defined, and if the rendering system supports
 * texture.
 *
 * Instances of vtkTexture are associated with actors via the actor's
 * SetTexture() method. Actors can share texture maps (this is encouraged
 * to save memory resources.)
 *
 * @warning
 * Currently only 2D texture maps are supported, even though the data pipeline
 * supports 1,2, and 3D texture coordinates.
 *
 * @warning
 * Some renderers such as old OpenGL require that the texture map dimensions
 * are a power of two in each direction. If a non-power of two texture map is
 * used, it is automatically resampled to a power of two in one or more
 * directions, at the cost of an expensive computation. If the OpenGL
 * implementation is recent enough (OpenGL>=2.0 or
 * extension GL_ARB_texture_non_power_of_two exists) there is no such
 * restriction and no extra computational cost.
 * @sa
 * vtkActor vtkRenderer vtkOpenGLTexture
 */

#ifndef vtkTexture_h
#define vtkTexture_h

#include "vtkImageAlgorithm.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"      // For VTK_COLOR_MODE_*
#include "vtkWrappingHints.h"       // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkScalarsToColors;
class vtkRenderer;
class vtkUnsignedCharArray;
class vtkWindow;
class vtkDataArray;
class vtkTransform;

#define VTK_TEXTURE_QUALITY_DEFAULT 0
#define VTK_TEXTURE_QUALITY_16BIT 16
#define VTK_TEXTURE_QUALITY_32BIT 32

class VTKRENDERINGCORE_EXPORT VTK_MARSHALMANUAL vtkTexture : public vtkImageAlgorithm
{
public:
  static vtkTexture* New();
  vtkTypeMacro(vtkTexture, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Renders a texture map. It first checks the object's modified time
   * to make sure the texture maps Input is valid, then it invokes the
   * Load() method.
   */
  virtual void Render(vtkRenderer* ren);

  /**
   * Cleans up after the texture rendering to restore the state of the
   * graphics context.
   */
  virtual void PostRender(vtkRenderer*) {}

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) {}

  /**
   * Abstract interface to renderer. Each concrete subclass of
   * vtkTexture will load its data into graphics system in response
   * to this method invocation.
   */
  virtual void Load(vtkRenderer*) {}

  ///@{
  /**
   * Turn on/off linear interpolation of the texture map when rendering.
   */
  vtkGetMacro(Interpolate, vtkTypeBool);
  vtkSetMacro(Interpolate, vtkTypeBool);
  vtkBooleanMacro(Interpolate, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off use of mipmaps when rendering.
   */
  vtkGetMacro(Mipmap, bool);
  vtkSetMacro(Mipmap, bool);
  vtkBooleanMacro(Mipmap, bool);
  ///@}

  ///@{
  /**
   * Set/Get the maximum anisotropic filtering to use. 1.0 means use no
   * anisotropic filtering. The default value is 4.0 and a high value would
   * be 16. This setting is only applied when mipmaps are used. This might
   * not be supported on all machines.
   */
  vtkSetMacro(MaximumAnisotropicFiltering, float);
  vtkGetMacro(MaximumAnisotropicFiltering, float);
  ///@}

  ///@{
  /**
   * Force texture quality to 16-bit or 32-bit.
   * This might not be supported on all machines.
   */
  vtkSetMacro(Quality, int);
  vtkGetMacro(Quality, int);
  void SetQualityToDefault() { this->SetQuality(VTK_TEXTURE_QUALITY_DEFAULT); }
  void SetQualityTo16Bit() { this->SetQuality(VTK_TEXTURE_QUALITY_16BIT); }
  void SetQualityTo32Bit() { this->SetQuality(VTK_TEXTURE_QUALITY_32BIT); }
  ///@}

  ///@{
  /**
   * Default: ColorModeToDefault. unsigned char scalars are treated
   * as colors, and NOT mapped through the lookup table (set with SetLookupTable),
   * while other kinds of scalars are. ColorModeToDirectScalar extends
   * ColorModeToDefault such that all integer types are treated as
   * colors with values in the range 0-255 and floating types are
   * treated as colors with values in the range 0.0-1.0. Setting
   * ColorModeToMapScalars means that all scalar data will be mapped
   * through the lookup table.
   */
  vtkSetMacro(ColorMode, int);
  vtkGetMacro(ColorMode, int);
  void SetColorModeToDefault() { this->SetColorMode(VTK_COLOR_MODE_DEFAULT); }
  void SetColorModeToMapScalars() { this->SetColorMode(VTK_COLOR_MODE_MAP_SCALARS); }
  void SetColorModeToDirectScalars() { this->SetColorMode(VTK_COLOR_MODE_DIRECT_SCALARS); }
  ///@}

  /**
   * Get the input as a vtkImageData object.  This method is for
   * backwards compatibility.
   */
  vtkImageData* GetInput();

  ///@{
  /**
   * Specify the lookup table to convert scalars if necessary
   */
  void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);
  ///@}

  ///@{
  /**
   * Get Mapped Scalars
   */
  vtkGetObjectMacro(MappedScalars, vtkUnsignedCharArray);
  ///@}

  /**
   * Map scalar values into color scalars.
   */
  unsigned char* MapScalarsToColors(vtkDataArray* scalars);

  ///@{
  /**
   * Set a transform on the texture which allows one to scale,
   * rotate and translate the texture.
   */
  void SetTransform(vtkTransform* transform);
  vtkGetObjectMacro(Transform, vtkTransform);
  ///@}

  /**
   * Used to specify how the texture will blend its RGB and Alpha values
   * with other textures and the fragment the texture is rendered upon.
   */
  enum VTKTextureBlendingMode
  {
    VTK_TEXTURE_BLENDING_MODE_NONE = 0,
    VTK_TEXTURE_BLENDING_MODE_REPLACE,
    VTK_TEXTURE_BLENDING_MODE_MODULATE,
    VTK_TEXTURE_BLENDING_MODE_ADD,
    VTK_TEXTURE_BLENDING_MODE_ADD_SIGNED,
    VTK_TEXTURE_BLENDING_MODE_INTERPOLATE,
    VTK_TEXTURE_BLENDING_MODE_SUBTRACT
  };

  ///@{
  /**
   * Used to specify how the texture will blend its RGB and Alpha values
   * with other textures and the fragment the texture is rendered upon.
   */
  vtkGetMacro(BlendingMode, int);
  vtkSetMacro(BlendingMode, int);
  ///@}

  ///@{
  /**
   * Whether the texture colors are premultiplied by alpha.
   * Initial value is false.
   */
  vtkGetMacro(PremultipliedAlpha, bool);
  vtkSetMacro(PremultipliedAlpha, bool);
  vtkBooleanMacro(PremultipliedAlpha, bool);
  ///@}

  ///@{
  /**
   * When the texture is forced to be a power of 2, the default behavior is
   * for the "new" image's dimensions to be greater than or equal to with
   * respects to the original.  Setting RestrictPowerOf2ImageSmaller to be
   * 1 (or ON) with force the new image's dimensions to be less than or equal
   * to with respects to the original.
   */
  vtkGetMacro(RestrictPowerOf2ImageSmaller, vtkTypeBool);
  vtkSetMacro(RestrictPowerOf2ImageSmaller, vtkTypeBool);
  vtkBooleanMacro(RestrictPowerOf2ImageSmaller, vtkTypeBool);
  ///@}

  /**
   * Is this Texture Translucent?
   * returns false (0) if the texture is either fully opaque or has
   * only fully transparent pixels and fully opaque pixels and the
   * Interpolate flag is turn off.
   */
  virtual int IsTranslucent();

  /**
   * Return the texture unit used for this texture
   */
  virtual int GetTextureUnit() { return 0; }

  ///@{
  /**
   * Is this texture a cube map, if so it needs 6 inputs
   * one for each side of the cube. You must set this before
   * connecting the inputs. The inputs must all have the same
   * size, data type, and depth
   */
  vtkGetMacro(CubeMap, bool);
  vtkBooleanMacro(CubeMap, bool);
  void SetCubeMap(bool val);
  ///@}

  ///@{
  /**
   * Is this texture using the sRGB color space. If you are using a
   * sRGB framebuffer or window then you probably also want to be
   * using sRGB color textures for proper handling of gamma and
   * associated color mixing.
   */
  vtkGetMacro(UseSRGBColorSpace, bool);
  vtkSetMacro(UseSRGBColorSpace, bool);
  vtkBooleanMacro(UseSRGBColorSpace, bool);
  ///@}

  ///@{
  /**
   * Border Color (RGBA). The values can be any valid float value,
   * if the gpu supports it. Initial value is (0.0f, 0.0f, 0.0f, 0.0f),
   * as in the OpenGL spec.
   *
   * \note
   * This property is ignored for OpenGL ES <= 3.2
   */
  vtkSetVector4Macro(BorderColor, float);
  vtkGetVector4Macro(BorderColor, float);
  ///@}

  enum
  {
    ClampToEdge = 0,
    Repeat,
    MirroredRepeat,
    ClampToBorder,
    NumberOfWrapModes
  };

  ///@{
  /**
   * Wrap mode for the texture coordinates
   * Valid values are:
   * - ClampToEdge
   * - Repeat
   * - MirroredRepeat
   * - ClampToBorder
   * Initial value is Repeat (as in OpenGL spec)
   *
   * \note
   * ClampToBorder is not supported with OpenGL ES <= 3.2.
   * Wrap will default to ClampToEdge if it is set to ClampToBorder in this case.
   */
  vtkGetMacro(Wrap, int);
  vtkSetClampMacro(Wrap, int, ClampToEdge, ClampToBorder);
  ///@}

  ///@{
  /**
   * Convenience functions to maintain backwards compatibility.
   * For new code, use the SetWrap API.
   */
  virtual void SetRepeat(vtkTypeBool r) { this->SetWrap(r ? Repeat : ClampToEdge); }
  virtual vtkTypeBool GetRepeat() { return (this->GetWrap() == Repeat); }
  virtual void RepeatOn() { this->SetRepeat(true); }
  virtual void RepeatOff() { this->SetRepeat(false); }
  virtual void SetEdgeClamp(vtkTypeBool)
  { /* This wasn't doing anything before. */
  }
  virtual vtkTypeBool GetEdgeClamp() { return (this->GetWrap() == ClampToEdge); }
  virtual void EdgeClampOn() { this->SetEdgeClamp(true); }
  virtual void EdgeClampOff() { this->SetEdgeClamp(false); }
  ///@}

protected:
  vtkTexture();
  ~vtkTexture() override;

  // A texture is a sink, so there is no need to do anything.
  // This definition avoids a warning when doing Update() on a vtkTexture object.
  void ExecuteData(vtkDataObject*) override {}

  bool Mipmap;
  float MaximumAnisotropicFiltering;
  int Wrap;
  float BorderColor[4];
  vtkTypeBool Interpolate;
  int Quality;
  int ColorMode;
  vtkScalarsToColors* LookupTable;
  vtkUnsignedCharArray* MappedScalars;
  vtkTransform* Transform;

  int BlendingMode;
  vtkTypeBool RestrictPowerOf2ImageSmaller;
  // this is to duplicated the previous behavior of SelfCreatedLookUpTable
  int SelfAdjustingTableRange;
  bool PremultipliedAlpha;
  bool CubeMap;
  bool UseSRGBColorSpace;

  // the result of HasTranslucentPolygonalGeometry is cached
  vtkTimeStamp TranslucentComputationTime;
  int TranslucentCachedResult;

private:
  vtkTexture(const vtkTexture&) = delete;
  void operator=(const vtkTexture&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
