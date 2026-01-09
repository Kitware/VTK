// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUGlyph3DMapper.h"
#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkColor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkGlyph3DMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkQuaternion.h"
#include "vtkRenderer.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"

#include <sstream>
#include <stack>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUGlyph3DMapperHelper : public vtkWebGPUPolyDataMapper
{
public:
  static vtkWebGPUGlyph3DMapperHelper* New()
  {
    VTK_STANDARD_NEW_BODY(vtkWebGPUGlyph3DMapperHelper);
  }
  vtkTypeMacro(vtkWebGPUGlyph3DMapperHelper, vtkWebGPUPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

  void Initialize(vtkPolyData* mesh, int numPoints, std::vector<vtkTypeFloat32>* colors,
    std::vector<vtkTypeFloat32>* transforms, std::vector<vtkTypeFloat32>* normalTransforms,
    vtkTypeUInt32 flatIndex, bool pickable, vtkMTimeType buildMTime)
  {
    this->CurrentInput = this->CachedInput = mesh;
    this->NumberOfGlyphPoints = numPoints;
    this->InstanceColors = colors;
    this->InstanceTransforms = transforms;
    this->InstanceNormalTransforms = normalTransforms;
    if (flatIndex != this->FlatIndex)
    {
      this->PickingAttributesModified = true;
      this->FlatIndex = flatIndex;
    }
    if (pickable != this->Pickable)
    {
      this->PickingAttributesModified = true;
      this->Pickable = pickable;
    }
    this->GlyphStructuresBuildTime = buildMTime;
  }

  void RenderPiece(vtkRenderer* renderer, vtkActor* actor) override
  {
    auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
    auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

    const std::string label = "InstanceProperties-" + this->CurrentInput->GetObjectDescription();
    if (this->InstancePropertiesBuffer == nullptr)
    {
      this->InstancePropertiesBuffer = wgpuConfiguration->CreateBuffer(sizeof(InstanceProperties),
        wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
        /*mappedAtCreation=*/false, label.c_str());
      // Rebuild pipeline and bindgroups when buffer is re-created.
      this->RebuildGraphicsPipelines = true;
    }

    auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
    switch (wgpuRenderer->GetRenderStage())
    {
      case vtkWebGPURenderer::RenderStageEnum::SyncDeviceResources:
        this->UpdateInstanceAttributeBuffers(wgpuConfiguration);
        break;
      default:
        break;
    }
    this->Superclass::RenderPiece(renderer, actor);
    if (this->PickingAttributesModified)
    {
      this->UpdateInstancePropertiesBuffer(wgpuConfiguration);
    }
  }

  std::vector<wgpu::VertexBufferLayout> GetVertexBufferLayouts() override
  {
    // matCxR types are not allowed as vertex attributes.
    // For this reason the columns of the matrices are
    // sent as vertex attributes and the shader assembles
    // matrices from the individual columns.
    std::size_t instanceAttributesIdx = 0;
    std::vector<wgpu::VertexBufferLayout> layouts;
    {
      wgpu::VertexBufferLayout layout = {};
      layout.arrayStride = 4 * sizeof(vtkTypeFloat32);
      layout.attributeCount = 1;
      layout.attributes = &this->InstanceAttributes[instanceAttributesIdx];
      layout.stepMode = wgpu::VertexStepMode::Instance;
      layouts.emplace_back(layout);
      instanceAttributesIdx += 1;
    }
    {
      wgpu::VertexBufferLayout layout = {};
      layout.arrayStride = 4 * 4 * sizeof(vtkTypeFloat32);
      layout.attributeCount = 4; // 1 attribute per column which is a vec4f
      layout.attributes = &this->InstanceAttributes[instanceAttributesIdx];
      layout.stepMode = wgpu::VertexStepMode::Instance;
      layouts.emplace_back(layout);
      instanceAttributesIdx += 4;
    }
    {
      wgpu::VertexBufferLayout layout = {};
      layout.arrayStride = 3 * 3 * sizeof(vtkTypeFloat32);
      layout.attributeCount = 3; // 1 attribute per column which is a vec3f
      layout.attributes = &this->InstanceAttributes[instanceAttributesIdx];
      layout.stepMode = wgpu::VertexStepMode::Instance;
      layouts.emplace_back(layout);
      instanceAttributesIdx += 3;
    }
    return layouts;
  }

  /**
   * Overriden to pass instance attribtue buffers into the vertex buffer slots.
   */
  void SetVertexBuffers(const wgpu::RenderPassEncoder& encoder) override
  {
    for (int attributeIndex = 0; attributeIndex < InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES;
         ++attributeIndex)
    {
      encoder.SetVertexBuffer(
        attributeIndex, this->InstanceAttributesBuffers[attributeIndex].Buffer);
    }
  }

  /**
   * Overriden to pass instance attribtue buffers into the vertex buffer slots.
   */
  void SetVertexBuffers(const wgpu::RenderBundleEncoder& encoder) override
  {
    for (int attributeIndex = 0; attributeIndex < InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES;
         ++attributeIndex)
    {
      encoder.SetVertexBuffer(
        attributeIndex, this->InstanceAttributesBuffers[attributeIndex].Buffer);
    }
  }

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override
  {
    // Release mesh buffers, bind groups and reset the attribute build timestamps.
    for (int attributeIndex = 0; attributeIndex < InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES;
         ++attributeIndex)
    {
      this->InstanceAttributesBuffers[attributeIndex] = {};
      this->InstanceAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
    }
    this->InstancePropertiesBuffer = nullptr;
    this->RebuildGraphicsPipelines = true;
    this->Superclass::ReleaseGraphicsResources(window);
  }

  /**
   * All the attributes supported by the instance data buffer
   */
  enum InstanceDataAttributes : int
  {
    INSTANCE_COLORS = 0,
    INSTANCE_TRANSFORMS,
    INSTANCE_NORMAL_TRANSFORMS,
    NUM_INSTANCE_ATTRIBUTES,
    INSTANCE_UNDEFINED
  };

  /**
   * Returns the size of the 'sub-buffer' within the whole cell data SSBO for the given attribute
   */
  unsigned long GetInstanceAttributeByteSize(
    vtkWebGPUGlyph3DMapperHelper::InstanceDataAttributes attribute)
  {
    switch (attribute)
    {
      case InstanceDataAttributes::INSTANCE_COLORS:
        if (this->InstanceColors)
        {
          return this->InstanceColors->size() * sizeof(vtkTypeFloat32);
        }

        break;

      case InstanceDataAttributes::INSTANCE_TRANSFORMS:
        if (this->InstanceTransforms)
        {
          return this->InstanceTransforms->size() * sizeof(vtkTypeFloat32);
        }

        break;

      case InstanceDataAttributes::INSTANCE_NORMAL_TRANSFORMS:
        if (this->InstanceNormalTransforms)
        {
          return this->InstanceNormalTransforms->size() * sizeof(vtkTypeFloat32);
        }

        break;

      default:
        break;
    }

    return 0;
  }

  /**
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the cell attributes. See
   * vtkWebGPUGlyph3DMapperHelper::InstanceDataAttributes for the kinds of attributes.
   */
  unsigned long GetExactInstanceBufferSize(InstanceDataAttributes attribute)
  {
    unsigned long result = 0;
    switch (attribute)
    {
      case INSTANCE_COLORS:
        result = this->GetInstanceAttributeByteSize(InstanceDataAttributes::INSTANCE_COLORS);
        break;
      case INSTANCE_TRANSFORMS:
        result = this->GetInstanceAttributeByteSize(InstanceDataAttributes::INSTANCE_TRANSFORMS);
        break;
      case INSTANCE_NORMAL_TRANSFORMS:
        result =
          this->GetInstanceAttributeByteSize(InstanceDataAttributes::INSTANCE_NORMAL_TRANSFORMS);
        break;
      case NUM_INSTANCE_ATTRIBUTES:
      case INSTANCE_UNDEFINED:
        break;
    }

    result = vtkWebGPUConfiguration::Align(result, 32);
    return result;
  }

protected:
  vtkWebGPUGlyph3DMapperHelper()
  {
    std::uint32_t shaderLocation = 0;
    this->InstanceAttributes[shaderLocation].nextInChain = nullptr;
    this->InstanceAttributes[shaderLocation].format = wgpu::VertexFormat::Float32x4;
    this->InstanceAttributes[shaderLocation].offset = 0;
    this->InstanceAttributes[shaderLocation].shaderLocation = shaderLocation;
    shaderLocation++;

    // matCxR types are not allowed as vertex attributes.
    // For this reason the columns of the matrices are
    // sent as vertex attributes and the shader assembles
    // matrices from the individual columns.
    for (int i = 0; i < 4; ++i)
    {
      this->InstanceAttributes[shaderLocation].nextInChain = nullptr;
      this->InstanceAttributes[shaderLocation].format = wgpu::VertexFormat::Float32x4;
      this->InstanceAttributes[shaderLocation].offset = i * 4 * sizeof(float);
      this->InstanceAttributes[shaderLocation].shaderLocation = shaderLocation;
      shaderLocation++;
    }

    for (int i = 0; i < 3; ++i)
    {
      this->InstanceAttributes[shaderLocation].nextInChain = nullptr;
      this->InstanceAttributes[shaderLocation].format = wgpu::VertexFormat::Float32x3;
      this->InstanceAttributes[shaderLocation].offset = i * 3 * sizeof(float);
      this->InstanceAttributes[shaderLocation].shaderLocation = shaderLocation;
      shaderLocation++;
    }
  }
  ~vtkWebGPUGlyph3DMapperHelper() override = default;

  struct InstanceProperties
  {
    vtkTypeUInt32 CompositeId;
    vtkTypeUInt32 Pickable;
    vtkTypeUInt32 ProcessId;
  };
  wgpu::Buffer InstancePropertiesBuffer;
  AttributeBuffer InstanceAttributesBuffers[NUM_INSTANCE_ATTRIBUTES];
  wgpu::VertexAttribute InstanceAttributes[1 + 4 + 3]; // matrices sent as column vectors

  vtkTimeStamp InstanceAttributesBuildTimestamp[NUM_INSTANCE_ATTRIBUTES];

  std::uint32_t NumberOfGlyphPoints = 0;
  std::vector<vtkTypeFloat32>* InstanceColors;
  std::vector<vtkTypeFloat32>* InstanceTransforms;
  std::vector<vtkTypeFloat32>* InstanceNormalTransforms;
  vtkTypeUInt32 FlatIndex = 0;
  bool Pickable = false;
  bool PickingAttributesModified = false;
  vtkMTimeType GlyphStructuresBuildTime = 0;

  /**
   * Order in which the instance data attributes are concatenated into the mapper mesh SSBO
   */
  const InstanceDataAttributes
    InstanceDataAttributesOrder[InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES] = {
      InstanceDataAttributes::INSTANCE_COLORS, InstanceDataAttributes::INSTANCE_TRANSFORMS,
      InstanceDataAttributes::INSTANCE_NORMAL_TRANSFORMS
    };

  std::vector<wgpu::BindGroupLayoutEntry> GetMeshBindGroupLayoutEntries() override
  {
    // extend superclass bindings with additional entry for `Mesh` buffer.
    auto entries = this->Superclass::GetMeshBindGroupLayoutEntries();
    std::uint32_t bindingId = entries.size();

    entries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
      bindingId++, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
      wgpu::BufferBindingType::Uniform });
    return entries;
  }

  std::vector<wgpu::BindGroupEntry> GetMeshBindGroupEntries() override
  {
    // extend superclass bindings with additional entry for `Mesh` buffer.
    auto entries = this->Superclass::GetMeshBindGroupEntries();
    std::uint32_t bindingId = entries.size();

    const auto bindingInit = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++,
      this->InstancePropertiesBuffer, 0 };
    entries.emplace_back(bindingInit.GetAsBinding());
    return entries;
  }

  void UpdateInstanceAttributeBuffers(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration)
  {
    const char* instanceAttribLabels[InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES] = {
      "instance_colors", "instanceNormals", "instance_normal_transforms"
    };
    for (int attributeIndex = 0; attributeIndex < InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES;
         ++attributeIndex)
    {
      uint64_t currentBufferSize = 0;
      const uint64_t requiredBufferSize =
        this->GetExactInstanceBufferSize(static_cast<InstanceDataAttributes>(attributeIndex));
      if (this->InstanceAttributesBuffers[attributeIndex].Buffer)
      {
        currentBufferSize = this->InstanceAttributesBuffers[attributeIndex].Size;
      }
      if (currentBufferSize != requiredBufferSize)
      {
        if (this->InstanceAttributesBuffers[attributeIndex].Buffer)
        {
          this->InstanceAttributesBuffers[attributeIndex].Buffer.Destroy();
          this->InstanceAttributesBuffers[attributeIndex].Size = 0;
        }
        wgpu::BufferDescriptor descriptor{};
        descriptor.size = requiredBufferSize;
        const auto label = instanceAttribLabels[attributeIndex] + std::string("-") +
          this->CurrentInput->GetObjectDescription();
        descriptor.label = label.c_str();
        descriptor.mappedAtCreation = false;
        descriptor.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
        this->InstanceAttributesBuffers[attributeIndex].Buffer =
          wgpuConfiguration->CreateBuffer(descriptor);
        this->InstanceAttributesBuffers[attributeIndex].Size = requiredBufferSize;
        // invalidate timestamp
        this->InstanceAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
        this->RebuildGraphicsPipelines = true;
      }
      switch (InstanceDataAttributesOrder[attributeIndex])
      {
        case INSTANCE_COLORS:
          if (this->InstanceColors &&
            this->GlyphStructuresBuildTime > this->InstanceAttributesBuildTimestamp[attributeIndex])
          {
            wgpuConfiguration->WriteBuffer(this->InstanceAttributesBuffers[attributeIndex].Buffer,
              0, this->InstanceColors->data(),
              this->InstanceColors->size() * sizeof(vtkTypeFloat32),
              instanceAttribLabels[attributeIndex]);
            this->InstanceAttributesBuildTimestamp[attributeIndex].Modified();
          }
          break;
        case INSTANCE_TRANSFORMS:
          if (this->InstanceTransforms &&
            this->GlyphStructuresBuildTime > this->InstanceAttributesBuildTimestamp[attributeIndex])
          {
            wgpuConfiguration->WriteBuffer(this->InstanceAttributesBuffers[attributeIndex].Buffer,
              0, this->InstanceTransforms->data(),
              this->InstanceTransforms->size() * sizeof(vtkTypeFloat32),
              instanceAttribLabels[attributeIndex]);
            this->InstanceAttributesBuildTimestamp[attributeIndex].Modified();
          }
          break;
        case INSTANCE_NORMAL_TRANSFORMS:
          if (this->InstanceNormalTransforms &&
            this->GlyphStructuresBuildTime > this->InstanceAttributesBuildTimestamp[attributeIndex])
          {
            wgpuConfiguration->WriteBuffer(this->InstanceAttributesBuffers[attributeIndex].Buffer,
              0, this->InstanceNormalTransforms->data(),
              this->InstanceNormalTransforms->size() * sizeof(vtkTypeFloat32),
              instanceAttribLabels[attributeIndex]);
            this->InstanceAttributesBuildTimestamp[attributeIndex].Modified();
          }
          break;
        case NUM_INSTANCE_ATTRIBUTES:
        case INSTANCE_UNDEFINED:
          break;
      }
    }
  }

  void UpdateInstancePropertiesBuffer(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration)
  {
    InstanceProperties instanceProperties = {};
    instanceProperties.CompositeId = this->FlatIndex;
    instanceProperties.Pickable = this->Pickable ? 1u : 0u;
    instanceProperties.ProcessId = 1;
    wgpuConfiguration->WriteBuffer(this->InstancePropertiesBuffer, 0, &instanceProperties,
      sizeof(InstanceProperties), "InstanceProperties");
  }

  // Defines parametric coordinates for a TriangleList (6 elements) instead of TriangleStrip (4
  // elements) because we use the instance_id for glyphing.
  void ReplaceShaderConstantsDef(GraphicsPipelineType pipelineType, vtkWebGPURenderer* wgpuRenderer,
    vtkWebGPUActor* wgpuActor, std::string& vss, std::string& fss) override
  {
    std::string code;
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      {
        code = R"(
/**
* (-1, 1) |-------------------------------|(1, 1)
*         |-                              |
*         |    -                          |
*         |        -                      |
* (-1, 0) |              -                |
*         |                   -           |
*         |                        -      |
*         |                              -|
* (-1,-1) |-------------------------------|(1, -1)
*/
// this triangle strip describes a quad spanning a bi-unit domain.
const TRIANGLE_VERTS = array(
  vec2f(-1, -1),
  vec2f(1, -1),
  vec2f(-1, 1),
  vec2f(-1, 1),
  vec2f(1, -1),
  vec2f(1, 1),
);)";
        break;
      }
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      {
        code = R"(
  /**
    * (0, 0.5) |-------------------------------|(1, 0.5)
    *          |-                              |
    *          |    -                          |
    *          |        -                      |
    * (0, 0)   |              -                |
    *          |                   -           |
    *          |                        -      |
    *          |                              -|
    * (0,-0.5) |-------------------------------|(1, -0.5)
    */
  const TRIANGLE_VERTS = array(
    vec2(0, -0.5),
    vec2(1, -0.5),
    vec2(0, 0.5),
    vec2(0, 0.5),
    vec2(1, -0.5),
    vec2(1, 0.5),
  );)";
        break;
      }
      default:
        break;
    }
    if (!code.empty())
    {
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Constants::Def", code, /*all=*/true);
    }
    else
    {
      this->Superclass::ReplaceShaderConstantsDef(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
    }
  }

  void ReplaceShaderCustomDef(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss, std::string& fss) override
  {
    const std::string code = R"(struct InstanceProperties
{
  composite_id: u32,
  pickable: u32,
  process_id: u32,
};)";
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Custom::Def", code,
      /*all=*/false);
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Custom::Def", code,
      /*all=*/false);
  }

  void ReplaceShaderCustomBindings(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss, std::string& fss) override
  {
    auto& bindingId = this->NumberOfBindings[GROUP_MESH];
    std::stringstream codeStream;
    codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
               << ") var<uniform> instance_properties: InstanceProperties;\n";
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Custom::Bindings", codeStream.str(),
      /*all=*/false);
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Custom::Bindings", codeStream.str(),
      /*all=*/false);
  }

  void ReplaceVertexShaderInputDef(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss) override
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexInput::Def", R"(struct VertexInput
{
  @location(0) color: vec4f,
  @location(1) glyph_transform_row_1: vec4f,
  @location(2) glyph_transform_row_2: vec4f,
  @location(3) glyph_transform_row_3: vec4f,
  @location(4) glyph_transform_row_4: vec4f,
  @location(5) glyph_normal_transform_row_1: vec3f,
  @location(6) glyph_normal_transform_row_2: vec3f,
  @location(7) glyph_normal_transform_row_3: vec3f,
  @builtin(instance_index) instance_id: u32,
  @builtin(vertex_index) vertex_id: u32
};)",
      /*all=*/true);
  }

  void ReplaceVertexShaderCamera(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss) override
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Camera::Impl",
      R"(let glyph_transform = mat4x4<f32>(
          vertex.glyph_transform_row_1,
          vertex.glyph_transform_row_2,
          vertex.glyph_transform_row_3,
          vertex.glyph_transform_row_4,
        );
  let model_view_projection = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform;)",
      /*all=*/true);
  }

  void ReplaceVertexShaderNormalTransform(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss) override
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::NormalTransform::Impl",
      R"(let glyph_normal_transform = mat3x3<f32>(
        vertex.glyph_normal_transform_row_1,
        vertex.glyph_normal_transform_row_2,
        vertex.glyph_normal_transform_row_3,
        );
  let normal_model_view = scene_transform.normal * actor.transform.normal * glyph_normal_transform;)",
      /*all=*/true);
  }

  //------------------------------------------------------------------------------
  void ReplaceVertexShaderVertexId(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss) override
  {
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS:
      case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
          R"(let pull_vertex_id: u32 = vertex.vertex_id;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
          R"(let pull_vertex_id: u32 = vertex.vertex_id / 6;
  let p_coord_id = vertex.vertex_id % 6;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_LINES:
      case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
          R"(let line_id: u32 = vertex.vertex_id / 2;
  let pull_vertex_id: u32 = vertex.vertex_id;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
          R"(let line_id: u32 = vertex.vertex_id / 6;
  let p_coord_id = vertex.vertex_id % 6;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
          R"(let line_id: u32 = vertex.vertex_id / 36;
  let p_coord_id = vertex.vertex_id % 36;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
          R"(let line_id: u32 = vertex.vertex_id / 6;
  let p_coord_id = vertex.vertex_id % 6;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_TRIANGLES:
      case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
          R"(let pull_vertex_id: u32 = vertex.vertex_id;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_NB_TYPES:
        break;
    }
  }

  //------------------------------------------------------------------------------
  void ReplaceVertexShaderPrimitiveId(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* wgpuRenderer, vtkWebGPUActor* wgpuActor, std::string& vss) override
  {
    switch (pipelineType)
    {
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
        // Superclass assigns vertex.instance_id to primitive_id,
        // however this mapper uses instance_id to denote multiple glyphs.
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::PrimitiveId::Impl",
          R"(let primitive_id: u32 = line_id;
  let primitive_size: u32 = 2u;)",
          /*all=*/true);
        break;
      default:
        this->Superclass::ReplaceVertexShaderPrimitiveId(
          pipelineType, wgpuRenderer, wgpuActor, vss);
        break;
    }
  }

  void ReplaceVertexShaderPicking(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss) override
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Picking::Impl",
      R"(if (instance_properties.pickable == 1u)
  {
    // Write indices
    output.cell_id = cell_id;
    output.prop_id = actor.color_options.id;
    output.composite_id = instance_properties.composite_id;
    output.process_id = instance_properties.process_id;
  })",
      /*all=*/true);
  }

  void ReplaceVertexShaderColors(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& vss) override
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Colors::Impl",
      "output.color = vertex.color;",
      /*all=*/true);
  }

  void ReplaceFragmentShaderColors(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& fss) override
  {
    std::string basicColorFSImpl = R"(var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
    var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
    var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);
    var opacity: f32;
    ambient_color = vertex.color.rgb;
    diffuse_color = vertex.color.rgb;
    opacity = vertex.color.a;
  )";
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS:
      case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl",
          basicColorFSImpl +
            R"(// Colors are acquired either from a global per-actor color, or from per-vertex colors, or from cell colors.
    let show_vertices = getVertexVisibility(actor.render_options.flags);
    if (show_vertices)
    {
      // use vertex color instead of point scalar colors when drawing vertices.
      ambient_color = actor.color_options.vertex_color;
      diffuse_color = actor.color_options.vertex_color;
      opacity = actor.color_options.opacity;
    })",
          /*all=*/true);
        break;
      case GFX_PIPELINE_LINES:
      case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_TRIANGLES:
      case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl", basicColorFSImpl,
          /*all=*/true);
        break;
      case GFX_PIPELINE_NB_TYPES:
        break;
    }
  }

  void ReplaceFragmentShaderPicking(GraphicsPipelineType vtkNotUsed(pipelineType),
    vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
    std::string& fss) override
  {
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Picking::Impl",
      R"(if (instance_properties.pickable == 1u)
  {
    output.ids.x = vertex.cell_id + 1;
    output.ids.y = vertex.prop_id + 1;
    output.ids.z = vertex.composite_id + 1;
    output.ids.w = vertex.process_id + 1;
  })",
      /*all=*/true);
  }

  // Uses TriangleList for pipeline types that originally used TriangleStrip
  // because we use the instance_id for glyphing.
  wgpu::PrimitiveTopology GetPrimitiveTopologyForPipeline(
    GraphicsPipelineType pipelineType) override
  {
    wgpu::PrimitiveTopology topology = wgpu::PrimitiveTopology::Undefined;
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
        topology = wgpu::PrimitiveTopology::TriangleList;
        break;
      default:
        topology = this->Superclass::GetPrimitiveTopologyForPipeline(pipelineType);
        break;
    }
    return topology;
  }

  vtkWebGPUPolyDataMapper::DrawCallArgs GetDrawCallArgs(GraphicsPipelineType pipelineType,
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType) override
  {
    const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
    switch (topologySourceType)
    {
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS:
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS:
        if (pipelineType == GFX_PIPELINE_POINTS ||
          pipelineType == GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE)
        {
          return { /*vertexCount=*/bgInfo.VertexCount,
            /*instanceCount=*/this->NumberOfGlyphPoints };
        }
        if (pipelineType == GFX_PIPELINE_POINTS_SHAPED ||
          pipelineType == GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE)
        {
          // ReplaceShaderConstantsDef declares a quad with two triangles
          // when pipeline is specialized for shaped points.
          // total 6 imposter vertices
          return { /*vertexCount=*/6 * bgInfo.VertexCount,
            /*instanceCount=*/this->NumberOfGlyphPoints };
        }
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES:
        if (pipelineType == GFX_PIPELINE_LINES ||
          pipelineType == GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE)
        {
          return { /*vertexCount=*/bgInfo.VertexCount,
            /*instanceCount=*/this->NumberOfGlyphPoints };
        }
        // ReplaceShaderConstantsDef declares a quad with two triangles
        // when pipeline is specialized for thick lines and miter joined lines.
        // total 6 imposter vertices, but each line has two source vertices, so divide
        // by 2.
        // effectively, there are total 3 imposter vertices
        if (pipelineType == GFX_PIPELINE_LINES_THICK ||
          pipelineType == GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE)
        {
          return { /*vertexCount=*/3 * bgInfo.VertexCount,
            /*instanceCount=*/this->NumberOfGlyphPoints };
        }
        if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN ||
          pipelineType == GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE)
        {
          return { /*vertexCount=*/3 * bgInfo.VertexCount,
            /*instanceCount=*/this->NumberOfGlyphPoints };
        }
        // Similar logic for effective total no. of imposter verts
        if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN ||
          pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE)
        {
          return { /*vertexCount=*/18 * bgInfo.VertexCount,
            /*instanceCount=*/this->NumberOfGlyphPoints };
        }
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
        return { /*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/this->NumberOfGlyphPoints };
      case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
      default:
        break;
    }
    return {};
  }

  vtkWebGPUPolyDataMapper::DrawCallArgs GetDrawCallArgsForDrawingVertices(
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType) override
  {
    // See comment in GetDrawCallArgs for explaination of 6 imposter verts.
    const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
    return { /*VertexCount=*/6 * bgInfo.VertexCount, /*InstanceCount=*/this->NumberOfGlyphPoints };
  }

