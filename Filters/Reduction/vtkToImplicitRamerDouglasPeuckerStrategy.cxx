// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToImplicitRamerDouglasPeuckerStrategy.h"

#include "vtkAffineArray.h"
#include "vtkArrayDispatch.h"
#include "vtkCompositeArray.h"
#include "vtkConstantArray.h"
#include "vtkDataArrayRange.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <set>

namespace
{
//-------------------------------------------------------------------------
/*
 * An implementation of the Ramer-Douglas-Peucker algorithm for reducing polylines applied to arrays
 * here in a manner compatible with the array dispatch
 */
struct RDPAlgorithm
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, double tol, std::set<vtkIdType>& vertices)
  {
    vertices.insert(0);

    auto range = vtk::DataArrayValueRange(arr).GetSubRange(0, arr->GetNumberOfValues() - 1);

    if (range.size() == 0)
    {
      return;
    }

    vertices.insert(range.size());
    this->Recurse(range.begin(), range.end(), vertices, tol);
  }

  template <typename Iterator>
  void Recurse(
    Iterator begin, Iterator end, std::set<vtkIdType>& vertices, double tol, vtkIdType bIdx = 0)
  {
    // natural stopping criterion
    if (begin == end || begin + 1 == end)
    {
      return;
    }
    Iterator maxIt = begin;
    double maxDist = 0.0;
    // this scope is meant to clean up temporary variables so that they don't stay in memory during
    // following recursions
    {
      auto slope = (*end - *begin) / std::distance(begin, end);
      // for computing the distance between the affine representation and the actual value
      auto distance = [&](const Iterator& it) {
        return std::abs(static_cast<double>(slope * std::distance(begin, it) + *begin - *it));
      };
      // compute max distance loop
      for (Iterator it = begin + 1; it != end; ++it)
      {
        double dist = distance(it);
        if (dist > maxDist)
        {
          maxDist = dist;
          maxIt = it;
        }
      }
    }
    // if we are too far from the actual array, divide into two parts and recurse
    if (maxDist > tol)
    {
      vtkIdType idx = bIdx + std::distance(begin, maxIt);
      vertices.insert(idx);
      this->Recurse(begin, maxIt, vertices, tol, bIdx);
      this->Recurse(maxIt, end, vertices, tol, idx);
    }
  }
};

//-------------------------------------------------------------------------
/*
 * Dispatch compatible struct for generating the compressed array from the base array and result of
 * the RDP algorithm
 */
struct GenerateFunctionalRepresentation
{
  template <typename ArrayT>
  void operator()(
    ArrayT* arr, double tol, std::set<vtkIdType>& vertices, vtkSmartPointer<vtkDataArray>& result)
  {
    using VType = vtk::GetAPIType<ArrayT>;

    auto makeConstant = [](VType val, vtkIdType diff) {
      vtkNew<vtkConstantArray<VType>> constant;
      constant->SetBackend(std::make_shared<vtkConstantImplicitBackend<VType>>(val));
      constant->SetNumberOfComponents(1);
      constant->SetNumberOfTuples(diff);
      return constant;
    };

    auto range = vtk::DataArrayValueRange(arr);
    // if there is only one index in vertices, the array is constant and can be represented by a
    // simple constant array
    if (vertices.size() == 1)
    {
      result = makeConstant(range[*vertices.begin()], arr->GetNumberOfValues());
      return;
    }

    // create the parts of the resulting array in order
    std::vector<vtkSmartPointer<vtkDataArray>> parts;
    const auto beforeLast = std::prev(vertices.end());
    auto nxtIt = std::next(vertices.begin());
    /*
     * This skipSingles mechanic comes from the fact that most single index jumps can be skipped in
     * this context. The only ones that cannot be skipped are those that follow another single index
     * skip, hence the flipping of this boolean, or if they are first or last in the vertex list.
     */
    bool skipSingles = false;
    for (auto it = vertices.begin(); it != beforeLast; ++it, ++nxtIt)
    {
      vtkIdType dist = *nxtIt - *it;
      if ((dist == 1) && skipSingles)
      {
        parts.back()->SetNumberOfTuples(parts.back()->GetNumberOfTuples() + 1);
        skipSingles = false;
        continue;
      }
      if ((dist == 1) || std::abs(static_cast<double>(range[*nxtIt] - range[*it])) < tol)
      {
        parts.emplace_back(makeConstant(range[*it], dist));
      }
      else
      {
        vtkNew<vtkAffineArray<VType>> affine;
        affine->SetBackend(std::make_shared<vtkAffineImplicitBackend<VType>>(
          (range[*nxtIt] - range[*it]) / dist, range[*it]));
        affine->SetNumberOfComponents(1);
        affine->SetNumberOfTuples(dist);
        parts.emplace_back(affine);
      }
      skipSingles = (nxtIt != beforeLast);
    }
    if (parts.size() == 1)
    {
      parts[0]->SetNumberOfComponents(arr->GetNumberOfComponents());
      parts[0]->SetNumberOfTuples(arr->GetNumberOfTuples());
      parts[0]->SetName(arr->GetName());
      result = parts[0];
      return;
    }
    // if more than 1 array, pack up all parts into composite
    std::vector<vtkDataArray*> weakParts;
    weakParts.assign(parts.begin(), parts.end());
    vtkNew<vtkCompositeArray<VType>> composite;
    composite->SetBackend(std::make_shared<vtkCompositeImplicitBackend<VType>>(weakParts));
    composite->SetNumberOfComponents(arr->GetNumberOfComponents());
    composite->SetNumberOfTuples(arr->GetNumberOfTuples());
    composite->SetName(arr->GetName());
    result = composite;
  }
};

}

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------
struct vtkToImplicitRamerDouglasPeuckerStrategy::vtkInternals
{
  using Dispatch = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllArrays>;

public:
  /*
   * Release cached vertices
   */
  void ClearCache()
  {
    if (!this->Vertices.empty())
    {
      this->Vertices.clear();
    }
    if (this->CachedArray)
    {
      this->CachedArray = nullptr;
      this->ArrayMTimeAtCaching = vtkMTimeType();
    }
  }

