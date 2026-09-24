// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkmDataArrayFactory.h"

#include "vtkMemoryDescriptor.h"
#include "vtkmDataArray.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkType.h"

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/internal/Buffer.h>
#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>

#include <cstring>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkmDataArrayFactory);

namespace
{

//------------------------------------------------------------------------------
// Map a memory space to the Viskores device that can read it.
//
// Returns false when the space cannot be honoured. That distinction matters:
// falling back to "host" for anything unrecognised hands VTK a device pointer
// to dereference, which is a crash at best and silent garbage at worst. It is
// also the failure mode that hides a misconfigured build -- a "cuda" buffer on
// a build without CUDA support would quietly become a host buffer.
bool MemorySpaceToDevice(const char* space, viskores::cont::DeviceAdapterId& device)
{
  // Set before any early return, so no path leaves @a device untouched.
  // Undefined is not a failure marker here: it is what host memory uses,
  // meaning "no particular device", which is why the answer travels in the
  // return value rather than in the id.
  device = viskores::cont::DeviceAdapterTagUndefined{};

  if (!space || std::strcmp(space, "host") == 0)
  {
    return true;
  }

#ifdef VISKORES_ENABLE_CUDA
  if (std::strcmp(space, "cuda") == 0)
  {
    device = viskores::cont::DeviceAdapterTagCuda{};
    return true;
  }
#endif
#ifdef VISKORES_ENABLE_KOKKOS
  if (std::strcmp(space, "cuda") == 0 || std::strcmp(space, "hip") == 0)
  {
    device = viskores::cont::DeviceAdapterTagKokkos{};
    return true;
  }
#endif

  // "level_zero", or "cuda"/"hip" on a build without the matching device
  // adapter. Either way the memory is not readable from where we are.
  return false;
}

//------------------------------------------------------------------------------
// What a wrapped buffer holds on behalf of its producer.
//
// Viskores calls the deleter with the *container* pointer when it releases the
// buffer, which is the hook that lets an external producer know its memory is
// no longer referenced. One of these is the container.
struct ExternalHold
{
  vtkMemoryDescriptor::ReleaseFunction Release = nullptr;
  void* ReleaseContext = nullptr;
  vtkObject* Owner = nullptr;
};

//------------------------------------------------------------------------------
// Called by Viskores when the buffer is released. Runs on whichever thread
// dropped the last reference, which may be a worker thread deep inside a
// filter, so it must not assume any particular runtime state.
void ReleaseExternalHold(void* container)
{
  auto* hold = static_cast<ExternalHold*>(container);
  if (!hold)
  {
    return;
  }
  if (hold->Release)
  {
    hold->Release(hold->ReleaseContext);
  }
  if (hold->Owner)
  {
    hold->Owner->UnRegister(nullptr);
  }
  delete hold;
}

//------------------------------------------------------------------------------
// Take over whatever keeps @a descriptor's memory alive.
//
// After this the descriptor no longer holds the memory open -- the returned
// hold does, and it is freed by ReleaseExternalHold when Viskores is done.
ExternalHold* TakeHold(vtkMemoryDescriptor* descriptor)
{
  auto* hold = new ExternalHold;
  if (!descriptor->TakeRelease(hold->Release, hold->ReleaseContext))
  {
    // No callback. A VTK owner is held by reference instead.
    if (vtkObject* owner = descriptor->GetOwner())
    {
      owner->Register(nullptr);
      hold->Owner = owner;
    }
  }
  return hold;
}

//------------------------------------------------------------------------------
// Wrap an external pointer as a Viskores buffer that does not own its memory
// but does hold the producer open.
viskores::cont::internal::Buffer WrapPointer(vtkTypeInt64 pointer, vtkTypeInt64 sizeInBytes,
  viskores::cont::DeviceAdapterId device, ExternalHold* hold)
{
  void* ptr = reinterpret_cast<void*>(pointer);

  // InvalidRealloc, not a no-op: Viskores updates its recorded size whatever
  // the reallocation function does, so a no-op leaves it believing the buffer
  // grew and writing past the end of memory it does not own. Throwing turns
  // that into an error at the point of the mistake.
  viskores::cont::internal::BufferInfo bufInfo(device, ptr, hold,
    static_cast<viskores::BufferSizeType>(sizeInBytes), ReleaseExternalHold,
    viskores::cont::internal::InvalidRealloc);

  viskores::cont::internal::Buffer buffer;
  buffer.Reset(bufInfo);
  return buffer;
}

//------------------------------------------------------------------------------
template <typename T>
vtkDataArray* CreateAOSTyped(vtkMemoryDescriptor* descriptor, int numComponents)
{
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagUndefined{};
  MemorySpaceToDevice(descriptor->GetMemorySpace(), device);
  auto buffer = WrapPointer(
    descriptor->GetPointer(), descriptor->GetSizeInBytes(), device, TakeHold(descriptor));

  viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> handle;
  handle.GetBuffers()[0] = buffer;

  auto* result = vtkmDataArray<T>::New();
  result->SetNumberOfComponents(numComponents);
  result->SetVtkmArrayHandle(viskores::cont::ArrayHandleRuntimeVec<T>(numComponents, handle));
  return result;
}

//------------------------------------------------------------------------------
template <typename T>
vtkDataArray* CreateSOATyped(
  const std::vector<vtkMemoryDescriptor*>& descriptors, int numComponents)
{
  std::vector<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>> components(
    numComponents);
  for (int c = 0; c < numComponents; ++c)
  {
    auto* descriptor = descriptors[c];
    viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagUndefined{};
    MemorySpaceToDevice(descriptor->GetMemorySpace(), device);
    components[c].GetBuffers()[0] = WrapPointer(
      descriptor->GetPointer(), descriptor->GetSizeInBytes(), device, TakeHold(descriptor));
  }

  auto* result = vtkmDataArray<T>::New();
  result->SetNumberOfComponents(numComponents);

  switch (numComponents)
  {
    case 1:
      result->SetVtkmArrayHandle(components[0]);
      break;
    case 2:
      result->SetVtkmArrayHandle(viskores::cont::make_ArrayHandleSOA(components[0], components[1]));
      break;
    case 3:
      result->SetVtkmArrayHandle(
        viskores::cont::make_ArrayHandleSOA(components[0], components[1], components[2]));
      break;
    case 4:
      result->SetVtkmArrayHandle(viskores::cont::make_ArrayHandleSOA(
        components[0], components[1], components[2], components[3]));
      break;
    default:
      result->Delete();
      return nullptr;
  }
  return result;
}

//------------------------------------------------------------------------------
// Dispatch on VTK type. Returns nullptr for types Viskores arrays do not cover.
template <typename Creator>
vtkDataArray* DispatchOnType(int dataType, Creator&& create)
{
  switch (dataType)
  {
    case VTK_FLOAT:
      return create(static_cast<float*>(nullptr));
    case VTK_DOUBLE:
      return create(static_cast<double*>(nullptr));
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
      return create(static_cast<signed char*>(nullptr));
    case VTK_UNSIGNED_CHAR:
      return create(static_cast<unsigned char*>(nullptr));
    case VTK_SHORT:
      return create(static_cast<short*>(nullptr));
    case VTK_UNSIGNED_SHORT:
      return create(static_cast<unsigned short*>(nullptr));
    case VTK_INT:
      return create(static_cast<int*>(nullptr));
    case VTK_UNSIGNED_INT:
      return create(static_cast<unsigned int*>(nullptr));
    case VTK_LONG:
      return create(static_cast<long*>(nullptr));
    case VTK_UNSIGNED_LONG:
      return create(static_cast<unsigned long*>(nullptr));
    case VTK_LONG_LONG:
    case VTK_ID_TYPE:
      return create(static_cast<long long*>(nullptr));
    case VTK_UNSIGNED_LONG_LONG:
      return create(static_cast<unsigned long long*>(nullptr));
    default:
      return nullptr;
  }
}

} // anonymous namespace

