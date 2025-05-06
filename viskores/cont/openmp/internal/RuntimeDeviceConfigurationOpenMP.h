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
#ifndef viskores_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h
#define viskores_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h

#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>
#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#include <viskores/cont/Logging.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <omp.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagOpenMP>
  : public viskores::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagOpenMP>()
    : HardwareMaxThreads(InitializeHardwareMaxThreads())
    , CurrentNumThreads(this->HardwareMaxThreads)
  {
  }

  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override final
  {
    return viskores::cont::DeviceAdapterTagOpenMP{};
  }

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(
    const viskores::Id& value) override final
  {
    if (omp_in_parallel())
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                     "OpenMP SetThreads: Error, currently in parallel");
      return RuntimeDeviceConfigReturnCode::NOT_APPLIED;
    }
    if (value > 0)
    {
      if (value > this->HardwareMaxThreads)
      {
        VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                       "OpenMP: You may be oversubscribing your CPU cores: "
                         << "process threads available: " << this->HardwareMaxThreads
                         << ", requested threads: " << value);
      }
      this->CurrentNumThreads = value;
      omp_set_num_threads(this->CurrentNumThreads);
    }
    else
    {
      this->CurrentNumThreads = this->HardwareMaxThreads;
      omp_set_num_threads(this->CurrentNumThreads);
    }
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(
    viskores::Id& value) const override final
  {
    value = this->CurrentNumThreads;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(
    viskores::Id& value) const override final
  {
    value = this->HardwareMaxThreads;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

private:
  VISKORES_CONT viskores::Id InitializeHardwareMaxThreads() const
  {
    viskores::Id count = 0;

    if (omp_in_parallel())
    {
      count = omp_get_num_threads();
    }
    else
    {
      VISKORES_OPENMP_DIRECTIVE(parallel)
      {
        VISKORES_OPENMP_DIRECTIVE(atomic)
        ++count;
      }
    }
    return count;
  }

  viskores::Id HardwareMaxThreads;
  viskores::Id CurrentNumThreads;
};
} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif //viskores_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h
