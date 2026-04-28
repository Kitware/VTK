// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkONNXInternalUtils
 * @brief   some utilities related to ONNX
 *
 * Internal utility functions for the ONNX filters, mainly to manipulate tensors.
 */

#ifndef vtkONNXInternalUtils_h
#define vtkONNXInternalUtils_h

#include "vtkSMPTools.h"

#include <algorithm>
#include <numeric>
#include <onnxruntime_cxx_api.h>
#include <vector>

namespace vtkONNXInternalUtils
{

/**
 * Wraps a raw float buffer into a ONNX Runtime tensor. Note that the ONNX object directly
 * references the memory pointed by data and does not manage or own it.
 */
inline Ort::Value RawToTensor(float* data, const std::vector<int64_t>& shape)
{
  int64_t numberElements = std::accumulate(shape.begin(), shape.end(), 1LL, std::multiplies<>());

  Ort::MemoryInfo memInfo =
    Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

  return Ort::Value::CreateTensor<float>(memInfo, data, numberElements, shape.data(), shape.size());
}

/**
 * This checks if a sequence actually represents a permutation.
 */
inline bool IsPermutation(const std::vector<int>& permutation)
{
  std::vector<int> identity(permutation.size());
  std::iota(identity.begin(), identity.end(), 0);
  return std::is_permutation(identity.begin(), identity.end(), identity.begin());
}

/**
 * Computes the inverse of the input permutation, in other words the permutation you
 * need to apply after the input to retrieve the original sequence.
 */
inline std::vector<int> InversePermutation(const std::vector<int>& permutation)
{
  std::vector<int> inversePermutation(permutation.size());
  for (size_t i = 0; i < permutation.size(); ++i)
  {
    inversePermutation[permutation[i]] = i;
  }
  return inversePermutation;
}

/**
 * This reorders the memory pointed by data so that it matches the layout defined by outputShape and
 * permutation.
 */
inline void Permute(
  float* data, const std::vector<int64_t>& outputShape, const std::vector<int>& permutation)
{
  const size_t nDim = outputShape.size();
  int64_t numElements =
    std::accumulate(outputShape.begin(), outputShape.end(), 1LL, std::multiplies<>());

  // Compute intput shape
  std::vector<int> inversePermutation = InversePermutation(permutation);
  std::vector<int64_t> inputShape(outputShape.size());
  for (size_t i = 0; i < nDim; ++i)
  {
    inputShape[i] = outputShape[inversePermutation[i]];
  }

  // Compute input/output memory strides
  auto computeStrides = [nDim](const std::vector<int64_t>& shape)
  {
    std::vector<int64_t> strides(nDim);
    strides[nDim - 1] = 1;
    for (int i = static_cast<int>(nDim) - 2; i >= 0; --i)
    {
      strides[i] = strides[i + 1] * shape[i + 1];
    }
    return strides;
  };

  std::vector<int64_t> inputStrides = computeStrides(inputShape);
  std::vector<int64_t> outputStrides = computeStrides(outputShape);

  // Permutation loop
  std::vector<float> buffer(numElements);
  vtkSMPTools::For(0, numElements,
    [&](int64_t begin, int64_t end)
    {
      std::vector<int64_t> inputCoords(nDim);
      std::vector<int64_t> outputCoords(nDim);

      for (int64_t inputIndex = begin; inputIndex < end; ++inputIndex)
      {
        int tmpInputIndex = inputIndex;
        // Input shape coords
        for (size_t i = 0; i < nDim; ++i)
        {
          inputCoords[i] = tmpInputIndex / inputStrides[i];
          tmpInputIndex %= inputStrides[i];
        }

        // Apply permutation
        for (size_t i = 0; i < nDim; ++i)
        {
          outputCoords[i] = inputCoords[permutation[i]];
        }

        // Output shape coords
        int outputIndex = 0;
        for (size_t i = 0; i < nDim; ++i)
        {
          outputIndex += outputCoords[i] * outputStrides[i];
        }

        buffer[outputIndex] = data[inputIndex];
      }
    });

  std::copy(buffer.begin(), buffer.end(), data);
}

} // namespace vtkONNXInternalUtils

#endif