  /*
   * Perform RDP algorithm and estimate by how much we can reduce the current array
   */
  vtkToImplicitStrategy::Optional EstimateReduction(vtkDataArray* arr, double tol)
  {
    this->ClearCache();
    this->CachedArray = arr;
    this->ArrayMTimeAtCaching = arr->GetMTime();
    ::RDPAlgorithm algo;
    if (!Dispatch::Execute(arr, algo, tol, this->Vertices))
    {
      algo(arr, tol, this->Vertices);
    }
    return vtkToImplicitStrategy::Optional(
      static_cast<double>(this->EstimateCompressedSize(arr, tol)) / arr->GetNumberOfValues());
  }

  /*
   * Perform RDP algorithm if no cache present and return compressed array
   */
  vtkSmartPointer<vtkDataArray> Reduce(vtkDataArray* arr, double tol)
  {
    if (this->Vertices.empty() || (arr != this->CachedArray) ||
      this->ArrayMTimeAtCaching < arr->GetMTime())
    {
      this->EstimateReduction(arr, tol);
      if (this->Vertices.empty())
      {
        vtkWarningWithObjectMacro(nullptr, "Could not successfully reduce array");
        return nullptr;
      }
    }
    vtkSmartPointer<vtkDataArray> res = nullptr;
    ::GenerateFunctionalRepresentation generator;
    if (!Dispatch::Execute(arr, generator, tol, this->Vertices, res))
    {
      generator(arr, tol, this->Vertices, res);
    }
    this->ClearCache();
    return res;
  }

private:
  /*
   * Estimate the number of independent variables in the resulting compressed array accoutning for
   * constant arrays
   */
  std::size_t EstimateCompressedSize(vtkDataArray* arr, double tol)
  {
    std::size_t compressedSize = this->Vertices.size();
    if (!compressedSize)
    {
      vtkWarningWithObjectMacro(nullptr, "Number of vertices is zero");
      return compressedSize;
    }
    if (compressedSize == 1)
    {
      return compressedSize;
    }
    compressedSize = (compressedSize - 1) * 2;
    auto range = vtk::DataArrayValueRange(arr);
    const auto beforeLast = std::prev(this->Vertices.end());
    auto nxtIt = std::next(this->Vertices.begin());
    bool skipSingles = false;
    for (auto it = this->Vertices.begin(); it != beforeLast; ++it, ++nxtIt)
    {
      bool distEqOne = ((*nxtIt - *it) == 1);
      if (distEqOne && skipSingles)
      {
        compressedSize -= 2;
        skipSingles = false;
        continue;
      }
      if (distEqOne || std::abs(static_cast<double>(range[*nxtIt] - range[*it])) < tol)
      {
        compressedSize--;
      }
      skipSingles = true;
    }
    return compressedSize;
  }

  /*
   * Cache for storing the result of the RDP algorithm, std::set chosen because it is sorted and
   * keys are unique
   */
  std::set<vtkIdType> Vertices;
  vtkDataArray* CachedArray = nullptr;
  vtkMTimeType ArrayMTimeAtCaching;
};

//-------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkToImplicitRamerDouglasPeuckerStrategy);

//-------------------------------------------------------------------------
vtkToImplicitRamerDouglasPeuckerStrategy::vtkToImplicitRamerDouglasPeuckerStrategy()
  : Internals(std::unique_ptr<vtkInternals>(new vtkInternals()))
{
}

//-------------------------------------------------------------------------
vtkToImplicitRamerDouglasPeuckerStrategy::~vtkToImplicitRamerDouglasPeuckerStrategy() = default;

//-------------------------------------------------------------------------
void vtkToImplicitRamerDouglasPeuckerStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << std::flush;
}

//-------------------------------------------------------------------------
vtkToImplicitStrategy::Optional vtkToImplicitRamerDouglasPeuckerStrategy::EstimateReduction(
  vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to affine/constant by parts array.");
    return vtkToImplicitStrategy::Optional();
  }
  vtkIdType nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return vtkToImplicitStrategy::Optional();
  }
  return this->Internals->EstimateReduction(arr, this->Tolerance);
}

//-------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkToImplicitRamerDouglasPeuckerStrategy::Reduce(vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to affine/constant by parts array.");
    return nullptr;
  }
  vtkIdType nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return nullptr;
  }
  return this->Internals->Reduce(arr, this->Tolerance);
}

//-------------------------------------------------------------------------
void vtkToImplicitRamerDouglasPeuckerStrategy::ClearCache()
{
  this->Internals->ClearCache();
}
VTK_ABI_NAMESPACE_END