private:
  vtkWebGPUGlyph3DMapperHelper(const vtkWebGPUGlyph3DMapperHelper&) = delete;
  void operator=(const vtkWebGPUGlyph3DMapperHelper&) = delete;
};

#define vtkInternalsDebugMacro(x) vtkDebugWithObjectMacro(this->Self, x)
#define vtkInternalsWarningMacro(x) vtkWarningWithObjectMacro(this->Self, x)
#define vtkInternalsErrorMacro(x) vtkErrorWithObjectMacro(this->Self, x)

class vtkWebGPUGlyph3DMapper::vtkInternals
{
  vtkWebGPUGlyph3DMapper* Self;

  class vtkColorMapper : public vtkMapper
  {
  public:
    vtkTypeMacro(vtkColorMapper, vtkMapper);
    static vtkColorMapper* New() { VTK_STANDARD_NEW_BODY(vtkColorMapper); }
    void Render(vtkRenderer*, vtkActor*) override {}
    vtkUnsignedCharArray* GetColors() { return this->Colors; }
  };

  struct GlyphParameters
  {
    // As many as the no. of points on the input dataset which are glyphed with source.
    std::vector<vtkTypeFloat32> Colors;
    std::vector<vtkTypeFloat32> Transforms;       // transposed
    std::vector<vtkTypeFloat32> NormalTransforms; // transposed
    vtkTimeStamp BuildTime;
    // May be polydata or composite dataset:
    vtkSmartPointer<vtkDataObject> SourceDataObject;
    // maps composite dataset flat index to polydatamapper. Key = -1 for polydata
    // DataObject.
    typedef std::map<int, vtkSmartPointer<vtkWebGPUGlyph3DMapperHelper>> MapperMap;
    MapperMap Mappers;
    int NumberOfPoints;
  };