//------------------------------------------------------------------------------
vtkmDataArrayFactory::vtkmDataArrayFactory() = default;

//------------------------------------------------------------------------------
vtkmDataArrayFactory::~vtkmDataArrayFactory()
{
  this->ClearBuffers();
}

//------------------------------------------------------------------------------
void vtkmDataArrayFactory::AddBuffer(vtkMemoryDescriptor* descriptor)
{
  if (descriptor)
  {
    descriptor->Register(this);
    this->Buffers.push_back(descriptor);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkmDataArrayFactory::ClearBuffers()
{
  for (auto* descriptor : this->Buffers)
  {
    if (descriptor)
    {
      descriptor->UnRegister(this);
    }
  }
  this->Buffers.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkmDataArrayFactory::GetNumberOfBuffers() const
{
  return static_cast<int>(this->Buffers.size());
}

//------------------------------------------------------------------------------
vtkDataArray* vtkmDataArrayFactory::CreateArray()
{
  const int numBuffers = this->GetNumberOfBuffers();
  if (numBuffers == 0)
  {
    vtkErrorMacro("No buffers added. Call AddBuffer() before CreateArray().");
    return nullptr;
  }
  if (numBuffers != 1 && numBuffers != this->NumberOfComponents)
  {
    vtkErrorMacro("Buffer count (" << numBuffers << ") must be 1 for AoS, or "
                                   << this->NumberOfComponents << " for SoA.");
    return nullptr;
  }
  if (this->NumberOfComponents < 1)
  {
    vtkErrorMacro("NumberOfComponents must be at least 1.");
    return nullptr;
  }
  if (numBuffers > 1 && this->NumberOfComponents > 4)
  {
    // Checked here, before anything is wrapped. Viskores' SoA handles stop
    // at four components, and finding that out down in CreateSOATyped would
    // mean the descriptors' holds had already been taken and installed on
    // buffers -- which are then released on the way out, calling the
    // producer's release from a call that reports failure.
    vtkErrorMacro("SoA layout is limited to 4 components; got " << this->NumberOfComponents
                                                                << ". Use an AoS buffer instead.");
    return nullptr;
  }

  for (int i = 0; i < numBuffers; ++i)
  {
    if (this->Buffers[i]->GetPointer() == 0)
    {
      vtkErrorMacro("Buffer " << i << " has a null pointer.");
      return nullptr;
    }
    viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagUndefined{};
    if (!MemorySpaceToDevice(this->Buffers[i]->GetMemorySpace(), device))
    {
      const char* space = this->Buffers[i]->GetMemorySpace();
      vtkErrorMacro("Buffer " << i << " is in memory space '" << (space ? space : "(null)")
                              << "', which this build cannot read. Either the space has no "
                                 "Viskores device adapter, or VTK was built without support "
                                 "for it. Refusing rather than treating it as host memory, "
                                 "which would hand out a pointer that cannot be dereferenced.");
      return nullptr;
    }
    if (!this->Buffers[i]->HasOwnership())
    {
      // Legal, but the caller is now responsible for outliving every filter
      // that touches this array -- and a mistake shows up as wrong numbers
      // from freed device memory, not as a crash.
      vtkWarningMacro(
        "Buffer " << i
                  << " carries no ownership: nothing keeps this memory alive while "
                     "Viskores holds it. Use vtkMemoryDescriptor::SetOwner() or "
                     "SetRelease() unless the lifetime is guaranteed some other way.");
    }
  }

  vtkDataArray* result = nullptr;
  if (numBuffers == 1)
  {
    auto* descriptor = this->Buffers[0];
    const int numComponents = this->NumberOfComponents;
    result = DispatchOnType(this->DataType,
      [descriptor, numComponents](auto* tag) -> vtkDataArray*
      {
        using T = std::remove_pointer_t<decltype(tag)>;
        return CreateAOSTyped<T>(descriptor, numComponents);
      });
  }
  else
  {
    const auto& descriptors = this->Buffers;
    const int numComponents = this->NumberOfComponents;
    result = DispatchOnType(this->DataType,
      [&descriptors, numComponents](auto* tag) -> vtkDataArray*
      {
        using T = std::remove_pointer_t<decltype(tag)>;
        return CreateSOATyped<T>(descriptors, numComponents);
      });
  }

  if (!result)
  {
    vtkErrorMacro("Could not create an array for data type "
      << this->DataType << " with " << this->NumberOfComponents << " components.");
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkmDataArrayFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTuples: " << this->NumberOfTuples << "\n";
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << "\n";
  os << indent << "DataType: " << this->DataType << "\n";
  os << indent << "NumberOfBuffers: " << this->GetNumberOfBuffers() << "\n";
}

VTK_ABI_NAMESPACE_END
