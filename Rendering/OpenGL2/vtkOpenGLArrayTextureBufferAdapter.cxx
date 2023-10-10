// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLArrayTextureBufferAdapter.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkTextureObject.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeUInt32Array.h"

// Uncomment to debug upload timestamps.
// #define vtkOpenGLArrayTextureBufferAdapter_DEBUG

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkOpenGLArrayTextureBufferAdapter::vtkOpenGLArrayTextureBufferAdapter()
  : Array(nullptr)
  , Texture(vtkSmartPointer<vtkTextureObject>::New())
  , Buffer(vtkSmartPointer<vtkOpenGLBufferObject>::New())
  , BufferType(vtkOpenGLBufferObject::ObjectType::TextureBuffer)
  , IntegerTexture(true)
  , ScalarComponents(false)
{
}

//------------------------------------------------------------------------------
vtkOpenGLArrayTextureBufferAdapter::vtkOpenGLArrayTextureBufferAdapter(
  vtkDataArray* array, bool asScalars, bool* integerTexture)
  : Array(array)
  , Texture(vtkSmartPointer<vtkTextureObject>::New())
  , Buffer(vtkSmartPointer<vtkOpenGLBufferObject>::New())
  , BufferType(vtkOpenGLBufferObject::ObjectType::TextureBuffer)
  , IntegerTexture((integerTexture ? *integerTexture : array->IsIntegral()))
  , ScalarComponents(asScalars)
{
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferAdapter::Upload(vtkOpenGLRenderWindow* renderWindow, bool force)
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
#if VTK_ID_TYPE_IMPL == VTK_INT
      array->ShallowCopy(this->Array);
#else
      array->DeepCopy(this->Array);
#endif
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
#ifdef vtkOpenGLArrayTextureBufferAdapter_DEBUG
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

VTK_ABI_NAMESPACE_END
