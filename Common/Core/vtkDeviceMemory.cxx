// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeviceMemory.h"

#include <vtksys/DynamicLoader.hxx>

#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
//------------------------------------------------------------------------------
// The CUDA driver API, declared here rather than included: VTK builds without
// the CUDA toolkit. These types and values are part of the driver's stable ABI.
using CUresult = int;
using CUdevice = int;
using CUdeviceptr = unsigned long long;
using CUcontext = struct CUctx_st*;

constexpr CUresult CUDA_SUCCESS = 0;
constexpr int CU_POINTER_ATTRIBUTE_CONTEXT = 1;
constexpr int CU_POINTER_ATTRIBUTE_MEMORY_TYPE = 2;
constexpr int CU_POINTER_ATTRIBUTE_IS_MANAGED = 8;
constexpr int CU_POINTER_ATTRIBUTE_DEVICE_ORDINAL = 9;
constexpr unsigned int CU_MEMORYTYPE_DEVICE = 2;

struct CudaDriver
{
  bool Loaded = false;
  std::string Error;

  CUresult (*Init)(unsigned int) = nullptr;
  CUresult (*PointerGetAttribute)(void*, int, CUdeviceptr) = nullptr;
  CUresult (*DeviceGet)(CUdevice*, int) = nullptr;
  CUresult (*DevicePrimaryCtxRetain)(CUcontext*, CUdevice) = nullptr;
  CUresult (*DevicePrimaryCtxRelease)(CUdevice) = nullptr;
  CUresult (*CtxPushCurrent)(CUcontext) = nullptr;
  CUresult (*CtxPopCurrent)(CUcontext*) = nullptr;
  CUresult (*CtxSynchronize)() = nullptr;
  CUresult (*MemcpyDtoH)(void*, CUdeviceptr, std::size_t) = nullptr;
  CUresult (*GetErrorName)(CUresult, const char**) = nullptr;

  std::string Describe(CUresult result) const
  {
    const char* name = nullptr;
    if (this->GetErrorName && this->GetErrorName(result, &name) == CUDA_SUCCESS && name)
    {
      return name;
    }
    return "CUDA error " + std::to_string(result);
  }
};

template <typename Function>
bool Bind(vtksys::DynamicLoader::LibraryHandle library, const char* symbol, Function& function,
  std::string& error)
{
  function = reinterpret_cast<Function>(vtksys::DynamicLoader::GetSymbolAddress(library, symbol));
  if (!function)
  {
    error = std::string("the CUDA driver has no ") + symbol;
    return false;
  }
  return true;
}

CudaDriver LoadCuda()
{
  CudaDriver cuda;
#if defined(_WIN32)
  const char* name = "nvcuda.dll";
#elif defined(__APPLE__)
  const char* name = nullptr;
#else
  const char* name = "libcuda.so.1";
#endif
  if (!name)
  {
    cuda.Error = "there is no CUDA driver on this platform";
    return cuda;
  }
  // Never closed: arrays may read through it until the process ends.
  auto library = vtksys::DynamicLoader::OpenLibrary(name);
  if (library == vtksys::DynamicLoader::InvalidLibrary)
  {
    const char* reason = vtksys::DynamicLoader::LastError();
    cuda.Error = std::string("could not load ") + name + (reason ? std::string(": ") + reason : "");
    return cuda;
  }
  std::string& error = cuda.Error;
  // The _v2 names are what cuda.h maps the plain names to.
  if (!Bind(library, "cuInit", cuda.Init, error) ||
    !Bind(library, "cuPointerGetAttribute", cuda.PointerGetAttribute, error) ||
    !Bind(library, "cuDeviceGet", cuda.DeviceGet, error) ||
    !Bind(library, "cuDevicePrimaryCtxRetain", cuda.DevicePrimaryCtxRetain, error) ||
    !Bind(library, "cuDevicePrimaryCtxRelease_v2", cuda.DevicePrimaryCtxRelease, error) ||
    !Bind(library, "cuCtxPushCurrent_v2", cuda.CtxPushCurrent, error) ||
    !Bind(library, "cuCtxPopCurrent_v2", cuda.CtxPopCurrent, error) ||
    !Bind(library, "cuCtxSynchronize", cuda.CtxSynchronize, error) ||
    !Bind(library, "cuMemcpyDtoH_v2", cuda.MemcpyDtoH, error) ||
    !Bind(library, "cuGetErrorName", cuda.GetErrorName, error))
  {
    return cuda;
  }
  CUresult result = cuda.Init(0);
  if (result != CUDA_SUCCESS)
  {
    cuda.Error = "cuInit failed: " + cuda.Describe(result);
    return cuda;
  }
  cuda.Loaded = true;
  return cuda;
}

const CudaDriver& Cuda()
{
  static const CudaDriver cuda = LoadCuda(); // loaded once, on first use
  return cuda;
}

bool Fail(std::string* why, const std::string& reason)
{
  if (why)
  {
    *why = reason;
  }
  return false;
}

