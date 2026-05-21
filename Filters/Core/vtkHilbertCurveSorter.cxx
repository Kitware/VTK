// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHilbertCurveSorter.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <numeric>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHilbertCurveSorter);

//------------------------------------------------------------------------------
void vtkHilbertCurveSorter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkHilbertCurveSorter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

namespace
{

//------------------------------------------------------------------------------
// Hilbert curve spatial sort (Skilling, AIP Conf. Proc. 707, 381, 2004).
// Converts n-dimensional axis coordinates to a transposed Hilbert index.

//------------------------------------------------------------------------------
// 2D variant.
void AxesToHilbertTranspose2D(unsigned int X[2], int order)
{
  unsigned int M = 1U << (order - 1);
  for (unsigned int Q = M; Q > 1; Q >>= 1)
  {
    unsigned int P = Q - 1;
    for (int i = 1; i >= 0; --i)
    {
      if (X[i] & Q)
      {
        X[0] ^= P;
      }
      else
      {
        unsigned int t = (X[0] ^ X[i]) & P;
        X[0] ^= t;
        X[i] ^= t;
      }
    }
  }
  X[1] ^= X[0];
  unsigned int t = 0;
  for (unsigned int Q = M; Q > 1; Q >>= 1)
  {
    if (X[1] & Q)
    {
      t ^= Q - 1;
    }
  }
  X[0] ^= t;
  X[1] ^= t;
}

//------------------------------------------------------------------------------
// 3D variant.
void AxesToHilbertTranspose3D(unsigned int X[3], int order)
{
  unsigned int M = 1U << (order - 1);
  for (unsigned int Q = M; Q > 1; Q >>= 1)
  {
    unsigned int P = Q - 1;
    for (int i = 0; i < 3; i++)
    {
      if (X[i] & Q)
      {
        X[0] ^= P;
      }
      else
      {
        unsigned int t = (X[0] ^ X[i]) & P;
        X[0] ^= t;
        X[i] ^= t;
      }
    }
  }
  for (int i = 1; i < 3; i++)
  {
    X[i] ^= X[i - 1];
  }
  unsigned int t = 0;
  for (unsigned int Q = M; Q > 1; Q >>= 1)
  {
    if (X[2] & Q)
    {
      t ^= Q - 1;
    }
  }
  for (int i = 0; i < 3; i++)
  {
    X[i] ^= t;
  }
}

//------------------------------------------------------------------------------
// Interleave transposed coordinates into a single 64-bit index.
vtkTypeUInt64 HilbertTransposeToIndex(const unsigned int X[], int dims, int order)
{
  vtkTypeUInt64 h = 0;
  for (int i = order - 1; i >= 0; i--)
  {
    for (int d = 0; d < dims; d++)
    {
      h = (h << 1) | ((X[d] >> i) & 1U);
    }
  }
  return h;
}

//------------------------------------------------------------------------------
// Compute hilbert index from coordinates + bounds.
template <int Dims>
struct ComputeHilbertIndex
{
  template <typename TPointArray>
  void operator()(TPointArray* pointArray, const double bounds[6], vtkIdList* hilbertIndex)
  {
    hilbertIndex->SetNumberOfIds(pointArray->GetNumberOfTuples());

    double range[3];
    for (int d = 0; d < 3; d++)
    {
      range[d] = bounds[2 * d + 1] - bounds[2 * d];
      if (range[d] <= 0.0)
      {
        range[d] = 1.0;
      }
    }
    const int hilbertOrder = 20;
    const double gridSize = static_cast<double>((1U << hilbertOrder) - 1);

    auto points = vtk::DataArrayTupleRange<3>(pointArray);
    if constexpr (Dims == 2)
    {
      vtkSMPTools::For(0, points.size(),
        [&](vtkIdType begin, vtkIdType end)
        {
          for (vtkIdType i = begin; i < end; ++i)
          {
            const auto pt = points[i];
            unsigned int X[2];
            for (int d = 0; d < 2; d++)
            {
              double nrm = (pt[d] - bounds[2 * d]) / range[d];
              if (nrm < 0.0)
              {
                nrm = 0.0;
              }
              else if (nrm > 1.0)
              {
                nrm = 1.0;
              }
              X[d] = static_cast<unsigned int>(nrm * gridSize);
            }
            AxesToHilbertTranspose2D(X, hilbertOrder);
            hilbertIndex->SetId(i, HilbertTransposeToIndex(X, 2, hilbertOrder));
          }
        });
    }
    else if constexpr (Dims == 3)
    {
      vtkSMPTools::For(0, points.size(),
        [&](vtkIdType begin, vtkIdType end)
        {
          for (vtkIdType i = begin; i < end; ++i)
          {
            const auto pt = points[i];
            unsigned int X[3];
            for (int d = 0; d < 3; d++)
            {
              double nrm = (pt[d] - bounds[2 * d]) / range[d];
              if (nrm < 0.0)
              {
                nrm = 0.0;
              }
              else if (nrm > 1.0)
              {
                nrm = 1.0;
              }
              X[d] = static_cast<unsigned int>(nrm * gridSize);
            }
            AxesToHilbertTranspose3D(X, hilbertOrder);
            hilbertIndex->SetId(i, HilbertTransposeToIndex(X, 3, hilbertOrder));
          }
        });
    }
  }
};

//  Update the cell connectivity array.
struct UpdateCellArrayConnectivity : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* vtkNotUsed(offsets), ConnectivityT* connectivity, vtkIdType* ptMap)
  {
    auto connRange = GetRange(connectivity);
    vtkSMPTools::For(0, connectivity->GetNumberOfValues(),
      [&](vtkIdType beginId, vtkIdType endId)
      {
        std::transform(connRange.begin() + beginId, connRange.begin() + endId,
          connRange.begin() + beginId, [&ptMap](vtkIdType ptId) { return ptMap[ptId]; });
      }); // end lambda
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
int vtkHilbertCurveSorter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkPointSet::GetData(inputVector[0]);
  auto output = vtkPointSet::GetData(outputVector);
  if (!input || !output)
  {
    vtkErrorMacro("Invalid input or output");
    return 0;
  }
  if (input->GetNumberOfPoints() <= 1)
  {
    output->ShallowCopy(input);
    return 1;
  }

  auto points = input->GetPoints();
  vtkIdType numPoints = points->GetNumberOfPoints();
  double bounds[6];
  points->GetBounds(bounds);

  // Select 2D or 3D based on z-range relative to xy-range.
  double xyRange = std::max(bounds[1] - bounds[0], bounds[3] - bounds[2]);
  double zRange = bounds[5] - bounds[4];
  const int dims = (xyRange > 0.0 && zRange / xyRange < 1.0e-8) ? 2 : 3;

  vtkNew<vtkIdList> hilbertIndex;
  if (dims == 2)
  {
    ComputeHilbertIndex<2> worker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
          points->GetData(), worker, bounds, hilbertIndex))
    {
      worker(points->GetData(), bounds, hilbertIndex);
    }
  }
  else // if (dims == 3)
  {
    ComputeHilbertIndex<3> worker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
          points->GetData(), worker, bounds, hilbertIndex))
    {
      worker(points->GetData(), bounds, hilbertIndex);
    }
  }

  // create new point ordering using hilbert index
  this->Permutation->SetNumberOfIds(numPoints);
  std::iota(this->Permutation->GetPointer(0), this->Permutation->GetPointer(0) + numPoints, 0);

  // Sort the permutation by Hilbert index.
  vtkSMPTools::Sort(this->Permutation->GetPointer(0), this->Permutation->GetPointer(0) + numPoints,
    [&](vtkIdType a, vtkIdType b) { return hilbertIndex->GetId(a) < hilbertIndex->GetId(b); });

  if (this->ComputePermutationOnly)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // create output points
  vtkNew<vtkPoints> outPoints;
  outPoints->SetDataType(points->GetDataType());
  outPoints->GetData()->InsertTuplesStartingAt(0, this->Permutation, points->GetData());
  output->SetPoints(outPoints);

  // copy point data
  auto outPD = output->GetPointData();
  outPD->CopyAllocate(input->GetPointData(), numPoints);
  outPD->CopyData(input->GetPointData(), this->Permutation);

  // pass cell data
  auto outCD = output->GetCellData();
  outCD->PassData(input->GetCellData());

  if (input->GetNumberOfCells() == 0)
  {
    return 1;
  }

  // build inverse permutation
  vtkNew<vtkIdList> inversePermutation;
  inversePermutation->SetNumberOfIds(numPoints);
  vtkSMPTools::For(0, numPoints,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        inversePermutation->SetId(this->Permutation->GetId(i), i);
      }
    });

  // update connectivity
  UpdateCellArrayConnectivity updateConn;
  if (auto inPolydata = vtkPolyData::SafeDownCast(input))
  {
    auto outPolydata = vtkPolyData::SafeDownCast(output);
    if (inPolydata->GetVerts() && inPolydata->GetVerts()->GetNumberOfCells() > 0)
    {
      vtkNew<vtkCellArray> verts;
      verts->DeepCopy(inPolydata->GetVerts());
      verts->Dispatch(updateConn, inversePermutation->GetPointer(0));
      outPolydata->SetVerts(verts);
    }
    if (inPolydata->GetLines() && inPolydata->GetLines()->GetNumberOfCells() > 0)
    {
      vtkNew<vtkCellArray> lines;
      lines->DeepCopy(inPolydata->GetLines());
      lines->Dispatch(updateConn, inversePermutation->GetPointer(0));
      outPolydata->SetLines(lines);
    }
    if (inPolydata->GetPolys() && inPolydata->GetPolys()->GetNumberOfCells() > 0)
    {
      vtkNew<vtkCellArray> polys;
      polys->DeepCopy(inPolydata->GetPolys());
      polys->Dispatch(updateConn, inversePermutation->GetPointer(0));
      outPolydata->SetPolys(polys);
    }
    if (inPolydata->GetStrips() && inPolydata->GetStrips()->GetNumberOfCells() > 0)
    {
      vtkNew<vtkCellArray> strips;
      strips->DeepCopy(inPolydata->GetStrips());
      strips->Dispatch(updateConn, inversePermutation->GetPointer(0));
      outPolydata->SetStrips(strips);
    }
  }
  else if (auto inUG = vtkUnstructuredGrid::SafeDownCast(input))
  {
    auto outUG = vtkUnstructuredGrid::SafeDownCast(output);
    if (inUG->GetCells() && inUG->GetCells()->GetNumberOfCells() > 0)
    {
      vtkNew<vtkCellArray> cells;
      cells->DeepCopy(inUG->GetCells());
      cells->Dispatch(updateConn, inversePermutation->GetPointer(0));
      vtkNew<vtkCellArray> faces, faceLocations;
      if (inUG->GetPolyhedronFaces() && inUG->GetPolyhedronFaces()->GetNumberOfCells() > 0)
      {
        faces->DeepCopy(inUG->GetPolyhedronFaces());
        faces->Dispatch(updateConn, inversePermutation->GetPointer(0));
        outUG->SetPolyhedralCells(
          inUG->GetCellTypes(), cells, inUG->GetPolyhedronFaceLocations(), faces);
      }
      else
      {
        outUG->SetPolyhedralCells(inUG->GetCellTypes(), cells, nullptr, nullptr);
      }
    }
  }
  else if (auto inESG = vtkExplicitStructuredGrid::SafeDownCast(input))
  {
    auto outESG = vtkExplicitStructuredGrid::SafeDownCast(output);
    if (inESG->GetCells() && inESG->GetCells()->GetNumberOfCells() > 0)
    {
      vtkNew<vtkCellArray> cells;
      cells->DeepCopy(inESG->GetCells());
      cells->Dispatch(updateConn, inversePermutation->GetPointer(0));
      outESG->SetCells(cells);
    }
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
