// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLImageSliceMapper
 * @brief   OpenGL mapper for image slice display
 *
 * vtkOpenGLImageSliceMapper is a concrete implementation of the abstract
 * class vtkImageSliceMapper that interfaces to the OpenGL library.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 */

#ifndef vtkOpenGLImageSliceMapper_h
#define vtkOpenGLImageSliceMapper_h

#include "vtkImageSliceMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderWindow;
class vtkOpenGLRenderWindow;
class vtkActor;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLImageSliceMapper
  : public vtkImageSliceMapper
{
public:
  static vtkOpenGLImageSliceMapper* New();
  vtkTypeMacro(vtkOpenGLImageSliceMapper, vtkImageSliceMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.  Perform the render.
   */
  void Render(vtkRenderer* ren, vtkImageSlice* prop) override;

  /**
   * Release any graphics resources that are being consumed by this
   * mapper, the image texture in particular. Using the same texture
   * in multiple render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkOpenGLImageSliceMapper();
  ~vtkOpenGLImageSliceMapper() override;

  /**
   * Recursive internal method, will call the non-recursive method
   * as many times as necessary if the texture must be broken up into
   * pieces that are small enough for the GPU to render
   */
  void RecursiveRenderTexturedPolygon(vtkRenderer* ren, vtkImageProperty* property,
    vtkImageData* image, int extent[6], bool recursive);

  /**
   * Non-recursive internal method, generate a single texture
   * and its corresponding geometry.
   */
  void RenderTexturedPolygon(vtkRenderer* ren, vtkImageProperty* property, vtkImageData* image,
    int extent[6], bool recursive);

  /**
   * Basic polygon rendering, if the textured parameter is set the tcoords
   * are included, otherwise they aren't.
   */
  void RenderPolygon(vtkActor* actor, vtkPoints* points, const int extent[6], vtkRenderer* ren);

  /**
   * Render the background, which means rendering everything within the
   * plane of the image except for the polygon that displays the image data.
   */
  void RenderBackground(vtkActor* actor, vtkPoints* points, const int extent[6], vtkRenderer* ren);

  /**
   * Given an extent that describes a slice (it must have unit thickness
   * in one of the three directions), return the dimension indices that
   * correspond to the texture "x" and "y", provide the x, y image size,
   * and provide the texture size (padded to a power of two if the hardware
   * requires).
   */
  void ComputeTextureSize(
    const int extent[6], int& xdim, int& ydim, int imageSize[2], int textureSize[2]) override;

  /**
   * Test whether a given texture size is supported.  This includes a
   * check of whether the texture will fit into texture memory.
   */
  bool TextureSizeOK(const int size[2], vtkRenderer* ren);

  vtkRenderWindow* RenderWindow; // RenderWindow used for previous render
  int TextureSize[2];
  int TextureBytesPerPixel;
  int LastOrientation;
  int LastSliceNumber;

  vtkActor* PolyDataActor;
  vtkActor* BackingPolyDataActor;
  vtkActor* BackgroundPolyDataActor;

  vtkTimeStamp LoadTime;

private:
  vtkOpenGLImageSliceMapper(const vtkOpenGLImageSliceMapper&) = delete;
  void operator=(const vtkOpenGLImageSliceMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
