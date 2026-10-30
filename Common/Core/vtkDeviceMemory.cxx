// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeviceMemory.h"

#include <vtksys/DynamicLoader.hxx>

#if !defined(_WIN32)
#include <dlfcn.h> // For RTLD_NOLOAD
#endif

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

//------------------------------------------------------------------------------
// The HIP runtime API, declared here rather than included: VTK builds without
// ROCm. The values are ROCm 7.1.1's (hip/driver_types.h, hip/hip_runtime_api.h).
using hipError_t = int;

constexpr hipError_t hipSuccess = 0;
constexpr int HIP_POINTER_ATTRIBUTE_MEMORY_TYPE = 2;
constexpr int HIP_POINTER_ATTRIBUTE_IS_MANAGED = 8;
constexpr int HIP_POINTER_ATTRIBUTE_DEVICE_ORDINAL = 9;
constexpr int hipMemoryTypeDevice = 2;
constexpr int hipMemoryTypeManaged = 3;
constexpr int hipMemcpyDeviceToHost = 2;

struct HipRuntime
{
  bool Loaded = false;
  std::string Error;

  hipError_t (*GetDeviceCount)(int*) = nullptr;
  hipError_t (*PointerGetAttribute)(void*, int, void*) = nullptr;
  hipError_t (*GetDevice)(int*) = nullptr;
  hipError_t (*SetDevice)(int) = nullptr;
  hipError_t (*DeviceSynchronize)() = nullptr;
  hipError_t (*Memcpy)(void*, const void*, std::size_t, int) = nullptr;
  hipError_t (*GetLastError)() = nullptr;
  const char* (*GetErrorName)(hipError_t) = nullptr;
  const char* (*GetErrorString)(hipError_t) = nullptr;

  // A failed call also leaves its error as the thread's last error, which
  // successful calls do not clear. A producer checking hipGetLastError after
  // its own launches (PyTorch does) would take it for its own, so take it here.
  void ClearError() const { this->GetLastError(); }

  std::string Describe(hipError_t result) const
  {
    this->ClearError();
    const char* name = this->GetErrorName(result);
    const char* text = this->GetErrorString(result);
    return (name ? std::string(name) : "HIP error " + std::to_string(result)) +
      (text ? std::string(": ") + text : "");
  }
};

// The HIP runtime the process has loaded already, if any: hip-python, PyTorch
// and CuPy load one, and two copies of the runtime in a process share no
// devices, streams or allocations. The handle is never closed.
vtksys::DynamicLoader::LibraryHandle LoadedHip(const char* name)
{
#if defined(_WIN32)
  HMODULE module = nullptr;
  return GetModuleHandleExA(0, name, &module) ? module : nullptr;
#else
  return dlopen(name, RTLD_NOLOAD | RTLD_LAZY);
#endif
}

HipRuntime LoadHip()
{
  HipRuntime hip;
#if defined(_WIN32)
  const char* const names[] = { "amdhip64_7.dll", "amdhip64_6.dll" };
#elif defined(__APPLE__)
  const char* const names[] = { nullptr };
#else
  const char* const names[] = { "libamdhip64.so.7", "libamdhip64.so.6", "libamdhip64.so" };
#endif
  if (!names[0])
  {
    hip.Error = "there is no HIP runtime on this platform";
    return hip;
  }
  const char* name = nullptr;
  auto library = vtksys::DynamicLoader::InvalidLibrary;
  for (const char* candidate : names)
  {
    if ((library = LoadedHip(candidate)) != vtksys::DynamicLoader::InvalidLibrary)
    {
      name = candidate;
      break;
    }
  }
  // None loaded: the first that exists. Never closed, as CUDA's driver.
  for (const char* candidate : names)
  {
    if (library != vtksys::DynamicLoader::InvalidLibrary)
    {
      break;
    }
    library = vtksys::DynamicLoader::OpenLibrary(candidate);
    name = candidate;
  }
  if (library == vtksys::DynamicLoader::InvalidLibrary)
  {
    const char* reason = vtksys::DynamicLoader::LastError();
    hip.Error = std::string("could not load ") + names[0] + " or an older HIP runtime" +
      (reason ? std::string(": ") + reason : "");
    return hip;
  }
  auto bind = [&](const char* symbol, auto& function)
  {
    if (!Bind(library, symbol, function, hip.Error))
    {
      hip.Error = std::string("the HIP runtime ") + name + " has no " + symbol;
      return false;
    }
    return true;
  };
  if (!bind("hipGetDeviceCount", hip.GetDeviceCount) ||
    !bind("hipPointerGetAttribute", hip.PointerGetAttribute) ||
    !bind("hipGetDevice", hip.GetDevice) || !bind("hipSetDevice", hip.SetDevice) ||
    !bind("hipDeviceSynchronize", hip.DeviceSynchronize) || !bind("hipMemcpy", hip.Memcpy) ||
    !bind("hipGetLastError", hip.GetLastError) || !bind("hipGetErrorName", hip.GetErrorName) ||
    !bind("hipGetErrorString", hip.GetErrorString))
  {
    return hip;
  }
  int count = 0;
  hipError_t result = hip.GetDeviceCount(&count);
  if (result != hipSuccess)
  {
    hip.Error = "hipGetDeviceCount failed: " + hip.Describe(result);
    return hip;
  }
  if (count == 0)
  {
    hip.Error = "there is no HIP device";
    return hip;
  }
  hip.Loaded = true;
  return hip;
}

