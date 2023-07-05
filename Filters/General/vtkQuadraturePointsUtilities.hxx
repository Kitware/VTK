// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkQuadraturePointsUtilities_hxx
#define vtkQuadraturePointsUtilities_hxx

#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkQuadratureSchemeDefinition.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

namespace vtkQuadraturePointsUtilities
{
VTK_ABI_NAMESPACE_BEGIN

// Description:
// For all cells in the input "usg", for a specific array
// "V" interpolate to quadrature points using the given
// dictionary "dict" into "interpolated". Additionally if
// "indices" is not 0 then track the indices of where the
// values from each cell start as well. In the case of
// an error the return is 0.
struct InterpolateWorker
{

  // Version without offsets:
  template <typename ValueArrayT>
  void operator()(ValueArrayT* valueArray, vtkUnstructuredGrid* usg, const vtkIdType nCellsUsg,
    std::vector<vtkQuadratureSchemeDefinition*>& dict, vtkDoubleArray* interpolated,
    vtkAlgorithm* self)
  {
    this->operator()(valueArray, static_cast<vtkAOSDataArrayTemplate<vtkIdType>*>(nullptr), usg,
      nCellsUsg, dict, interpolated, self);
  }

  // Version with offsets:
  template <typename ValueArrayT, typename IndexArrayT>
  void operator()(ValueArrayT* valueArray, IndexArrayT* indexArray, vtkUnstructuredGrid* usg,
    const vtkIdType nCellsUsg, std::vector<vtkQuadratureSchemeDefinition*>& dict,
    vtkDoubleArray* interpolated, vtkAlgorithm* self)
  {
    using IndexType = vtk::GetAPIType<IndexArrayT>;
    const vtk::ComponentIdType nCompsV = valueArray->GetNumberOfComponents();
    const auto valueTuples = vtk::DataArrayTupleRange(valueArray);
    bool abort = false;

    // Walk cells.
    vtkIdType currentIndex = 0;
    for (vtkIdType cellId = 0; cellId < nCellsUsg && !abort; ++cellId)
    {
      if (indexArray)
      {
        // Point to the start of the data associated with this cell.
        auto indices = vtk::DataArrayValueRange<1>(indexArray);
        indices[cellId] = static_cast<IndexType>(currentIndex);
      }

      // Grab the cell's associated shape function definition.
      int cellType = usg->GetCellType(cellId);
      vtkQuadratureSchemeDefinition* def = dict[cellType];
      if (def == nullptr)
      {
        // no quadrature scheme been specified for this cell type
        // skipping the cell.
        continue;
      }
      vtkIdType nNodes = def->GetNumberOfNodes();
      int nQPts = def->GetNumberOfQuadraturePoints();
      // Grab the cell's node ids.
      const vtkIdType* cellNodeIds = nullptr;
      usg->GetCellPoints(cellId, nNodes, cellNodeIds);
      // Walk quadrature points.
      for (int qPtId = 0; qPtId < nQPts; ++qPtId)
      {
        if (self->CheckAbort())
        {
          abort = true;
          break;
        }
        // Grab the result and initialize.
        double* r = interpolated->WritePointer(currentIndex, nCompsV);
        for (int q = 0; q < nCompsV; ++q)
        {
          r[q] = 0.0;
        }
        // Grab shape function weights.
        const double* N = def->GetShapeFunctionWeights(qPtId);
        // Walk the cell's nodes.
        for (int j = 0; j < nNodes; ++j)
        {
          // Apply shape function weights.
          const auto tuple = valueTuples[cellNodeIds[j]];
          for (int q = 0; q < nCompsV; ++q)
          {
            r[q] += N[j] * tuple[q];
          }
        }
        // Update the result index.
        currentIndex += nCompsV;
      }
    }
  }
};

//------------------------------------------------------------------------------
template <class T>
void ApplyShapeFunction(double* r, double N_j, T* A, int nComps)
{
  for (int q = 0; q < nComps; ++q)
  {
    r[q] += N_j * A[q];
  }
}

VTK_ABI_NAMESPACE_END
}

#endif
