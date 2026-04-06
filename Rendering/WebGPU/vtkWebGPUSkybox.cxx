// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUSkybox.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderPipelineCache.h"
#include "vtkWebGPURenderTextureDeviceResource.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWebGPUTexture.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGPUSkybox);

//------------------------------------------------------------------------------
vtkWebGPUSkybox::vtkWebGPUSkybox()
{
  // Create a quad mapper so that vtkActor::RenderOpaqueGeometry doesn't bail
  // out early. The skybox manages its own pipeline and ignores this mapper.
  vtkNew<vtkPolyData> poly;
  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(4);
  pts->SetPoint(0, -1, -1, 0);
  pts->SetPoint(1, 1, -1, 0);
  pts->SetPoint(2, 1, 1, 0);
  pts->SetPoint(3, -1, 1, 0);
  poly->SetPoints(pts);
  vtkNew<vtkCellArray> polys;
  poly->SetPolys(polys);
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);

  vtkNew<vtkPolyDataMapper> quadMapper;
  quadMapper->SetInputData(poly);
  this->SetMapper(quadMapper);

  this->GetProperty()->SetDiffuse(0.0);
  this->GetProperty()->SetAmbient(1.0);
  this->GetProperty()->SetSpecular(0.0);
}

//------------------------------------------------------------------------------
vtkWebGPUSkybox::~vtkWebGPUSkybox() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUSkybox::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebGPUSkybox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastProjection: " << this->LastProjection << "\n";
  os << indent << "LastGammaCorrect: " << this->LastGammaCorrect << "\n";
}

