// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeviceMemory.h"

#include <vtksys/DynamicLoader.hxx>

#if !defined(_WIN32)
#include <dlfcn.h> // For RTLD_NOLOAD
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

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

// Level Zero, declared here rather than included: VTK builds without oneAPI.
// The types and values are ze_api.h's (API 1.9, level-zero-dev 1.16.15), checked
// against loader 1.21.9; they are part of the specification's stable ABI.
using ze_result_t = int;
using ze_driver_handle_t = struct _ze_driver_handle_t*;
using ze_device_handle_t = struct _ze_device_handle_t*;
using ze_context_handle_t = struct _ze_context_handle_t*;
using ze_command_list_handle_t = struct _ze_command_list_handle_t*;
using ze_event_handle_t = struct _ze_event_handle_t*;

constexpr ze_result_t ZE_RESULT_SUCCESS = 0;
constexpr unsigned int ZE_INIT_FLAG_GPU_ONLY = 1;
constexpr unsigned int ZE_DEVICE_TYPE_GPU = 1;
constexpr unsigned int ZE_MEMORY_TYPE_DEVICE = 2;
constexpr unsigned int ZE_MEMORY_TYPE_SHARED = 3;
constexpr unsigned int ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY = 2;
constexpr unsigned int ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS = 1;
constexpr unsigned int ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES = 0x3;
constexpr unsigned int ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES = 0x6;
constexpr unsigned int ZE_STRUCTURE_TYPE_CONTEXT_DESC = 0xd;
constexpr unsigned int ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC = 0xe;
constexpr unsigned int ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES = 0x17;

struct ze_device_properties_t
{
  unsigned int stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  void* pNext = nullptr;
  unsigned int type = 0;
  std::uint32_t vendorId = 0;
  std::uint32_t deviceId = 0;
  std::uint32_t flags = 0;
  std::uint32_t subdeviceId = 0;
  std::uint32_t coreClockRate = 0;
  std::uint64_t maxMemAllocSize = 0;
  std::uint32_t maxHardwareContexts = 0;
  std::uint32_t maxCommandQueuePriority = 0;
  std::uint32_t numThreadsPerEU = 0;
  std::uint32_t physicalEUSimdWidth = 0;
  std::uint32_t numEUsPerSubslice = 0;
  std::uint32_t numSubslicesPerSlice = 0;
  std::uint32_t numSlices = 0;
  std::uint64_t timerResolution = 0;
  std::uint32_t timestampValidBits = 0;
  std::uint32_t kernelTimestampValidBits = 0;
  std::uint8_t uuid[16] = {};
  char name[256] = {};
};

struct ze_memory_allocation_properties_t
{
  unsigned int stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  void* pNext = nullptr;
  unsigned int type = 0;
  std::uint64_t id = 0;
  std::uint64_t pageSize = 0;
};

struct ze_command_queue_group_properties_t
{
  unsigned int stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
  void* pNext = nullptr;
  std::uint32_t flags = 0;
  std::size_t maxMemoryFillPatternSize = 0;
  std::uint32_t numQueues = 0;
};

struct ze_context_desc_t
{
  unsigned int stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
  const void* pNext = nullptr;
  std::uint32_t flags = 0;
};

struct ze_command_queue_desc_t
{
  unsigned int stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  const void* pNext = nullptr;
  std::uint32_t ordinal = 0;
  std::uint32_t index = 0;
  std::uint32_t flags = 0;
  unsigned int mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
  unsigned int priority = 0; // ZE_COMMAND_QUEUE_PRIORITY_NORMAL
};

struct LevelZeroDriver
{
  bool Loaded = false;
  std::string Error;
  // The first driver with a GPU device, and that device: where VTK makes its
  // own context if no one gives it one.
  ze_driver_handle_t Driver = nullptr;
  ze_device_handle_t Device = nullptr;