bool CudaIsDevicePointer(const void* pointer, std::string* why)
{
  const CudaDriver& cuda = Cuda();
  if (!cuda.Loaded)
  {
    return Fail(why, cuda.Error);
  }
  const auto address = reinterpret_cast<CUdeviceptr>(pointer);
  unsigned int type = 0;
  CUresult result = cuda.PointerGetAttribute(&type, CU_POINTER_ATTRIBUTE_MEMORY_TYPE, address);
  if (result != CUDA_SUCCESS)
  {
    return Fail(why, "not memory CUDA knows (" + cuda.Describe(result) + ")");
  }
  unsigned int managed = 0;
  cuda.PointerGetAttribute(&managed, CU_POINTER_ATTRIBUTE_IS_MANAGED, address);
  if (type != CU_MEMORYTYPE_DEVICE && !managed)
  {
    return Fail(why, "CUDA host memory, not device memory");
  }
  return true;
}

bool CudaCopyToHost(void* destination, const void* source, std::size_t bytes, std::string* why)
{
  const CudaDriver& cuda = Cuda();
  if (!cuda.Loaded)
  {
    return Fail(why, cuda.Error);
  }
  const auto address = reinterpret_cast<CUdeviceptr>(source);

  // Copy in the context the memory belongs to. Memory from a pool or mapped
  // through the virtual memory API has none of its own; it is its device's,
  // and that device's primary context reaches it.
  CUcontext context = nullptr;
  CUdevice device = 0;
  bool retained = false;
  if (cuda.PointerGetAttribute(&context, CU_POINTER_ATTRIBUTE_CONTEXT, address) != CUDA_SUCCESS ||
    !context)
  {
    int ordinal = -1;
    CUresult result =
      cuda.PointerGetAttribute(&ordinal, CU_POINTER_ATTRIBUTE_DEVICE_ORDINAL, address);
    if (result != CUDA_SUCCESS)
    {
      return Fail(why, "not memory CUDA knows (" + cuda.Describe(result) + ")");
    }
    if ((result = cuda.DeviceGet(&device, ordinal)) != CUDA_SUCCESS ||
      (result = cuda.DevicePrimaryCtxRetain(&context, device)) != CUDA_SUCCESS)
    {
      return Fail(why,
        "no context for device " + std::to_string(ordinal) + " (" + cuda.Describe(result) + ")");
    }
    retained = true;
  }

  CUresult result = cuda.CtxPushCurrent(context);
  if (result == CUDA_SUCCESS)
  {
    // The producer's work may still be queued on a stream of its own.
    result = cuda.CtxSynchronize();
    if (result == CUDA_SUCCESS)
    {
      result = cuda.MemcpyDtoH(destination, address, bytes);
    }
    CUcontext popped = nullptr;
    cuda.CtxPopCurrent(&popped);
  }
  if (retained)
  {
    cuda.DevicePrimaryCtxRelease(device);
  }
  if (result != CUDA_SUCCESS)
  {
    return Fail(why,
      "copying " + std::to_string(bytes) + " bytes off the GPU failed (" + cuda.Describe(result) +
        ")");
  }
  return true;
}

const char* NotYet(vtkDataArray::MemorySpace space)
{
  return space == vtkDataArray::HipDeviceMemory ? "HIP memory is not supported yet"
                                                : "Level Zero memory is not supported yet";
}
} // namespace

//------------------------------------------------------------------------------
bool vtkDeviceMemory::SpaceFromName(const char* name, vtkDataArray::MemorySpace& space)
{
  if (!name)
  {
    return false;
  }
  if (std::strcmp(name, "cuda") == 0)
  {
    space = vtkDataArray::CudaDeviceMemory;
  }
  else if (std::strcmp(name, "hip") == 0)
  {
    space = vtkDataArray::HipDeviceMemory;
  }
  else if (std::strcmp(name, "level_zero") == 0)
  {
    space = vtkDataArray::LevelZeroDeviceMemory;
  }
  else
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
const char* vtkDeviceMemory::NameOf(vtkDataArray::MemorySpace space)
{
  switch (space)
  {
    case vtkDataArray::CudaDeviceMemory:
      return "cuda";
    case vtkDataArray::HipDeviceMemory:
      return "hip";
    case vtkDataArray::LevelZeroDeviceMemory:
      return "level_zero";
    default:
      return "host";
  }
}

//------------------------------------------------------------------------------
bool vtkDeviceMemory::IsAvailable(vtkDataArray::MemorySpace space, std::string* why)
{
  if (space == vtkDataArray::CudaDeviceMemory)
  {
    return Cuda().Loaded || Fail(why, Cuda().Error);
  }
  if (space == vtkDataArray::HostMemory)
  {
    return Fail(why, "host memory is not device memory");
  }
  return Fail(why, NotYet(space));
}

//------------------------------------------------------------------------------
bool vtkDeviceMemory::IsDevicePointer(
  vtkDataArray::MemorySpace space, const void* pointer, std::string* why)
{
  if (space == vtkDataArray::CudaDeviceMemory)
  {
    return CudaIsDevicePointer(pointer, why);
  }
  return vtkDeviceMemory::IsAvailable(space, why);
}

//------------------------------------------------------------------------------
bool vtkDeviceMemory::CopyToHost(vtkDataArray::MemorySpace space, void* destination,
  const void* source, std::size_t bytes, std::string* why)
{
  if (bytes == 0)
  {
    return true;
  }
  if (space == vtkDataArray::CudaDeviceMemory)
  {
    return CudaCopyToHost(destination, source, bytes, why);
  }
  return vtkDeviceMemory::IsAvailable(space, why);
}

VTK_ABI_NAMESPACE_END