//------------------------------------------------------------------------------
std::string vtkWebGPUSkybox::BuildShaderSource()
{
  std::ostringstream ss;

  // Uniform struct and resource bindings
  ss << R"(
struct SkyboxUniforms {
  camera_pos: vec4<f32>,
  floor_plane: vec4<f32>,
  floor_right: vec4<f32>,
  floor_front: vec4<f32>,
  floor_tcoord_scale: vec2<f32>,
  left_eye: f32,
  _padding: f32,
  // mat3x3 stored as 3 x vec4 (padded columns)
  rotation_col0: vec4<f32>,
  rotation_col1: vec4<f32>,
  rotation_col2: vec4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: SkyboxUniforms;
@group(0) @binding(1) var skybox_sampler: sampler;
)";

  if (this->Projection == vtkSkybox::Cube)
  {
    ss << "@group(0) @binding(2) var skybox_texture: texture_cube<f32>;\n";
  }
  else
  {
    ss << "@group(0) @binding(2) var skybox_texture: texture_2d<f32>;\n";
  }

  // Vertex shader: fullscreen quad using vertex_index
  ss << R"(
struct VertexOutput {
  @builtin(position) position: vec4<f32>,
  @location(0) tex_coords: vec3<f32>,
}

@group(1) @binding(0) var<uniform> inv_mcdc: mat4x4<f32>;

@vertex
fn vertexMain(@builtin(vertex_index) vertex_id: u32) -> VertexOutput {
  var coords = array<vec2<f32>, 4>(
    vec2<f32>(-1.0, -1.0),
    vec2<f32>( 1.0, -1.0),
    vec2<f32>(-1.0,  1.0),
    vec2<f32>( 1.0,  1.0)
  );

  var output: VertexOutput;
  let clip_pos = vec4<f32>(coords[vertex_id], 1.0, 1.0);
  output.position = clip_pos;
  let world_pos = inv_mcdc * clip_pos;
  output.tex_coords = world_pos.xyz / world_pos.w;
  return output;
}

const PI: f32 = 3.14159265358979323846;

fn get_rotation_matrix() -> mat3x3<f32> {
  return mat3x3<f32>(
    uniforms.rotation_col0.xyz,
    uniforms.rotation_col1.xyz,
    uniforms.rotation_col2.xyz
  );
}

struct FragmentOutput {
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>,
}
)";

  // Fragment shader per projection mode
  if (this->Projection == vtkSkybox::Cube)
  {
    ss << R"(
@fragment
fn fragmentMain(@location(0) tex_coords: vec3<f32>) -> FragmentOutput {
  var output: FragmentOutput;
  let dir_i = normalize(tex_coords - uniforms.camera_pos.xyz);
  let rot = get_rotation_matrix();
  var dir_v = rot * dir_i;
  // Negate Z because forward axis points to -Z instead of +Z for cube maps
  dir_v.z = -dir_v.z;
  var color = textureSampleLevel(skybox_texture, skybox_sampler, dir_v, 0.0);
)";
  }
  else if (this->Projection == vtkSkybox::Sphere)
  {
    ss << R"(
@fragment
fn fragmentMain(@location(0) tex_coords: vec3<f32>) -> FragmentOutput {
  var output: FragmentOutput;
  let dir_i = normalize(tex_coords - uniforms.camera_pos.xyz);
  let rot = get_rotation_matrix();
  let dir_v = rot * dir_i;
  let phi_x = length(vec2<f32>(dir_v.x, dir_v.z));
  let u = 0.5 * atan2(dir_v.z, dir_v.x) / PI + 0.5;
  let v = atan2(dir_v.y, phi_x) / PI + 0.5;
  var color = textureSampleLevel(skybox_texture, skybox_sampler, vec2<f32>(u, v), 0.0);
)";
  }
  else if (this->Projection == vtkSkybox::StereoSphere)
  {
    ss << R"(
@fragment
fn fragmentMain(@location(0) tex_coords: vec3<f32>) -> FragmentOutput {
  var output: FragmentOutput;
  let dir_i = normalize(tex_coords - uniforms.camera_pos.xyz);
  let rot = get_rotation_matrix();
  let dir_v = rot * dir_i;
  let phi_x = length(vec2<f32>(dir_v.x, dir_v.z));
  let u = 0.5 * atan2(dir_v.z, dir_v.x) / PI + 0.5;
  let v = 0.5 * atan2(dir_v.y, phi_x) / PI + 0.25 + 0.5 * uniforms.left_eye;
  var color = textureSampleLevel(skybox_texture, skybox_sampler, vec2<f32>(u, v), 0.0);
)";
  }
  else if (this->Projection == vtkSkybox::Floor)
  {
    ss << R"(
@fragment
fn fragmentMain(@builtin(position) frag_position: vec4<f32>, @location(0) tex_coords: vec3<f32>) -> FragmentOutput {
  var output: FragmentOutput;
  let dir_v = normalize(tex_coords - uniforms.camera_pos.xyz);
  let den = dot(uniforms.floor_plane.xyz, dir_v);
  if (abs(den) < 0.0001) {
    discard;
  }
  let p0 = -1.0 * uniforms.floor_plane.w * uniforms.floor_plane.xyz;
  let p0l0 = p0 - uniforms.camera_pos.xyz;
  let t = dot(p0l0, uniforms.floor_plane.xyz) / den;
  if (t < 0.0) {
    discard;
  }
  let pos = dir_v * t - p0l0;
  let u = dot(uniforms.floor_right.xyz, pos) / uniforms.floor_tcoord_scale.x;
  let v = dot(uniforms.floor_front.xyz, pos) / uniforms.floor_tcoord_scale.y;
  var color = textureSample(skybox_texture, skybox_sampler, vec2<f32>(u, v));
)";
  }

  // Gamma correction
  if (this->GammaCorrect)
  {
    ss << "  output.color = vec4<f32>(pow(color.rgb, vec3<f32>(1.0 / 2.2)), color.a);\n";
  }
  else
  {
    ss << "  output.color = color;\n";
  }

  // Floor projection: fade near horizon
  if (this->Projection == vtkSkybox::Floor)
  {
    ss << "  output.color.a *= 50.0 * min(0.02, abs(den));\n";
  }

  ss << "  return output;\n}\n";

  return ss.str();
}

