// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUBatchedLabeledDataMapperInternals.h"
#include "vtkWebGPUBatchedLabeledDataMapper.h"

#include "vtkActor.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkStringFormatter.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderPipelineCache.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindow.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUBatchedLabeledDataMapperInternals);

//=============================================================================
// Host-side mirror of the WGSL LabelUniforms2D struct.
// Identical to the 3D version except for the added display_offset and padding
// to keep vp 16-byte aligned.
struct LabelUniforms2D
{
  // Viewport and atlas dimensions
  int32_t atlasDims[2];     // vec2<i32>  offset   0
  int32_t vpDims[2];        // vec2<i32>  offset   8
  int32_t winDims[2];       // vec2<i32>  offset  16
  int32_t anchorCenter[2];  // vec2<i32>  offset  24
  int32_t displayOffset[2]; // vec2<i32>  offset  32
  int32_t padding[2];       // (pad)      offset  40
  float vp[4];              // vec4<f32>  offset  48
  float nvp[4];             // vec4<f32>  offset  64
  // array<vec4<f32>, vtkBatchedLabeledDataMapper::MaxTextProperties>  offset  80
  float backgroundColors[vtkBatchedLabeledDataMapper::MaxTextProperties * 4];
  // array<i32, vtkBatchedLabeledDataMapper::MaxTextProperties>  offset 80 +
  // vtkBatchedLabeledDataMapper::MaxTextProperties*16
  int32_t frameWidths[vtkBatchedLabeledDataMapper::MaxTextProperties];
  // array<i32, vtkBatchedLabeledDataMapper::MaxTextProperties>  offset 80 +
  // vtkBatchedLabeledDataMapper::MaxTextProperties*20
  int32_t maxGlyphHeights[vtkBatchedLabeledDataMapper::MaxTextProperties];
  // array<i32, vtkBatchedLabeledDataMapper::MaxTextProperties>  offset 80 +
  // vtkBatchedLabeledDataMapper::MaxTextProperties*24
  int32_t descenders[vtkBatchedLabeledDataMapper::MaxTextProperties];
  // Total: 80 + vtkBatchedLabeledDataMapper::MaxTextProperties * 28 bytes
};
// 80 bytes of fixed fields + vtkBatchedLabeledDataMapper::MaxTextProperties * (4 floats + 3 ints) *
// 4 bytes each
static_assert(sizeof(LabelUniforms2D) == 80 + vtkBatchedLabeledDataMapper::MaxTextProperties * 28,
  "LabelUniforms2D size mismatch — update when MaxTextProperties changes");

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapperInternals::RenderPiece(vtkRenderer* renderer, vtkActor* actor)
{
  if (!this->Parent)
  {
    vtkErrorMacro(<< "Parent is null!");
    return;
  }

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  if (!wgpuRenderWindow)
  {
    vtkErrorMacro(<< "Render window is not a vtkWebGPURenderWindow");
    return;
  }
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

  if (!this->LabelUniformBuffer)
  {
    this->LabelUniformBuffer = wgpuConfiguration->CreateBuffer(sizeof(LabelUniforms2D),
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
      /*mappedAtCreation=*/false, "LabelUniforms2D");
    this->RebuildGraphicsPipelines = true;
  }

  if (!this->GlyphsSampler)
  {
    wgpu::SamplerDescriptor samplerDesc{};
    samplerDesc.magFilter = wgpu::FilterMode::Nearest;
    samplerDesc.minFilter = wgpu::FilterMode::Nearest;
    samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    this->GlyphsSampler = wgpuConfiguration->GetDevice().CreateSampler(&samplerDesc);
  }

  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  switch (wgpuRenderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::SyncDeviceResources:
      this->UpdateInstanceBuffers(wgpuConfiguration);
      this->UpdateUniformBuffer(renderer, wgpuConfiguration);
      if (this->LabelCountChanged)
      {
        wgpuRenderer->InvalidateBundle();
        this->LabelCountChanged = false;
      }
      break;
    default:
      break;
  }
  this->Superclass::RenderPiece(renderer, actor);
}

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapperInternals::ReleaseGraphicsResources(vtkWindow* window)
{
  this->GlyphsTexture = wgpu::Texture{};
  this->GlyphsTextureView = wgpu::TextureView{};
  this->GlyphsSampler = wgpu::Sampler{};
  this->LabelUniformBuffer = wgpu::Buffer{};
  for (int i = 0; i < NUM_INSTANCE_ATTRIBS; ++i)
  {
    this->InstanceBuffers[i] = wgpu::Buffer{};
    this->InstanceBufferSizes[i] = 0;
  }
  this->RebuildGraphicsPipelines = true;
  this->Superclass::ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
bool vtkWebGPUBatchedLabeledDataMapperInternals::IsPipelineSupported(
  GraphicsPipelineType pipelineType)
{
  return pipelineType == GFX_PIPELINE_POINTS_SHAPED ||
    pipelineType == GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE;
}

//----------------------------------------------------------------------------
vtkWebGPUBatchedLabeledDataMapperInternals::DrawCallArgs
vtkWebGPUBatchedLabeledDataMapperInternals::GetDrawCallArgs(GraphicsPipelineType pipelineType,
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType vtkNotUsed(topologySourceType))
{
  if (pipelineType == GFX_PIPELINE_POINTS_SHAPED ||
    pipelineType == GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE)
  {
    return { /*VertexOffset=*/0, /*VertexCount=*/18, /*InstanceOffset=*/0,
      /*InstanceCount=*/this->NumberOfLabels };
  }
  return {};
}

vtkWebGPUBatchedLabeledDataMapperInternals::DrawCallArgs
vtkWebGPUBatchedLabeledDataMapperInternals::GetDrawCallArgsForDrawingVertices(
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType vtkNotUsed(topologySourceType))
{
  return { /*VertexOffset=*/0, /*VertexCount=*/18, /*InstanceOffset=*/0,
    /*InstanceCount=*/this->NumberOfLabels };
}

//----------------------------------------------------------------------------
std::vector<wgpu::VertexBufferLayout>
vtkWebGPUBatchedLabeledDataMapperInternals::GetVertexBufferLayouts()
{
  std::vector<wgpu::VertexBufferLayout> layouts;

  this->InstanceAttributes[GLYPH_EXTENTS] = { /*nextInChain=*/nullptr,
    /*format=*/wgpu::VertexFormat::Float32x4, /*offset=*/0, /*shaderLocation=*/0 };
  {
    wgpu::VertexBufferLayout l{};
    l.arrayStride = 4 * sizeof(float);
    l.attributeCount = 1;
    l.attributes = &this->InstanceAttributes[GLYPH_EXTENTS];
    l.stepMode = wgpu::VertexStepMode::Instance;
    layouts.emplace_back(l);
  }

  this->InstanceAttributes[COFF_PROPID] = { nullptr, wgpu::VertexFormat::Float32x2, 0, 1 };
  {
    wgpu::VertexBufferLayout l{};
    l.arrayStride = 2 * sizeof(float);
    l.attributeCount = 1;
    l.attributes = &this->InstanceAttributes[COFF_PROPID];
    l.stepMode = wgpu::VertexStepMode::Instance;
    layouts.emplace_back(l);
  }

  this->InstanceAttributes[FRAME_COLORS] = { nullptr, wgpu::VertexFormat::Float32x3, 0, 2 };
  {
    wgpu::VertexBufferLayout l{};
    l.arrayStride = 3 * sizeof(float);
    l.attributeCount = 1;
    l.attributes = &this->InstanceAttributes[FRAME_COLORS];
    l.stepMode = wgpu::VertexStepMode::Instance;
    layouts.emplace_back(l);
  }

  return layouts;
}

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapperInternals::SetVertexBuffers(
  const wgpu::RenderPassEncoder& encoder)
{
  for (int i = 0; i < NUM_INSTANCE_ATTRIBS; ++i)
  {
    if (this->InstanceBuffers[i])
    {
      encoder.SetVertexBuffer(i, this->InstanceBuffers[i]);
    }
  }
}

void vtkWebGPUBatchedLabeledDataMapperInternals::SetVertexBuffers(
  const wgpu::RenderBundleEncoder& encoder)
{
  for (int i = 0; i < NUM_INSTANCE_ATTRIBS; ++i)
  {
    if (this->InstanceBuffers[i])
    {
      encoder.SetVertexBuffer(i, this->InstanceBuffers[i]);
    }
  }
}

//----------------------------------------------------------------------------
std::vector<wgpu::BindGroupLayoutEntry>
vtkWebGPUBatchedLabeledDataMapperInternals::GetMeshBindGroupLayoutEntries()
{
  auto entries = this->Superclass::GetMeshBindGroupLayoutEntries();
  uint32_t bindingId = static_cast<uint32_t>(entries.size());

  entries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
    bindingId++, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
    wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D });
  entries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
    bindingId++, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::NonFiltering });
  entries.emplace_back(
    vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{ bindingId++,
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform });

  return entries;
}

