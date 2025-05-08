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

#include <viskores/cont/tbb/internal/DeviceAdapterAlgorithmTBB.h>

namespace viskores
{
namespace cont
{

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagTBB>::ScheduleTask(
  viskores::exec::tbb::internal::TaskTiling1D& functor,
  viskores::Id size)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "Schedule Task TBB 1D");

  const viskores::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  ::tbb::blocked_range<viskores::Id> range(0, size, tbb::TBB_GRAIN_SIZE);

  ::tbb::parallel_for(
    range, [&](const ::tbb::blocked_range<viskores::Id>& r) { functor(r.begin(), r.end()); });

  if (errorMessage.IsErrorRaised())
  {
    throw viskores::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagTBB>::ScheduleTask(
  viskores::exec::tbb::internal::TaskTiling3D& functor,
  viskores::Id3 size)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "Schedule Task TBB 3D");

  static constexpr viskores::UInt32 TBB_GRAIN_SIZE_3D[3] = { 1, 4, 256 };
  const viskores::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  //memory is generally setup in a way that iterating the first range
  //in the tightest loop has the best cache coherence.
  ::tbb::blocked_range3d<viskores::Id> range(0,
                                             size[2],
                                             TBB_GRAIN_SIZE_3D[0],
                                             0,
                                             size[1],
                                             TBB_GRAIN_SIZE_3D[1],
                                             0,
                                             size[0],
                                             TBB_GRAIN_SIZE_3D[2]);
  ::tbb::parallel_for(range,
                      [&](const ::tbb::blocked_range3d<viskores::Id>& r)
                      {
                        for (viskores::Id k = r.pages().begin(); k != r.pages().end(); ++k)
                        {
                          for (viskores::Id j = r.rows().begin(); j != r.rows().end(); ++j)
                          {
                            const viskores::Id start = r.cols().begin();
                            const viskores::Id end = r.cols().end();
                            functor(size, start, end, j, k);
                          }
                        }
                      });

  if (errorMessage.IsErrorRaised())
  {
    throw viskores::cont::ErrorExecution(errorString);
  }
}
}
}