//------------------------------------------------------------------------------
void vtkWebGPUSkybox::CreatePipeline(vtkWebGPURenderWindow* renWin)
{
  auto* wgpuPipelineCache = renWin->GetWGPUPipelineCache();
  auto* wgpuConfiguration = renWin->GetWGPUConfiguration();
  const auto& device = wgpuConfiguration->GetDevice();

  // Group 0: skybox uniforms + sampler + texture
  std::vector<wgpu::BindGroupLayoutEntry> skyboxBGLEntries;
  skyboxBGLEntries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
    0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform });
  skyboxBGLEntries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
    1, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::Filtering });
  if (this->Projection == vtkSkybox::Cube)
  {
    skyboxBGLEntries.emplace_back(
      vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{ 2,
        wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float,
        wgpu::TextureViewDimension::Cube, false });
  }
  else
  {
    skyboxBGLEntries.emplace_back(
      vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{ 2,
        wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float,
        wgpu::TextureViewDimension::e2D, false });
  }
  this->BindGroupLayout = vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(
    device, skyboxBGLEntries, "SkyboxBindGroupLayout");

  // Group 1: inverse MCDC matrix
  std::vector<wgpu::BindGroupLayoutEntry> matrixBGLEntries;
  matrixBGLEntries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
    0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform });
  this->MatrixBindGroupLayout = vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(
    device, matrixBGLEntries, "SkyboxMatrixBindGroupLayout");

  // Pipeline layout
  wgpu::BindGroupLayout layouts[2] = { this->BindGroupLayout, this->MatrixBindGroupLayout };
  wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor;
  pipelineLayoutDescriptor.bindGroupLayoutCount = 2;
  pipelineLayoutDescriptor.bindGroupLayouts = layouts;
  pipelineLayoutDescriptor.label = "SkyboxPipelineLayout";
  auto pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDescriptor);

  std::string shaderSource = this->BuildShaderSource();

  vtkWebGPURenderPipelineDescriptorInternals pipelineDescriptor;
  pipelineDescriptor.layout = pipelineLayout;
  pipelineDescriptor.vertex.entryPoint = "vertexMain";
  pipelineDescriptor.vertex.bufferCount = 0;
  pipelineDescriptor.cFragment.entryPoint = "fragmentMain";
  pipelineDescriptor.cTargets[0].format = renWin->GetPreferredSurfaceTextureFormat();
  // Prepare selection ids output.
  pipelineDescriptor.cTargets[1].format = renWin->GetPreferredSelectorIdsTextureFormat();
  pipelineDescriptor.cFragment.targetCount++;
  pipelineDescriptor.DisableBlending(1);

  auto depthState = pipelineDescriptor.EnableDepthStencil(renWin->GetDepthStencilFormat());
  depthState->depthWriteEnabled = false;
  depthState->depthCompare = wgpu::CompareFunction::LessEqual;

  pipelineDescriptor.primitive.frontFace = wgpu::FrontFace::CCW;
  pipelineDescriptor.primitive.cullMode = wgpu::CullMode::None;
  pipelineDescriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;

  if (this->Projection == vtkSkybox::Floor)
  {
    auto* blendState = pipelineDescriptor.EnableBlending(0);
    blendState->color.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blendState->color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blendState->alpha.srcFactor = wgpu::BlendFactor::One;
    blendState->alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
  }

  this->PipelineKey = wgpuPipelineCache->GetPipelineKey(&pipelineDescriptor, shaderSource.c_str());
  wgpuPipelineCache->CreateRenderPipeline(&pipelineDescriptor, renWin, shaderSource.c_str());
  this->Pipeline = wgpuPipelineCache->GetRenderPipeline(this->PipelineKey);

  this->LastProjection = this->Projection;
  this->LastGammaCorrect = this->GammaCorrect;
}

//------------------------------------------------------------------------------
void vtkWebGPUSkybox::CreateBindGroup(vtkWebGPUConfiguration* wgpuConfiguration)
{
  const auto& device = wgpuConfiguration->GetDevice();

  // Create uniform buffer
  const auto uniformSize = vtkWebGPUConfiguration::Align(sizeof(SkyboxUniforms), 16);
  this->UniformBuffer = wgpuConfiguration->CreateBuffer(uniformSize,
    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false, "SkyboxUniformBuffer");

  std::vector<wgpu::BindGroupEntry> bgEntries;
  auto uniformBinding = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ 0,
    this->UniformBuffer, 0, uniformSize };
  bgEntries.emplace_back(uniformBinding.GetAsBinding());

  if (auto* wgpuTexture = vtkWebGPUTexture::SafeDownCast(this->GetTexture()))
  {
    if (auto devRc = wgpuTexture->GetDeviceResource())
    {
      bgEntries.emplace_back(devRc->MakeSamplerBindGroupEntry(1));
      bgEntries.emplace_back(devRc->MakeTextureViewBindGroupEntry(2));
    }
  }

  this->BindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(
    device, this->BindGroupLayout, bgEntries, "SkyboxBindGroup");

  // Create matrix buffer and bind group for group 1
  const auto matrixBufferSize = vtkWebGPUConfiguration::Align(16 * sizeof(float), 16);
  this->MatrixBuffer = wgpuConfiguration->CreateBuffer(matrixBufferSize,
    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false, "SkyboxMatrixBuffer");

  std::vector<wgpu::BindGroupEntry> matBGEntries;
  auto matBinding = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ 0, this->MatrixBuffer,
    0, matrixBufferSize };
  matBGEntries.emplace_back(matBinding.GetAsBinding());
  this->MatrixBindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(
    device, this->MatrixBindGroupLayout, matBGEntries, "SkyboxMatrixBindGroup");
}

