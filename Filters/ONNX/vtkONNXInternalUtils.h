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
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace vtkONNXInternalUtils
{

/**
 * Helper to find the total number of elements given the list of dimensions of a tensor.
 */
inline int64_t TensorNumberOfElements(const std::vector<int64_t>& shape)
{
  return std::accumulate(shape.begin(), shape.end(), 1LL, std::multiplies<>());
}

/**
 * This checks if a sequence actually represents a permutation.
 */
inline bool IsPermutation(const std::vector<int>& permutation)
{
  std::cout << std::endl;
  std::vector<int> identity(permutation.size());
  std::iota(identity.begin(), identity.end(), 0);
  return std::is_permutation(identity.begin(), identity.end(), permutation.begin());
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
  int64_t numElements = TensorNumberOfElements(outputShape);

  // Compute input shape
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

/**
 * This function tries to find the permutation required to match the dimensions of
 * a VTK array to any N dimensional array.
 */
inline std::vector<int> FindMatchingPermutation(
  int64_t numTuples, int64_t numComponents, const std::vector<int64_t> modelShape)
{
  if (modelShape.empty())
  {
    return {};
  }

  const int64_t vtkShape[2] = { numTuples, numComponents };
  constexpr int vtkRank = 2;
  const int modelRank = static_cast<int>(modelShape.size());

  const int64_t modelTotalElements = TensorNumberOfElements(modelShape);
  const int64_t vtkTotalElements = numTuples * numComponents;

  if (modelTotalElements != vtkTotalElements)
  {
    return {};
  }

  if (modelRank <= 1)
  {
    return {};
  }

  if (modelRank == 2)
  {
    if (modelShape[0] == numTuples && modelShape[1] == numComponents)
    {
      return {};
    }

    if (modelShape[0] == numComponents && modelShape[1] == numTuples)
    {
      return { 1, 0 };
    }
  }

  // For high rank cases, first look for direct matches
  std::array<std::vector<int>, vtkRank> vtkToModelMapping;

  std::vector<bool> usedModel(modelRank, false);
  std::vector<bool> usedVTK(vtkRank, false);

  for (int i = 0; i < modelRank; ++i)
  {
    for (int j = 0; j < vtkRank; ++j)
    {
      if (modelShape[i] == vtkShape[j] && !usedVTK[j] && !usedModel[i])
      {
        vtkToModelMapping[j].push_back(i);
        usedModel[i] = true;
        usedVTK[j] = true;
      }
    }
  }

  // This iterates through each bit of the `productMask` and apply `func` if true
  auto forEachSelectedDimension = [&](int productMask, auto&& func)
  {
    int bitShift = 0;

    for (size_t i = 0; i < usedModel.size(); ++i)
    {
      if (!usedModel[i])
      {
        if (productMask & (1 << bitShift))
        {
          func(i);
        }
        ++bitShift;
      }
    }
  };

  // Then, brute-force remaining dimensions
  for (int j = 0; j < vtkRank; ++j)
  {
    if (usedVTK[j])
    {
      continue;
    }
    const int64_t dimension = vtkShape[j];
    int unusedShapeElements =
      std::count_if(usedModel.begin(), usedModel.end(), [](bool used) { return !used; });

    for (int productMask = (1 << unusedShapeElements); productMask > 0; --productMask)
    {
      int64_t extractedDimension = 1;

      forEachSelectedDimension(productMask, [&](size_t i) { extractedDimension *= modelShape[i]; });

      if (extractedDimension == dimension)
      {
        usedVTK[j] = true;

        forEachSelectedDimension(productMask,
          [&](size_t i)
          {
            vtkToModelMapping[j].push_back(i);
            usedModel[i] = true;
          });

        break;
      }
    }
  }

  if (vtkToModelMapping[0].empty() || vtkToModelMapping[1].empty())
  {
    return {};
  }

  std::vector<int> permutation;
  permutation.reserve(modelRank);
  for (int i = 0; i < vtkRank; ++i)
  {
    for (int j = 0; j < static_cast<int>(vtkToModelMapping[i].size()); ++j)
    {
      permutation.push_back(vtkToModelMapping[i][j]);
    }
  }

  if (!IsPermutation(permutation))
  {
    return {};
  }

  return permutation;
}

} // namespace vtkONNXInternalUtils
VTK_ABI_NAMESPACE_END
#endif
