/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelExtentIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPixelExtentIO.h"

#include "vtkUnstructuredGrid.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkCellData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkFloatArray.h"
#include "vtkDataSetWriter.h"

using std::deque;

// ----------------------------------------------------------------------------
vtkUnstructuredGrid &operator<<(
        vtkUnstructuredGrid &data,
        const vtkPixelExtent &ext)
{
  // initialize empty dataset
  if (data.GetNumberOfCells()<1)
    {
    vtkPoints *opts=vtkPoints::New();
    data.SetPoints(opts);
    opts->Delete();

    vtkCellArray *cells=vtkCellArray::New();
    vtkUnsignedCharArray *types=vtkUnsignedCharArray::New();
    vtkIdTypeArray *locs=vtkIdTypeArray::New();

    data.SetCells(types,locs,cells);

    cells->Delete();
    types->Delete();
    locs->Delete();
    }

  // cell to node
  vtkPixelExtent next(ext);
  next.CellToNode();

  // build the cell
  vtkFloatArray *pts=dynamic_cast<vtkFloatArray*>(data.GetPoints()->GetData());
  vtkIdType ptId=pts->GetNumberOfTuples();
  float *ppts=pts->WritePointer(3*ptId,12);

  int id[12]={
        0,2,-1,
        1,2,-1,
        1,3,-1,
        0,3,-1};

  vtkIdType ptIds[4];

  for (int i=0; i<4; ++i)
    {
    ppts[3*i+2]=0.0;
    for (int j=0; j<2; ++j)
      {
      int q=3*i+j;
      ppts[q]=next[id[q]];
      }
    ptIds[i]=ptId+i;
    }

  data.InsertNextCell(VTK_QUAD,4,ptIds);

  return data;
}

// ----------------------------------------------------------------------------
void vtkPixelExtentIO::Write(
      int commRank,
      const char *fileName,
      const deque<deque<vtkPixelExtent> >&exts)
{
  if (commRank!=0)
    {
    // only rank 0 writes
    return;
    }

  vtkUnstructuredGrid *data=vtkUnstructuredGrid::New();

  vtkIntArray *rank=vtkIntArray::New();
  rank->SetName("rank");
  data->GetCellData()->AddArray(rank);
  rank->Delete();

  vtkIntArray *block=vtkIntArray::New();
  block->SetName("block");
  data->GetCellData()->AddArray(block);
  block->Delete();

  size_t nRanks=exts.size();

  for (size_t i=0; i<nRanks; ++i)
    {
    size_t nBlocks = exts[i].size();
    for (size_t j=0; j<nBlocks; ++j)
      {
      const vtkPixelExtent &ext = exts[i][j];
      *data << ext;

      rank->InsertNextTuple1(i);
      block->InsertNextTuple1(j);
      }
    }

  vtkDataSetWriter *idw=vtkDataSetWriter::New();
  idw->SetFileName(fileName);
  idw->SetInputData(data);
  idw->Write();
  idw->Delete();

  data->Delete();
}

// ----------------------------------------------------------------------------
void vtkPixelExtentIO::Write(
      int commRank,
      const char *fileName,
      const deque<vtkPixelExtent> &exts)
{
  if (commRank!=0)
   {
   // only rank 0 will write
   return;
   }

  vtkUnstructuredGrid *data=vtkUnstructuredGrid::New();

  vtkIntArray *rank=vtkIntArray::New();
  rank->SetName("rank");
  data->GetCellData()->AddArray(rank);
  rank->Delete();

  int nExts=static_cast<int>(exts.size());
  rank->SetNumberOfTuples(nExts);

  int *pRank = rank->GetPointer(0);

  for (int i=0; i<nExts; ++i)
    {
    const vtkPixelExtent &ext = exts[i];
    *data << ext;
    pRank[i] = i;
    }

  vtkDataSetWriter *idw=vtkDataSetWriter::New();
  idw->SetFileName(fileName);
  idw->SetInputData(data);
  idw->Write();
  idw->Delete();

  data->Delete();
}

// ----------------------------------------------------------------------------
void vtkPixelExtentIO::Write(
      int commRank,
      const char *fileName,
      const vtkPixelExtent &ext)
{
  vtkUnstructuredGrid *data=vtkUnstructuredGrid::New();

  vtkIntArray *rank=vtkIntArray::New();
  rank->SetName("rank");
  data->GetCellData()->AddArray(rank);
  rank->Delete();

  rank->SetNumberOfTuples(1);
  int *pRank = rank->GetPointer(0);

  *data << ext;
  pRank[0] = commRank;

  vtkDataSetWriter *idw=vtkDataSetWriter::New();
  idw->SetFileName(fileName);
  idw->SetInputData(data);
  idw->Write();
  idw->Delete();

  data->Delete();
}
