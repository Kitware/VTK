// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUImageSliceMapper
 * @brief   Draws an image slice as a textured quad with WebGPU.
 *
 * Without this class a vtkImageSlice - a vtkImageActor, for instance - created
 * while the WebGPU backend is preferred still gets the OpenGL slice mapper,
 * because that is the only override the factory knows about. That mapper then
 * issues GL calls into a window that has no GL context, which crashes.
 *
 * The slice itself is backend independent: vtkImageMapper3D already turns the
 * image, the property's colour window/level and its lookup table into texture
 * data, and computes the quad and its texture coordinates. This class does that
 * work and hands the result to an ordinary actor, mapper and texture, which the
 * factory resolves to their WebGPU implementations.
 */

#ifndef vtkWebGPUImageSliceMapper_h
#define vtkWebGPUImageSliceMapper_h

#include "vtkImageSliceMapper.h"
#include "vtkNew.h"                   // for ivars
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkImageSlice;
class vtkOverrideAttribute;
class vtkPolyData;
class vtkRenderer;
class vtkTexture;
class vtkWindow;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUImageSliceMapper
  : public vtkImageSliceMapper
{
public:
  vtkTypeMacro(vtkWebGPUImageSliceMapper, vtkImageSliceMapper);
  static vtkWebGPUImageSliceMapper* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Draw the slice. Called by vtkImageSlice.
   */
  void Render(vtkRenderer* renderer, vtkImageSlice* prop) override;

  /**
   * Release any graphics resources held by the proxy actor and its texture.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkWebGPUImageSliceMapper();
  ~vtkWebGPUImageSliceMapper() override;

  /**
   * Rebuild the quad and the texture that the proxy actor draws.
   */
  void BuildTexturedQuad(vtkImageSlice* prop);

  vtkNew<vtkActor> ProxyActor;
  vtkNew<vtkPolyData> Quad;
  vtkNew<vtkTexture> Texture;

  // The quad and the texture are rebuilt when the input, the displayed extent
  // or the image property changes.
  vtkTimeStamp BuildTime;
  int BuiltExtent[6] = { 0, -1, 0, -1, 0, -1 };
  // What the texture currently holds. MakeTextureData reads these to decide
  // whether it can reuse the existing texture and the input's own memory.
  int TextureSize[2] = { 0, 0 };
  int TextureBytesPerPixel = 4;

private:
  vtkWebGPUImageSliceMapper(const vtkWebGPUImageSliceMapper&) = delete;
  void operator=(const vtkWebGPUImageSliceMapper&) = delete;
};
#define vtkWebGPUImageSliceMapper_OVERRIDE_ATTRIBUTES                                              \
  vtkWebGPUImageSliceMapper::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
