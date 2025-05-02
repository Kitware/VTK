// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLArrayTextureBufferAdapter.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkTextureObject.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeUInt32Array.h"

// Uncomment to print upload events.
// #define vtkOpenGLArrayTextureBufferAdapter_DEBUG

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkOpenGLArrayTextureBufferAdapter::vtkOpenGLArrayTextureBufferAdapter()
  : Arrays({})
  , BufferType(vtkOpenGLBufferObject::ObjectType::TextureBuffer)
  , BufferUsage(vtkOpenGLBufferObject::ObjectUsage::StaticDraw)
  , IntegerTexture(true)
  , ScalarComponents(false)
{
}

//------------------------------------------------------------------------------
vtkOpenGLArrayTextureBufferAdapter::vtkOpenGLArrayTextureBufferAdapter(
  vtkDataArray* array, bool asScalars, bool* integerTexture)
  : Arrays({ array })
  , BufferType(vtkOpenGLBufferObject::ObjectType::TextureBuffer)
  , BufferUsage(vtkOpenGLBufferObject::ObjectUsage::StaticDraw)
  , IntegerTexture((integerTexture ? *integerTexture : array->IsIntegral()))
  , ScalarComponents(asScalars)
{
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferAdapter::Upload(vtkOpenGLRenderWindow* renderWindow, bool force)
{
  if ((this->Buffer && this->Buffer->IsReady()) && !force)
  {
    // We don't need to re-upload.
    return;
  }
  if (this->Arrays.empty())
  {
    // There are no arrays to upload.
    return;
  }
  if (this->Buffer == nullptr)
  {
    // We need to create the buffer.
    this->Buffer = vtkSmartPointer<vtkOpenGLBufferObject>::New();
    this->Buffer->SetType(this->BufferType);
  }
  if (this->Texture == nullptr)
  {
    // We need to create the texture.
    this->Texture = vtkSmartPointer<vtkTextureObject>::New();
    this->Texture->SetRequireTextureInteger(this->IntegerTexture);
    this->Texture->SetContext(renderWindow);
  }
  std::size_t nbytes = 0;
  vtkIdType numberOfTuples = 0;
  int numberOfComponents = 0;
  int vtktype = 0;
  std::vector<vtkSmartPointer<vtkDataArray>> arraysToUpload;
  // 1. Prepare a list of arrays to upload, also figuring out the size of the huge buffer
  // allocation.
  for (auto& actualArray : this->Arrays)
  {
    auto array = actualArray;
    // Narrow arrays of large values to a precision supported by base-OpenGL:
    switch (array->GetDataType())
    {
      case VTK_DOUBLE:
      {
        array = vtkSmartPointer<vtkFloatArray>::New();
        array->DeepCopy(actualArray);
      }
      break;
      case VTK_ID_TYPE:
      {
        // FIXME: We should check that truncating to 32 bits is OK.
        array = vtkSmartPointer<vtkTypeInt32Array>::New();
#if VTK_ID_TYPE_IMPL == VTK_INT
        array->ShallowCopy(actualArray);
#else
        array->DeepCopy(actualArray);
#endif
      }
      break;
#if VTK_TYPE_UINT64 == VTK_UNSIGNED_LONG
      case VTK_LONG:
      {
        array = vtkSmartPointer<vtkTypeInt32Array>::New();
        array->DeepCopy(actualArray);
      }
      break;
      case VTK_UNSIGNED_LONG:
      {
        array = vtkSmartPointer<vtkTypeUInt32Array>::New();
        array->DeepCopy(actualArray);
      }
      break;
#endif
      case VTK_LONG_LONG:
      {
        array = vtkSmartPointer<vtkTypeInt32Array>::New();
        array->DeepCopy(actualArray);
      }
      break;
      case VTK_UNSIGNED_LONG_LONG:
      {
        array = vtkSmartPointer<vtkTypeUInt32Array>::New();
        array->DeepCopy(actualArray);
      }
      break;
      default:
        // Do nothing
        break;
    }
    vtktype = array->GetDataType();
    nbytes += array->GetDataSize() * array->GetDataTypeSize();
    numberOfComponents = this->ScalarComponents ? 1 : array->GetNumberOfComponents();
    numberOfTuples += (this->ScalarComponents ? array->GetMaxId() + 1 : array->GetNumberOfTuples());
    arraysToUpload.emplace_back(array);
  }

  // 2. Request an allocation for the GPU buffer.
  this->Buffer->Allocate(nbytes, this->BufferType, this->BufferUsage);

  // 3. Upload each array one by one into a gigantic GPU buffer.
  ptrdiff_t offset = 0;
  for (auto& array : arraysToUpload)
  {
#ifdef vtkOpenGLArrayTextureBufferAdapter_DEBUG
    std::cout << "Uploading Array: " << actualArray->GetObjectDescription()
              << "(name: " << actualArray->GetName() << ")" << std::endl;
#endif
    // Now upload the array
    switch (array->GetDataType())
    {
      vtkTemplateMacro(this->Buffer->UploadRange(static_cast<VTK_TT*>(array->GetVoidPointer(0)),
        offset, array->GetMaxId() + 1, this->BufferType));
    }
    offset += array->GetDataSize() * array->GetDataTypeSize();
  }

  // 4. Sync the buffer with a texture.
  // if (updateInternalTextureFormat)
  {
    this->Texture->GetInternalFormat(vtktype, numberOfComponents, this->IntegerTexture);
  }
  this->Texture->CreateTextureBuffer(numberOfTuples, numberOfComponents, vtktype, this->Buffer);
  // this->Texture->Activate();
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferAdapter::ReleaseGraphicsResources(vtkWindow* window)
{
  if (this->Texture)
  {
    this->Texture->ReleaseGraphicsResources(window);
    this->Texture = nullptr;
  }
  if (this->Buffer)
  {
    this->Buffer->ReleaseGraphicsResources();
    this->Buffer = nullptr;
  }
}
VTK_ABI_NAMESPACE_END