  struct GlyphParametersCollection
  {
    // No. of entries is equal to number of source data objects.
    std::vector<std::unique_ptr<GlyphParameters>> Entries;
    vtkTimeStamp BuildTime;
  };

  struct RenderBlockState
  {
    std::stack<double> Opacity;
    std::stack<bool> Visibility;
    std::stack<bool> Pickability;
    std::stack<vtkColor3d> Color;
  };

  // No. of items is equal to number of input data sets. (composite datasets are expanded into
  // leaves)
  std::map<const vtkDataSet*, std::shared_ptr<GlyphParametersCollection>> GlyphInputDataSets;
  // Last time BlockAttributes was modified.
  vtkMTimeType BlockMTime;
  vtkNew<vtkColorMapper> ColorMapper;
  RenderBlockState BlockState;

public:
  vtkInternals(vtkWebGPUGlyph3DMapper* self)
    : Self(self)
  {
  }

  //------------------------------------------------------------------------------
  int GetNumberOfChildren(vtkDataObjectTree* tree)
  {
    int result = 0;
    if (tree)
    {
      auto it = vtk::TakeSmartPointer(tree->NewTreeIterator());
      it->SetTraverseSubTree(false);
      it->SetVisitOnlyLeaves(false);
      for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
      {
        ++result;
      }
    }
    return result;
  }