std::vector<wgpu::BindGroupEntry>
vtkWebGPUBatchedLabeledDataMapperInternals::GetMeshBindGroupEntries()
{
  auto entries = this->Superclass::GetMeshBindGroupEntries();
  uint32_t bindingId = static_cast<uint32_t>(entries.size());

  {
    const auto init = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++,
      this->GlyphsTextureView };
    entries.emplace_back(init.GetAsBinding());
  }
  {
    const auto init =
      vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++, this->GlyphsSampler };
    entries.emplace_back(init.GetAsBinding());
  }
  {
    const auto init = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++,
      this->LabelUniformBuffer, 0 };
    entries.emplace_back(init.GetAsBinding());
  }

  return entries;
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceShaderConstantsDef(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss, std::string& vtkNotUsed(fss))
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Constants::Def",
    R"(
  const QUAD_C = array<vec2<f32>, 6>(
    vec2f(0.0, 0.0), vec2f(1.0, 0.0), vec2f(1.0, 1.0),
    vec2f(0.0, 0.0), vec2f(1.0, 1.0), vec2f(0.0, 1.0));
  const UV_C = array<vec2<f32>, 6>(
    vec2f(0.0, 0.0), vec2f(1.0, 0.0), vec2f(1.0, 1.0),
    vec2f(0.0, 0.0), vec2f(1.0, 1.0), vec2f(0.0, 1.0));)",
    /*all=*/true);
}

