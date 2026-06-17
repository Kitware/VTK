// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUSkybox
 * @brief   WebGPU Skybox
 *
 * vtkWebGPUSkybox is a concrete implementation of the abstract class vtkSkybox.
 * vtkWebGPUSkybox interfaces to the WebGPU rendering library.
 *
 * It creates a dedicated render pipeline with WGSL shaders and manages its own
 * bind groups for the skybox texture, sampler, and uniform data. Unlike the OpenGL
 * implementation, this class directly manages GPU resources (buffers, bind groups,
 * pipelines) following the explicit resource management style of modern graphics APIs.
 */

#ifndef vtkWebGPUSkybox_h
#define vtkWebGPUSkybox_h

#include "vtkSkybox.h"

#include "vtkNew.h"                   // for ivars
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO
#include "vtk_wgpu.h"                 // for webgpu

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;
class vtkOverrideAttribute;
class vtkWebGPUConfiguration;
class vtkWebGPURenderWindow;
class vtkWebGPURenderer;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUSkybox : public vtkSkybox
{
public:
  static vtkWebGPUSkybox* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUSkybox, vtkSkybox);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual Skybox render method.
   */
  void Render(vtkRenderer* ren, vtkMapper* mapper) override;

  /**
   * Release any graphics resources that are being consumed by this skybox.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkWebGPUSkybox();
  ~vtkWebGPUSkybox() override;

private:
  vtkWebGPUSkybox(const vtkWebGPUSkybox&) = delete;
  void operator=(const vtkWebGPUSkybox&) = delete;

  /**
   * Build the WGSL shader source for the current projection mode.
   */
  std::string BuildShaderSource();

  /**
   * Create or recreate the render pipeline when projection or gamma settings change.
   */
  void CreatePipeline(vtkWebGPURenderWindow* wgpuRenderWindow);

  /**
   * Create or update the bind group for the skybox texture and uniforms.
   */
  void CreateBindGroup(vtkWebGPUConfiguration* wgpuConfiguration);

  /**
   * Upload uniform data to the GPU buffer.
   */
  void UpdateUniformBuffer(vtkWebGPUConfiguration* wgpuConfiguration, vtkRenderer* ren);

  // Uniform block matching the WGSL struct layout
  struct SkyboxUniforms
  {
    float CameraPosition[4];              // vec4 (padded from vec3)
    float FloorPlane[4];                  // vec4
    float FloorRight[4];                  // vec4 (padded from vec3)
    float FloorFront[4];                  // vec4 (padded from vec3)
    float FloorTexCoordScale[2];          // vec2
    float LeftEye;                        // f32
    float ProjectionMode;                 // f32: 0=Cube, 1=Sphere, 2=StereoSphere, 3=Floor
    alignas(16) float RotationMatrix[12]; // mat3x3 in WGSL = 3 x vec4 (each row padded to vec4)
  };

  int LastProjection = -1;
  bool LastGammaCorrect = false;

  wgpu::RenderPipeline Pipeline;
  wgpu::BindGroupLayout BindGroupLayout;
  wgpu::BindGroup BindGroup;
  wgpu::Buffer UniformBuffer;
  wgpu::Buffer MatrixBuffer;
  wgpu::BindGroupLayout MatrixBindGroupLayout;
  wgpu::BindGroup MatrixBindGroup;
  std::string PipelineKey;

  vtkNew<vtkMatrix3x3> RotationMatrix;

  vtkMTimeType TextureBuildTime = 0;
};

#define vtkWebGPUSkybox_OVERRIDE_ATTRIBUTES vtkWebGPUSkybox::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
