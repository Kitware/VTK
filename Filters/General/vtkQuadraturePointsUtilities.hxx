/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraturePointsUtilities.hxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
    std::vector<vtkQuadratureSchemeDefinition*>& dict, vtkDoubleArray* interpolated)
  {
    this->operator()(valueArray, static_cast<vtkAOSDataArrayTemplate<vtkIdType>*>(nullptr), usg,
      nCellsUsg, dict, interpolated);
  }

  // Version with offsets:
  template <typename ValueArrayT, typename IndexArrayT>
  void operator()(ValueArrayT* valueArray, IndexArrayT* indexArray, vtkUnstructuredGrid* usg,
    const vtkIdType nCellsUsg, std::vector<vtkQuadratureSchemeDefinition*>& dict,
    vtkDoubleArray* interpolated)
  {
    using IndexType = vtk::GetAPIType<IndexArrayT>;
    const vtk::ComponentIdType nCompsV = valueArray->GetNumberOfComponents();
    const auto valueTuples = vtk::DataArrayTupleRange(valueArray);

    // Walk cells.
    vtkIdType currentIndex = 0;
    for (vtkIdType cellId = 0; cellId < nCellsUsg; ++cellId)
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

};

#endif
