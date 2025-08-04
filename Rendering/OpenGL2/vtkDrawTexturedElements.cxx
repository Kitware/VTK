// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDrawTexturedElements.h"

#include "vtkCollectionIterator.h"
#include "vtkColorSeries.h"
#include "vtkDataArray.h"
#include "vtkGLSLModifierBase.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLUniforms.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkTexture.h"
#include "vtkTextureObject.h"

#include "vtk_glad.h"

// Uncomment to print shader/color info to cout
// #define vtkDrawTexturedElements_DEBUG

VTK_ABI_NAMESPACE_BEGIN

struct vtkDrawTexturedElements::Internal
{
  /// The type of primitives to draw (the default is GL_TRIANGLE_STRIP).
  GLenum Primitive;
  /// The total number of vertices.
  GLsizei Count;
  /// Cull face saver
  // Turn off face culling (especially when HasTranslucentPolygonalGeometry()
  // returns true, since this will break depth peeling/OIT).
  std::unique_ptr<vtkOpenGLState::ScopedglEnableDisable> CullFaceSaver;
};

vtkDrawTexturedElements::vtkDrawTexturedElements()
{
  this->P = new Internal;
  // Pre-populate the shaders as vtkOpenGLShaderCache::ReadyShaderProgram() will
  // crash if there is not a shader of each type. Bleh.
  this->GetShader(vtkShader::Fragment);
  this->GetShader(vtkShader::Vertex);
  this->GetShader(vtkShader::Geometry);
}

vtkDrawTexturedElements::~vtkDrawTexturedElements()
{
  while (!this->Shaders.empty())
  {
    this->Shaders.begin()->second->Delete();
    this->Shaders.erase(this->Shaders.begin());
  }
  this->Shaders.clear();
  delete this->P;
  this->P = nullptr;
}

vtkShader* vtkDrawTexturedElements::GetShader(vtkShader::Type shaderType)
{
  auto it = this->Shaders.find(shaderType);
  if (it == this->Shaders.end())
  {
    auto* shader = vtkShader::New();
    shader->SetType(shaderType);
    it = this->Shaders.insert(std::make_pair(shaderType, shader)).first;
  }
  return it->second;
}

void vtkDrawTexturedElements::BindArrayToTexture(
  vtkStringToken textureName, vtkDataArray* array, bool asScalars)
{
  auto it = this->Arrays.find(textureName);
#ifdef vtkDrawTexturedElements_DEBUG
  std::cout << "Bind " << array->GetObjectDescription() << "to texture " << textureName.Data()
            << std::endl;
#endif
  if (it == this->Arrays.end())
  {
    this->Arrays[textureName] = vtkOpenGLArrayTextureBufferAdapter(array, asScalars);
    return;
  }
  it->second.Arrays = { array };
  // needs to be re-uploaded.
  if (it->second.Buffer)
  {
    it->second.Buffer->FlagBufferAsDirty();
  }
  it->second.ScalarComponents = asScalars;
}

bool vtkDrawTexturedElements::UnbindArray(vtkStringToken textureName)
{
  auto it = this->Arrays.find(textureName);
  if (it == this->Arrays.end())
  {
    return false;
  }
  this->Arrays.erase(it);
  return true;
}

void vtkDrawTexturedElements::AppendArrayToTexture(
  vtkStringToken textureName, vtkDataArray* array, bool asScalars)
{
  auto it = this->Arrays.find(textureName);
#ifdef vtkDrawTexturedElements_DEBUG
  std::cout << "Append " << array->GetObjectDescription() << "to texture " << textureName.Data()
            << std::endl;
#endif
  if (it == this->Arrays.end())
  {
    this->Arrays[textureName] = vtkOpenGLArrayTextureBufferAdapter(array, asScalars);
    return;
  }
  else
  {
    it->second.Arrays.emplace_back(array);
    // needs to be re-uploaded.
    if (it->second.Buffer)
    {
      it->second.Buffer->FlagBufferAsDirty();
    }
  }
}