  //------------------------------------------------------------------------------
  vtkDataObject* GetChildDataObject(vtkDataObjectTree* tree, std::size_t child)
  {
    vtkDataObject* result = nullptr;
    if (tree)
    {
      auto it = vtk::TakeSmartPointer(tree->NewTreeIterator());
      it->SetTraverseSubTree(false);
      it->SetVisitOnlyLeaves(false);
      it->InitTraversal();
      for (std::size_t i = 0; i < child; ++i)
      {
        it->GoToNextItem();
      }
      result = it->GetCurrentDataObject();
    }
    return result;
  }

  //------------------------------------------------------------------------------
  void Render(vtkRenderer* renderer, vtkActor* actor, vtkDataObject* inputDataObject)
  {
    // Render the input dataset or every dataset within the input composite dataset.
    this->BlockMTime = this->Self->BlockAttributes ? this->Self->BlockAttributes->GetMTime() : 0;

    if (auto* inputDataSet = vtkDataSet::SafeDownCast(inputDataObject))
    {
      this->RenderDataSet(renderer, actor, inputDataSet, 0, true);
    }
    else if (auto* inputCompositeDataSet = vtkCompositeDataSet::SafeDownCast(inputDataObject))
    {
      vtkNew<vtkActor> blockAct;
      vtkNew<vtkProperty> blockProp;
      blockAct->ShallowCopy(actor);
      blockProp->DeepCopy(blockAct->GetProperty());
      blockAct->SetProperty(blockProp.GetPointer());
      double origColor[4];
      blockProp->GetColor(origColor);

      // Push base-values on the state stack.
      this->BlockState.Visibility.push(true);
      this->BlockState.Pickability.push(true);
      this->BlockState.Opacity.push(blockProp->GetOpacity());
      this->BlockState.Color.emplace(origColor);

      unsigned int flatIndex = 0;
      this->RenderChildren(renderer, blockAct, inputCompositeDataSet, flatIndex);

      // Pop base-values from the state stack.
      this->BlockState.Visibility.pop();
      this->BlockState.Pickability.pop();
      this->BlockState.Opacity.pop();
      this->BlockState.Color.pop();
    }
  }