const HipRuntime& Hip()
{
  static const HipRuntime hip = LoadHip(); // loaded once, on first use
  return hip;
}

bool HipIsDevicePointer(const void* pointer, std::string* why)
{
  const HipRuntime& hip = Hip();
  if (!hip.Loaded)
  {
    return Fail(why, hip.Error);
  }
  void* address = const_cast<void*>(pointer);
  int type = -1;
  hipError_t result = hip.PointerGetAttribute(&type, HIP_POINTER_ATTRIBUTE_MEMORY_TYPE, address);
  if (result != hipSuccess)
  {
    return Fail(why, "not memory HIP knows (" + hip.Describe(result) + ")");
  }
  // HIP writes this one as a one-byte bool, so it starts from zero. Managed
  // memory says it is host memory (ROCm 7.1.1); this flag tells them apart.
  unsigned int managed = 0;
  if (hip.PointerGetAttribute(&managed, HIP_POINTER_ATTRIBUTE_IS_MANAGED, address) != hipSuccess)
  {
    hip.ClearError();
  }
  if (type != hipMemoryTypeDevice && type != hipMemoryTypeManaged && !managed)
  {
    return Fail(why, "HIP host memory, not device memory");
  }
  return true;
}

bool HipCopyToHost(void* destination, const void* source, std::size_t bytes, std::string* why)
{
  const HipRuntime& hip = Hip();
  if (!hip.Loaded)
  {
    return Fail(why, hip.Error);
  }
  int ordinal = -1;
  hipError_t result = hip.PointerGetAttribute(
    &ordinal, HIP_POINTER_ATTRIBUTE_DEVICE_ORDINAL, const_cast<void*>(source));
  if (result != hipSuccess)
  {
    return Fail(why, "not memory HIP knows (" + hip.Describe(result) + ")");
  }

  // Copy on the memory's device, then give the thread its own device back.
  int current = -1;
  if ((result = hip.GetDevice(&current)) != hipSuccess)
  {
    return Fail(why, "hipGetDevice failed (" + hip.Describe(result) + ")");
  }
  const bool switched = current != ordinal;
  if (switched && (result = hip.SetDevice(ordinal)) != hipSuccess)
  {
    return Fail(
      why, "could not use device " + std::to_string(ordinal) + " (" + hip.Describe(result) + ")");
  }
  // The producer's work may still be queued on a stream of its own.
  result = hip.DeviceSynchronize();
  if (result == hipSuccess)
  {
    result = hip.Memcpy(destination, source, bytes, hipMemcpyDeviceToHost);
  }
  if (switched)
  {
    hip.SetDevice(current);
  }
  if (result != hipSuccess)
  {
    return Fail(why,
      "copying " + std::to_string(bytes) + " bytes off the GPU failed (" + hip.Describe(result) +
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
  if (space == vtkDataArray::HipDeviceMemory)
  {
    return Hip().Loaded || Fail(why, Hip().Error);
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
  if (space == vtkDataArray::HipDeviceMemory)
  {
    return HipIsDevicePointer(pointer, why);
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
  if (space == vtkDataArray::HipDeviceMemory)
  {
    return HipCopyToHost(destination, source, bytes, why);
  }
  return vtkDeviceMemory::IsAvailable(space, why);
}

VTK_ABI_NAMESPACE_END