//===========================================================================
//   * LabelUniforms2D adds display_offset and _padding0
//   * anchor computation applies display_offset after the centering step
//===========================================================================

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceShaderCustomDef(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss, std::string& fss)
{
  static_assert(vtkBatchedLabeledDataMapper::MaxTextProperties % 4 == 0);

  const int maxProps = vtkBatchedLabeledDataMapper::MaxTextProperties;
  const int maxPropsDiv4 = maxProps / 4;
  const std::string code = std::string("struct LabelUniforms2D\n{\n"
                                       "  atlas_dims     : vec2<i32>,\n"
                                       "  vp_dims        : vec2<i32>,\n"
                                       "  win_dims       : vec2<i32>,\n"
                                       "  anchor_center  : vec2<i32>,\n"
                                       "  display_offset : vec2<i32>,\n"
                                       "  _padding0      : vec2<i32>,\n"
                                       "  vp             : vec4<f32>,\n"
                                       "  nvp            : vec4<f32>,\n"
                                       "  background_colors  : array<vec4<f32>, ") +
    vtk::to_string(maxProps) +
    ">,\n"
    "  frame_widths       : array<vec4<i32>, " +
    vtk::to_string(maxPropsDiv4) +
    ">,\n"
    "  max_glyph_heights  : array<vec4<i32>, " +
    vtk::to_string(maxPropsDiv4) +
    ">,\n"
    "  descenders         : array<vec4<i32>, " +
    vtk::to_string(maxPropsDiv4) +
    ">,\n"
    "};";
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Custom::Def", code, /*all=*/false);
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Custom::Def", code, /*all=*/false);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceShaderCustomBindings(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss, std::string& fss)
{
  auto& bindingId = this->NumberOfBindings[GROUP_MESH];
  std::stringstream codeStream;
  codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
             << ") var atlas_texture : texture_2d<f32>;\n";
  codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
             << ") var atlas_sampler : sampler;\n";
  codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
             << ") var<uniform> label_uniforms : LabelUniforms2D;\n";
  const std::string code = codeStream.str();
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Custom::Bindings", code, /*all=*/false);
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Custom::Bindings", code, /*all=*/false);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceVertexShaderInputDef(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexInput::Def",
    R"(struct VertexInput
{
  @builtin(instance_index) instance_id : u32,
  @builtin(vertex_index)   vertex_id   : u32,
  @location(0) glyph_extents : vec4<f32>,
  @location(1) coff_propid   : vec2<f32>,
  @location(2) frame_colors  : vec3<f32>,
};)",
    /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceVertexShaderVertexId(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
    R"(let label_layer  : u32 = vertex.vertex_id / 6u;
  let label_corner : u32 = vertex.vertex_id % 6u;
  let pull_vertex_id : u32 = vertex.instance_id;)",
    /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceVertexShaderPosition(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl",
    R"(
  let pos_x = point_coordinates[3u * vertex.instance_id];
  let pos_y = point_coordinates[3u * vertex.instance_id + 1u];
  let pos_z = point_coordinates[3u * vertex.instance_id + 2u];
  let vertex_MC = vec4f(pos_x, pos_y, pos_z, 1.0);

  let clip      = model_view_projection * vertex_MC;
  let resolution = scene_transform.viewport.zw;
  var anchor    = resolution * (0.5 * clip.xy / clip.w + 0.5);

  let glyph_ext    = vec4i(vertex.glyph_extents + vec4f(0.5));
  let propid       = u32(vertex.coff_propid.y);
  let prop_idx     = propid / 4u;
  let prop_comp    = propid % 4u;
  let frame_width  = label_uniforms.frame_widths     [prop_idx][prop_comp];
  let descender    = label_uniforms.descenders       [prop_idx][prop_comp];
  let bg_height    = label_uniforms.max_glyph_heights[prop_idx][prop_comp];
  let atlas_pad    = 5;
  let glyph_w      = glyph_ext[1] - glyph_ext[0] + 1 - 2 * atlas_pad;
  let glyph_h      = glyph_ext[3] - glyph_ext[2] + 1 - 2 * atlas_pad;

  let aw     = f32(label_uniforms.atlas_dims[0]);
  let ah     = f32(label_uniforms.atlas_dims[1]);
  let tc_min = vec2f(f32(glyph_ext[0] + atlas_pad) / aw, f32(glyph_ext[2] + atlas_pad) / ah);
  let tc_max = vec2f(f32(glyph_ext[1] + 1 - atlas_pad) / aw, f32(glyph_ext[3] + 1 - atlas_pad) / ah);

  anchor.x += round(vertex.coff_propid.x);
  var acenter_x : i32 = 0;
  var acenter_y : i32 = 0;
  if      (label_uniforms.anchor_center[0] < 0) { acenter_x =  (frame_width + 1 + descender); }
  else if (label_uniforms.anchor_center[0] > 0) { acenter_x = -(frame_width + 1 + descender); }
  if      (label_uniforms.anchor_center[1] < 0) { acenter_y =  frame_width; }
  else if (label_uniforms.anchor_center[1] == 0){ acenter_y = -(descender + glyph_h) / 2; }
  else                                           { acenter_y = -(frame_width + descender + glyph_h); }
  anchor = floor(anchor + vec2f(f32(acenter_x), f32(acenter_y)));
  anchor += vec2f(f32(label_uniforms.display_offset[0]), f32(label_uniforms.display_offset[1]));

  let bl0 = anchor;
  let tr0 = anchor + vec2f(f32(glyph_w), f32(glyph_h));

  let bl1 = vec2f(anchor.x - 1.0 - f32(descender), anchor.y);
  let tr1 = vec2f(anchor.x + f32(glyph_w + descender), anchor.y + f32(bg_height + descender));

  let bl2 = bl1 - vec2f(f32(frame_width));
  let tr2 = tr1 + vec2f(f32(frame_width));

  var quad_bl : vec2f;
  var quad_tr : vec2f;
  if (label_layer == 0u) {
    quad_bl = bl0; quad_tr = tr0;
  } else if (label_layer == 1u) {
    quad_bl = bl1; quad_tr = tr1;
  } else {
    if (frame_width == 0) { quad_bl = bl0; quad_tr = bl0; }
    else                  { quad_bl = bl2; quad_tr = tr2; }
  }

  let quad_size  = quad_tr - quad_bl;
  let screen_pos = quad_bl + quad_size * QUAD_C[label_corner];

  output.position = vec4f(clip.w * ((2.0 * screen_pos) / resolution - 1.0), clip.z, clip.w);

  let uv_size        = tc_max - tc_min;
  let label_atlas_uv = tc_min + uv_size * UV_C[label_corner];
  let label_encoded_propid = f32(propid);
)",
    /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceVertexShaderColors(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Colors::Impl",
    R"(output.color   = vec4f(vertex.frame_colors, f32(label_layer));
  output.lut_uv  = vec2f(label_encoded_propid, 0.0);)",
    /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceVertexShaderUVs(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::UVs::Impl", "output.uv = label_atlas_uv;",
    /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceVertexShaderNormals(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::Normals::Impl", "output.normal_VC = vec3f(0.0, 0.0, 1.0);", /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceVertexShaderTangents(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Tangents::Impl", "", /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceFragmentShaderColors(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl",
    R"(var ambient_color : vec3<f32>;
    var diffuse_color : vec3<f32> = vec3f(0.0);
    var opacity       : f32;
    let fs_label_layer      = u32(round(vertex.color.a));
    let fs_label_encoded_prop_id    = u32(round(vertex.lut_uv.x));
    let fs_frame_col  = vertex.color.rgb;
    if (fs_label_layer == 0u) {
      let samp = textureSampleLevel(atlas_texture, atlas_sampler, vertex.uv, 0.0);
      if (samp.a < 0.01) { discard; }
      ambient_color = samp.rgb; opacity = samp.a;
    } else if (fs_label_layer == 1u) {
      let bg = label_uniforms.background_colors[fs_label_encoded_prop_id];
      if (bg.a < 0.01) { discard; }
      ambient_color = bg.rgb; opacity = bg.a;
    } else {
      ambient_color = fs_frame_col; opacity = 1.0;
    })",
    /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceFragmentShaderNormals(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Normals::Impl", "", /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceFragmentShaderLights(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Lights::Impl",
    "output.color = vec4f(ambient_color + diffuse_color, opacity);", /*all=*/true);
}

void vtkWebGPUBatchedLabeledDataMapperInternals::ReplaceFragmentShaderCoincidentOffset(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(renderer),
  vtkWebGPUActor* vtkNotUsed(actor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::FragmentMain::End",
    R"({
    let fs_label_layer_for_depth = u32(round(vertex.color.a));
    if      (fs_label_layer_for_depth == 1u) { output.frag_depth += 0.000032; }
    else if (fs_label_layer_for_depth == 2u) { output.frag_depth += 0.000064; }
  }
  return output;
})",
    /*all=*/true);
}