bool vtkDrawTexturedElements::SetNumberOfElements(vtkIdType numberOfElements)
{
  if (this->NumberOfElements == numberOfElements)
  {
    return false;
  }
  this->NumberOfElements = numberOfElements;
  // this->Modified();
  return true;
}

bool vtkDrawTexturedElements::SetNumberOfInstances(vtkIdType numberOfInstances)
{
  if (this->NumberOfInstances == numberOfInstances)
  {
    return false;
  }
  this->NumberOfInstances = numberOfInstances;
  // this->Modified();
  return true;
}

bool vtkDrawTexturedElements::SetElementType(int elementType)
{
  if (elementType == this->ElementType || elementType < ElementShape::Point ||
    elementType > ElementShape::AbstractPatches)
  {
    return false;
  }
  this->ElementType = elementType;
  return true;
}

bool vtkDrawTexturedElements::SetPatchType(int patchType)
{
  if (patchType == this->PatchType || patchType < PatchLine || patchType > PatchQuadrilateral)
  {
    return false;
  }
  this->PatchType = patchType;
  return true;
}

bool vtkDrawTexturedElements::SetIncludeColormap(bool includeColormap)
{
  if (this->IncludeColormap == includeColormap)
  {
    return false;
  }
  this->IncludeColormap = includeColormap;
  return true;
}

void vtkDrawTexturedElements::ReadyShaderProgram(vtkRenderer* ren)
{
  auto* renderWindow = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renderWindow)
  {
    vtkWarningWithObjectMacro(ren, "Renderer has no OpenGL render-window.");
    return;
  }
  bool lastSyncGLSLVersionDisabled = !renderWindow->GetShaderCache()->GetSyncGLSLShaderVersion();
  if (this->ElementType == AbstractPatches && lastSyncGLSLVersionDisabled)
  {
    renderWindow->GetShaderCache()->SyncGLSLShaderVersionOn();
  }
  this->ShaderProgram = renderWindow->GetShaderCache()->ReadyShaderProgram(this->Shaders);
  if (lastSyncGLSLVersionDisabled)
  {
    renderWindow->GetShaderCache()->SyncGLSLShaderVersionOff();
  }
  vtkOpenGLStaticCheckErrorMacro("Failed readying shader program");
}

void vtkDrawTexturedElements::ReportUnsupportedLineWidth(
  float width, float maxWidth, vtkMapper* mapper)
{
  const char* glVersion = (const char*)glGetString(GL_VERSION);
  vtkWarningWithObjectMacro(mapper, << "Line width (" << width
                                    << ") is less than maximum line width (" << maxWidth
                                    << ") supported by your OpenGL driver " << glVersion);
}

void vtkDrawTexturedElements::PreDraw(vtkRenderer* ren, vtkActor* actor, vtkMapper* mapper)
{
  if (this->ShaderProgram == nullptr)
  {
    // can be nullptr if glsl failed to compile or link
    return;
  }
  auto* renderWindow = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renderWindow)
  {
    vtkWarningWithObjectMacro(ren, "Renderer has no OpenGL render-window.");
    return;
  }
  // Turn off face culling (especially when HasTranslucentPolygonalGeometry()
  // returns true, since this will break depth peeling/OIT).
  vtkOpenGLState* ostate = renderWindow->GetState();
  this->P->CullFaceSaver.reset(new vtkOpenGLState::ScopedglEnableDisable(ostate, GL_CULL_FACE));
