// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWGPUContext.h"
#include "vtkObject.h"
#include "vtk_fmt.h"
// clang-format off
#include VTK_FMT(fmt/core.h)
// clang-format on

#include <memory>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

#define vtkWGPUContextLog(x)                                                                       \
  vtkOStreamWrapper::EndlType endl;                                                                \
  vtkOStreamWrapper::UseEndl(endl);                                                                \
  vtkOStrStreamWrapper vtkmsg;                                                                     \
  vtkmsg << "(vtkWGPUContext): " << x;                                                             \
  std::string _filename = vtksys::SystemTools::GetFilenameName(__FILE__);                          \
  vtkOutputWindowDisplayDebugText(_filename.c_str(), __LINE__, vtkmsg.str(), nullptr);             \
  vtkmsg.rdbuf()->freeze(0);

#define vtkWGPUContextError(x)                                                                     \
  vtkOStreamWrapper::EndlType endl;                                                                \
  vtkOStreamWrapper::UseEndl(endl);                                                                \
  vtkOStrStreamWrapper vtkmsg;                                                                     \
  vtkmsg << "(vtkWGPUContext): " << x;                                                             \
  std::string _filename = vtksys::SystemTools::GetFilenameName(__FILE__);                          \
  vtkOutputWindowDisplayErrorText(_filename.c_str(), __LINE__, vtkmsg.str(), nullptr);             \
  vtkmsg.rdbuf()->freeze(0);                                                                       \
  vtkObject::BreakOnError()