  //------------------------------------------------------------------------------
  void RenderDataSet(vtkRenderer* renderer, vtkActor* actor, vtkDataSet* inputDataSet,
    unsigned int flatIndex, bool pickable)
  {
    const auto numPoints = inputDataSet->GetNumberOfPoints();
    if (numPoints < 1)
    {
      vtkInternalsDebugMacro(<< "Cannot glyph because there are no points in the input dataset!");
      return;
    }

    // make sure we have glyph parameters for this dataset.
    bool rebuild = false;
    std::shared_ptr<GlyphParametersCollection> glyphParametersCollection;
    auto glyphParametersFound = this->GlyphInputDataSets.find(inputDataSet);
    if (glyphParametersFound == this->GlyphInputDataSets.end())
    {
      glyphParametersCollection = std::make_shared<GlyphParametersCollection>();
      this->GlyphInputDataSets.insert(std::make_pair(inputDataSet, glyphParametersCollection));
      rebuild = true;
    }
    else
    {
      glyphParametersCollection = glyphParametersFound->second;
    }

    // make sure there are entries for each source dataobject.
    auto* sourceTableTree = this->Self->GetSourceTableTree();
    const int sttSize = this->GetNumberOfChildren(sourceTableTree);
    const int numSourceDataSets = this->Self->GetNumberOfInputConnections(1);
    const std::size_t numberOfSources =
      this->Self->UseSourceTableTree ? sttSize : numSourceDataSets;
    bool numberOfSourcesChanged = false;
    if (numberOfSources != glyphParametersCollection->Entries.size())
    {
      glyphParametersCollection->Entries.clear();
      glyphParametersCollection->Entries.reserve(numberOfSources);
      for (std::size_t i = 0; i < numberOfSources; ++i)
      {
        glyphParametersCollection->Entries.emplace_back(new GlyphParameters());
      }
      numberOfSourcesChanged = true;
    }

    // make sure sources are up to date.
    vtkSmartPointer<vtkDataObjectTreeIterator> sttIterator;
    // when a source table tree is present, iterate over all sources and update our cache.
    if (sourceTableTree)
    {
      sttIterator = vtk::TakeSmartPointer(sourceTableTree->NewTreeIterator());
      sttIterator->SetTraverseSubTree(false);
      sttIterator->SetVisitOnlyLeaves(false);
      sttIterator->InitTraversal();
    }
    for (std::size_t i = 0; i < glyphParametersCollection->Entries.size(); ++i)
    {
      // for each source data object
      auto* sourceDataObject = this->Self->UseSourceTableTree ? sttIterator->GetCurrentDataObject()
                                                              : this->Self->GetSource(i);
      auto& glyphParameters = glyphParametersCollection->Entries[i];
      if (glyphParameters->SourceDataObject &&
        !glyphParameters->SourceDataObject->IsA(sourceDataObject->GetClassName()))
      {
        glyphParameters->SourceDataObject = nullptr;
      }
      if (!glyphParameters->SourceDataObject)
      {
        glyphParameters->SourceDataObject = vtk::TakeSmartPointer(sourceDataObject->NewInstance());
      }
      if (numberOfSourcesChanged ||
        sourceDataObject->GetMTime() > glyphParameters->SourceDataObject->GetMTime() ||
        this->Self->GetMTime() > glyphParameters->BuildTime)
      {
        glyphParameters->SourceDataObject->ShallowCopy(sourceDataObject);
      }

      // Create the individual mappers which render the source data object.
      vtkSmartPointer<vtkCompositeDataIterator> sourceCompositeDataIterator;
      if (auto* sourceCompositeDataSet =
            vtkCompositeDataSet::SafeDownCast(glyphParameters->SourceDataObject))
      {
        sourceCompositeDataIterator = sourceCompositeDataSet->NewIterator();
        sourceCompositeDataIterator->InitTraversal();
      }

      while (true)
      {
        vtkSmartPointer<vtkWebGPUGlyph3DMapperHelper> mapper;
        int mapperIdx = sourceCompositeDataIterator
          ? static_cast<int>(sourceCompositeDataIterator->GetCurrentFlatIndex())
          : -1;
        auto mapperFound = glyphParameters->Mappers.find(mapperIdx);
        if (mapperFound == glyphParameters->Mappers.end())
        {
          mapper = vtk::TakeSmartPointer(vtkWebGPUGlyph3DMapperHelper::New());
          glyphParameters->Mappers.insert(std::make_pair(mapperIdx, mapper));
        }
        else
        {
          mapper = mapperFound->second;
        }
        this->CopyInformationToSubMapper(mapper);

        if (sourceCompositeDataIterator)
        {
          sourceCompositeDataIterator->GoToNextItem();
        }
        if (!sourceCompositeDataIterator || sourceCompositeDataIterator->IsDoneWithTraversal())
        {
          break;
        }
      } // end while(true)

      if (sttIterator)
      {
        sttIterator->GoToNextItem();
      }
    } // end for each source data object

    // get the mask array
    vtkBitArray* maskArray = nullptr;
    if (this->Self->Masking)
    {
      maskArray = vtkArrayDownCast<vtkBitArray>(this->Self->GetMaskArray(inputDataSet));
      if (maskArray == nullptr)
      {
        vtkInternalsDebugMacro(<< "masking is enabled but there is no mask array. Ignore masking.");
      }
      else
      {
        if (maskArray->GetNumberOfComponents() != 1)
        {
          vtkInternalsErrorMacro("expecting a mask array with one component, getting "
            << maskArray->GetNumberOfComponents() << " components.");
          return;
        }
      }
    }

    // rebuild all sources for this dataset
    if (rebuild)
    {
      this->RebuildStructures(glyphParametersCollection, numPoints, actor, inputDataSet, maskArray);
    }

    // for each source data object
    for (const auto& glyphParameters : glyphParametersCollection->Entries)
    {
      if (glyphParameters->NumberOfPoints <= 0)
      {
        continue;
      }

      vtkDataObject* sourceDataObject = glyphParameters->SourceDataObject;
      vtkPolyData* mesh = vtkPolyData::SafeDownCast(sourceDataObject);
      vtkCompositeDataSet* sourceCompositeDataSet =
        mesh ? nullptr : vtkCompositeDataSet::SafeDownCast(sourceDataObject);

      vtkSmartPointer<vtkCompositeDataIterator> sourceCompositeDataIterator;
      if (sourceCompositeDataSet)
      {
        sourceCompositeDataIterator = sourceCompositeDataSet->NewIterator();
        sourceCompositeDataIterator->InitTraversal();
      }

      // Either render the polydata, or loop through the composite dataset and
      // render each polydata leaf
      for (;;)
      {
        int mapperIdx = -1;
        if (sourceCompositeDataIterator)
        {
          mesh = vtkPolyData::SafeDownCast(sourceCompositeDataIterator->GetCurrentDataObject());
          mapperIdx = sourceCompositeDataIterator->GetCurrentFlatIndex();
          sourceCompositeDataIterator->GoToNextItem();
        }

        if (mesh && mesh->GetNumberOfPoints() > 0)
        {
          auto mapper = glyphParameters->Mappers[mapperIdx];
          mapper->StaticOn();
          // scalars are pre-mapped into glyphParameters->Colors using the ColorMapper
          mapper->ScalarVisibilityOff();
          mapper->Initialize(mesh, glyphParameters->NumberOfPoints, &glyphParameters->Colors,
            &glyphParameters->Transforms, &glyphParameters->NormalTransforms, flatIndex, pickable,
            glyphParameters->BuildTime);
          mapper->RenderPiece(renderer, actor);
        }

        if (!sourceCompositeDataIterator || sourceCompositeDataIterator->IsDoneWithTraversal())
        {
          break;
        }
      } // end composite glyph iteration
    }   // end entries
  }