#ifndef GL_ES_VERSION_3_0
  // For GLES 3.0, none of these are supported. It is recommended to set gl_PointSize in shader
  // and render wide lines using instanced rendering.
  switch (this->ElementType)
  {
    case ElementShape::Point:
      ostate->vtkglPointSize(actor->GetProperty()->GetPointSize());
      break;
    case ElementShape::Line:
    case ElementShape::LineStrip:
    {
      const float width = actor->GetProperty()->GetLineWidth();
      const float maxSupportedWidth = renderWindow->GetMaximumHardwareLineWidth();
      if (width <= maxSupportedWidth)
      {
        ostate->vtkglLineWidth(width);
      }
      else
      {
        this->ReportUnsupportedLineWidth(width, maxSupportedWidth, mapper);
      }
    }
    break;
    default:
    case ElementShape::AbstractPatches:
      if (this->PatchType == PatchShape::PatchLine)
      {
        const float width = actor->GetProperty()->GetLineWidth();
        const float maxSupportedWidth = renderWindow->GetMaximumHardwareLineWidth();
        if (width <= maxSupportedWidth)
        {
          ostate->vtkglLineWidth(width);
        }
        else
        {
          this->ReportUnsupportedLineWidth(width, maxSupportedWidth, mapper);
        }
      }
      break;
  }
#endif

  // Determine primitive type number of invocations of the vertex shader.
  switch (this->ElementType)
  {
    case vtkDrawTexturedElements::ElementShape::Point:
      this->P->Primitive = GL_POINTS;
      break;
    case vtkDrawTexturedElements::ElementShape::Line:
      this->P->Primitive = GL_LINES;
      break;
    case vtkDrawTexturedElements::ElementShape::LineStrip:
      this->P->Primitive = GL_LINE_STRIP;
      break;
    case vtkDrawTexturedElements::ElementShape::Triangle:
      this->P->Primitive = GL_TRIANGLES;
      break;
    case vtkDrawTexturedElements::ElementShape::TriangleStrip:
      this->P->Primitive = GL_TRIANGLE_STRIP;
      break;
    case vtkDrawTexturedElements::ElementShape::TriangleFan:
      this->P->Primitive = GL_TRIANGLE_FAN;
      break;
    case vtkDrawTexturedElements::ElementShape::AbstractPatches:
#ifdef GL_ARB_tessellation_shader
      this->P->Primitive = GL_PATCHES;
      break;
#else
      vtkErrorWithObjectMacro(mapper, << "ElementType cannot be \'AbstractPatches\' because "
                                         "GL_PATCHES is not supported in this build of VTK.");
      break;
#endif
    default:
    {
      vtkGenericWarningMacro("Invalid element type " << this->ElementType << ".");
      break;
    }
  }
  if (this->IncludeColormap)
  {
    // Upload the colormap (or create one if none exists).
    vtkImageData* colorTexture = mapper->GetColorTextureMap();
    this->ColorTextureGL->RepeatOff(); // Turn off repeat before assigning input.
    if (!colorTexture)
    {
      vtkNew<vtkColorSeries> palette;
      vtkSmartPointer<vtkImageData> paletteImage;
      palette->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_11);
      vtkLookupTable* lkup = palette->CreateLookupTable(vtkColorSeries::ORDINAL);
      paletteImage = vtkMapper::BuildColorTextureImage(lkup, mapper->GetColorMode());
      this->ColorTextureGL->SetInputData(paletteImage);
    }
    else
    {
      this->ColorTextureGL->SetInputData(colorTexture);
    }
    this->ColorTextureGL->Load(ren);
    int tunit = this->ColorTextureGL->GetTextureUnit();
    if (this->ShaderProgram->IsUniformUsed("color_map") &&
      !this->ShaderProgram->SetUniformi("color_map", tunit))
    {
      vtkWarningWithObjectMacro(ren, << this->ShaderProgram->GetError());
    }
    vtkOpenGLStaticCheckErrorMacro("Failed readying colormap texture");
  }

  // Upload texture data (if needed) and bind textures to the shader program.
  // I. Upload data to texture objects as needed.
  for (auto& entry : this->Arrays)
  {
#ifdef vtkDrawTexturedElements_DEBUG
    std::cout << "Attempt to upload \"" << entry.first.Data() << "\"\n";
#endif
    entry.second.Upload(vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
  }
  // II. Activate each texture (bind it).
  for (const auto& entry : this->Arrays)
  {
    auto samplerName = entry.first.Data();
    if (!this->ShaderProgram->IsUniformUsed(samplerName.c_str()))
    {
#ifdef vtkDrawTexturedElements_DEBUG
      std::cout << "Skipping \"" << samplerName << "\"\n";
#endif
      continue;
    }
#ifdef vtkDrawTexturedElements_DEBUG
    std::cout << "Activate \"" << entry.first.Data() << " for sampler " << samplerName << "\"\n";
#endif
    entry.second.Texture->Activate();
    int textureUnit = entry.second.Texture->GetTextureUnit();
    this->ShaderProgram->SetUniformi(samplerName.c_str(), textureUnit);
    vtkOpenGLStaticCheckErrorMacro("Failed trying to activate \"" << samplerName << "\".");
  }
  // set shader parameter for GLSL mods.
  auto modsIter = vtk::TakeSmartPointer(this->GLSLMods->NewIterator());
  auto oglRen = static_cast<vtkOpenGLRenderer*>(ren);
  for (modsIter->InitTraversal(); !modsIter->IsDoneWithTraversal(); modsIter->GoToNextItem())
  {
    auto mod = static_cast<vtkGLSLModifierBase*>(modsIter->GetCurrentObject());
    mod->SetPrimitiveType(this->P->Primitive);
    mod->SetShaderParameters(oglRen, this->ShaderProgram, mapper, actor, this->VAO);
    vtkOpenGLStaticCheckErrorMacro("Failed after applying mod shader parameters");
  }
  // set shader parameter for different render passes.
  vtkInformation* info = actor->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      rp->SetShaderParameters(this->ShaderProgram, mapper, actor);
    }
  }

  // Add custom uniforms provided by the actor's shader property.
  this->SetCustomUniforms(ren, actor);

  // Bind the (null) VAO and the IBO
  this->VAO->Bind();
  vtkOpenGLStaticCheckErrorMacro("Failed after binding VAO.");
}