  ze_result_t (*Init)(unsigned int) = nullptr;
  ze_result_t (*DriverGet)(std::uint32_t*, ze_driver_handle_t*) = nullptr;
  ze_result_t (*DeviceGet)(ze_driver_handle_t, std::uint32_t*, ze_device_handle_t*) = nullptr;
  ze_result_t (*DeviceGetProperties)(ze_device_handle_t, ze_device_properties_t*) = nullptr;
  ze_result_t (*ContextCreate)(
    ze_driver_handle_t, const ze_context_desc_t*, ze_context_handle_t*) = nullptr;
  ze_result_t (*MemGetAllocProperties)(ze_context_handle_t, const void*,
    ze_memory_allocation_properties_t*, ze_device_handle_t*) = nullptr;
  ze_result_t (*DeviceGetCommandQueueGroupProperties)(
    ze_device_handle_t, std::uint32_t*, ze_command_queue_group_properties_t*) = nullptr;
  ze_result_t (*CommandListCreateImmediate)(ze_context_handle_t, ze_device_handle_t,
    const ze_command_queue_desc_t*, ze_command_list_handle_t*) = nullptr;
  ze_result_t (*CommandListAppendMemoryCopy)(ze_command_list_handle_t, void*, const void*,
    std::size_t, ze_event_handle_t, std::uint32_t, ze_event_handle_t*) = nullptr;
  // Optional (API 1.6): a synchronous list's copy has finished when it returns anyway.
  ze_result_t (*CommandListHostSynchronize)(ze_command_list_handle_t, std::uint64_t) = nullptr;

  static std::string Describe(ze_result_t result)
  {
    char hex[16];
    std::snprintf(hex, sizeof(hex), "0x%x", static_cast<unsigned int>(result));
    return std::string("Level Zero error ") + hex;
  }
};

LevelZeroDriver LoadLevelZero()
{
  LevelZeroDriver ze;
#if defined(_WIN32)
  const char* name = "ze_loader.dll";
#elif defined(__APPLE__)
  const char* name = nullptr;
#else
  const char* name = "libze_loader.so.1";
#endif
  if (!name)
  {
    ze.Error = "there is no Level Zero loader on this platform";
    return ze;
  }
  // Never closed: arrays may read through it until the process ends.
  auto library = vtksys::DynamicLoader::OpenLibrary(name);
  if (library == vtksys::DynamicLoader::InvalidLibrary)
  {
    const char* reason = vtksys::DynamicLoader::LastError();
    ze.Error = std::string("could not load ") + name + (reason ? std::string(": ") + reason : "");
    return ze;
  }
  auto bind = [&](const char* symbol, auto& function)
  {
    function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(
      vtksys::DynamicLoader::GetSymbolAddress(library, symbol));
    if (!function)
    {
      ze.Error = std::string("the Level Zero loader has no ") + symbol;
    }
    return function != nullptr;
  };
  if (!bind("zeInit", ze.Init) || !bind("zeDriverGet", ze.DriverGet) ||
    !bind("zeDeviceGet", ze.DeviceGet) || !bind("zeDeviceGetProperties", ze.DeviceGetProperties) ||
    !bind("zeContextCreate", ze.ContextCreate) ||
    !bind("zeMemGetAllocProperties", ze.MemGetAllocProperties) ||
    !bind("zeDeviceGetCommandQueueGroupProperties", ze.DeviceGetCommandQueueGroupProperties) ||
    !bind("zeCommandListCreateImmediate", ze.CommandListCreateImmediate) ||
    !bind("zeCommandListAppendMemoryCopy", ze.CommandListAppendMemoryCopy))
  {
    return ze;
  }
  if (!bind("zeCommandListHostSynchronize", ze.CommandListHostSynchronize))
  {
    ze.Error.clear();
  }

  ze_result_t result = ze.Init(ZE_INIT_FLAG_GPU_ONLY);
  if (result != ZE_RESULT_SUCCESS)
  {
    ze.Error = "zeInit failed: " + LevelZeroDriver::Describe(result);
    return ze;
  }
  std::uint32_t count = 0;
  ze.DriverGet(&count, nullptr);
  std::vector<ze_driver_handle_t> drivers(count);
  if (count == 0 || ze.DriverGet(&count, drivers.data()) != ZE_RESULT_SUCCESS)
  {
    ze.Error = "no Level Zero driver";
    return ze;
  }
  for (ze_driver_handle_t driver : drivers)
  {
    std::uint32_t devices = 0;
    ze.DeviceGet(driver, &devices, nullptr);
    std::vector<ze_device_handle_t> handles(devices);
    if (devices == 0 || ze.DeviceGet(driver, &devices, handles.data()) != ZE_RESULT_SUCCESS)
    {
      continue;
    }
    for (ze_device_handle_t device : handles)
    {
      ze_device_properties_t properties;
      if (ze.DeviceGetProperties(device, &properties) == ZE_RESULT_SUCCESS &&
        properties.type == ZE_DEVICE_TYPE_GPU)
      {
        ze.Driver = driver;
        ze.Device = device;
        ze.Loaded = true;
        return ze;
      }
    }
  }
  ze.Error = "no Level Zero GPU";
  return ze;
}

