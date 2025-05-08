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

#include <viskores/cont/openmp/internal/DeviceAdapterAlgorithmOpenMP.h>
#include <viskores/cont/openmp/internal/FunctorsOpenMP.h>

#include <viskores/cont/ErrorExecution.h>

#include <omp.h>

namespace viskores
{
namespace cont
{

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagOpenMP>::ScheduleTask(
  viskores::exec::openmp::internal::TaskTiling1D& functor,
  viskores::Id size)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  static constexpr viskores::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  // Divide n by chunks and round down to nearest power of two. Result is clamped between
  // min and max:
  auto computeChunkSize =
    [](viskores::Id n, viskores::Id chunks, viskores::Id min, viskores::Id max) -> viskores::Id
  {
    const viskores::Id chunkSize = (n + chunks - 1) / chunks;
    viskores::Id result = 1;
    while (result < chunkSize)
    {
      result *= 2;
    }
    result /= 2; // Round down
    return std::min(max, std::max(min, result));
  };

  // Figure out how to chunk the data:
  const viskores::Id chunkSize = computeChunkSize(size, 256, 1, 1024);
  const viskores::Id numChunks = (size + chunkSize - 1) / chunkSize;

  VISKORES_OPENMP_DIRECTIVE(parallel for
                        schedule(guided))
  for (viskores::Id i = 0; i < numChunks; ++i)
  {
    const viskores::Id first = i * chunkSize;
    const viskores::Id last = std::min((i + 1) * chunkSize, size);
    functor(first, last);
  }

  if (errorMessage.IsErrorRaised())
  {
    throw viskores::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagOpenMP>::ScheduleTask(
  viskores::exec::openmp::internal::TaskTiling3D& functor,
  viskores::Id3 size)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  static constexpr viskores::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  viskores::Id3 chunkDims;
  if (size[0] > 512)
  {
    chunkDims = { 1024, 4, 1 };
  }
  else if (size[0] > 256)
  {
    chunkDims = { 512, 4, 2 };
  }
  else if (size[0] > 128)
  {
    chunkDims = { 256, 4, 4 };
  }
  else if (size[0] > 64)
  {
    chunkDims = { 128, 8, 4 };
  }
  else if (size[0] > 32)
  {
    chunkDims = { 64, 8, 8 };
  }
  else if (size[0] > 16)
  {
    chunkDims = { 32, 16, 8 };
  }
  else
  {
    chunkDims = { 16, 16, 16 };
  }

  const viskores::Id3 numChunks{ openmp::CeilDivide(size[0], chunkDims[0]),
                                 openmp::CeilDivide(size[1], chunkDims[1]),
                                 openmp::CeilDivide(size[2], chunkDims[2]) };
  const viskores::Id chunkCount = numChunks[0] * numChunks[1] * numChunks[2];

  // Lambda to convert chunkIdx into a start/end {i, j, k}:
  auto computeIJK = [&](const viskores::Id& chunkIdx, viskores::Id3& start, viskores::Id3& end)
  {
    start[0] = chunkIdx % numChunks[0];
    start[1] = (chunkIdx / numChunks[0]) % numChunks[1];
    start[2] = (chunkIdx / (numChunks[0] * numChunks[1]));
    start *= chunkDims; // c-wise mult

    end[0] = std::min(start[0] + chunkDims[0], size[0]);
    end[1] = std::min(start[1] + chunkDims[1], size[1]);
    end[2] = std::min(start[2] + chunkDims[2], size[2]);
  };

  // Iterate through each chunk, converting the chunkIdx into an ijk range:
  VISKORES_OPENMP_DIRECTIVE(parallel for
                        schedule(guided))
  for (viskores::Id chunkIdx = 0; chunkIdx < chunkCount; ++chunkIdx)
  {
    viskores::Id3 startIJK;
    viskores::Id3 endIJK;
    computeIJK(chunkIdx, startIJK, endIJK);

    for (viskores::Id k = startIJK[2]; k < endIJK[2]; ++k)
    {
      for (viskores::Id j = startIJK[1]; j < endIJK[1]; ++j)
      {
        functor(size, startIJK[0], endIJK[0], j, k);
      }
    }
  }

  if (errorMessage.IsErrorRaised())
  {
    throw viskores::cont::ErrorExecution(errorString);
  }
}
}
} // end namespace viskores::cont