void vtkDrawTexturedElements::PostDraw(vtkRenderer* ren, vtkActor*, vtkMapper*)
{
  if (this->ShaderProgram == nullptr)
  {
    // can be nullptr if glsl failed to compile or link
    return;
  }
  auto* renderWindow = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renderWindow)
  {
    vtkWarningWithObjectMacro(ren, "Renderer has no OpenGL render-window.");
    return;
  }

#if 1
  for (const auto& entry : this->Arrays)
  {
    entry.second.Texture->Deactivate();
  }
#endif
  vtkOpenGLStaticCheckErrorMacro("Just after texture release");

  this->VAO->Release();
  if (this->IncludeColormap)
  {
    this->ColorTextureGL->PostRender(ren);
  }
  this->P->CullFaceSaver.reset(nullptr);
}

void vtkDrawTexturedElements::DrawInstancedElementsImpl(
  vtkRenderer* ren, vtkActor*, vtkMapper* mapper)
{
  if (this->ShaderProgram == nullptr)
  {
    // can be nullptr if glsl failed to compile or link
    return;
  }

  // Determine number of invocations of the vertex shader.
  this->P->Count = static_cast<GLsizei>(this->NumberOfElements);
  switch (this->ElementType)
  {
    case vtkDrawTexturedElements::ElementShape::Point:
      break;
    case vtkDrawTexturedElements::ElementShape::Line:
      this->P->Count *= 2;
      break;
    case vtkDrawTexturedElements::ElementShape::LineStrip:
      ++this->P->Count;
      break;
    case vtkDrawTexturedElements::ElementShape::Triangle:
      this->P->Count *= 3;
      break;
    case vtkDrawTexturedElements::ElementShape::TriangleStrip:
      this->P->Count += 2;
      break;
    case vtkDrawTexturedElements::ElementShape::TriangleFan:
      this->P->Count += 2;
      break;
    case vtkDrawTexturedElements::ElementShape::AbstractPatches:
#ifdef GL_ARB_tessellation_shader
    {
      (void)mapper;
      const int patchVertexCount = this->PatchVertexCountFromPrimitive(this->PatchType);
      this->P->Count *= patchVertexCount;
      glPatchParameteri(GL_PATCH_VERTICES, patchVertexCount);
      break;
    }
#else
      vtkErrorWithObjectMacro(mapper, << "ElementType cannot be \'AbstractPatches\' because "
                                         "GL_PATCHES is not supported in this build of VTK.");
      break;
#endif
    default:
    {
      vtkGenericWarningMacro("Invalid element type " << this->ElementType << ".");
      break;
    }
  }
  auto instances = static_cast<GLsizei>(this->NumberOfInstances);
  vtkOpenGLStaticCheckErrorMacro("Just before draw instanced");
  // Render the element instances:
#ifdef GL_ES_VERSION_3_0
  (void)ren;
  glDrawArraysInstanced(this->P->Primitive, this->FirstVertexId, this->P->Count, instances);
#else
  if (GLAD_GL_VERSION_3_1)
  {
    glDrawArraysInstanced(this->P->Primitive, this->FirstVertexId, this->P->Count, instances);
  }
  else if (GLAD_GL_ARB_instanced_arrays)
  {
    glDrawArraysInstancedARB(this->P->Primitive, this->FirstVertexId, this->P->Count, instances);
  }
  else
  {
    vtkErrorWithObjectMacro(ren, "No support for glDrawArraysInstanced.");
  }
#endif
  vtkOpenGLStaticCheckErrorMacro("Just after draw");
}