//----------------------------------------------------------------------------
// Uploads per-label arrays as instance vertex buffers. This is the WebGPU equivalent
// of the OpenGL SetupHelper call (MapDataArrayToVertexAttribute), which registers the
// same arrays as named VBO attributes consumed by the vertex shader. Both must expose
// the same data: glyphExtents (Float32x4), coff+propid packed as (Float32x2), and
// frameColors (Float32x3).
void vtkWebGPUBatchedLabeledDataMapperInternals::UpdateInstanceBuffers(
  vtkWebGPUConfiguration* wgpuConfiguration)
{
  if (!this->Parent)
  {
    return;
  }

  vtkPolyData* poly = this->Parent->GetPreparedPolyData();
  if (!poly)
  {
    return;
  }
  const vtkIdType N = poly->GetNumberOfPoints();
  if (N == 0)
  {
    return;
  }

  if (this->NumberOfLabels != static_cast<uint32_t>(N))
  {
    this->NumberOfLabels = static_cast<uint32_t>(N);
    this->LabelCountChanged = true;
  }

  auto* glyphExtArray = vtkIntArray::SafeDownCast(poly->GetPointData()->GetArray("glyphExtents"));
  auto* coffArray = vtkFloatArray::SafeDownCast(poly->GetPointData()->GetArray("coff"));
  auto* propidArray = vtkFloatArray::SafeDownCast(poly->GetPointData()->GetArray("propid"));
  auto* frameColorsArray =
    vtkFloatArray::SafeDownCast(poly->GetPointData()->GetArray("framecolors"));

  if (!glyphExtArray || !coffArray || !propidArray || !frameColorsArray)
  {
    return;
  }

  // The GPU vertex buffer expects Float32x4; convert integer atlas extents to float.
  std::vector<float> glyphExtF(static_cast<size_t>(N) * 4);
  for (vtkIdType i = 0; i < N * 4; ++i)
  {
    glyphExtF[i] = static_cast<float>(glyphExtArray->GetValue(i));
  }

  std::vector<float> coffPropid(N * 2);
  for (vtkIdType i = 0; i < N; ++i)
  {
    coffPropid[2 * i] = coffArray->GetValue(i);
    coffPropid[2 * i + 1] = propidArray->GetValue(i);
  }

  const uint64_t glyphExtSz = static_cast<uint64_t>(N) * 4 * sizeof(float);
  const uint64_t coffPropSz = static_cast<uint64_t>(N) * 2 * sizeof(float);
  const uint64_t frameColSz = static_cast<uint64_t>(N) * 3 * sizeof(float);
  const uint64_t sizes[NUM_INSTANCE_ATTRIBS] = { glyphExtSz, coffPropSz, frameColSz };
  const char* bufLabels[NUM_INSTANCE_ATTRIBS] = { "label2d_glyph_extents", "label2d_coff_propid",
    "label2d_frame_colors" };

  for (int attr = 0; attr < NUM_INSTANCE_ATTRIBS; ++attr)
  {
    if (this->InstanceBufferSizes[attr] != sizes[attr])
    {
      if (this->InstanceBuffers[attr])
      {
        this->InstanceBuffers[attr].Destroy();
      }
      wgpu::BufferDescriptor desc{};
      desc.size = sizes[attr];
      desc.label = bufLabels[attr];
      desc.mappedAtCreation = false;
      desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
      this->InstanceBuffers[attr] = wgpuConfiguration->CreateBuffer(desc);
      this->InstanceBufferSizes[attr] = sizes[attr];
      this->RebuildGraphicsPipelines = true;
    }
  }

  wgpuConfiguration->WriteBuffer(
    this->InstanceBuffers[GLYPH_EXTENTS], 0, glyphExtF.data(), glyphExtSz, "label2d_glyph_extents");
  wgpuConfiguration->WriteBuffer(
    this->InstanceBuffers[COFF_PROPID], 0, coffPropid.data(), coffPropSz, "label2d_coff_propid");
  wgpuConfiguration->WriteBuffer(this->InstanceBuffers[FRAME_COLORS], 0,
    frameColorsArray->GetPointer(0), frameColSz, "label2d_frame_colors");
}