#ifdef __EMSCRIPTEN__
namespace vtkWGPUEmscriptenImpl
{
//------------------------------------------------------------------------------
static void LogAvailableAdapters() {}

//------------------------------------------------------------------------------
static void GetAdapterInfo(char (*adapter_info)[256])
{
  (void)adapter_info;
}

//------------------------------------------------------------------------------
static wgpu::Adapter RequestAdapter(const wgpu::RequestAdapterOptions&)
{
  return nullptr;
}

//------------------------------------------------------------------------------
static wgpu::Device RequestDevice(const wgpu::Adapter&, const wgpu::DeviceDescriptor&)
{
  return wgpu::Device(emscripten_webgpu_get_device());
}

//------------------------------------------------------------------------------
static wgpu::Surface CreateSurface(const wgpu::ChainedStruct& surfaceDescriptor)
{
  wgpu::SurfaceDescriptor descriptor;
  descriptor.nextInChain = &surfaceDescriptor;
  wgpu::Instance instance = {};
  return instance.CreateSurface(&descriptor);
}

//------------------------------------------------------------------------------
static void WaitABit()
{
  // not supported yet. emscripten suggests we `requestAnimationFrame`, doesn't seem to work.
}

}
namespace vtkWGPUImpl = vtkWGPUEmscriptenImpl;
#elif VTK_USE_DAWN_WEBGPU
#include <cstring>
#include <dawn/dawn_proc.h>
#include <dawn/native/DawnNative.h>
namespace vtkWGPUDawnImpl
{

static const char* BackendTypeName(wgpu::BackendType);
static const char* AdapterTypeName(wgpu::AdapterType);

static struct
{
  struct
  {
    DawnProcTable ProcTable;
    std::unique_ptr<dawn::native::Instance> Instance = nullptr;
  } DawnNativeEntryPoint;
  struct
  {
    dawn::native::Adapter Handle;
    wgpu::BackendType DawnBackendType;
    struct
    {
      const char* Name;
      const char* TypeName;
      const char* BackendName;
    } Info;
  } Adapter;
  bool Initialized = false;
} GPUContext = {};

//------------------------------------------------------------------------------
static void Initialize()
{
  if (GPUContext.Initialized)
  {
    return;
  }

  // Set up the native procs for the global proctable
  GPUContext.DawnNativeEntryPoint.ProcTable = dawn::native::GetProcs();
  dawnProcSetProcs(&GPUContext.DawnNativeEntryPoint.ProcTable);
  GPUContext.DawnNativeEntryPoint.Instance =
    std::unique_ptr<dawn::native::Instance>(new dawn::native::Instance());
  GPUContext.DawnNativeEntryPoint.Instance->DiscoverDefaultAdapters();
  GPUContext.DawnNativeEntryPoint.Instance->EnableBackendValidation(true);

  // Dawn backend type.
  // Default to D3D12, Metal, Vulkan, OpenGL in that order as D3D12 and Metal
  // are the preferred on their respective platforms, and Vulkan is preferred to
  // OpenGL
  GPUContext.Adapter.DawnBackendType =
#if defined(_WIN32)
    wgpu::BackendType::D3D12;
#elif defined(__APPLE__)
    wgpu::BackendType::Metal;
#else
    wgpu::BackendType::Vulkan;
#endif
  GPUContext.Adapter.Handle = nullptr;
  GPUContext.Initialized = true;
}

//------------------------------------------------------------------------------
static void WaitABit()
{
  wgpuInstanceProcessEvents(GPUContext.DawnNativeEntryPoint.Instance->Get());
}

//------------------------------------------------------------------------------
static void SetAdapterInfo(const wgpu::AdapterProperties& ap)
{
  GPUContext.Adapter.Info.Name = ap.name;
  GPUContext.Adapter.Info.TypeName = AdapterTypeName(ap.adapterType);
  GPUContext.Adapter.Info.BackendName = BackendTypeName(ap.backendType);
}

//------------------------------------------------------------------------------
static wgpu::Adapter RequestAdapter(const wgpu::RequestAdapterOptions& options)
{
  Initialize();

  std::vector<dawn::native::Adapter> adapters =
    GPUContext.DawnNativeEntryPoint.Instance->EnumerateAdapters(&options);
  for (const dawn::native::Adapter& adapter : adapters)
  {
    GPUContext.Adapter.Handle = adapter;
    wgpu::AdapterProperties ap;
    adapter.GetProperties(&ap);
    SetAdapterInfo(ap);
    std::string msg = vtkfmt::format(
      "Selected adapter {0} (device={1:#x} vendor={2:#x} type={3}/{4})", ap.name, ap.deviceID,
      ap.vendorID, GPUContext.Adapter.Info.TypeName, GPUContext.Adapter.Info.BackendName);
    vtkWGPUContextLog(msg);
    return wgpu::Adapter(GPUContext.Adapter.Handle.Get());
  }

  return nullptr;
}

//------------------------------------------------------------------------------
static wgpu::Device RequestDevice(
  const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor& deviceDescriptor)
{
  return adapter.CreateDevice(&deviceDescriptor);
}

//------------------------------------------------------------------------------
static wgpu::Surface CreateSurface(const wgpu::ChainedStruct& surfaceDescriptor)
{
  wgpu::SurfaceDescriptor descriptor;
  descriptor.nextInChain = &surfaceDescriptor;
  wgpu::Surface surface =
    wgpu::Instance(GPUContext.DawnNativeEntryPoint.Instance->Get()).CreateSurface(&descriptor);
  if (!surface)
  {
    return nullptr;
  }
  return surface;
}

//------------------------------------------------------------------------------
static void LogAvailableAdapters()
{
  Initialize();

  std::stringstream msg;
  msg << "Available adapters:\n";
  for (auto&& a : GPUContext.DawnNativeEntryPoint.Instance->EnumerateAdapters())
  {
    wgpu::AdapterProperties p;
    a.GetProperties(&p);
    msg << vtkfmt::format("  {0}({1})\n"
                          "    deviceID={2}, vendorID={3:#x}, BackendType::{4}, AdapterType::{5}\n",
      p.name, p.driverDescription, p.deviceID, p.vendorID, BackendTypeName(p.backendType),
      AdapterTypeName(p.adapterType));
  }
  vtkWGPUContextLog(msg.str());
}

//------------------------------------------------------------------------------
static void GetAdapterInfo(char (*adapter_info)[256])
{
  strncpy(adapter_info[0], GPUContext.Adapter.Info.Name, 256);
  strncpy(adapter_info[1], GPUContext.Adapter.Info.TypeName, 256);
  strncpy(adapter_info[2], GPUContext.Adapter.Info.BackendName, 256);
}

//------------------------------------------------------------------------------
static const char* BackendTypeName(wgpu::BackendType t)
{
  switch (t)
  {
    case wgpu::BackendType::Null:
      return "Null";
    case wgpu::BackendType::WebGPU:
      return "WebGPU";
    case wgpu::BackendType::D3D11:
      return "D3D11";
    case wgpu::BackendType::D3D12:
      return "D3D12";
    case wgpu::BackendType::Metal:
      return "Metal";
    case wgpu::BackendType::Vulkan:
      return "Vulkan";
    case wgpu::BackendType::OpenGL:
      return "OpenGL";
    case wgpu::BackendType::OpenGLES:
      return "OpenGL ES";
  }
  return "?";
}

//------------------------------------------------------------------------------
static const char* AdapterTypeName(wgpu::AdapterType t)
{
  switch (t)
  {
    case wgpu::AdapterType::DiscreteGPU:
      return "Discrete GPU";
    case wgpu::AdapterType::IntegratedGPU:
      return "Integrated GPU";
    case wgpu::AdapterType::CPU:
      return "CPU";
    case wgpu::AdapterType::Unknown:
      return "Unknown";
  }
  return "?";
}
}
namespace vtkWGPUImpl = vtkWGPUDawnImpl;
#endif // VTK_USE_DAWN_WEBGPU

//------------------------------------------------------------------------------
void vtkWGPUContext::LogAvailableAdapters()
{
  vtkWGPUImpl::LogAvailableAdapters();
}

//------------------------------------------------------------------------------
void vtkWGPUContext::GetAdapterInfo(char (*adapter_info)[256])
{
  vtkWGPUImpl::GetAdapterInfo(adapter_info);
}

//------------------------------------------------------------------------------
wgpu::Adapter vtkWGPUContext::RequestAdapter(const wgpu::RequestAdapterOptions& options)
{
  return vtkWGPUImpl::RequestAdapter(options);
}

//------------------------------------------------------------------------------
wgpu::Device vtkWGPUContext::RequestDevice(
  const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor& deviceDescriptor)
{
  return vtkWGPUImpl::RequestDevice(adapter, deviceDescriptor);
}

//------------------------------------------------------------------------------
wgpu::Surface vtkWGPUContext::CreateSurface(const wgpu::ChainedStruct& surfaceDescriptor)
{
  return vtkWGPUImpl::CreateSurface(surfaceDescriptor);
}

//------------------------------------------------------------------------------
std::size_t vtkWGPUContext::Align(std::size_t value, int alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

//------------------------------------------------------------------------------
void vtkWGPUContext::WaitABit()
{
  vtkWGPUImpl::WaitABit();
}

VTK_ABI_NAMESPACE_END
