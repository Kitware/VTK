//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/DIYMemoryManagement.h>

#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#ifdef VISKORES_ENABLE_GPU_MPI
#include <viskores/cont/kokkos/DeviceAdapterKokkos.h>
#endif

namespace
{

thread_local viskores::cont::DeviceAdapterId DIYCurrentDeviceAdaptor =
  viskores::cont::DeviceAdapterTagSerial();

viskores::cont::internal::DeviceAdapterMemoryManagerBase& GetMemoryManager(
  viskores::cont::DeviceAdapterId device)
{
  return viskores::cont::RuntimeDeviceInformation().GetMemoryManager(device);
}

viskoresdiy::MemoryManagement GetDIYMemoryManagement(viskores::cont::DeviceAdapterId device)
{
  return viskoresdiy::MemoryManagement(
    [device](int, size_t n)
    { return static_cast<char*>(GetMemoryManager(device).AllocateRawPointer(n)); },
    [device](const char* p) { GetMemoryManager(device).DeleteRawPointer(const_cast<char*>(p)); },
    [device](char* dest, const char* src, size_t count)
    { GetMemoryManager(device).CopyDeviceToDeviceRawPointer(src, dest, count); });
}

}

namespace viskores
{
namespace cont
{

viskores::cont::DeviceAdapterId GetDIYDeviceAdapter()
{
  return DIYCurrentDeviceAdaptor;
}

void DIYMasterExchange(viskoresdiy::Master& master, bool remote)
{
#ifdef VISKORES_ENABLE_GPU_MPI
  try
  {
    DIYCurrentDeviceAdaptor = viskores::cont::DeviceAdapterTagKokkos();
    master.exchange(remote, GetDIYMemoryManagement(viskores::cont::DeviceAdapterTagKokkos()));
    DIYCurrentDeviceAdaptor = viskores::cont::DeviceAdapterTagSerial();
  }
  catch (...)
  {
    DIYCurrentDeviceAdaptor = viskores::cont::DeviceAdapterTagSerial();
    throw;
  }
#else
  DIYCurrentDeviceAdaptor = viskores::cont::DeviceAdapterTagSerial();
  master.exchange(remote);
#endif
}

}
}