  //------------------------------------------------------------------------------
  void CopyInformationToSubMapper(vtkWebGPUGlyph3DMapperHelper* mapper)
  {
    assert("pre: mapper_exists" && mapper != nullptr);
    mapper->SetStatic(this->Self->Static);
    mapper->ScalarVisibilityOff();
  }

  //------------------------------------------------------------------------------
  void SetupColorMapper() { this->ColorMapper->ShallowCopy(this->Self); }

  //------------------------------------------------------------------------------
  void RenderChildren(
    vtkRenderer* renderer, vtkActor* actor, vtkDataObject* dobj, unsigned int& flatIndex)
  {
    // Push overridden attributes onto the stack.
    // Keep track of attributes that were pushed so that they can be popped after they're
    // applied to the batch element.
    vtkCompositeDataDisplayAttributes* cda = this->Self->BlockAttributes;
    bool overrides_visibility = (cda && cda->HasBlockVisibility(dobj));
    if (overrides_visibility)
    {
      this->BlockState.Visibility.push(cda->GetBlockVisibility(dobj));
    }
    bool overrides_pickability = (cda && cda->HasBlockPickability(dobj));
    if (overrides_pickability)
    {
      this->BlockState.Pickability.push(cda->GetBlockPickability(dobj));
    }
    bool overrides_opacity = (cda && cda->HasBlockOpacity(dobj));
    if (overrides_opacity)
    {
      this->BlockState.Opacity.push(cda->GetBlockOpacity(dobj));
    }
    bool overrides_color = (cda && cda->HasBlockColor(dobj));
    if (overrides_color)
    {
      vtkColor3d color = cda->GetBlockColor(dobj);
      this->BlockState.Color.push(color);
    }
    // Advance flat-index. After this point, flatIndex no longer points to this
    // block.
    const auto originalFlatIndex = flatIndex;
    flatIndex++;

    if (auto dObjTree = vtkDataObjectTree::SafeDownCast(dobj))
    {
      using Opts = vtk::DataObjectTreeOptions;
      for (vtkDataObject* child : vtk::Range(dObjTree, Opts::None))
      {
        if (!child)
        {
          ++flatIndex;
        }
        else
        {
          this->RenderChildren(renderer, actor, child, flatIndex);
        }
      }
    }
    else
    {
      auto ds = vtkDataSet::SafeDownCast(dobj);
      // Skip invisible blocks and unpickable ones when performing selection:
      bool blockVis = this->BlockState.Visibility.top();
      bool blockPick = this->BlockState.Pickability.top();
      if (blockVis)
      {
        if (ds)
        {
          actor->GetProperty()->SetColor(this->BlockState.Color.top().GetData());
          actor->GetProperty()->SetOpacity(this->BlockState.Opacity.top());
          this->RenderDataSet(renderer, actor, ds, originalFlatIndex, blockPick);
        }
        else
        {
          vtkInternalsErrorMacro(<< "Expected a vtkDataObjectTree or vtkDataSet input. Got "
                                 << dobj->GetClassName());
        }
      }
    }
    if (overrides_color)
    {
      this->BlockState.Color.pop();
    }
    if (overrides_opacity)
    {
      this->BlockState.Opacity.pop();
    }
    if (overrides_pickability)
    {
      this->BlockState.Pickability.pop();
    }
    if (overrides_visibility)
    {
      this->BlockState.Visibility.pop();
    }
  }

