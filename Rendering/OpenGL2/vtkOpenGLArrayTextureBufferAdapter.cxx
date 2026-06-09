// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLArrayTextureBufferAdapter.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkOpenGLArrayTextureBufferCache.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureObject.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeUInt32Array.h"

#include "vtk_glad.h" // for GL_ES_VERSION_3_0, which selects the direct-upload path below

#include <cstddef>
#include <iostream>

// Uncomment to print upload events.
// #define vtkOpenGLArrayTextureBufferAdapter_DEBUG

VTK_ABI_NAMESPACE_BEGIN

namespace
{
// Appends one (already narrowed) data array's values to the end of a byte vector, using a typed
// value range so no unsafe vtkDataArray::GetVoidPointer() access is required. The destination is
// grown to fit and the values are written packed; the upload paths then source straight from the
// byte vector.
struct ByteAppender
{
  std::vector<unsigned char>* Destination = nullptr;

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    const auto values = vtk::DataArrayValueRange(array);
    using ValueType = vtk::GetAPIType<ArrayT>;
    const std::size_t base = this->Destination->size();
    this->Destination->resize(base + static_cast<std::size_t>(values.size()) * sizeof(ValueType));
    auto* out = reinterpret_cast<ValueType*>(this->Destination->data() + base);
    vtkIdType i = 0;
    for (const auto value : values)
    {
      out[i++] = value;
    }
  }
};
} // anonymous namespace

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
  if (this->Arrays.empty())
  {
    // There are no arrays to upload.
    return;
  }

  // Single-array adapters share their uploaded texture across mappers through the
  // per-context cache. Multi-array (concatenated) adapters cannot be keyed on a single
  // array, so they keep owning their texture (the uncached path below). The cache is also
  // unavailable when there is no render window.
  vtkOpenGLArrayTextureBufferCache* cache =
    renderWindow ? renderWindow->GetArrayTextureBufferCache() : nullptr;
  vtkDataArray* sourceArray = this->Arrays.size() == 1 ? this->Arrays.front().Get() : nullptr;
  if (cache && sourceArray)
  {
    auto entry = cache->GetTextureBuffer(sourceArray, this->ScalarComponents, this->IntegerTexture);
    if (entry)
    {
      // If this adapter was previously on the uncached path it owns a texture that the shared
      // entry is about to replace; release it so it is not leaked.
      if (this->OwnsTexture)
      {
        if (this->Texture && this->Texture != entry->Texture)
        {
          this->Texture->ReleaseGraphicsResources(renderWindow);
          this->Texture = nullptr;
        }
      }
      if (this->OwnsTexture)
      {
        if (this->Buffer && this->Buffer != entry->Buffer)
        {
          this->Buffer->ReleaseGraphicsResources();
          this->Buffer = nullptr;
        }
      }
      // Reference the shared GL objects; the cache, not this adapter, owns their lifetime.
      this->Texture = entry->Texture;
      this->Buffer = entry->Buffer;
      this->OwnsTexture = false;
      const vtkMTimeType sourceMTime = sourceArray->GetMTime();
      if (!force && entry->Texture && entry->UploadTime >= sourceMTime)
      {
        // A peer mapper already uploaded this array unchanged; reuse it.
        this->Uploaded = true;
        return;
      }
      this->UploadImpl(renderWindow);
      // Store back the (possibly newly created) GL objects and the version we uploaded.
      entry->Texture = this->Texture;
      entry->Buffer = this->Buffer;
      entry->UploadTime = sourceMTime;
      this->Uploaded = true;
      return;
    }
  }

  // Uncached path: this adapter owns its texture/buffer.
  this->OwnsTexture = true;
  if (this->Uploaded && !force)
  {
    // We don't need to re-upload.
    return;
  }
  this->UploadImpl(renderWindow);
  this->Uploaded = true;
}

