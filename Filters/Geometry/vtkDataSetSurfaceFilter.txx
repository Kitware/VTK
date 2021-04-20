/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetSurfaceFilter.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

//------------------------------------------------------------------------------
template <class GridDataSetT>
int vtkDataSetSurfaceFilter::StructuredWithBlankingExecuteImpl(
  GridDataSetT* input, vtkPolyData* output)
{
  vtkIdType newCellId;
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkCell* face;
  double x[3];
  vtkIdList* cellIds;
  vtkIdList* pts;
  vtkPoints* newPts;
  vtkIdType ptId, pt;
  int npts;
  vtkPointData* pd = input->GetPointData();
  vtkCellData* cd = input->GetCellData();
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  if (numCells == 0)
  {
    vtkDebugMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(numCells);
    outputCD->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = vtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPts);
    outputPD->AddArray(this->OriginalPointIds);
  }

  cellIds = vtkIdList::New();
  pts = vtkIdList::New();

  vtkDebugMacro(<< "Executing geometry filter");

  // Allocate
  //
  newPts = vtkPoints::New();
  // we don't know what type of data the input points are so
  // we keep the output points to have the default type (float)
  newPts->Allocate(numPts, numPts / 2);
  output->AllocateEstimate(numCells, 3);
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd, numPts, numPts / 2);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  // Traverse cells to extract geometry
  //
  int abort = 0;
  int dims[3];
  input->GetCellDims(dims);
  vtkIdType d01 = static_cast<vtkIdType>(dims[0]) * dims[1];
  for (int k = 0; k < dims[2] && !abort; ++k)
  {
    vtkDebugMacro(<< "Process cell #" << d01 * k);
    this->UpdateProgress(k / dims[2]);
    abort = this->GetAbortExecute();
    for (int j = 0; j < dims[1]; ++j)
    {
      for (int i = 0; i < dims[0]; ++i)
      {
        vtkIdType cellId = d01 * k + dims[0] * j + i;
        if (!input->IsCellVisible(cellId))
        {
          continue;
        }
        vtkCell* cell = input->GetCell(i, j, k);
        switch (cell->GetCellDimension())
        {
          // create new points and then cell
          case 0:
          case 1:
          case 2:
            npts = cell->GetNumberOfPoints();
            pts->Reset();
            for (int l = 0; l < npts; ++l)
            {
              ptId = cell->GetPointId(l);
              input->GetPoint(ptId, x);
              pt = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd, ptId, pt);
              this->RecordOrigPointId(pt, ptId);
              pts->InsertId(l, pt);
            }
            newCellId = output->InsertNextCell(cell->GetCellType(), pts);
            outputCD->CopyData(cd, cellId, newCellId);
            this->RecordOrigCellId(newCellId, cellId);
            break;
          case 3:
            int even[3] = { i, j, k };
            int odd[3] = { i + 1, j + 1, k + 1 };
            for (int m = 0; m < cell->GetNumberOfFaces(); ++m)
            {
              face = cell->GetFace(m);
              if (m % 2)
              {
                input->GetCellNeighbors(cellId, face->PointIds, cellIds, odd);
              }
              else
              {
                input->GetCellNeighbors(cellId, face->PointIds, cellIds, even);
              }
              // faces with only blank neighbors count as external faces
              bool noNeighbors = cellIds->GetNumberOfIds() <= 0;
              for (vtkIdType ci = 0; ci < cellIds->GetNumberOfIds(); ci++)
              {
                if (input->IsCellVisible(cellIds->GetId(ci)))
                {
                  noNeighbors = false;
                  break;
                }
              }
              if (noNeighbors)
              {
                npts = face->GetNumberOfPoints();
                pts->Reset();
                for (int n = 0; n < npts; ++n)
                {
                  ptId = face->GetPointId(n);
                  input->GetPoint(ptId, x);
                  pt = newPts->InsertNextPoint(x);
                  outputPD->CopyData(pd, ptId, pt);
                  this->RecordOrigPointId(pt, ptId);
                  pts->InsertId(n, pt);
                }
                newCellId = output->InsertNextCell(face->GetCellType(), pts);
                outputCD->CopyData(cd, cellId, newCellId);
                this->RecordOrigCellId(newCellId, cellId);
              }
            }
            break;
        } // switch
      }
    }
  } // for all cells

  vtkDebugMacro(<< "Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  if (this->OriginalCellIds)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  // free storage
  output->Squeeze();

  cellIds->Delete();
  pts->Delete();

  return 1;
}

//------------------------------------------------------------------------------
template <class GridDataSetT>
int vtkDataSetSurfaceFilter::StructuredDataSetExecute(
  vtkDataSet* input, vtkPolyData* output, vtkIdType* wholeExt)
{
  GridDataSetT* grid = GridDataSetT::SafeDownCast(input);
  vtkIdType ext[6];
  if (grid->HasAnyBlankCells())
  {
    return this->StructuredWithBlankingExecute(grid, output);
  }
  else
  {
    int* tmpext = grid->GetExtent();
    std::copy(tmpext, tmpext + 6, ext);
    return this->StructuredExecute(grid, output, ext, wholeExt);
  }
}