  //------------------------------------------------------------------------------
  void RebuildStructures(std::shared_ptr<GlyphParametersCollection> glyphParametersCollection,
    vtkIdType numPoints, vtkActor* actor, vtkDataSet* dataset, vtkBitArray* maskArray)
  {
    auto* mapper = this->Self;
    auto* displayProperty = actor->GetProperty();
    double rangeSize = mapper->Range[1] - mapper->Range[0];
    if (rangeSize == 0.0)
    {
      rangeSize = 1.0;
    }
    std::array<vtkTypeFloat32, 4> color;
    if (auto* actorColor = displayProperty->GetColor())
    {
      color[0] = actorColor[0];
      color[1] = actorColor[1];
      color[2] = actorColor[2];
      color[3] = displayProperty->GetOpacity();
    }
    // Verify OrientationArray is consistent with the OrientationMode
    auto* orientationArray = mapper->GetOrientationArray(dataset);
    if (orientationArray != nullptr)
    {
      const int numComponents = orientationArray->GetNumberOfComponents();
      if ((mapper->OrientationMode == ROTATION || mapper->OrientationMode == DIRECTION) &&
        numComponents != 3)
      {
        vtkInternalsErrorMacro("Expected an orientation array with 3 components, got "
          << numComponents << " components");
        return;
      }
      else if (mapper->OrientationMode == QUATERNION && numComponents != 4)
      {
        vtkInternalsErrorMacro("Expected an orientation array with 4 components, got "
          << numComponents << " components");
        return;
      }
    }

    auto* indexArray = mapper->GetSourceIndexArray(dataset);
    auto* scaleArray = mapper->GetScaleArray(dataset);

    this->ColorMapper->SetInputDataObject(dataset);
    this->ColorMapper->MapScalars(displayProperty->GetOpacity());
    auto* colors = this->ColorMapper->GetColors();

    // Traverse all points on input dataset, and transform points on source.
    const auto& numEntries = glyphParametersCollection->Entries.size();
    // how many points from the input dataset are glyphed with Source dataset.
    std::vector<int> numberOfPointsGlyphedPerSource(numEntries, 0);
    if (numEntries > 1 && indexArray)
    {
      // loop over every point
      int index = 0;
      for (vtkIdType pointId = 0; pointId < numPoints; pointId++)
      {
        if (maskArray && maskArray->GetValue(pointId) == 0)
        {
          continue;
        }

        // Compute index into table of glyphs
        double value =
          vtkMath::Norm(indexArray->GetTuple(pointId), indexArray->GetNumberOfComponents());
        index = static_cast<int>(value);
        index = vtkMath::ClampValue(index, 0, static_cast<int>(numEntries) - 1);
        numberOfPointsGlyphedPerSource[index]++;
      }
    }
    else
    {
      numberOfPointsGlyphedPerSource[0] = numPoints;
    }

    // Allocate data structures for each entry.
    for (std::size_t i = 0; i < glyphParametersCollection->Entries.size(); ++i)
    {
      auto& glyphParameters = glyphParametersCollection->Entries[i];
      glyphParameters->Colors.resize(numberOfPointsGlyphedPerSource[i] * 4);
      glyphParameters->Transforms.resize(numberOfPointsGlyphedPerSource[i] * 16);
      glyphParameters->NormalTransforms.resize(numberOfPointsGlyphedPerSource[i] * 9);
      glyphParameters->NumberOfPoints = 0;
      glyphParameters->BuildTime.Modified();
    }

    // loop over every point and fill structures
    int index = 0;
    auto* sourceTableTree = mapper->GetSourceTableTree();

    // cache sources to improve performances
    std::vector<vtkDataObject*> sourceCache(numEntries);
    for (std::size_t i = 0; i < numEntries; i++)
    {
      sourceCache[i] = mapper->UseSourceTableTree ? this->GetChildDataObject(sourceTableTree, i)
                                                  : mapper->GetSource(i);
    }

    double transform[16];
    double normalTransform[9];

    // for each input point
    for (vtkIdType pointId = 0; pointId < numPoints; ++pointId)
    {
      if (!(pointId % 10000))
      {
        mapper->UpdateProgress(static_cast<double>(pointId) / static_cast<double>(numPoints));
        if (mapper->GetAbortExecute())
        {
          break;
        }
      }

      // Skip glyphing masked point.
      if (maskArray && maskArray->GetValue(pointId) == 0)
      {
        continue;
      }

      // Compute index into table of glyphs
      if (indexArray)
      {
        // Compute index into table of glyphs
        double value =
          vtkMath::Norm(indexArray->GetTuple(pointId), indexArray->GetNumberOfComponents());
        index = static_cast<int>(value);
        index = vtkMath::ClampValue(index, 0, static_cast<int>(numEntries) - 1);
      }

      // if source exists at `index`.
      auto* source = (index < static_cast<int>(sourceCache.size()) ? sourceCache[index] : nullptr);
      if (source)
      {
        auto& glyphParameters = glyphParametersCollection->Entries[index];

        std::copy(color.begin(), color.end(),
          &glyphParameters->Colors[glyphParameters->NumberOfPoints * 4]);

        double scaleX = 1.0, scaleY = 1.0, scaleZ = 1.0;
        // Get the scalar and vector data
        if (scaleArray)
        {
          double* tuple = scaleArray->GetTuple(pointId);
          switch (mapper->ScaleMode)
          {
            case SCALE_BY_MAGNITUDE:
              scaleX = scaleY = scaleZ = vtkMath::Norm(tuple, scaleArray->GetNumberOfComponents());
              break;
            case SCALE_BY_COMPONENTS:
              if (scaleArray->GetNumberOfComponents() != 3)
              {
                vtkInternalsErrorMacro("Cannot scale by components since the array \'"
                  << scaleArray->GetName() << "\' does not have 3 components.");
              }
              else
              {
                scaleX = tuple[0];
                scaleY = tuple[1];
                scaleZ = tuple[2];
              }
              break;
            case NO_DATA_SCALING:
            default:
              break;
          }

          // Clamp data scale if enabled
          if (mapper->Clamping && mapper->ScaleMode != NO_DATA_SCALING)
          {
            scaleX =
              (scaleX < mapper->Range[0] ? mapper->Range[0]
                                         : (scaleX > mapper->Range[1] ? mapper->Range[1] : scaleX));
            scaleX = (scaleX - mapper->Range[0]) / rangeSize;
            scaleY =
              (scaleY < mapper->Range[0] ? mapper->Range[0]
                                         : (scaleY > mapper->Range[1] ? mapper->Range[1] : scaleY));
            scaleY = (scaleY - mapper->Range[0]) / rangeSize;
            scaleZ =
              (scaleZ < mapper->Range[0] ? mapper->Range[0]
                                         : (scaleZ > mapper->Range[1] ? mapper->Range[1] : scaleZ));
            scaleZ = (scaleZ - mapper->Range[0]) / rangeSize;
          }
        } // if scaleArray

        scaleX *= mapper->ScaleFactor;
        scaleY *= mapper->ScaleFactor;
        scaleZ *= mapper->ScaleFactor;

        // Now begin copying/transforming glyph
        vtkMatrix4x4::Identity(transform);
        vtkMatrix3x3::Identity(normalTransform);

        // translate Source to Input point
        double x[3];
        dataset->GetPoint(pointId, x);
        transform[3] = x[0];
        transform[7] = x[1];
        transform[11] = x[2];

        if (orientationArray)
        {
          double orientation[4];
          orientationArray->GetTuple(pointId, orientation);

          double rotMatrix[3][3];
          vtkQuaterniond quaternion;

          switch (mapper->OrientationMode)
          {
            case ROTATION:
            {
              double angle = vtkMath::RadiansFromDegrees(orientation[2]);
              vtkQuaterniond qz(cos(0.5 * angle), 0.0, 0.0, sin(0.5 * angle));

              angle = vtkMath::RadiansFromDegrees(orientation[0]);
              vtkQuaterniond qx(cos(0.5 * angle), sin(0.5 * angle), 0.0, 0.0);

              angle = vtkMath::RadiansFromDegrees(orientation[1]);
              vtkQuaterniond qy(cos(0.5 * angle), 0.0, sin(0.5 * angle), 0.0);

              quaternion = qz * qx * qy;
              break;
            }
            case QUATERNION:
              quaternion.Set(orientation);
              break;
            case DIRECTION:
            default:
            {
              if (orientation[1] == 0.0 && orientation[2] == 0.0)
              {
                if (orientation[0] < 0) // just flip x if we need to
                {
                  quaternion.Set(0.0, 0.0, 1.0, 0.0);
                }
              }
              else
              {
                double vMag = vtkMath::Norm(orientation);
                double vNew[3];
                vNew[0] = (orientation[0] + vMag) / 2.0;
                vNew[1] = orientation[1] / 2.0;
                vNew[2] = orientation[2] / 2.0;

                double f = 1.0 / sqrt(vNew[0] * vNew[0] + vNew[1] * vNew[1] + vNew[2] * vNew[2]);
                vNew[0] *= f;
                vNew[1] *= f;
                vNew[2] *= f;

                quaternion.Set(0.0, vNew[0], vNew[1], vNew[2]);
              }
              break;
            }
          }

          quaternion.ToMatrix3x3(rotMatrix);

          for (int i = 0; i < 3; i++)
          {
            for (int j = 0; j < 3; j++)
            {
              transform[4 * i + j] = rotMatrix[i][j];
              normalTransform[3 * i + j] = rotMatrix[j][i]; // transpose
            }
          }
        } // if orientationArray

        if (colors)
        {
          std::array<unsigned char, 4> ubColor;
          colors->GetTypedTuple(pointId, ubColor.data());
          std::transform(ubColor.begin(), ubColor.end(),
            &(glyphParameters->Colors[glyphParameters->NumberOfPoints * 4]),
            [](unsigned char in) { return in / 255.0; });
        }

        // scale data if appropriate
        if (mapper->Scaling)
        {
          if (scaleX == 0.0)
          {
            scaleX = 1.0e-10;
          }
          if (scaleY == 0.0)
          {
            scaleY = 1.0e-10;
          }
          if (scaleZ == 0.0)
          {
            scaleZ = 1.0e-10;
          }

          for (int i = 0; i < 3; i++)
          {
            // inverse of normal matrix is directly computed with inverse scale
            transform[4 * i] *= scaleX;
            normalTransform[i] /= scaleX;
            transform[4 * i + 1] *= scaleY;
            normalTransform[i + 3] /= scaleY;
            transform[4 * i + 2] *= scaleZ;
            normalTransform[i + 6] /= scaleZ;
          }
        }

        // Transpose matrices and copy into vtkTypeFloat32 arrays.
        vtkTypeFloat32* matrices =
          &glyphParameters->Transforms[glyphParameters->NumberOfPoints * 16];
        vtkTypeFloat32* normalTransforms =
          &glyphParameters->NormalTransforms[glyphParameters->NumberOfPoints * 9];
        for (int i = 0; i < 4; i++)
        {
          for (int j = 0; j < 4; j++)
          {
            matrices[i * 4 + j] = transform[j * 4 + i];
          }
        }

        for (int i = 0; i < 3; i++)
        {
          for (int j = 0; j < 3; j++)
          {
            normalTransforms[i * 3 + j] = normalTransform[i * 3 + j];
          }
        }
        glyphParameters->NumberOfPoints++;
      } // if source
    }   // for each input point

    glyphParametersCollection->BuildTime.Modified();
  }

