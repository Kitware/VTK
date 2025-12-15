// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor2D.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkMatrix4x4.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUPolyDataMapper2D.h"
#include "vtkWebGPURenderPipelineCache.h"
#include "vtkWebGPURenderTextureDeviceResource.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "vtkPolyData2DFSWGSL.h"
#include "vtkPolyData2DVSWGSL.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPUPolyDataMapper2DInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
template <typename DestT>
struct WriteTypedArray
{
  std::size_t ByteOffset = 0;
  std::size_t NumberOfBytesWritten = 0;
  const wgpu::Buffer& DstBuffer;
  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;
  float Denominator = 1.0;

  template <typename SrcArrayT>
  void operator()(SrcArrayT* array, const char* description)
  {
    if (array == nullptr || this->DstBuffer.Get() == nullptr)
    {
      return;
    }
    const auto values = vtk::DataArrayValueRange(array);
    vtkNew<vtkAOSDataArrayTemplate<DestT>> data;
    for (const auto& value : values)
    {
      data->InsertNextValue(value / this->Denominator);
    }
    const std::size_t nbytes = data->GetNumberOfValues() * sizeof(DestT);
    this->WGPUConfiguration->WriteBuffer(
      this->DstBuffer, this->ByteOffset, data->GetPointer(0), nbytes, description);
    this->ByteOffset += nbytes;
    this->NumberOfBytesWritten += nbytes;
  }
};
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper2DInternals::vtkWebGPUPolyDataMapper2DInternals() = default;

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper2DInternals::~vtkWebGPUPolyDataMapper2DInternals() = default;

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper2DInternals::CreateMeshAttributeBindGroupLayout(
  const wgpu::Device& device, const std::string& label,
  vtkWebGPURenderTextureDeviceResource* deviceTextureRc)
{
  using BGLEntryInitializer = vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper;
  std::vector<wgpu::BindGroupLayoutEntry> entries;
  // Mapper2Dstate
  entries.emplace_back(
    BGLEntryInitializer{ 0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
      wgpu::BufferBindingType::ReadOnlyStorage });
  // mesh_attributes
  entries.emplace_back(
    BGLEntryInitializer{ 1, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
      wgpu::BufferBindingType::ReadOnlyStorage });
  // mesh_data
  entries.emplace_back(
    BGLEntryInitializer{ 2, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage });
  if (deviceTextureRc)
  {
    // texture sampler
    entries.emplace_back(
      deviceTextureRc->MakeSamplerBindGroupLayoutEntry(3, wgpu::ShaderStage::Fragment));
    // texture data
    entries.emplace_back(
      deviceTextureRc->MakeTextureViewBindGroupLayoutEntry(4, wgpu::ShaderStage::Fragment));
  }
  return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device, entries, label);
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper2DInternals::CreateTopologyBindGroupLayout(
  const wgpu::Device& device, const std::string& label, bool homogeneousCellSize)
{
  if (homogeneousCellSize)
  {
    return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      {
        // clang-format off
        // connectivity
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // cell_id_offset
        { 2, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform },
        // clang-format on
      },
      label);
  }
  else
  {
    return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      {
        // clang-format off
        // connectivity
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // cell_ids
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // clang-format on
      },
      label);
  }
}