const LevelZeroDriver& LevelZero()
{
  static const LevelZeroDriver ze = LoadLevelZero(); // loaded once, on first use
  return ze;
}

// VTK's one Level Zero context -- one a producer gave, or one VTK made on first
// request and kept for the process -- and the lists it copies with.
struct LevelZeroState
{
  std::mutex Mutex;
  ze_driver_handle_t Driver = nullptr;
  ze_device_handle_t Device = nullptr;
  ze_context_handle_t Context = nullptr;
  // One synchronous immediate list per (context, device), never destroyed: the
  // context may be another library's, destroyed first.
  std::map<std::pair<ze_context_handle_t, ze_device_handle_t>, ze_command_list_handle_t> Lists;
};

LevelZeroState& LevelZeroShared()
{
  static LevelZeroState* state = new LevelZeroState; // never destroyed: used at exit
  return *state;
}

// The context, made if there is none yet. The state's mutex must be held.
ze_context_handle_t LevelZeroContextLocked(LevelZeroState& state, std::string* why)
{
  if (state.Context)
  {
    return state.Context;
  }
  const LevelZeroDriver& ze = LevelZero();
  if (!ze.Loaded)
  {
    Fail(why, ze.Error);
    return nullptr;
  }
  ze_context_desc_t desc;
  ze_context_handle_t context = nullptr;
  ze_result_t result = ze.ContextCreate(ze.Driver, &desc, &context);
  if (result != ZE_RESULT_SUCCESS)
  {
    Fail(why, "zeContextCreate failed: " + LevelZeroDriver::Describe(result));
    return nullptr;
  }
  state.Driver = ze.Driver;
  state.Device = ze.Device;
  state.Context = context;
  return context;
}

