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

namespace
{

thread_local std::vector<GLubyte> iotaByte;
thread_local std::vector<GLushort> iotaShort;
thread_local std::vector<GLuint> iotaInt;

// glIota() is a function to create an array to upload into an index-buffer object (IBO).
// Since this class uses vertex-pulling to assign coordinates and other values in the
// vertex shader, no vertex array data is used and the IBO is typically minimal (just a
// few triangles at most). The OpenGL-provided gl_VertexID and gl_InstanceID are used to
// fetch or compute vertex data as required.
//
// glIotaInternal is templated on integer type so that the smallest integer type that
// supports the required index values can be used.
template <typename IotaType>
IotaType* glIotaInternal(vtkOpenGLIndexBufferObject* ibo, GLsizei count);

template <>
GLubyte* glIotaInternal<GLubyte>(vtkOpenGLIndexBufferObject* ibo, GLsizei count)
{
  if (count > 255)
  {
    return nullptr;
  }
  if (count > static_cast<GLsizei>(iotaByte.size()))
  {
    iotaByte.resize(count);
    std::iota(iotaByte.begin(), iotaByte.end(), 0);
  }
  ibo->Upload(iotaByte, vtkOpenGLBufferObject::ObjectType::ElementArrayBuffer);
  return &iotaByte[0];
}

template <>
GLushort* glIotaInternal<GLushort>(vtkOpenGLIndexBufferObject* ibo, GLsizei count)
{
  if (count > 255)
  {
    return nullptr;
  }
  if (count > static_cast<GLsizei>(iotaShort.size()))
  {
    iotaShort.resize(count);
    std::iota(iotaShort.begin(), iotaShort.end(), 0);
  }
  ibo->Upload(iotaShort, vtkOpenGLBufferObject::ObjectType::ElementArrayBuffer);
  return &iotaShort[0];
}

template <>
GLuint* glIotaInternal<GLuint>(vtkOpenGLIndexBufferObject* ibo, GLsizei count)
{
  if (count > 255)
  {
    return nullptr;
  }
  if (count > static_cast<GLsizei>(iotaInt.size()))
  {
    iotaInt.resize(count);
    std::iota(iotaInt.begin(), iotaInt.end(), 0);
  }
  ibo->Upload(iotaInt, vtkOpenGLBufferObject::ObjectType::ElementArrayBuffer);
  return &iotaInt[0];
}

// See documentation for glIotaInternal above for details.
void glIota(vtkOpenGLIndexBufferObject* ibo, GLenum& indexType, void*& indices, GLsizei count)
{
  // We do not support more than 2**30 vertices since OpenGL does not provide an
  // integer that can distinguish between them. In that case, we will not upload an IBO.
  if (count > (1 << 30))
  {
    indices = nullptr;
  }
  if (count > 16383)
  {
    indices = glIotaInternal<GLuint>(ibo, count);
    indexType = GL_UNSIGNED_INT;
  }
  else if (count > 255)
  {
    indices = glIotaInternal<GLushort>(ibo, count);
    indexType = GL_UNSIGNED_SHORT;
  }
  else
  {
    indices = glIotaInternal<GLubyte>(ibo, count);
    indexType = GL_UNSIGNED_BYTE;
  }
  // Unbind after uploading
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}

VTK_ABI_NAMESPACE_BEGIN

struct vtkDrawTexturedElements::Internal
{
  /// True when the IBO has up-to-date data loaded.
  bool UploadedIndexBuffer{ false };
  /// The data type for indices in the IBO (one of GL_UNSIGNED_{BYTE,SHORT,INT}).
  GLenum IndexType;
  /// The type of primitives represented by the IBO (the default is GL_TRIANGLE_STRIP).
  GLenum Primitive;
  /// The number of connectivity entries in the IBO.
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
  if (it == this->Arrays.end())
  {
    this->Arrays[textureName] = ArrayTextureData(array, asScalars);
    return;
  }
  it->second.Array = array;
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
  this->P->UploadedIndexBuffer = false; // We need to re-upload our IBO.
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

vtkDrawTexturedElements::ArrayTextureData::ArrayTextureData()
  : Array(nullptr)
  , Texture(vtkSmartPointer<vtkTextureObject>::New())
  , Buffer(vtkSmartPointer<vtkOpenGLBufferObject>::New())
  , BufferType(vtkOpenGLBufferObject::ObjectType::TextureBuffer)
  , IntegerTexture(true)
  , ScalarComponents(false)
{
}

vtkDrawTexturedElements::ArrayTextureData::ArrayTextureData(
  vtkDataArray* array, bool asScalars, bool* integerTexture)
  : Array(array)
  , Texture(vtkSmartPointer<vtkTextureObject>::New())
  , Buffer(vtkSmartPointer<vtkOpenGLBufferObject>::New())
  , BufferType(vtkOpenGLBufferObject::ObjectType::TextureBuffer)
  , IntegerTexture((integerTexture ? *integerTexture : array->IsIntegral()))
  , ScalarComponents(asScalars)
{
}

void vtkDrawTexturedElements::ArrayTextureData::Upload(
  vtkOpenGLRenderWindow* renderWindow, bool force)
{
  if (this->Buffer->IsReady() && !force)
  {
    // We don't need to re-upload.
    return;
  }
  this->Buffer->SetType(this->BufferType);
  this->Texture->SetRequireTextureInteger(this->IntegerTexture);
  this->Texture->SetContext(renderWindow);
  vtkSmartPointer<vtkDataArray> array = this->Array;
  // Narrow arrays of large values to a precision supported by base-OpenGL:
  switch (array->GetDataType())
  {
    case VTK_DOUBLE:
    {
      array = vtkSmartPointer<vtkFloatArray>::New();
      array->DeepCopy(this->Array);
    }
    break;
    case VTK_ID_TYPE:
    {
      // FIXME: We should check that truncating to 32 bits is OK.
      array = vtkSmartPointer<vtkTypeInt32Array>::New();
      array->DeepCopy(this->Array);
    }
    break;
#if VTK_TYPE_UINT64 == VTK_UNSIGNED_LONG
    case VTK_LONG:
    {
      array = vtkSmartPointer<vtkTypeInt32Array>::New();
      array->DeepCopy(this->Array);
    }
    break;
    case VTK_UNSIGNED_LONG:
    {
      array = vtkSmartPointer<vtkTypeUInt32Array>::New();
      array->DeepCopy(this->Array);
    }
    break;
#endif
    case VTK_LONG_LONG:
    {
      array = vtkSmartPointer<vtkTypeInt32Array>::New();
      array->DeepCopy(this->Array);
    }
    break;
    case VTK_UNSIGNED_LONG_LONG:
    {
      array = vtkSmartPointer<vtkTypeUInt32Array>::New();
      array->DeepCopy(this->Array);
    }
    break;
    default:
      // Do nothing
      break;
  }
#ifdef vtkDrawTexturedElements_DEBUG
  std::cout << "Uploading Array: " << this->Array->GetName()
            << " with MTime: " << this->Array->GetMTime() << " into Buffer: " << this->Buffer
            << " with MTime: " << this->Texture->GetMTime() << std::endl;
#endif
  // Now upload the array
  switch (array->GetDataType())
  {
    vtkTemplateMacro(this->Buffer->Upload(
      static_cast<VTK_TT*>(array->GetVoidPointer(0)), array->GetMaxId() + 1, this->BufferType));
  }
  int numberOfComponents = this->ScalarComponents ? 1 : array->GetNumberOfComponents();
  vtkIdType numberOfTuples =
    this->ScalarComponents ? array->GetMaxId() + 1 : array->GetNumberOfTuples();
  // if (updateInternalTextureFormat)
  {
    this->Texture->GetInternalFormat(
      array->GetDataType(), numberOfComponents, this->IntegerTexture);
  }
  this->Texture->CreateTextureBuffer(
    numberOfTuples, numberOfComponents, array->GetDataType(), this->Buffer);
  // this->Texture->Activate();
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
      ostate->vtkglPointSize(actor->GetProperty()->GetPointSize());
      break;
    case ElementShape::Line:
    case ElementShape::LineStrip:
      ostate->vtkglLineWidth(actor->GetProperty()->GetLineWidth());
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

  // Either create+bind a new index-buffer or bind an existing index-buffer.
  this->PrepareIBO();

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
  this->IBO->Bind();
  vtkOpenGLStaticCheckErrorMacro("Failed after binding VAO and IBO.");

  // Render the element instances:
#ifdef GL_ES_VERSION_3_0
  glDrawElementsInstanced(
    this->P->Primitive, this->P->Count, this->P->IndexType, nullptr /* indices */, instances);
#else
#if 1
  if (GLEW_VERSION_3_1)
  {
    glDrawElementsInstanced(
      this->P->Primitive, this->P->Count, this->P->IndexType, nullptr /* indices */, instances);
  }
  else if (GL_ARB_instanced_arrays)
  {
    glDrawElementsInstancedARB(
      this->P->Primitive, this->P->Count, this->P->IndexType, nullptr /* indices */, instances);
  }
#else
  if (GLEW_VERSION_3_1)
  {
    glDrawArraysInstanced(this->P->Primitive, 0, this->P->Count, instances);
  }
  else if (GL_ARB_instanced_arrays && &glDrawArraysInstancedARB)
  {
    glDrawArraysInstancedARB(this->P->Primitive, 0, this->P->Count, instances);
  }
#endif
  else
  {
    vtkErrorWithObjectMacro(ren, "No support for glDrawElementsInstanced.");
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

  this->IBO->Release();
  this->VAO->Release();
  this->ColorTextureGL->PostRender(ren);
}

void vtkDrawTexturedElements::ReleaseResources(vtkWindow* window)
{
  this->IBO->ReleaseGraphicsResources();
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

void vtkDrawTexturedElements::PrepareIBO()
{
  if (this->P->UploadedIndexBuffer)
  {
    return; // Fast path is when we have already uploaded the IBO.
  }

  // It would be nice to test this in the constructor
  // but there is no current context at that point.
  // Instead, we test here to avoid testing on every render.
#ifndef VTK_MODULE_vtkglew_GLES3
  if (!GLEW_VERSION_3_1)
  {
    vtkErrorWithObjectMacro(this->IBO, "OpenGL 3.1 or newer required.");
    throw std::runtime_error("OpenGL 3.1 or newer required.");
  }
#endif

  this->P->UploadedIndexBuffer = this->UploadIBO();
  if (!this->P->UploadedIndexBuffer)
  {
    vtkErrorWithObjectMacro(this->IBO, "Unable to upload IBO for array renderer.");
    return;
  }
}

bool vtkDrawTexturedElements::UploadIBO()
{
  // Create a "connectivity" array (indices) for the elements.
  // The array renderer assumes these are just a sequence of integers 0..(N-1).
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
      return false;
    }
  }

  void* indices = nullptr;
  glIota(this->IBO, this->P->IndexType, indices, this->P->Count);
  return true;
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
