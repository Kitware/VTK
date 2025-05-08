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

#include <viskores/cont/internal/ParallelRadixSort.h>

#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#include <omp.h>

namespace viskores
{
namespace cont
{
namespace openmp
{
namespace sort
{
namespace radix
{

struct RadixThreaderOpenMP
{
  size_t GetAvailableCores() const
  {
    viskores::Id result;
    viskores::cont::RuntimeDeviceInformation{}
      .GetRuntimeConfiguration(viskores::cont::DeviceAdapterTagOpenMP())
      .GetThreads(result);
    return static_cast<size_t>(result);
  }

  template <typename TaskType>
  void RunParentTask(TaskType task) const
  {
    assert(!omp_in_parallel());
#pragma omp parallel default(none) shared(task)
    {
#pragma omp single
      {
        task();
      }
    } // Implied barrier ensures that child tasks will finish.
  }

  template <typename TaskType, typename ThreadData>
  void RunChildTasks(ThreadData, TaskType left, TaskType right) const
  {
    assert(omp_in_parallel());
#pragma omp task default(none) firstprivate(right)
    {
      right();
    }

    // Execute the left task in the existing thread.
    left();
  }
};

VISKORES_INSTANTIATE_RADIX_SORT_FOR_THREADER(RadixThreaderOpenMP)
}
} // end namespace sort::radix
}
}
} // end namespace viskores::cont::openmp
