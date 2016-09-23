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

#include "vtkQuadratureSchemeDefinition.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkUnstructuredGrid.h"

namespace {


// Description:
// For all cells in the input "usg", for a specific array
// "V" interpolate to quadrature points using the given
// dictionary "dict" into "interpolated". Additionally if
// "indices" is not 0 then track the indices of where the
// values from each cell start as well. In the case of
// an error the return is 0.
template<class TV, class TI>
int Interpolate(
        vtkUnstructuredGrid *usg,
        const vtkIdType nCellsUsg,
        TV *pV,
        const int nCompsV,
        vtkQuadratureSchemeDefinition **dict,
        vtkDoubleArray *interpolated,
        TI *indices)
{
  // Walk cells.
  vtkIdType currentIndex=0;
  for (vtkIdType cellId=0; cellId<nCellsUsg; ++cellId)
  {
    // Point to the start of the data associated with this cell.
    if (indices!=NULL)
    {
      indices[cellId]=static_cast<TI>(currentIndex);
    }
    // Grab the cell's associated shape function definition.
    int cellType=usg->GetCellType(cellId);
    vtkQuadratureSchemeDefinition *def=dict[cellType];
    if (def==NULL)
    {
      // no quadrature scheme been specified for this cell type
      // skipping the cell.
      continue;
    }
    vtkIdType nNodes=def->GetNumberOfNodes();
    int nQPts=def->GetNumberOfQuadraturePoints();
    // Grab the cell's node ids.
    vtkIdType *cellNodeIds=0;
    usg->GetCellPoints(cellId,nNodes,cellNodeIds);
    // Walk quadrature points.
    for (int qPtId=0; qPtId<nQPts; ++qPtId)
    {
      // Grab the result and initialize.
      double *r=interpolated->WritePointer(currentIndex,nCompsV);
      for (int q=0; q<nCompsV; ++q)
      {
        r[q]=0.0;
      }
      // Grab shape function weights.
      const double *N=def->GetShapeFunctionWeights(qPtId);
      // Walk the cell's nodes.
      for (int j=0; j<nNodes; ++j)
      {
        vtkIdType tupIdx=cellNodeIds[j]*nCompsV;
        // Apply shape function weights.
        for (int q=0; q<nCompsV; ++q)
        {
          r[q]+=N[j]*pV[tupIdx+q];
        }
      }
      // Update the result index.
      currentIndex+=nCompsV;
    }
  }
  return 1;
}

// Description:
// Dispatch helper, descides what type of indices we are working with
template<class TV>
int Interpolate(
        vtkUnstructuredGrid *usg,
        const vtkIdType nCellsUsg,
        TV *pV,
        const int nCompsV,
        vtkQuadratureSchemeDefinition **dict,
        vtkDoubleArray *interpolated,
        void *indices,
        int indexType)
{
  switch(indexType)
  {
    vtkTemplateMacro(
      return Interpolate(usg,nCellsUsg,pV,nCompsV,dict,interpolated,(VTK_TT*)indices);
      );
  }
  return 0;
}

//------------------------------------------------------------------------------
template <class T>
void ApplyShapeFunction(double *r,double N_j,T *A,int nComps)
{
  for (int q=0; q<nComps; ++q)
  {
    r[q]+=N_j*A[q];
  }
}

};

#endif