//------------------------------------------------------------------------------
vtkOpenGLArrayTextureBufferAdapter::NarrowedArrayInfo
vtkOpenGLArrayTextureBufferAdapter::AppendNarrowed(
  vtkDataArray* actualArray, std::vector<unsigned char>& dest) const
{
  vtkSmartPointer<vtkDataArray> array = actualArray;
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
  array = array->ToAOSDataArray();

  // Append the narrowed values as raw bytes through a typed dispatch worker (no GetVoidPointer).
  ByteAppender appender;
  appender.Destination = &dest;
  if (!vtkArrayDispatch::Dispatch::Execute(array, appender))
  {
    // Fallback for value types outside the dispatch list (should not occur after narrowing +
    // ToAOSDataArray() above, but keeps the copy correct if it ever does).
    appender(array.Get());
  }

  NarrowedArrayInfo info;
  info.Vtktype = array->GetDataType();
  info.NumComps = this->ScalarComponents ? 1 : array->GetNumberOfComponents();
  info.TexelCount = this->ScalarComponents ? (array->GetMaxId() + 1) : array->GetNumberOfTuples();
  info.ByteLength = static_cast<std::size_t>(array->GetDataSize()) * array->GetDataTypeSize();
  return info;
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferAdapter::UploadImpl(vtkOpenGLRenderWindow* renderWindow)
{
  if (this->Texture == nullptr)
  {
    // We need to create the texture.
    this->Texture = vtkSmartPointer<vtkTextureObject>::New();
    this->Texture->SetRequireTextureInteger(this->IntegerTexture);
    this->Texture->SetContext(renderWindow);
  }

  const std::size_t n = this->Arrays.size();
  // Per-sub-array working state. `sliceBytes[i]` holds the narrowed bytes of a *changed* block
  // (or is left empty for a reused one): only sub-arrays whose data changed since the last
  // upload (or all of them, on a layout change / first upload) are narrowed, so unchanged blocks
  // cost neither a value copy nor a GPU transfer.
  std::vector<std::vector<unsigned char>> sliceBytes(n);
  std::vector<std::size_t> byteLength(n, 0);
  std::vector<vtkIdType> texelCount(n, 0);
  std::vector<vtkMTimeType> srcMTime(n, 0);
  std::vector<const void*> srcId(n, nullptr);
  std::vector<bool> changed(n, true);

  const bool layoutCountMatches = this->LastUpload.Valid && this->LastUpload.Subs.size() == n &&
    this->LastUpload.Scalars == this->ScalarComponents;

  int vtktype = 0;
  int numberOfComponents = 0;
  bool haveFormat = false;

  // 1. Figure out, per sub-array, whether it changed and how many bytes/texels it contributes.
  // Reuse the recorded size for unchanged blocks (skipping the narrow); narrow changed ones into
  // their slice byte buffer.
  for (std::size_t i = 0; i < n; ++i)
  {
    vtkDataArray* actualArray = this->Arrays[i];
    srcId[i] = actualArray;
    srcMTime[i] = actualArray->GetMTime();
    const bool reuse = layoutCountMatches &&
      this->LastUpload.Subs[i].SourceId == static_cast<const void*>(actualArray) &&
      this->LastUpload.Subs[i].MTime == srcMTime[i];
    if (reuse)
    {
      changed[i] = false;
      byteLength[i] = this->LastUpload.Subs[i].ByteLength;
      texelCount[i] = this->LastUpload.Subs[i].TexelCount;
    }
    else
    {
      const NarrowedArrayInfo info = this->AppendNarrowed(actualArray, sliceBytes[i]);
      byteLength[i] = info.ByteLength;
      texelCount[i] = info.TexelCount;
      if (!haveFormat)
      {
        // The value type and component count are shared by every sub-array bound to one texture.
        vtktype = info.Vtktype;
        numberOfComponents = info.NumComps;
        haveFormat = true;
      }
    }
  }
  if (!haveFormat)
  {
    // Every block was reused; recover the format from the recorded layout.
    vtktype = this->LastUpload.Vtktype;
    numberOfComponents = this->LastUpload.NumComps;
  }

  // 2. Compute slice offsets and totals.
  std::vector<std::size_t> byteOffset(n, 0);
  std::vector<vtkIdType> texelOffset(n, 0);
  std::size_t nbytes = 0;
  vtkIdType numberOfTuples = 0;
  for (std::size_t i = 0; i < n; ++i)
  {
    byteOffset[i] = nbytes;
    texelOffset[i] = numberOfTuples;
    nbytes += byteLength[i];
    numberOfTuples += texelCount[i];
  }

  // 3. Decide whether the layout is unchanged so we can push only the changed slices.
  // Restricted to adapters that own their texture: the shared-cache path (OwnsTexture == false)
  // dedupes whole single arrays by MTime already and its texture/buffer are owned elsewhere.
  bool canPartial = this->OwnsTexture && this->LastUpload.Valid && layoutCountMatches &&
    this->LastUpload.TotalBytes == nbytes && this->LastUpload.Vtktype == vtktype &&
    this->LastUpload.NumComps == numberOfComponents && this->Texture != nullptr;
#ifndef GL_ES_VERSION_3_0
  canPartial = canPartial && this->Buffer != nullptr && this->Buffer->GetHandle() != 0;
#endif
  if (canPartial)
  {
    for (std::size_t i = 0; i < n; ++i)
    {
      if (byteOffset[i] != this->LastUpload.Subs[i].ByteOffset ||
        byteLength[i] != this->LastUpload.Subs[i].ByteLength)
      {
        canPartial = false;
        break;
      }
    }
  }

  if (canPartial)
  {
    // Partial upload: only re-transfer the slices whose source data changed.
    for (std::size_t i = 0; i < n; ++i)
    {
      if (!changed[i])
      {
        continue;
      }
#ifdef GL_ES_VERSION_3_0
      this->Texture->UpdateTextureBuffer2DRegion(static_cast<unsigned int>(texelOffset[i]),
        static_cast<unsigned int>(texelCount[i]), numberOfComponents, vtktype,
        sliceBytes[i].data());
#else
      this->Buffer->UploadRange(sliceBytes[i].data(), static_cast<ptrdiff_t>(byteOffset[i]),
        sliceBytes[i].size(), this->BufferType);
#endif
    }
  }
  else
  {
    // Full (re)build: assemble the whole concatenated buffer (narrowing any blocks we skipped),
    // then reallocate and upload it in one transfer.
    std::vector<unsigned char> cpuData;
    cpuData.reserve(nbytes);
    for (std::size_t i = 0; i < n; ++i)
    {
      if (changed[i])
      {
        cpuData.insert(cpuData.end(), sliceBytes[i].begin(), sliceBytes[i].end());
      }
      else
      {
        this->AppendNarrowed(this->Arrays[i], cpuData);
      }
    }
#ifdef GL_ES_VERSION_3_0
    // GLES/WebGL2 has no real texture buffers; vtkTextureObject emulates them with a 2D
    // texture. Upload the (concatenated) data straight from client memory: ANGLE has no fast
    // GPU GL_PIXEL_UNPACK_BUFFER->texture path, so staging through a buffer object forces a CPU
    // copy of the unpack buffer on every upload (and wastes a redundant GPU allocation). See
    // vtkTextureObject::EmulateTextureBufferWith2DTexturesFromRaw.
    this->Texture->GetInternalFormat(vtktype, numberOfComponents, this->IntegerTexture);
    this->Texture->EmulateTextureBufferWith2DTexturesFromRaw(
      numberOfTuples, numberOfComponents, vtktype, cpuData.data());
#else
    // Request an allocation for the GPU buffer, then upload all the bytes in one call.
    if (this->Buffer == nullptr)
    {
      this->Buffer = vtkSmartPointer<vtkOpenGLBufferObject>::New();
      this->Buffer->SetType(this->BufferType);
    }
    this->Buffer->Allocate(nbytes, this->BufferType, this->BufferUsage);
    this->Buffer->UploadRange(cpuData.data(), 0, cpuData.size(), this->BufferType);

    // Sync the buffer with a texture.
    this->Texture->GetInternalFormat(vtktype, numberOfComponents, this->IntegerTexture);
    this->Texture->CreateTextureBuffer(numberOfTuples, numberOfComponents, vtktype, this->Buffer);
#endif
  }

  // 4. Record what we just uploaded so the next call can diff against it.
  this->LastUpload.Subs.resize(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    this->LastUpload.Subs[i] = SubArrayUpload{ srcId[i], srcMTime[i], byteOffset[i], byteLength[i],
      texelOffset[i], texelCount[i] };
  }
  this->LastUpload.TotalBytes = nbytes;
  this->LastUpload.TotalTuples = numberOfTuples;
  this->LastUpload.NumComps = numberOfComponents;
  this->LastUpload.Vtktype = vtktype;
  this->LastUpload.Scalars = this->ScalarComponents;
  this->LastUpload.Valid = true;
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferAdapter::ReleaseGraphicsResources(vtkWindow* window)
{
  // Only release the GL resources we own. When the texture/buffer belong to a shared
  // vtkOpenGLArrayTextureBufferCache entry, the cache releases them (and other adapters may
  // still reference them); here we just drop our reference.
  if (this->OwnsTexture)
  {
    if (this->Texture)
    {
      this->Texture->ReleaseGraphicsResources(window);
    }
    if (this->Buffer)
    {
      this->Buffer->ReleaseGraphicsResources();
    }
  }
  this->Texture = nullptr;
  this->Buffer = nullptr;
  this->Uploaded = false;
  // The GL resources backing the recorded layout are gone; force a full upload next time.
  this->LastUpload = UploadedLayout{};
}
VTK_ABI_NAMESPACE_END