//------------------------------------------------------------------------------
void vtkWebGPUSkybox::UpdateUniformBuffer(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkRenderer* ren)
{
  SkyboxUniforms uniforms{};

  double* pos = ren->GetActiveCamera()->GetPosition();
  uniforms.CameraPosition[0] = static_cast<float>(pos[0]);
  uniforms.CameraPosition[1] = static_cast<float>(pos[1]);
  uniforms.CameraPosition[2] = static_cast<float>(pos[2]);
  uniforms.CameraPosition[3] = 1.0f;

  double norm = vtkMath::Norm(this->FloorPlane, 3);
  uniforms.FloorPlane[0] = this->FloorPlane[0] / norm;
  uniforms.FloorPlane[1] = this->FloorPlane[1] / norm;
  uniforms.FloorPlane[2] = this->FloorPlane[2] / norm;
  uniforms.FloorPlane[3] = this->FloorPlane[3] / norm;

  uniforms.FloorRight[0] = this->FloorRight[0];
  uniforms.FloorRight[1] = this->FloorRight[1];
  uniforms.FloorRight[2] = this->FloorRight[2];
  uniforms.FloorRight[3] = 0.0f;

  float front[3];
  vtkMath::Cross(uniforms.FloorPlane, this->FloorRight, front);
  uniforms.FloorFront[0] = front[0];
  uniforms.FloorFront[1] = front[1];
  uniforms.FloorFront[2] = front[2];
  uniforms.FloorFront[3] = 0.0f;

  uniforms.FloorTexCoordScale[0] = this->FloorTexCoordScale[0];
  uniforms.FloorTexCoordScale[1] = this->FloorTexCoordScale[1];

  uniforms.LeftEye = ren->GetActiveCamera()->GetLeftEye() ? 1.0f : 0.0f;
  uniforms.Padding = 0.0f;

  // Rotation matrix: VTK stores row-major data. OpenGL sends this directly to
  // glUniformMatrix3fv without transpose, so GLSL interprets rows as columns (i.e. the
  // matrix is effectively transposed). Match that behavior by storing rows as WGSL columns.
  double* rotData = this->RotationMatrix->GetData();
  // WGSL column 0 = VTK row 0
  uniforms.RotationMatrix[0] = static_cast<float>(rotData[0]);
  uniforms.RotationMatrix[1] = static_cast<float>(rotData[1]);
  uniforms.RotationMatrix[2] = static_cast<float>(rotData[2]);
  uniforms.RotationMatrix[3] = 0.0f;
  // WGSL column 1 = VTK row 1
  uniforms.RotationMatrix[4] = static_cast<float>(rotData[3]);
  uniforms.RotationMatrix[5] = static_cast<float>(rotData[4]);
  uniforms.RotationMatrix[6] = static_cast<float>(rotData[5]);
  uniforms.RotationMatrix[7] = 0.0f;
  // WGSL column 2 = VTK row 2
  uniforms.RotationMatrix[8] = static_cast<float>(rotData[6]);
  uniforms.RotationMatrix[9] = static_cast<float>(rotData[7]);
  uniforms.RotationMatrix[10] = static_cast<float>(rotData[8]);
  uniforms.RotationMatrix[11] = 0.0f;

  wgpuConfiguration->WriteBuffer(
    this->UniformBuffer, 0, &uniforms, sizeof(SkyboxUniforms), "SkyboxUniformUpdate");

  // Update inverse MCDC matrix
  auto* camera = ren->GetActiveCamera();
  vtkNew<vtkMatrix4x4> mcdc;
  mcdc->DeepCopy(camera->GetCompositeProjectionTransformMatrix(ren->GetTiledAspectRatio(), -1, +1));
  mcdc->Invert();
  mcdc->Transpose();

  float matrixData[16];
  for (int i = 0; i < 16; ++i)
  {
    matrixData[i] = static_cast<float>(mcdc->GetData()[i]);
  }
  wgpuConfiguration->WriteBuffer(
    this->MatrixBuffer, 0, matrixData, sizeof(matrixData), "SkyboxMatrixUpdate");
}

