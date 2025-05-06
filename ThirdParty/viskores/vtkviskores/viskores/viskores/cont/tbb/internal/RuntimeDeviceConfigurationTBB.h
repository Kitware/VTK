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
#ifndef viskores_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h
#define viskores_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h

#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>
#include <viskores/cont/tbb/internal/DeviceAdapterTagTBB.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#if TBB_VERSION_MAJOR >= 2020
#define TBB_PREVIEW_GLOBAL_CONTROL
#include <tbb/global_control.h>
#include <tbb/task_arena.h>
#else
#include <tbb/tbb.h>
#endif
VISKORES_THIRDPARTY_POST_INCLUDE

#include <memory>

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagTBB>
  : public viskores::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  VISKORES_CONT
  RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagTBB>()
    :
#if TBB_VERSION_MAJOR >= 2020
    HardwareMaxThreads(::tbb::task_arena{}.max_concurrency())
    ,
#else
    HardwareMaxThreads(::tbb::task_scheduler_init::default_num_threads())
    ,
#endif
    CurrentNumThreads(this->HardwareMaxThreads)
  {
  }

  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const final
  {
    return viskores::cont::DeviceAdapterTagTBB{};
  }

  VISKORES_CONT RuntimeDeviceConfigReturnCode SetThreads(const viskores::Id& value) final
  {
    this->CurrentNumThreads = value > 0 ? value : this->HardwareMaxThreads;
#if TBB_VERSION_MAJOR >= 2020
    GlobalControl.reset(new ::tbb::global_control(::tbb::global_control::max_allowed_parallelism,
                                                  this->CurrentNumThreads));
#else
    TaskSchedulerInit.reset(
      new ::tbb::task_scheduler_init(static_cast<int>(this->CurrentNumThreads)));
#endif
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT RuntimeDeviceConfigReturnCode GetThreads(viskores::Id& value) const final
  {
#if TBB_VERSION_MAJOR >= 2020
    value = ::tbb::global_control::active_value(::tbb::global_control::max_allowed_parallelism);
#else
    value = this->CurrentNumThreads;
#endif
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT RuntimeDeviceConfigReturnCode GetMaxThreads(viskores::Id& value) const final
  {
    value = this->HardwareMaxThreads;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

private:
#if TBB_VERSION_MAJOR >= 2020
  std::unique_ptr<::tbb::global_control> GlobalControl;
#else
  std::unique_ptr<::tbb::task_scheduler_init> TaskSchedulerInit;
#endif
  viskores::Id HardwareMaxThreads;
  viskores::Id CurrentNumThreads;
};
} // namespace vktm::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif //viskores_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h
