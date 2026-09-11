// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkONNXInternalUtils.h"

#include "vtkLogger.h"

#include <cstdlib>
#include <vector>

namespace
{
bool AssertPermutation(const std::vector<int>& perm1, const std::vector<int>& perm2)
{

  if (perm1.size() != perm2.size())
  {
    vtkLog(ERROR, "Permutation size is different from the expected one.");
    return false;
  }
  for (size_t i = 0; i < perm1.size(); ++i)
  {
    if (perm1[i] != perm2[i])
    {
      vtkLog(ERROR, "Permutation is different from the expected one.");
      return false;
    }
  }
  return true;
}
}

int TestFindMatchingPermutation(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  std::vector<std::array<int64_t, 2>> vtkShapes = { { 100, 3 }, { 7, 6 }, { 100, 6 },
    { 1048576, 3 }, { 50, 50 }, { 122, 3 }, { 4, 2 }, { 60, 28 }, { 2, 100 } };
  std::vector<std::vector<int64_t>> tensorShape = { { 3, 10, 10 }, { 6, 7 }, { 3, 10, 10, 2 },
    { 3, 1024, 1024, 1 }, { 50, 50 }, { 57, 8, 1, 24 }, { 8 }, { 2, 1, 3, 4, 5, 2, 1, 7, 1 },
    { 2, 1, 100 } };
  std::vector<std::vector<int>> expectedPermutation = { { 1, 2, 0 }, { 1, 0 }, { 1, 2, 0, 3 },
    { 1, 2, 3, 0 }, {}, {}, {}, { 0, 1, 2, 4, 5, 6, 8, 3, 7 }, {} };

  bool test = true;
  for (size_t i = 0; i < vtkShapes.size(); ++i)
  {
    std::vector<int> actualPermutation = vtkONNXInternalUtils::FindMatchingPermutation(
      vtkShapes[i][0], vtkShapes[i][1], tensorShape[i]);
    test &= AssertPermutation(actualPermutation, expectedPermutation[i]);
  }
  return test ? EXIT_SUCCESS : EXIT_FAILURE;
}