//------------------------------------------------------------------------------
void vtkWebGPUSkybox::Render(vtkRenderer* ren, vtkMapper* vtkNotUsed(mapper))
{
  auto* wgpuRen = vtkWebGPURenderer::SafeDownCast(ren);
  if (!wgpuRen)
  {
    vtkErrorMacro("The renderer passed to vtkWebGPUSkybox::Render is not a WebGPU renderer.");
    return;
  }

  auto* renWin = vtkWebGPURenderWindow::SafeDownCast(wgpuRen->GetRenderWindow());
  if (!renWin)
  {
    vtkErrorMacro("RenderWindow is not a vtkWebGPURenderWindow.");
    return;
  }

  if (!this->GetTexture())
  {
    vtkErrorMacro("Skybox requires a texture. Use SetTexture() to provide one.");
    return;
  }

  auto* wgpuConfiguration = renWin->GetWGPUConfiguration();
  auto renderStage = wgpuRen->GetRenderStage();

  if (renderStage == vtkWebGPURenderer::SyncDeviceResources)
  {
    // Get the environment rotation matrix
    this->RotationMatrix->SetData(ren->GetEnvironmentRotationMatrix()->GetData());

    // Note: Texture is already loaded by vtkActor::RenderOpaqueGeometry
    // before this method is called.

    // Recreate pipeline if projection/gamma changed
    if (this->LastProjection != this->Projection || this->LastGammaCorrect != this->GammaCorrect ||
      !this->Pipeline)
    {
      this->CreatePipeline(renWin);
      // Force bind group recreation since pipeline layout changed
      this->BindGroup = nullptr;
    }

    // Recreate bind group if texture changed or not yet created
    bool needBindGroup = !this->BindGroup;
    if (auto* wgpuTexture = vtkWebGPUTexture::SafeDownCast(this->GetTexture()))
    {
      if (wgpuTexture->GetMTime() > this->TextureBuildTime)
      {
        needBindGroup = true;
      }
    }
    if (needBindGroup)
    {
      this->CreateBindGroup(wgpuConfiguration);
      this->TextureBuildTime = this->GetTexture()->GetMTime();
    }

    // Update uniform data every frame
    this->UpdateUniformBuffer(wgpuConfiguration, ren);
  }
  else if (renderStage == vtkWebGPURenderer::RecordingCommands)
  {
    if (!this->Pipeline || !this->BindGroup || !this->MatrixBindGroup)
    {
      return;
    }

    auto renderPassEncoder = wgpuRen->GetRenderPassEncoder();
    renderPassEncoder.SetPipeline(this->Pipeline);
    renderPassEncoder.SetBindGroup(0, this->BindGroup);
    renderPassEncoder.SetBindGroup(1, this->MatrixBindGroup);
    renderPassEncoder.Draw(4);
    // Restore the scene bind group so subsequent actors find the correct layout.
    renderPassEncoder.SetBindGroup(0, wgpuRen->GetSceneBindGroup());
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUSkybox::ReleaseGraphicsResources(vtkWindow* vtkNotUsed(window))
{
  this->Pipeline = nullptr;
  this->BindGroupLayout = nullptr;
  this->BindGroup = nullptr;
  this->UniformBuffer = nullptr;
  this->MatrixBuffer = nullptr;
  this->MatrixBindGroupLayout = nullptr;
  this->MatrixBindGroup = nullptr;
  this->PipelineKey.clear();
  this->LastProjection = -1;
  this->LastGammaCorrect = false;
  this->TextureBuildTime = 0;
}
VTK_ABI_NAMESPACE_END