//----------------------------------------------------------------------------
// Uploads per-frame data into the LabelUniforms2D uniform buffer. This is the
// WebGPU equivalent of vtkOpenGLBatchedLabeledDataMapperInternals::SetMapperShaderParameters,
// which uploads the same values as individual shader-program uniforms. Both must
// stay in sync: atlasDims, vpDims, winDims, anchorCenter, vp, nvp, displayOffset,
// backgroundColors, frameWidths, maxGlyphHeights, descenders.
void vtkWebGPUBatchedLabeledDataMapperInternals::UpdateUniformBuffer(
  vtkRenderer* renderer, vtkWebGPUConfiguration* wgpuConfiguration)
{
  if (!this->Parent)
  {
    return;
  }

  LabelUniforms2D u{};

  vtkImageData* atlas = this->Parent->GetGlyphAtlas();
  if (atlas)
  {
    u.atlasDims[0] = atlas->GetDimensions()[0];
    u.atlasDims[1] = atlas->GetDimensions()[1];
  }

  int* vSize = renderer->GetSize();
  u.vpDims[0] = vSize[0];
  u.vpDims[1] = vSize[1];

  int* wSize = renderer->GetRenderWindow()->GetSize();
  u.winDims[0] = wSize[0];
  u.winDims[1] = wSize[1];

  int anchorCenter[2] = { -1, -1 };
  switch (this->Parent->GetTextAnchor())
  {
    case vtkBatchedLabeledDataMapper::LowerEdge:
      anchorCenter[0] = 0;
      break;
    case vtkBatchedLabeledDataMapper::LowerRight:
      anchorCenter[0] = 1;
      break;
    case vtkBatchedLabeledDataMapper::LeftEdge:
      anchorCenter[1] = 0;
      break;
    case vtkBatchedLabeledDataMapper::Center:
      anchorCenter[0] = 0;
      anchorCenter[1] = 0;
      break;
    case vtkBatchedLabeledDataMapper::RightEdge:
      anchorCenter[0] = 1;
      anchorCenter[1] = 0;
      break;
    case vtkBatchedLabeledDataMapper::UpperLeft:
      anchorCenter[1] = 1;
      break;
    case vtkBatchedLabeledDataMapper::UpperEdge:
      anchorCenter[0] = 0;
      anchorCenter[1] = 1;
      break;
    case vtkBatchedLabeledDataMapper::UpperRight:
      anchorCenter[0] = 1;
      anchorCenter[1] = 1;
      break;
    case vtkBatchedLabeledDataMapper::LowerLeft:
    default:
      break;
  }
  u.anchorCenter[0] = anchorCenter[0];
  u.anchorCenter[1] = anchorCenter[1];

  const int* dispOffset = this->Parent->GetDisplayOffset();
  u.displayOffset[0] = dispOffset[0];
  u.displayOffset[1] = dispOffset[1];

  double vp[4];
  renderer->GetViewport(vp);
  for (int i = 0; i < 4; ++i)
  {
    u.vp[i] = static_cast<float>(vp[i]);
  }

  double tileVP[4];
  renderer->GetRenderWindow()->GetTileViewport(tileVP);
  u.nvp[0] = static_cast<float>(std::max(vp[0], tileVP[0]));
  u.nvp[1] = static_cast<float>(std::max(vp[1], tileVP[1]));
  u.nvp[2] = static_cast<float>(std::min(vp[2], tileVP[2]));
  u.nvp[3] = static_cast<float>(std::min(vp[3], tileVP[3]));

  const float* bgColors = this->Parent->GetBackgroundColors();
  const int* frameW = this->Parent->GetFrameWidths();
  const int* maxH = this->Parent->GetMaxGlyphHeights();
  const int* desc = this->Parent->GetDescenders();
  for (int i = 0; i < vtkBatchedLabeledDataMapper::MaxTextProperties; ++i)
  {
    u.backgroundColors[4 * i] = bgColors[4 * i];
    u.backgroundColors[4 * i + 1] = bgColors[4 * i + 1];
    u.backgroundColors[4 * i + 2] = bgColors[4 * i + 2];
    u.backgroundColors[4 * i + 3] = bgColors[4 * i + 3];
    u.frameWidths[i] = frameW[i];
    u.maxGlyphHeights[i] = maxH[i];
    u.descenders[i] = desc[i];
  }

  wgpuConfiguration->WriteBuffer(
    this->LabelUniformBuffer, 0, &u, sizeof(LabelUniforms2D), "LabelUniforms2D");
}

VTK_ABI_NAMESPACE_END