void vtkDrawTexturedElements::DrawInstancedElements(
  vtkRenderer* ren, vtkActor* actor, vtkMapper* mapper)
{
  this->ReadyShaderProgram(ren);
  this->PreDraw(ren, actor, mapper);
  this->DrawInstancedElementsImpl(ren, actor, mapper);
  this->PostDraw(ren, actor, mapper);
}

void vtkDrawTexturedElements::ReleaseResources(vtkWindow* window)
{
  this->VAO->ReleaseGraphicsResources();
  this->ColorTextureGL->ReleaseGraphicsResources(window);
  for (auto& entry : this->Arrays)
  {
    entry.second.ReleaseGraphicsResources(window);
  }
}

vtkShaderProgram* vtkDrawTexturedElements::GetShaderProgram()
{
  return this->ShaderProgram;
}

vtkCollection* vtkDrawTexturedElements::GetGLSLModCollection() const
{
  return this->GLSLMods;
}

void vtkDrawTexturedElements::SetCustomUniforms(vtkRenderer* vtkNotUsed(ren), vtkActor* actor)
{
  vtkShaderProperty* sp = actor->GetShaderProperty();
  auto vu = static_cast<vtkOpenGLUniforms*>(sp->GetVertexCustomUniforms());
  vu->SetUniforms(this->ShaderProgram);
  auto fu = static_cast<vtkOpenGLUniforms*>(sp->GetFragmentCustomUniforms());
  fu->SetUniforms(this->ShaderProgram);
  auto gu = static_cast<vtkOpenGLUniforms*>(sp->GetGeometryCustomUniforms());
  gu->SetUniforms(this->ShaderProgram);
  auto tcu = static_cast<vtkOpenGLUniforms*>(sp->GetTessControlCustomUniforms());
  tcu->SetUniforms(this->ShaderProgram);
  auto teu = static_cast<vtkOpenGLUniforms*>(sp->GetTessEvaluationCustomUniforms());
  teu->SetUniforms(this->ShaderProgram);
}

vtkIdType vtkDrawTexturedElements::PatchVertexCountFromPrimitive(int shape)
{
  switch (shape)
  {
    case PatchShape::PatchLine:
      return 2;
    case PatchShape::PatchQuadrilateral:
      return 4;
    case PatchShape::PatchTriangle:
    default:
      return 3;
  }
}

VTK_ABI_NAMESPACE_END
