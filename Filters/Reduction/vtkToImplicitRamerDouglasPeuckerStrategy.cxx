/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkToImplicitRamerDouglasPeuckerStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToImplicitRamerDouglasPeuckerStrategy.h"

#include "vtkAffineArray.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchImplicitArrayList.h"
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
  void operator()(ArrayT* arr, double tol, std::set<vtkIdType>& vertexes)
  {
    auto range = vtk::DataArrayValueRange(arr);
    vertexes.insert(0);
    if (range.size() == 1)
    {
      return;
    }
    vertexes.insert(range.size() - 1);
    this->Recurse(range.begin(), range.end(), vertexes, tol);
  }

  template <typename Iterator>
  void Recurse(
    Iterator begin, Iterator end, std::set<vtkIdType>& vertexes, double tol, vtkIdType bIdx = 0)
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
      vertexes.insert(idx);
      this->Recurse(begin, maxIt, vertexes, tol, bIdx);
      this->Recurse(maxIt, end, vertexes, tol, idx);
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
    ArrayT* arr, double tol, std::set<vtkIdType>& vertexes, vtkSmartPointer<vtkDataArray>& result)
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
    if (vertexes.size() == 1)
    {
      result = makeConstant(range[*vertexes.begin()], arr->GetNumberOfValues());
      return;
    }

    // create the parts of the resulting array in order
    std::vector<vtkSmartPointer<vtkDataArray>> parts;
    auto beforeLast = vertexes.end()--;
    /*
     * This skipSingles mechanic comes from the fact that most single index jumps can be skipped in
     * this context. The only ones that cannot be skipped are those that follow another single index
     * skip, hence the flipping of this boolean, or if they are first or last in the vertex list.
     */
    bool skipSingles = false;
    for (auto it = vertexes.begin(); it != beforeLast; ++it)
    {
      auto nxtIt = it;
      nxtIt++;
      vtkIdType dist = *nxtIt - *it;
      if (dist == 1)
      {
        if (skipSingles)
        {
          parts.back()->SetNumberOfTuples(parts.back()->GetNumberOfTuples() + 1);
          skipSingles = false;
          continue;
        }
      }
      if (dist == 1 || std::abs(static_cast<double>(range[*nxtIt] - range[*it])) < tol)
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
  void Squeeze() { this->Vertexes.clear(); }

  /*
   * Perform RDP algorithm and estimate by how much we can reduce the current array
   */
  Option<double> EstimateReduction(vtkDataArray* arr, double tol)
  {
    this->Squeeze();
    ::RDPAlgorithm algo;
    if (!Dispatch::Execute(arr, algo, tol, this->Vertexes))
    {
      algo(arr, tol, this->Vertexes);
    }
    return Option<double>(
      static_cast<double>(this->EstimateCompressedSize(arr, tol)) / arr->GetNumberOfValues());
  }

  /*
   * Perform RDP algorithm if no cache present and return compressed array
   */
  vtkSmartPointer<vtkDataArray> Reduce(vtkDataArray* arr, double tol)
  {
    if (this->Vertexes.empty())
    {
      this->EstimateReduction(arr, tol);
      if (this->Vertexes.empty())
      {
        vtkWarningWithObjectMacro(nullptr, "Could not successfully reduce array");
        return nullptr;
      }
    }
    vtkSmartPointer<vtkDataArray> res = nullptr;
    ::GenerateFunctionalRepresentation generator;
    if (!Dispatch::Execute(arr, generator, tol, this->Vertexes, res))
    {
      generator(arr, tol, this->Vertexes, res);
    }
    return res;
  }

private:
  /*
   * Estimate the number of independant variables in the resulting compressed array accoutning for
   * constant arrays
   */
  std::size_t EstimateCompressedSize(vtkDataArray* arr, double tol)
  {
    std::size_t compressedSize = this->Vertexes.size();
    if (!compressedSize)
    {
      vtkWarningWithObjectMacro(nullptr, "Number of vertexes is zero");
      return compressedSize;
    }
    if (compressedSize == 1)
    {
      return compressedSize;
    }
    compressedSize = (compressedSize - 1) * 2;
    auto range = vtk::DataArrayValueRange(arr);
    auto beforeLast = this->Vertexes.end()--;
    bool skipSingles = false;
    for (auto it = this->Vertexes.begin(); it != beforeLast; ++it)
    {
      auto nxtIt = it;
      nxtIt++;
      vtkIdType dist = *nxtIt - *it;
      if (dist == 1)
      {
        if (skipSingles)
        {
          compressedSize -= 2;
          skipSingles = false;
          continue;
        }
      }
      if (dist == 1 || std::abs(static_cast<double>(range[*nxtIt] - range[*it])) < tol)
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
  std::set<vtkIdType> Vertexes;
};

//-------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkToImplicitRamerDouglasPeuckerStrategy);

//-------------------------------------------------------------------------
vtkToImplicitRamerDouglasPeuckerStrategy::vtkToImplicitRamerDouglasPeuckerStrategy()
  : Internals(std::unique_ptr<vtkInternals>(new vtkInternals()))
{
}

//-------------------------------------------------------------------------
vtkToImplicitRamerDouglasPeuckerStrategy::~vtkToImplicitRamerDouglasPeuckerStrategy() {}

//-------------------------------------------------------------------------
void vtkToImplicitRamerDouglasPeuckerStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  os << indent << "vtkToImplicitRamerDouglasPeuckerStrategy: \n";
  this->Superclass::PrintSelf(os, indent);
  os << std::flush;
}

//-------------------------------------------------------------------------
Option<double> vtkToImplicitRamerDouglasPeuckerStrategy::EstimateReduction(vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to affine/constant by parts array.");
    return Option<double>();
  }
  int nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return Option<double>();
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
  int nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return nullptr;
  }
  return this->Internals->Reduce(arr, this->Tolerance);
}

//-------------------------------------------------------------------------
void vtkToImplicitRamerDouglasPeuckerStrategy::Squeeze()
{
  this->Internals->Squeeze();
}
VTK_ABI_NAMESPACE_END
