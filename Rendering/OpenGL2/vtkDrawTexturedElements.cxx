// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDrawTexturedElements.h"

#include "vtkCollectionIterator.h"
#include "vtkColorSeries.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGLSLModifierBase.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLUniforms.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkTexture.h"
#include "vtkTextureObject.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkUnsignedCharArray.h"

#include "vtk_glew.h"

#include <numeric> // for std::iota()

// Uncomment to print shader/color info to cout
// #define vtkDrawTexturedElements_DEBUG

VTK_ABI_NAMESPACE_BEGIN

struct vtkDrawTexturedElements::Internal
{
  /// The type of primitives to draw (the default is GL_TRIANGLE_STRIP).
  GLenum Primitive;
  /// The total number of vertices.
  GLsizei Count;
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
  if (array != it->second.Array)
  {
    it->second.Array = array;
    // needs to be re-uploaded.
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
    elementType > ElementShape::TriangleFan)
  {
    return false;
  }
  this->ElementType = elementType;
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

void vtkDrawTexturedElements::DrawInstancedElements(
  vtkRenderer* ren, vtkActor* actor, vtkMapper* mapper)
{
  auto* renderWindow = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renderWindow)
  {
    vtkWarningWithObjectMacro(ren, "Renderer has no OpenGL render-window.");
    return;
  }

  // Turn off face culling (especially when HasTranslucentPolygonalGeometry()
  // returns true, since this will break depth peeling/OIT).
  vtkOpenGLState* ostate = renderWindow->GetState();
  vtkOpenGLState::ScopedglEnableDisable cfsaver(ostate, GL_CULL_FACE);
  ostate->vtkglDisable(GL_CULL_FACE);
  switch (this->ElementType)
  {
    case ElementShape::Point:
#ifndef GL_ES_VERSION_3_0
      ostate->vtkglPointSize(actor->GetProperty()->GetPointSize());
#endif
      break;
    case ElementShape::Line:
    case ElementShape::LineStrip:
#ifndef GL_ES_VERSION_3_0
      ostate->vtkglLineWidth(actor->GetProperty()->GetLineWidth());
#endif
      break;
    default:
      break;
  }

  this->ShaderProgram = renderWindow->GetShaderCache()->ReadyShaderProgram(this->Shaders);
  vtkOpenGLStaticCheckErrorMacro("Failed readying shader program");
  if (this->ShaderProgram == nullptr)
  {
    // can be nullptr if glsl failed to compile or link
    return;
  }

  // Determine number of invocations of the vertex shader.
  this->P->Count = static_cast<GLsizei>(this->NumberOfElements);
  switch (this->ElementType)
  {
    case ElementShape::Point:
      this->P->Primitive = GL_POINTS;
      break;
    case ElementShape::Line:
      this->P->Primitive = GL_LINES;
      this->P->Count *= 2;
      break;
    case ElementShape::LineStrip:
      this->P->Primitive = GL_LINE_STRIP;
      ++this->P->Count;
      break;
    case ElementShape::Triangle:
      this->P->Primitive = GL_TRIANGLES;
      this->P->Count *= 3;
      break;
    case ElementShape::TriangleStrip:
      this->P->Primitive = GL_TRIANGLE_STRIP;
      this->P->Count += 2;
      break;
    case ElementShape::TriangleFan:
      this->P->Primitive = GL_TRIANGLE_FAN;
      this->P->Count += 2;
      break;
    default:
    {
      // vtkErrorMacro("Invalid element type " << this->ElementType << ".");
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
      auto* lkup = palette->CreateLookupTable(vtkColorSeries::ORDINAL);
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

  auto instances = static_cast<GLsizei>(this->NumberOfInstances);
  vtkOpenGLStaticCheckErrorMacro("Just before draw instanced");

  // Bind the (null) VAO and the IBO
  this->VAO->Bind();
  vtkOpenGLStaticCheckErrorMacro("Failed after binding VAO.");
  // Render the element instances:
#ifdef GL_ES_VERSION_3_0
  glDrawArraysInstanced(this->P->Primitive, 0, this->P->Count, instances);
#else
  if (GLEW_VERSION_3_1)
  {
    glDrawArraysInstanced(this->P->Primitive, 0, this->P->Count, instances);
  }
  else if (GL_ARB_instanced_arrays)
  {
    glDrawArraysInstancedARB(this->P->Primitive, 0, this->P->Count, instances);
  }
  else
  {
    vtkErrorWithObjectMacro(ren, "No support for glDrawArraysInstanced.");
  }
#endif
  vtkOpenGLStaticCheckErrorMacro("Just after draw");
#if 1
  for (const auto& entry : this->Arrays)
  {
    entry.second.Texture->Deactivate();
  }
#endif
  vtkOpenGLStaticCheckErrorMacro("Just after texture release");

  this->VAO->Release();
  this->ColorTextureGL->PostRender(ren);
}

void vtkDrawTexturedElements::ReleaseResources(vtkWindow* window)
{
  this->VAO->ReleaseGraphicsResources();
  this->ColorTextureGL->ReleaseGraphicsResources(window);
  for (auto& entry : this->Arrays)
  {
    entry.second.Texture->ReleaseGraphicsResources(window);
    entry.second.Buffer->ReleaseGraphicsResources();
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
}

VTK_ABI_NAMESPACE_END
