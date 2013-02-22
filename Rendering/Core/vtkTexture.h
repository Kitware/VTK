/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTexture - handles properties associated with a texture map
// .SECTION Description
// vtkTexture is an object that handles loading and binding of texture
// maps. It obtains its data from an input image data dataset type.
// Thus you can create visualization pipelines to read, process, and
// construct textures. Note that textures will only work if texture
// coordinates are also defined, and if the rendering system supports
// texture.
//
// Instances of vtkTexture are associated with actors via the actor's
// SetTexture() method. Actors can share texture maps (this is encouraged
// to save memory resources.)

// .SECTION Caveats
// Currently only 2D texture maps are supported, even though the data pipeline
// supports 1,2, and 3D texture coordinates.
//
// Some renderers such as old OpenGL require that the texture map dimensions
// are a power of two in each direction. If a non-power of two texture map is
// used, it is automatically resampled to a power of two in one or more
// directions, at the cost of an expensive computation. If the OpenGL
// implementation is recent enough (OpenGL>=2.0 or
// extension GL_ARB_texture_non_power_of_two exists) there is no such
// restriction and no extra computational cost.
// .SECTION See Also
// vtkActor vtkRenderer vtkOpenGLTexture

#ifndef __vtkTexture_h
#define __vtkTexture_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkImageData;
class vtkScalarsToColors;
class vtkRenderer;
class vtkUnsignedCharArray;
class vtkWindow;
class vtkDataArray;
class vtkTransform;

#define VTK_TEXTURE_QUALITY_DEFAULT 0
#define VTK_TEXTURE_QUALITY_16BIT   16
#define VTK_TEXTURE_QUALITY_32BIT   32

class VTKRENDERINGCORE_EXPORT vtkTexture : public vtkImageAlgorithm
{
public:
  static vtkTexture* New();
  vtkTypeMacro(vtkTexture, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Renders a texture map. It first checks the object's modified time
  // to make sure the texture maps Input is valid, then it invokes the
  // Load() method.
  virtual void Render(vtkRenderer* ren);

  // Description:
  // Cleans up after the texture rendering to restore the state of the
  // graphics context.
  virtual void PostRender(vtkRenderer*) {}

  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow*) {}

  // Description:
  // Abstract interface to renderer. Each concrete subclass of
  // vtkTexture will load its data into graphics system in response
  // to this method invocation.
  virtual void Load(vtkRenderer*) {}

  // Description:
  // Turn on/off the repetition of the texture map when the texture
  // coords extend beyond the [0,1] range.
  vtkGetMacro(Repeat, int);
  vtkSetMacro(Repeat, int);
  vtkBooleanMacro(Repeat, int);

  // Description:
  // Turn on/off the clamping of the texture map when the texture
  // coords extend beyond the [0,1] range.
  // Only used when Repeat is off, and edge clamping is supported by
  // the graphics card.
  vtkGetMacro(EdgeClamp, int);
  vtkSetMacro(EdgeClamp, int);
  vtkBooleanMacro(EdgeClamp, int);

  // Description:
  // Turn on/off linear interpolation of the texture map when rendering.
  vtkGetMacro(Interpolate, int);
  vtkSetMacro(Interpolate, int);
  vtkBooleanMacro(Interpolate, int);

  // Description:
  // Force texture quality to 16-bit or 32-bit.
  // This might not be supported on all machines.
  vtkSetMacro(Quality, int);
  vtkGetMacro(Quality, int);
  void SetQualityToDefault()
    { this->SetQuality(VTK_TEXTURE_QUALITY_DEFAULT); }
  void SetQualityTo16Bit()
    { this->SetQuality(VTK_TEXTURE_QUALITY_16BIT); }
  void SetQualityTo32Bit()
    { this->SetQuality(VTK_TEXTURE_QUALITY_32BIT); }

  // Description:
  // Turn on/off the mapping of color scalars through the lookup table.
  // The default is Off. If Off, unsigned char scalars will be used
  // directly as texture. If On, scalars will be mapped through the
  // lookup table to generate 4-component unsigned char scalars.
  // This ivar does not affect other scalars like unsigned short, float,
  // etc. These scalars are always mapped through lookup tables.
  vtkGetMacro(MapColorScalarsThroughLookupTable, int);
  vtkSetMacro(MapColorScalarsThroughLookupTable, int);
  vtkBooleanMacro(MapColorScalarsThroughLookupTable, int);

//BTX
  // Description:
  // Get the input as a vtkImageData object.  This method is for
  // backwards compatibility.
  vtkImageData* GetInput();
//ETX

  // Description:
  // Specify the lookup table to convert scalars if necessary
  void SetLookupTable(vtkScalarsToColors *);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);

  // Description:
  // Get Mapped Scalars
  vtkGetObjectMacro(MappedScalars, vtkUnsignedCharArray);

  // Description:
  // Map scalar values into color scalars.
  unsigned char* MapScalarsToColors(vtkDataArray* scalars);

  // Description:
  // Set a transform on the texture which allows one to scale,
  // rotate and translate the texture.
  void SetTransform(vtkTransform *transform);
  vtkGetObjectMacro(Transform, vtkTransform);

//BTX
  // Description:
  // Used to specify how the texture will blend its RGB and Alpha values
  // with other textures and the fragment the texture is rendered upon.
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
//ETX

  // Description:
  // Used to specify how the texture will blend its RGB and Alpha values
  // with other textures and the fragment the texture is rendered upon.
  vtkGetMacro(BlendingMode, int);
  vtkSetMacro(BlendingMode, int);

  // Description:
  // Whether the texture colors are premultiplied by alpha.
  // Initial value is false.
  vtkGetMacro(PremultipliedAlpha, bool);
  vtkSetMacro(PremultipliedAlpha, bool);
  vtkBooleanMacro(PremultipliedAlpha, bool);

  // Description:
  // When the texture is forced to be a power of 2, the default behavior is
  // for the "new" image's dimensions  to be greater than or equal to with
  // respects to the original.  Setting RestrictPowerOf2ImageSmaller to be
  // 1 (or ON) with force the new image's dimensions to be less than or equal
  // to with respects to the original.
  vtkGetMacro(RestrictPowerOf2ImageSmaller, int);
  vtkSetMacro(RestrictPowerOf2ImageSmaller, int);
  vtkBooleanMacro(RestrictPowerOf2ImageSmaller, int);

  // Description:
  // Is this Texture Translucent?
  // returns false (0) if the texture is either fully opaque or has
  // only fully transparent pixels and fully opaque pixels and the
  // Interpolate flag is turn off.
  virtual int IsTranslucent();

protected:
  vtkTexture();
  ~vtkTexture();

  int Repeat;
  int EdgeClamp;
  int Interpolate;
  int Quality;
  int MapColorScalarsThroughLookupTable;
  vtkScalarsToColors* LookupTable;
  vtkUnsignedCharArray* MappedScalars;
  vtkTransform * Transform;

  int BlendingMode;
  int RestrictPowerOf2ImageSmaller;
  // this is to duplicated the previous behavior of SelfCreatedLookUpTable
  int SelfAdjustingTableRange;
  bool PremultipliedAlpha;

  // the result of HasTranslucentPolygonalGeometry is cached
  vtkTimeStamp TranslucentComputationTime;
  int TranslucentCachedResult;

private:
  vtkTexture(const vtkTexture&);  // Not implemented.
  void operator=(const vtkTexture&);  // Not implemented.
};

#endif