//------------------------------------------------------------------------------
const char* vtkWebGPUPolyDataMapper2DInternals::GetGraphicsPipelineTypeAsString(
  GraphicsPipeline2DType graphicsPipelineType)
{
  switch (graphicsPipelineType)
  {
    case GFX_PIPELINE_2D_POINTS:
      return "GFX_PIPELINE_POINTS";
    case GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_2D_LINES:
      return "GFX_PIPELINE_LINES";
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_2D_TRIANGLES:
      return "GFX_PIPELINE_2D_TRIANGLES";
    case GFX_PIPELINE_2D_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_2D_TRIANGLES_HOMOGENEOUS_CELL_SIZE";
    case NUM_GFX_PIPELINE_2D_NB_TYPES:
      break;
  }
  return "";
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper2DInternals::IsPipelineForHomogeneousCellSize(
  GraphicsPipeline2DType graphicsPipelineType)
{
  bool isHomogenousCellSize = false;
  switch (graphicsPipelineType)
  {
    case GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_2D_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      isHomogenousCellSize = true;
      break;
    default:
      break;
  }
  return isHomogenousCellSize;
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ApplyShaderReplacements(
  GraphicsPipeline2DType pipelineType, std::string& vss, std::string& fss,
  vtkWebGPURenderWindow* wgpuRenderWindow, vtkActor2D* actor)
{
  // Vertex and Fragment shader replacements
  this->ReplaceShaderVertexOutputDef(vss, fss);
  this->ReplaceShaderMapperBindings(vss, fss, wgpuRenderWindow, actor);

  // Vertex Shader replacements
  this->ReplaceVertexShaderConstantsDef(pipelineType, vss);
  this->ReplaceVertexShaderMapper2DStateDef(vss);
  this->ReplaceVertexShaderMeshArraysDescriptorDef(vss);
  this->ReplaceVertexShaderTopologyBindings(vss);
  this->ReplaceVertexShaderVertexInputDef(vss);
  this->ReplaceVertexShaderUtilityMethodsDef(pipelineType, vss);
  this->ReplaceVertexShaderVertexMainStart(vss);
  this->ReplaceVertexShaderVertexIdImpl(pipelineType, vss);
  this->ReplaceVertexShaderPrimitiveIdImpl(pipelineType, vss);
  this->ReplaceVertexShaderCellIdImpl(pipelineType, vss);
  this->ReplaceVertexShaderPositionImpl(pipelineType, vss);
  this->ReplaceVertexShaderPickingImpl(vss);
  this->ReplaceVertexShaderColorsImpl(vss);
  this->ReplaceVertexShaderUVsImpl(vss);
  this->ReplaceVertexShaderVertexMainEnd(vss);

  // Fragment Shader replacements
  this->ReplaceFragmentShaderFragmentOutputDef(fss);
  this->ReplaceFragmentShaderFragmentMainStart(fss);
  this->ReplaceFragmentShaderPickingImpl(fss);
  this->ReplaceFragmentShaderColorImpl(fss);
  this->ReplaceFragmentShaderFragmentMainEnd(fss);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceShaderVertexOutputDef(
  std::string& vss, std::string& fss)
{
  for (auto& shaderSource : { &vss, &fss })
  {
    vtkWebGPURenderPipelineCache::Substitute(*shaderSource, "//VTK::VertexOutput::Def",
      R"(struct VertexOutput
{
  @builtin(position) position: vec4<f32>,
  @location(0) color: vec4<f32>,
  @location(1) uv: vec2<f32>,
  @location(2) @interpolate(flat) cell_id: u32,
})",
      true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderConstantsDef(
  GraphicsPipeline2DType pipelineType, std::string& vss)
{
  std::ostringstream codeStream;
  codeStream << "const BIT_POSITION_USE_CELL_COLOR: u32 = " << BIT_POSITION_USE_CELL_COLOR
             << "u;\n";
  codeStream << "const BIT_POSITION_USE_POINT_COLOR: u32 = " << BIT_POSITION_USE_POINT_COLOR
             << "u;\n";
  switch (pipelineType)
  {
    case GFX_PIPELINE_2D_POINTS:
    case GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE:
      codeStream << R"(///
// (-1, 1) |-------------------------------|(1, 1)
//         |-                              |
//         |    -                          |
//         |        -                      |
// (-1, 0) |              -                |
//         |                   -           |
//         |                        -      |
//         |                              -|
// (-1,-1) |-------------------------------|(1, -1)
///
// this triangle strip describes a quad spanning a bi-unit domain.
const VERTEX_PARAMETRIC_COORDS = array(
  vec2f(-1, -1),
  vec2f(1, -1),
  vec2f(-1, 1),
  vec2f(1, 1),
);)";
      break;
    case GFX_PIPELINE_2D_LINES:
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
      codeStream << R"(///
// (0, 0.5) |-------------------------------|(1, 0.5)
//          |-                              |
//          |    -                          |
//          |        -                      |
// (0, 0)   |              -                |
//          |                   -           |
//          |                        -      |
//          |                              -|
// (0,-0.5) |-------------------------------|(1, -0.5)
///
const LINE_PARAMETRIC_COORDS = array(
  vec2(0, -0.5),
  vec2(1, -0.5),
  vec2(0, 0.5),
  vec2(1, 0.5),
);)";
      break;
    default:
      break;
  }
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Constants::Def", codeStream.str(), true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderMapper2DStateDef(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Mapper2DState::Def",
    R"(struct Mapper2DState
{
  wcvc_matrix: mat4x4<f32>,
  color: vec4<f32>,
  point_size: f32,
  line_width: f32,
  flags: u32,
})",
    true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderMeshArraysDescriptorDef(
  std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::MeshArraysDescriptor::Def",
    R"(struct AttributeArrayDescriptor
{
  start: u32,
  num_tuples: u32,
  num_components: u32,
}
struct MeshAttributes
{
  positions: AttributeArrayDescriptor,
  uvs: AttributeArrayDescriptor,
  colors: AttributeArrayDescriptor,
})",
    true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceShaderMapperBindings(
  std::string& vss, std::string& fss, vtkWebGPURenderWindow* wgpuRenderWindow, vtkActor2D* actor)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Mapper::Bindings",
    R"(@group(0) @binding(0) var<storage, read> state: Mapper2DState;
@group(0) @binding(1) var<storage, read> mesh_attributes: MeshAttributes;
@group(0) @binding(2) var<storage, read> mesh_data: array<f32>;)",
    true);
  if (wgpuRenderWindow && actor)

  {
    if (auto* propertyKeys = actor->GetPropertyKeys())
    {
      if (propertyKeys->Has(vtkProp::GENERAL_TEXTURE_UNIT()))
      {
        const int textureUnit = propertyKeys->Get(vtkProp::GENERAL_TEXTURE_UNIT());
        if (auto devRc = wgpuRenderWindow->GetWGPUTextureCache()->GetRenderTexture(textureUnit))
        {
          const auto textureSampleTypeStr =
            vtkWebGPURenderTextureDeviceResource::GetTextureSampleTypeString(
              devRc->GetSampleType());
          std::ostringstream codeStream;
          // It is okay to not place the other bindings 0, 1, and 2 here again, as they are not used
          // in the fragment shader. However, we need to declare bindings 3 and 4 for the texture
          // sampler and texture data.
          codeStream << R"(@group(0) @binding(3) var texture_sampler: sampler;
@group(0) @binding(4) var texture_data: texture_2d<)"
                     << textureSampleTypeStr << ">;";
          vtkWebGPURenderPipelineCache::Substitute(
            fss, "//VTK::Mapper::Bindings", codeStream.str(), true);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderTopologyBindings(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Topology::Bindings",
    R"(@group(1) @binding(0) var<storage, read> connectivity: array<u32>;
@group(1) @binding(1) var<storage, read> cell_ids: array<u32>;
@group(1) @binding(2) var<uniform> cell_id_offset: u32;)",
    true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderVertexInputDef(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexInput::Def",
    R"(struct VertexInput
{
  @builtin(instance_index) instance_id: u32,
  @builtin(vertex_index) vertex_id: u32,
})",
    true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderUtilityMethodsDef(
  GraphicsPipeline2DType pipelineType, std::string& vss)
{
  std::ostringstream codeStream;
  codeStream << R"(fn getUseCellColor(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_USE_CELL_COLOR) & 0x1;
  return select(false, true, result == 1u);
}

fn getUsePointColor(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_USE_POINT_COLOR) & 0x1;
  return select(false, true, result == 1u);
}

fn getVertexColor(point_id: u32, cell_id: u32) -> vec4f
{
  if getUsePointColor(state.flags)
  {
    // Smooth shading
    return vec4f(
      mesh_data[mesh_attributes.colors.start + 4u * point_id],
      mesh_data[mesh_attributes.colors.start + 4u * point_id + 1u],
      mesh_data[mesh_attributes.colors.start + 4u * point_id + 2u],
      mesh_data[mesh_attributes.colors.start + 4u * point_id + 3u]
    );
  }
  if getUseCellColor(state.flags)
  {
    // Flat shading
    return vec4f(
      mesh_data[mesh_attributes.colors.start + 4u * cell_id],
      mesh_data[mesh_attributes.colors.start + 4u * cell_id + 1u],
      mesh_data[mesh_attributes.colors.start + 4u * cell_id + 2u],
      mesh_data[mesh_attributes.colors.start + 4u * cell_id + 3u]
    );
  }
  return state.color;
}

fn getVertexCoordinates(point_id: u32) -> vec4f
{
  return vec4f(
    mesh_data[mesh_attributes.positions.start + 3u * point_id],
    mesh_data[mesh_attributes.positions.start + 3u * point_id + 1u],
    mesh_data[mesh_attributes.positions.start + 3u * point_id + 2u], 1.0
  );
}

fn getVertexUVs(point_id: u32) -> vec2f
{
  return vec2f(
    mesh_data[mesh_attributes.uvs.start + 2u * point_id],
    mesh_data[mesh_attributes.uvs.start + 2u * point_id + 1u]
  );
})";
  switch (pipelineType)
  {
    case GFX_PIPELINE_2D_LINES:
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
      codeStream <<
        R"(
fn getLinePointWorldCoordinate(line_segment_id: u32, parametric_id: u32, out_point_id: ptr<function, u32>) -> vec4f
{
  var width = state.line_width;
  // The point rendering algorithm is unstable for line_width < 1.0
  if width < 1.0
  {
    width = 1.0;
  }

  let local_position = LINE_PARAMETRIC_COORDS[parametric_id];
  let p0_vertex_id: u32 = 2 * line_segment_id;
  let p1_vertex_id = p0_vertex_id + 1;

  let p0_point_id: u32 = connectivity[p0_vertex_id];
  let p1_point_id: u32 = connectivity[p1_vertex_id];
  let p = select(2 * line_segment_id, 2 * line_segment_id + 1, local_position.x == 1);
  // compute point id based on the x component of the parametric coordinate.
  *out_point_id = u32(mix(f32(p0_point_id), f32(p1_point_id), local_position.x));

  let p0_vertex_wc = getVertexCoordinates(p0_point_id);
  let p1_vertex_wc = getVertexCoordinates(p1_point_id);

  let x_basis = normalize(p1_vertex_wc.xy - p1_vertex_wc.xy);
  let y_basis = vec2(-x_basis.y, x_basis.x);

  var vertex_wc = mix(p0_vertex_wc, p1_vertex_wc, local_position.x);
  return vec4(vertex_wc.x, vertex_wc.y + local_position.y * width, vertex_wc.zw);
      })";
      break;
    default:
      break;
  }
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::UtilityMethods::Def", codeStream.str(), true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderVertexMainStart(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexMain::Start",
    R"(@vertex
fn main(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;)",
    true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderVertexIdImpl(
  GraphicsPipeline2DType pipelineType, std::string& vss)
{
  std::ostringstream codeStream;
  codeStream << "let pull_vertex_id: u32 =";
  switch (pipelineType)
  {
    case GFX_PIPELINE_2D_POINTS:
    case GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_2D_LINES:
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
      codeStream << " vertex.instance_id;";
      break;
    case GFX_PIPELINE_2D_TRIANGLES:
    case GFX_PIPELINE_2D_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      codeStream << " vertex.vertex_id;";
      break;
    default:
      break;
  }
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl", codeStream.str(), true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderPrimitiveIdImpl(
  GraphicsPipeline2DType pipelineType, std::string& vss)
{
  std::ostringstream codeStream;
  codeStream << "let primitive_id: u32 =";
  switch (pipelineType)
  {
    case GFX_PIPELINE_2D_POINTS:
    case GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE:
      codeStream << " pull_vertex_id;";
      break;
    case GFX_PIPELINE_2D_LINES:
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
      codeStream << " pull_vertex_id >> 1u;";
      break;
    case GFX_PIPELINE_2D_TRIANGLES:
    case GFX_PIPELINE_2D_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      codeStream << " pull_vertex_id / 3u;";
      break;
    default:
      break;
  }
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::PrimitiveId::Impl", codeStream.str(), true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderCellIdImpl(
  GraphicsPipeline2DType pipelineType, std::string& vss)
{
  std::ostringstream codeStream;
  codeStream << "let cell_id: u32 =";
  switch (pipelineType)
  {
    case GFX_PIPELINE_2D_POINTS:
    case GFX_PIPELINE_2D_LINES:
    case GFX_PIPELINE_2D_TRIANGLES:
      codeStream << " cell_ids[primitive_id];";
      break;
    case GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_2D_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      codeStream << " primitive_id + cell_id_offset;";
      break;
    default:
      break;
  }
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl", codeStream.str(), true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderPositionImpl(
  GraphicsPipeline2DType pipelineType, std::string& vss)
{
  std::ostringstream codeStream;
  switch (pipelineType)
  {
    case GFX_PIPELINE_2D_POINTS:
    case GFX_PIPELINE_2D_POINTS_HOMOGENEOUS_CELL_SIZE:
      codeStream << R"(// pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  var vertex_wc = getVertexCoordinates(point_id);
  var point_size = state.point_size;
  // The point rendering algorithm is unstable for point_size < 1.0
  if point_size < 1.0
  {
    point_size = 1.0;
  }
  let local_position = VERTEX_PARAMETRIC_COORDS[vertex.vertex_id];
  vertex_wc = vec4f(vertex_wc.xy + 0.5 * point_size * local_position, vertex_wc.zw);
  output.position = state.wcvc_matrix * vertex_wc;)";
      break;
    case GFX_PIPELINE_2D_LINES:
    case GFX_PIPELINE_2D_LINES_HOMOGENEOUS_CELL_SIZE:
      codeStream << R"(let line_segment_id = vertex.instance_id;
  let parametric_id = vertex.vertex_id;
  var point_id: u32;
  let vertex_wc = getLinePointWorldCoordinate(line_segment_id, parametric_id, &point_id);
  output.position = state.wcvc_matrix * vertex_wc;)";
      break;
    case GFX_PIPELINE_2D_TRIANGLES:
    case GFX_PIPELINE_2D_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      codeStream << R"(// pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  let vertex_wc = getVertexCoordinates(point_id);
  output.position = state.wcvc_matrix * vertex_wc;)";
      break;
    default:
      break;
  }
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl", codeStream.str(), true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderPickingImpl(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::Picking::Impl", "output.cell_id = cell_id;", true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderColorsImpl(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::Colors::Impl", "output.color = getVertexColor(point_id, cell_id);", true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderUVsImpl(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::UVs::Impl", "output.uv = getVertexUVs(point_id);", true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceVertexShaderVertexMainEnd(std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::VertexMain::End", "  return output;\n}", true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceFragmentShaderFragmentOutputDef(std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::FragmentOutput::Def",
    R"(struct FragmentOutput
{
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>, // cell_id, prop_id, composite_id, process_id
})",
    true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceFragmentShaderFragmentMainStart(std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::FragmentMain::Start",
    R"(@fragment
fn main(input: VertexOutput) -> FragmentOutput
{
  var output: FragmentOutput;)",
    true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceFragmentShaderPickingImpl(std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(
    fss, "//VTK::Picking::Impl", "output.ids = vec4<u32>(input.cell_id, 0u, 0u, 0u);", true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceFragmentShaderColorImpl(std::string& fss)
{
  if (this->ActorTextureUnit >= 0)
  {
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl",
      R"(let texture_color = textureSample(texture_data, texture_sampler, input.uv);
  output.color = input.color * texture_color;)",
      true);
  }
  else
  {
    vtkWebGPURenderPipelineCache::Substitute(
      fss, "//VTK::Colors::Impl", "output.color = input.color;", true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReplaceFragmentShaderFragmentMainEnd(std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(
    fss, "//VTK::FragmentMain::End", "  return output;\n}", true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReleaseGraphicsResources(vtkWindow* w)
{
  this->CellConverter->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::UpdateBuffers(
  vtkViewport* viewport, vtkActor2D* actor, vtkWebGPUPolyDataMapper2D* mapper)
{
  vtkPolyData* input = mapper->GetInput();
  if (input == nullptr)
  {
    vtkErrorWithObjectMacro(mapper, << "No input!");
    return;
  }
  mapper->GetInputAlgorithm()->Update();
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts == 0)
  {
    vtkDebugWithObjectMacro(mapper, << "No points!");
    return;
  }
  if (mapper->LookupTable == nullptr)
  {
    mapper->CreateDefaultLookupTable();
  }

  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(viewport);
  if (!wgpuRenderer)
  {
    vtkErrorWithObjectMacro(
      mapper, << "vtkWebGPUPolyDataMapper2DInternals::UpdateBuffers: no vtkWebGPURenderer");
    return;
  }
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(viewport->GetVTKWindow());
  if (!wgpuRenderWindow)
  {
    vtkErrorWithObjectMacro(
      mapper, << "vtkWebGPUPolyDataMapper2DInternals::UpdateBuffers: no vtkWebGPURenderWindow");
    return;
  }
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  if (!wgpuConfiguration)
  {
    vtkErrorWithObjectMacro(
      mapper, << "vtkWebGPUPolyDataMapper2DInternals::UpdateBuffers: no vtkWebGPUConfiguration");
    return;
  }
  auto* wgpuTextureCache = wgpuRenderWindow->GetWGPUTextureCache();
  if (!wgpuTextureCache)
  {
    vtkErrorWithObjectMacro(
      mapper, << "vtkWebGPUPolyDataMapper2DInternals::UpdateBuffers: no vtkWebGPUTextureCache");
    return;
  }

  bool recreateMeshBindGroup = false;
  if (this->Mapper2DStateData.Buffer == nullptr)
  {
    const auto label = "Mapper2DState-" + input->GetObjectDescription();
    this->Mapper2DStateData.Buffer = wgpuConfiguration->CreateBuffer(sizeof(Mapper2DState),
      wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage, false, label.c_str());
    this->Mapper2DStateData.Size = sizeof(Mapper2DState);
    const auto& device = wgpuConfiguration->GetDevice();
    recreateMeshBindGroup = true;
  }
  if ((this->Mapper2DStateData.BuildTimeStamp < actor->GetProperty()->GetMTime()) ||
    (this->Mapper2DStateData.BuildTimeStamp < actor->GetPositionCoordinate()->GetMTime()) ||
    (this->Mapper2DStateData.BuildTimeStamp < viewport->GetMTime()) ||
    (this->Mapper2DStateData.BuildTimeStamp < viewport->GetVTKWindow()->GetMTime()))
  {
    // Get the position of the actor
    int size[2];
    size[0] = viewport->GetSize()[0];
    size[1] = viewport->GetSize()[1];

    double* vport = viewport->GetViewport();
    int* actorPos = actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

    // get window info
    double* tileViewPort = viewport->GetVTKWindow()->GetTileViewport();
    double visVP[4];
    visVP[0] = (vport[0] >= tileViewPort[0]) ? vport[0] : tileViewPort[0];
    visVP[1] = (vport[1] >= tileViewPort[1]) ? vport[1] : tileViewPort[1];
    visVP[2] = (vport[2] <= tileViewPort[2]) ? vport[2] : tileViewPort[2];
    visVP[3] = (vport[3] <= tileViewPort[3]) ? vport[3] : tileViewPort[3];
    if (visVP[0] >= visVP[2])
    {
      return;
    }
    if (visVP[1] >= visVP[3])
    {
      return;
    }
    size[0] = static_cast<int>(std::round(size[0] * (visVP[2] - visVP[0]) / (vport[2] - vport[0])));
    size[1] = static_cast<int>(std::round(size[1] * (visVP[3] - visVP[1]) / (vport[3] - vport[1])));

    const int* winSize = viewport->GetVTKWindow()->GetSize();

    int xoff = static_cast<int>(actorPos[0] - (visVP[0] - vport[0]) * winSize[0]);
    int yoff = static_cast<int>(actorPos[1] - (visVP[1] - vport[1]) * winSize[1]);

    // set ortho projection
    float left = -xoff;
    float right = -xoff + size[0];
    float bottom = -yoff;
    float top = -yoff + size[1];

    // it's an error when either left==right or top==bottom
    if (left == right)
    {
      right = left + 1.0;
    }
    if (bottom == top)
    {
      top = bottom + 1.0;
    }

    float yAxisSign = -1.0f; // in webgpu, the y axis of window increases downwards.
    // compute the combined ModelView matrix and send it down to save time in the shader
    this->WCVCMatrix->Zero();
    this->WCVCMatrix->SetElement(0, 0, 2.0 / (right - left));
    this->WCVCMatrix->SetElement(1, 1, yAxisSign * 2.0 / (top - bottom));
    this->WCVCMatrix->SetElement(0, 3, -1.0 * (right + left) / (right - left));
    this->WCVCMatrix->SetElement(1, 3, yAxisSign * -1.0 * (top + bottom) / (top - bottom));
    this->WCVCMatrix->SetElement(2, 2, 0.0);
    this->WCVCMatrix->SetElement(
      2, 3, actor->GetProperty()->GetDisplayLocation() == VTK_FOREGROUND_LOCATION ? 0.0 : 1.0);
    this->WCVCMatrix->SetElement(3, 3, 1.0);
    // transpose and convert from double to float in one nested loop.
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        // transpose because, shader will interpret it in a column-major order.
        this->State.WCVCMatrix[j][i] = this->WCVCMatrix->GetElement(i, j);
      }
    }
    std::array<double, 3> color = {};
    actor->GetProperty()->GetColor(color.data());
    std::copy(color.begin(), color.end(), this->State.Color);

    this->State.Color[3] = actor->GetProperty()->GetOpacity();

    this->State.PointSize = actor->GetProperty()->GetPointSize();

    this->State.LineWidth = actor->GetProperty()->GetLineWidth();

    this->State.Flags = (this->UseCellScalarMapping ? 1 : 0) << BIT_POSITION_USE_CELL_COLOR;
    this->State.Flags |= ((this->UsePointScalarMapping ? 1 : 0) << BIT_POSITION_USE_POINT_COLOR);
    wgpuConfiguration->WriteBuffer(
      this->Mapper2DStateData.Buffer, 0, &(this->State), sizeof(Mapper2DState), "Mapper2DState");
  }

  if (this->MeshData.BuildTimeStamp < mapper->GetMTime() ||
    this->MeshData.BuildTimeStamp < actor->GetMTime() ||
    this->MeshData.BuildTimeStamp < input->GetMTime() ||
    (mapper->TransformCoordinate &&
      (this->MeshData.BuildTimeStamp < viewport->GetMTime() ||
        this->MeshData.BuildTimeStamp < viewport->GetVTKWindow()->GetMTime())))
  {
    // update point data buffer.
    mapper->MapScalars(actor->GetProperty()->GetOpacity());
    if (mapper->Colors != nullptr && mapper->Colors->GetNumberOfValues() > 0)
    {
      this->UsePointScalarMapping = true;
    }
    this->UseCellScalarMapping = false;
    if (mapper->ScalarVisibility)
    {
      // We must figure out how the scalars should be mapped to the polydata.
      if ((mapper->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
            mapper->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
            mapper->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
            !input->GetPointData()->GetScalars()) &&
        mapper->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA && mapper->Colors)
      {
        this->UseCellScalarMapping = true;
        this->UsePointScalarMapping = false;
      }
    }
    this->State.Flags = (this->UseCellScalarMapping ? 1 : 0);
    this->State.Flags |= ((this->UsePointScalarMapping ? 1 : 0) << 1);
    wgpuConfiguration->WriteBuffer(
      this->Mapper2DStateData.Buffer, 0, &(this->State), sizeof(Mapper2DState), "Mapper2DState");
    vtkDataArray* pointPositions = input->GetPoints()->GetData();
    // Transform the points, if necessary
    if (mapper->TransformCoordinate)
    {
      if (!this->TransformedPoints)
      {
        this->TransformedPoints = vtk::TakeSmartPointer(vtkPoints::New());
      }
      this->TransformedPoints->SetNumberOfPoints(numPts);
      for (vtkIdType j = 0; j < numPts; j++)
      {
        mapper->TransformCoordinate->SetValue(pointPositions->GetTuple(j));
        if (mapper->TransformCoordinateUseDouble)
        {
          double* dtmp = mapper->TransformCoordinate->GetComputedDoubleViewportValue(viewport);
          this->TransformedPoints->SetPoint(j, dtmp[0], dtmp[1], 0.0);
        }
        else
        {
          int* itmp = mapper->TransformCoordinate->GetComputedViewportValue(viewport);
          this->TransformedPoints->SetPoint(j, itmp[0], itmp[1], 0.0);
        }
      }
      pointPositions = this->TransformedPoints->GetData();
      // Flag as modified so that we re-upload the transformed positions.
      pointPositions->Modified();
    }
    vtkDataArray* pointColors =
      this->UsePointScalarMapping ? vtkDataArray::SafeDownCast(mapper->Colors) : nullptr;
    vtkDataArray* pointUVs = input->GetPointData()->GetTCoords();
    vtkDataArray* cellColors =
      this->UseCellScalarMapping ? vtkDataArray::SafeDownCast(mapper->Colors) : nullptr;
    std::size_t requiredBufferSize = 0;
    for (auto* array : { pointPositions, pointUVs, pointColors, cellColors })
    {
      if (array != nullptr)
      {
        requiredBufferSize += sizeof(vtkTypeFloat32) * array->GetDataSize();
      }
    }
    const std::string meshAttrDescriptorLabel =
      "MeshAttributeDescriptor-" + input->GetObjectDescription();
    if (this->AttributeDescriptorData.Buffer == nullptr)
    {
      recreateMeshBindGroup = true;
      this->AttributeDescriptorData.Buffer = wgpuConfiguration->CreateBuffer(
        sizeof(this->MeshArraysDescriptor), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        false, meshAttrDescriptorLabel.c_str());
      this->AttributeDescriptorData.Size = sizeof(this->MeshArraysDescriptor);
    }
    if (requiredBufferSize != this->MeshData.Size)
    {
      this->MeshData.Buffer = nullptr;
      // reset the build timestamp so that all data arrays are uploaded.
      this->MeshData.BuildTimeStamp = vtkTimeStamp();
    }

    if (this->MeshData.Buffer == nullptr)
    {
      recreateMeshBindGroup = true;
      const auto label = "MeshAttributes-" + input->GetObjectDescription();
      this->MeshData.Buffer = wgpuConfiguration->CreateBuffer(requiredBufferSize,
        wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage, false, label.c_str());
      this->MeshData.Size = requiredBufferSize;
    }
    using DispatchT = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllArrays>;
    ::WriteTypedArray<vtkTypeFloat32> meshDataWriter{ 0, 0, this->MeshData.Buffer,
      wgpuConfiguration, 1. };
    // Only write data that has changed since last build.
    // Upload positions
    if (pointPositions != nullptr)
    {
      if (pointPositions->GetMTime() > this->MeshData.BuildTimeStamp)
      {
        this->MeshArraysDescriptor.Positions.Start =
          meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointPositions, meshDataWriter, "Positions"))
        {
          meshDataWriter(pointPositions, "Positions");
        }
        this->MeshArraysDescriptor.Positions.NumComponents =
          pointPositions->GetNumberOfComponents();
        this->MeshArraysDescriptor.Positions.NumTuples = pointPositions->GetNumberOfTuples();
      }
      else
      {
        vtkDebugWithObjectMacro(mapper, << "Skipping point positions upload");
        meshDataWriter.ByteOffset += sizeof(vtkTypeFloat32) * pointPositions->GetDataSize();
      }
    }
    // Upload point UVs
    if (pointUVs != nullptr)
    {
      if (pointUVs->GetMTime() > this->MeshData.BuildTimeStamp)
      {
        this->MeshArraysDescriptor.UVs.Start = meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointUVs, meshDataWriter, "UVs"))
        {
          meshDataWriter(pointUVs, "UVs");
        }
        this->MeshArraysDescriptor.UVs.NumComponents = pointUVs->GetNumberOfComponents();
        this->MeshArraysDescriptor.UVs.NumTuples = pointUVs->GetNumberOfTuples();
      }
      else
      {
        vtkDebugWithObjectMacro(mapper, << "Skipping point positions upload");
        meshDataWriter.ByteOffset += sizeof(vtkTypeFloat32) * pointUVs->GetDataSize();
      }
    }
    // Upload point colors
    if (this->UsePointScalarMapping && pointColors != nullptr)
    {
      if (pointColors->GetMTime() > this->MeshData.BuildTimeStamp)
      {
        meshDataWriter.Denominator = 255.0;
        this->MeshArraysDescriptor.Colors.Start =
          meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointColors, meshDataWriter, "PointColors"))
        {
          meshDataWriter(pointColors, "PointColors");
        }
        meshDataWriter.Denominator = 1.0;
        this->MeshArraysDescriptor.Colors.NumComponents = pointColors->GetNumberOfComponents();
        this->MeshArraysDescriptor.Colors.NumTuples = pointColors->GetNumberOfTuples();
      }
      else
      {
        vtkDebugWithObjectMacro(mapper, << "Skipping point colors upload");
        meshDataWriter.ByteOffset += sizeof(vtkTypeFloat32) * pointColors->GetDataSize();
      }
    }
    // Upload cell colors
    else if (this->UseCellScalarMapping && cellColors != nullptr)
    {
      if (cellColors->GetMTime() > this->MeshData.BuildTimeStamp)
      {
        meshDataWriter.Denominator = 255.0;
        this->MeshArraysDescriptor.Colors.Start =
          meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(cellColors, meshDataWriter, "CellColors"))
        {
          meshDataWriter(cellColors, "CellColors");
        }
        meshDataWriter.Denominator = 1.0;
        this->MeshArraysDescriptor.Colors.NumComponents = cellColors->GetNumberOfComponents();
        this->MeshArraysDescriptor.Colors.NumTuples = cellColors->GetNumberOfTuples();
      }
      else
      {
        vtkDebugWithObjectMacro(mapper, << "Skipping cell colors upload");
        meshDataWriter.ByteOffset += sizeof(vtkTypeFloat32) * cellColors->GetDataSize();
      }
    }
    if (meshDataWriter.NumberOfBytesWritten > 0)
    {
      // This means something was actually written into a WebGPU buffer.
      this->MeshData.BuildTimeStamp.Modified();
    }
    wgpuConfiguration->WriteBuffer(this->AttributeDescriptorData.Buffer, 0,
      &this->MeshArraysDescriptor, sizeof(this->MeshArraysDescriptor),
      meshAttrDescriptorLabel.c_str());
    this->AttributeDescriptorData.BuildTimeStamp.Modified();

    vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> deviceTextureRc;
    if (auto* propertyKeys = actor->GetPropertyKeys())
    {
      if (propertyKeys->Has(vtkProp::GENERAL_TEXTURE_UNIT()))
      {
        const int textureUnit = propertyKeys->Get(vtkProp::GENERAL_TEXTURE_UNIT());
        if (this->ActorTextureUnit != textureUnit)
        {
          vtkLogF(TRACE, "Texture unit changed from %d to %d", this->ActorTextureUnit, textureUnit);
          this->TextureBindTime.Modified();
          // Update last used texture unit
          this->ActorTextureUnit = textureUnit;
          recreateMeshBindGroup = true;
        }
      }
    }
    deviceTextureRc = wgpuTextureCache->GetRenderTexture(this->ActorTextureUnit);
    if (deviceTextureRc && (this->TextureBindTime < deviceTextureRc->GetMTime()))
    {
      vtkLogF(TRACE, "Texture %d modified, updating bind group", this->ActorTextureUnit);
      this->TextureBindTime.Modified();
      recreateMeshBindGroup = true;
    }
    if (recreateMeshBindGroup)
    {
      const auto& device = wgpuConfiguration->GetDevice();
      const auto& layout = this->MeshAttributeBindGroupLayout =
        this->CreateMeshAttributeBindGroupLayout(
          device, "MeshAttributeBindGroup_LAYOUT", deviceTextureRc);
      std::vector<wgpu::BindGroupEntry> entries;
      const vtkWebGPUBindGroupInternals::BindingInitializationHelper mapper2DStateInitializer{ 0,
        this->Mapper2DStateData.Buffer, 0 };
      entries.push_back(mapper2DStateInitializer.GetAsBinding());
      const vtkWebGPUBindGroupInternals::BindingInitializationHelper attributeDescriptorInitializer{
        1, this->AttributeDescriptorData.Buffer, 0
      };
      entries.push_back(attributeDescriptorInitializer.GetAsBinding());
      const vtkWebGPUBindGroupInternals::BindingInitializationHelper meshDataInitializer{ 2,
        this->MeshData.Buffer, 0 };
      entries.push_back(meshDataInitializer.GetAsBinding());
      if (deviceTextureRc)
      {
        entries.push_back(deviceTextureRc->MakeSamplerBindGroupEntry(3));
        entries.push_back(deviceTextureRc->MakeTextureViewBindGroupEntry(4));
      }
      this->MeshAttributeBindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(
        device, layout, entries, "MeshAttributeBindGroup");
      this->RebuildGraphicsPipelines = true;
      // Invalidate render bundle because bindgroup was recreated.
      wgpuRenderer->InvalidateBundle();
    }
  }

  vtkTypeUInt32* vertexCounts[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  wgpu::Buffer* connectivityBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  wgpu::Buffer* cellIdBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  wgpu::Buffer* edgeArrayBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  wgpu::Buffer*
    cellIdOffsetUniformBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    auto& bgInfo = this->TopologyBindGroupInfos[i];
    vertexCounts[i] = &(bgInfo.VertexCount);
    connectivityBuffers[i] = &(bgInfo.ConnectivityBuffer);
    cellIdBuffers[i] = &(bgInfo.CellIdBuffer);
    edgeArrayBuffers[i] = nullptr;
    cellIdOffsetUniformBuffers[i] = &(bgInfo.CellIdOffsetUniformBuffer);
  }
  bool updateTopologyBindGroup = this->CellConverter->DispatchMeshToPrimitiveComputePipeline(
    wgpuConfiguration, input, VTK_SURFACE, vertexCounts, connectivityBuffers, cellIdBuffers,
    edgeArrayBuffers, cellIdOffsetUniformBuffers);

  // Rebuild topology bind group if required (when VertexCount > 0)
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    const auto topologySourceType = vtkWebGPUCellToPrimitiveConverter::TopologySourceType(i);
    auto& bgInfo = this->TopologyBindGroupInfos[i];
    // setup bind group
    if (updateTopologyBindGroup && bgInfo.VertexCount > 0)
    {
      const std::string& label =
        vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
      const auto& device = wgpuConfiguration->GetDevice();
      bool homogeneousCellSize = bgInfo.CellIdBuffer == nullptr;
      auto layout = this->CreateTopologyBindGroupLayout(
        device, "TopologyBindGroup_LAYOUT", homogeneousCellSize);
      if (homogeneousCellSize)
      {
        bgInfo.BindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
          {
            // clang-format off
          { 0, bgInfo.ConnectivityBuffer, 0},
          { 2, bgInfo.CellIdOffsetUniformBuffer, 0},
            // clang-format on
          },
          "TopologyBindGroup");
      }
      else
      {
        bgInfo.BindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
          {
            // clang-format off
          { 0, bgInfo.ConnectivityBuffer, 0},
          { 1, bgInfo.CellIdBuffer, 0},
            // clang-format on
          },
          "TopologyBindGroup");
      }
    }
    else if (bgInfo.VertexCount == 0)
    {
      if (bgInfo.ConnectivityBuffer)
      {
        bgInfo.ConnectivityBuffer.Destroy();
        bgInfo.ConnectivityBuffer = nullptr;
      }
      if (bgInfo.CellIdBuffer)
      {
        bgInfo.CellIdBuffer.Destroy();
        bgInfo.CellIdBuffer = nullptr;
      }
      if (bgInfo.CellIdOffsetUniformBuffer)
      {
        bgInfo.CellIdOffsetUniformBuffer.Destroy();
        bgInfo.CellIdOffsetUniformBuffer = nullptr;
      }
      bgInfo.BindGroup = nullptr;
    }
    this->RebuildGraphicsPipelines = true;
  }

  if (this->RebuildGraphicsPipelines)
  {
    const auto& device = wgpuConfiguration->GetDevice();
    auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

    vtkWebGPURenderPipelineDescriptorInternals descriptor;
    descriptor.vertex.bufferCount = 0;
    descriptor.vertex.entryPoint = "main";
    descriptor.cFragment.entryPoint = "main";
    descriptor.EnableBlending(0);
    descriptor.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();
    ///@{ TODO: Only for valid depth stencil formats
    auto depthState = descriptor.EnableDepthStencil(wgpuRenderWindow->GetDepthStencilFormat());
    depthState->depthWriteEnabled = true;
    depthState->depthCompare = wgpu::CompareFunction::Less;
    ///@}
    // Prepare selection ids output.
    descriptor.cTargets[1].format = wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat();
    descriptor.cFragment.targetCount++;
    descriptor.DisableBlending(1);

    // Update local parameters that decide whether a pipeline must be rebuilt.
    this->RebuildGraphicsPipelines = false;
    descriptor.primitive.cullMode = wgpu::CullMode::None;

    std::vector<wgpu::BindGroupLayout> basicBGLayouts = { this->MeshAttributeBindGroupLayout };

    for (int i = 0; i < GraphicsPipeline2DType::NUM_GFX_PIPELINE_2D_NB_TYPES; ++i)
    {
      auto pipelineType = GraphicsPipeline2DType(i);
      const bool homogeneousCellSize = IsPipelineForHomogeneousCellSize(pipelineType);
      auto bgls = basicBGLayouts;
      bgls.emplace_back(this->CreateTopologyBindGroupLayout(
        device, "TopologyBindGroupLayout", homogeneousCellSize));
      descriptor.layout = vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(
        device, bgls, "vtkPolyDataMapper2DPipelineLayout");
      descriptor.label = this->GetGraphicsPipelineTypeAsString(pipelineType);
      descriptor.primitive.topology = this->GraphicsPipeline2DPrimitiveTypes[i];
      std::string vertexShaderSource = vtkPolyData2DVSWGSL;
      std::string fragmentShaderSource = vtkPolyData2DFSWGSL;
      this->ApplyShaderReplacements(
        pipelineType, vertexShaderSource, fragmentShaderSource, wgpuRenderWindow, actor);
      // generate a unique key for the pipeline descriptor and shader source pointer
      this->GraphicsPipeline2DKeys[i] = wgpuPipelineCache->GetPipelineKey(
        &descriptor, vertexShaderSource.c_str(), fragmentShaderSource.c_str());
      // create a pipeline if it does not already exist
      if (wgpuPipelineCache->GetRenderPipeline(this->GraphicsPipeline2DKeys[i]) == nullptr)
      {
        wgpuPipelineCache->CreateRenderPipeline(
          &descriptor, wgpuRenderWindow, vertexShaderSource.c_str(), fragmentShaderSource.c_str());
      }
    }
    // Invalidate render bundle because pipeline was recreated.
    wgpuRenderer->InvalidateBundle();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::RecordDrawCommands(
  vtkViewport* viewport, const wgpu::RenderPassEncoder& encoder)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(viewport->GetVTKWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  encoder.SetBindGroup(0, this->MeshAttributeBindGroup);
  for (const auto& pipelineMapping : this->PipelineBindGroupCombos)
  {
    const auto& pipelineType = pipelineMapping.first;
    const auto& pipelineKey = this->GraphicsPipeline2DKeys[pipelineType];
    const auto topologySourceType = pipelineMapping.second;
    const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
    if (bgInfo.VertexCount == 0)
    {
      continue;
    }
    if (IsPipelineForHomogeneousCellSize(pipelineType) && bgInfo.CellIdBuffer != nullptr)
    {
      continue;
    }
    if (!IsPipelineForHomogeneousCellSize(pipelineType) && bgInfo.CellIdBuffer == nullptr)
    {
      continue;
    }

    encoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
    const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
    vtkScopedEncoderDebugGroup(encoder, pipelineLabel);

    encoder.SetBindGroup(1, bgInfo.BindGroup);
    const auto topologyBGInfoName =
      vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
    vtkScopedEncoderDebugGroup(encoder, topologyBGInfoName);
    switch (topologySourceType)
    {
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
        encoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
        break;
      case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
      default:
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::RecordDrawCommands(
  vtkViewport* viewport, const wgpu::RenderBundleEncoder& encoder)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(viewport->GetVTKWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  encoder.SetBindGroup(0, this->MeshAttributeBindGroup);
  for (const auto& pipelineMapping : this->PipelineBindGroupCombos)
  {
    const auto& pipelineType = pipelineMapping.first;
    const auto& pipelineKey = this->GraphicsPipeline2DKeys[pipelineType];
    const auto topologySourceType = pipelineMapping.second;
    const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
    if (bgInfo.VertexCount == 0)
    {
      continue;
    }
    if (IsPipelineForHomogeneousCellSize(pipelineType) && bgInfo.CellIdBuffer != nullptr)
    {
      continue;
    }
    if (!IsPipelineForHomogeneousCellSize(pipelineType) && bgInfo.CellIdBuffer == nullptr)
    {
      continue;
    }

    encoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
    const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
    vtkScopedEncoderDebugGroup(encoder, pipelineLabel);

    encoder.SetBindGroup(1, bgInfo.BindGroup);
    const auto topologyBGInfoName =
      vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
    vtkScopedEncoderDebugGroup(encoder, topologyBGInfoName);
    switch (topologySourceType)
    {
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
        encoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
        break;
      case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
      default:
        break;
    }
  }
}

VTK_ABI_NAMESPACE_END