// The memory's allocation properties in VTK's context, and its device.
bool LevelZeroProperties(LevelZeroState& state, const void* pointer,
  ze_memory_allocation_properties_t& properties, ze_device_handle_t& device, std::string* why)
{
  const LevelZeroDriver& ze = LevelZero();
  if (!ze.Loaded)
  {
    return Fail(why, ze.Error);
  }
  ze_context_handle_t context = LevelZeroContextLocked(state, why);
  if (!context)
  {
    return false;
  }
  ze_result_t result = ze.MemGetAllocProperties(context, pointer, &properties, &device);
  if (result != ZE_RESULT_SUCCESS)
  {
    return Fail(why, "not memory Level Zero knows (" + LevelZeroDriver::Describe(result) + ")");
  }
  if (properties.type != ZE_MEMORY_TYPE_DEVICE && properties.type != ZE_MEMORY_TYPE_SHARED)
  {
    // The driver answers for a pointer of any context, so this tells device
    // memory from host memory, not VTK's context from another.
    return Fail(why, "not Level Zero device or shared memory");
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

bool LevelZeroIsDevicePointer(const void* pointer, std::string* why)
{
  LevelZeroState& state = LevelZeroShared();
  std::lock_guard<std::mutex> lock(state.Mutex);
  ze_memory_allocation_properties_t properties;
  ze_device_handle_t device = nullptr;
  return LevelZeroProperties(state, pointer, properties, device, why);
}

bool LevelZeroCopyToHost(void* destination, const void* source, std::size_t bytes, std::string* why)
{
  LevelZeroState& state = LevelZeroShared();
  std::lock_guard<std::mutex> lock(state.Mutex);
  ze_memory_allocation_properties_t properties;
  ze_device_handle_t device = nullptr;
  if (!LevelZeroProperties(state, source, properties, device, why))
  {
    return false;
  }
  const LevelZeroDriver& ze = LevelZero();
  if (!device)
  {
    device = state.Device; // shared memory need not name one
  }

  // An immediate list on the memory's device, in synchronous mode: the copy
  // has finished when the call returns, so nothing reads or writes the host
  // memory after it (a pageable destination is fine).
  const auto key = std::make_pair(state.Context, device);
  auto found = state.Lists.find(key);
  if (found == state.Lists.end())
  {
    // A queue group that can copy; compute groups can, so usually ordinal 0.
    std::uint32_t count = 0;
    ze.DeviceGetCommandQueueGroupProperties(device, &count, nullptr);
    std::vector<ze_command_queue_group_properties_t> groups(count);
    ze_result_t result = ze.DeviceGetCommandQueueGroupProperties(device, &count, groups.data());
    ze_command_queue_desc_t desc;
    bool copies = false;
    for (std::uint32_t i = 0; result == ZE_RESULT_SUCCESS && i < count && !copies; ++i)
    {
      if (groups[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)
      {
        desc.ordinal = i;
        copies = true;
      }
    }
    if (!copies)
    {
      return Fail(why, "the memory's Level Zero device has no queue that can copy");
    }
    ze_command_list_handle_t list = nullptr;
    result = ze.CommandListCreateImmediate(state.Context, device, &desc, &list);
    if (result != ZE_RESULT_SUCCESS)
    {
      return Fail(why, "zeCommandListCreateImmediate failed: " + LevelZeroDriver::Describe(result));
    }
    found = state.Lists.emplace(key, list).first;
  }
  ze_command_list_handle_t list = found->second;

  ze_result_t result =
    ze.CommandListAppendMemoryCopy(list, destination, source, bytes, nullptr, 0, nullptr);
  if (result == ZE_RESULT_SUCCESS && ze.CommandListHostSynchronize)
  {
    result = ze.CommandListHostSynchronize(list, UINT64_MAX);
  }
  if (result != ZE_RESULT_SUCCESS)
  {
    return Fail(why,
      "copying " + std::to_string(bytes) + " bytes off the GPU failed (" +
        LevelZeroDriver::Describe(result) + ")");
  }
  return true;
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
  if (space == vtkDataArray::LevelZeroDeviceMemory)
  {
    return LevelZero().Loaded || Fail(why, LevelZero().Error);
  }
  if (space == vtkDataArray::HostMemory)
  {
    return Fail(why, "host memory is not device memory");
  }
  return Fail(why, "not a device memory space VTK knows");
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
  if (space == vtkDataArray::LevelZeroDeviceMemory)
  {
    return LevelZeroIsDevicePointer(pointer, why);
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
  if (space == vtkDataArray::LevelZeroDeviceMemory)
  {
    return LevelZeroCopyToHost(destination, source, bytes, why);
  }
  return vtkDeviceMemory::IsAvailable(space, why);
}

//------------------------------------------------------------------------------
void vtkDeviceMemory::SetLevelZeroContext(void* driver, void* device, void* context)
{
  LevelZeroState& state = LevelZeroShared();
  std::lock_guard<std::mutex> lock(state.Mutex);
  // A context VTK made is kept, not destroyed: arrays may still copy in it.
  // The lists of other contexts are forgotten (not destroyed: their context may
  // be gone), so a later context at a reused address never meets them.
  const auto kept = static_cast<ze_context_handle_t>(context);
  for (auto it = state.Lists.begin(); it != state.Lists.end();)
  {
    it = it->first.first == kept ? std::next(it) : state.Lists.erase(it);
  }
  state.Driver = context ? static_cast<ze_driver_handle_t>(driver) : nullptr;
  state.Device = context ? static_cast<ze_device_handle_t>(device) : nullptr;
  state.Context = static_cast<ze_context_handle_t>(context);
}

//------------------------------------------------------------------------------
bool vtkDeviceMemory::HasLevelZeroContext()
{
  LevelZeroState& state = LevelZeroShared();
  std::lock_guard<std::mutex> lock(state.Mutex);
  return state.Context != nullptr;
}

//------------------------------------------------------------------------------
bool vtkDeviceMemory::GetLevelZeroContext(
  void*& driver, void*& device, void*& context, std::string* why)
{
  LevelZeroState& state = LevelZeroShared();
  std::lock_guard<std::mutex> lock(state.Mutex);
  driver = device = context = nullptr;
  if (!LevelZeroContextLocked(state, why))
  {
    return false;
  }
  driver = state.Driver;
  device = state.Device;
  context = state.Context;
  return true;
}

VTK_ABI_NAMESPACE_END
