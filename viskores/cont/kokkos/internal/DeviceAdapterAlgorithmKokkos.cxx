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
#include <viskores/cont/kokkos/internal/DeviceAdapterAlgorithmKokkos.h>

#include <string>

namespace
{

constexpr static viskores::Id ErrorMessageMaxLength = 1024;

using ErrorMessageView = viskores::cont::kokkos::internal::KokkosViewExec<char>;

ErrorMessageView GetErrorMessageViewInstance()
{
  static thread_local ErrorMessageView local(std::string("ErrorMessageViewInstance"),
                                             ErrorMessageMaxLength);
  return local;
}

} // anonymous namespace

namespace viskores
{
namespace cont
{

viskores::exec::internal::ErrorMessageBuffer
DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagKokkos>::GetErrorMessageBufferInstance()
{
  return viskores::exec::internal::ErrorMessageBuffer(GetErrorMessageViewInstance().data(),
                                                      ErrorMessageMaxLength);
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagKokkos>::CheckForErrors()
{
  static thread_local char hostBuffer[ErrorMessageMaxLength] = "";

  auto deviceView = GetErrorMessageViewInstance();

  if (Kokkos::SpaceAccessibility<Kokkos::HostSpace, decltype(deviceView)::memory_space>::accessible)
  {
    viskores::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
    if (deviceView(0) != '\0')
    {
      auto excep = viskores::cont::ErrorExecution(deviceView.data());
      deviceView(0) = '\0'; // clear
      viskores::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
      throw excep;
    }
  }
  else
  {
    viskores::cont::kokkos::internal::KokkosViewCont<char> hostView(hostBuffer,
                                                                    ErrorMessageMaxLength);
    Kokkos::deep_copy(
      viskores::cont::kokkos::internal::GetExecutionSpaceInstance(), hostView, deviceView);
    viskores::cont::kokkos::internal::GetExecutionSpaceInstance().fence();

    if (hostView(0) != '\0')
    {
      auto excep = viskores::cont::ErrorExecution(hostView.data());
      hostView(0) = '\0'; // clear
      Kokkos::deep_copy(
        viskores::cont::kokkos::internal::GetExecutionSpaceInstance(), deviceView, hostView);
      throw excep;
    }
  }
}

}
} // viskores::cont