  //------------------------------------------------------------------------------
  void ReleaseGraphicsResources(vtkWindow* window)
  {
    for (auto& glyphParametersCollection : this->GlyphInputDataSets)
    {
      for (auto& glyphParameters : glyphParametersCollection.second->Entries)
      {
        for (auto& mapper : glyphParameters->Mappers)
        {
          mapper.second->ReleaseGraphicsResources(window);
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUGlyph3DMapper);

//------------------------------------------------------------------------------
vtkWebGPUGlyph3DMapper::vtkWebGPUGlyph3DMapper()
{
  this->Internals.reset(new vtkInternals(this));
}

//------------------------------------------------------------------------------
vtkWebGPUGlyph3DMapper::~vtkWebGPUGlyph3DMapper() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUGlyph3DMapper::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebGPUGlyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUGlyph3DMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  this->Internals->ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
void vtkWebGPUGlyph3DMapper::Render(vtkRenderer* render, vtkActor* actor)
{
  auto& internals = (*this->Internals);

  auto* inputDataObject = this->GetInputDataObject(0, 0);
  internals.SetupColorMapper();

  // Create a default source, if no source is specified.
  if (!this->UseSourceTableTree && this->GetSource(0) == nullptr)
  {
    vtkNew<vtkPolyData> defaultSource;
    vtkNew<vtkPoints> defaultPoints;
    defaultPoints->InsertNextPoint(0, 0, 0);
    defaultPoints->InsertNextPoint(1, 0, 0);
    vtkNew<vtkCellArray> lines;
    lines->InsertNextCell({ 0, 1 });
    defaultSource->SetLines(lines);
    this->SetSourceData(defaultSource);
  }

  // Check that configuration of sources on the seconds port are sane.
  auto* sourceTableTree = this->GetSourceTableTree();
  const int numSourceDataSets = this->GetNumberOfInputConnections(1);
  if (this->UseSourceTableTree)
  {
    if (numSourceDataSets > 1)
    {
      vtkErrorMacro("UseSourceTableTree is true, but multiple source datasets are set.");
      return;
    }
    if (!sourceTableTree)
    {
      vtkErrorMacro(
        "UseSourceTableTree is true, but the source dataset is not a vtkDataObjectTree.");
      return;
    }
    auto sttIterator = vtk::TakeSmartPointer(sourceTableTree->NewTreeIterator());
    sttIterator->SetTraverseSubTree(false);
    sttIterator->SetVisitOnlyLeaves(false);
    for (sttIterator->InitTraversal(); !sttIterator->IsDoneWithTraversal();
         sttIterator->GoToNextItem())
    {
      auto* node = sttIterator->GetCurrentDataObject();
      if (!(node->IsA("vtkPolyData") || node->IsA("vtkCompositeDataSet")))
      {
        vtkErrorMacro("The source table tree must only contain vtkPolyData or vtkCompositeDataSet "
                      "children, found a "
          << node->GetClassName() << " instead.");
        return;
      }
    }
  }
  else
  {
    for (int i = 0; i < numSourceDataSets; ++i)
    {
      if (!this->GetSource(i))
      {
        vtkErrorMacro("Source input at index " << i
                                               << " not set, or not "
                                                  "vtkPolyData.");
        return;
      }
    }
  }
  internals.Render(render, actor, inputDataObject);
  this->UpdateProgress(1.0);
}

VTK_ABI_NAMESPACE_END
