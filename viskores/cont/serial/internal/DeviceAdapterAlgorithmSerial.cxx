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

#include <viskores/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>

namespace viskores
{
namespace cont
{

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagSerial>::ScheduleTask(
  viskores::exec::serial::internal::TaskTiling1D& functor,
  viskores::Id size)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  const viskores::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  const viskores::Id iterations = size / 1024;
  viskores::Id index = 0;
  for (viskores::Id i = 0; i < iterations; ++i)
  {
    functor(index, index + 1024);
    index += 1024;
  }
  functor(index, size);

  if (errorMessage.IsErrorRaised())
  {
    throw viskores::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagSerial>::ScheduleTask(
  viskores::exec::serial::internal::TaskTiling3D& functor,
  viskores::Id3 size)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  const viskores::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  for (viskores::Id k = 0; k < size[2]; ++k)
  {
    for (viskores::Id j = 0; j < size[1]; ++j)
    {
      functor(size, 0, size[0], j, k);
    }
  }

  if (errorMessage.IsErrorRaised())
  {
    throw viskores::cont::ErrorExecution(errorString);
  }
}
}
}
