/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredGridGhostDataGenerator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridConnectivity.h"

#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkStructuredGridGhostDataGenerator);

//------------------------------------------------------------------------------
vtkStructuredGridGhostDataGenerator::vtkStructuredGridGhostDataGenerator()
{
  this->GridConnectivity = vtkStructuredGridConnectivity::New();
}

//------------------------------------------------------------------------------
vtkStructuredGridGhostDataGenerator::~vtkStructuredGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::RegisterGrids(vtkMultiBlockDataSet* in)
{
  assert("pre: Input multi-block is nullptr" && (in != nullptr));
  assert("pre: Grid connectivity should not be nullptr" && (this->GridConnectivity != nullptr));

  this->GridConnectivity->SetNumberOfGrids(in->GetNumberOfBlocks());
  this->GridConnectivity->SetNumberOfGhostLayers(0);
  this->GridConnectivity->SetWholeExtent(
    in->GetInformation()->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  for (unsigned int i = 0; i < in->GetNumberOfBlocks(); ++i)
  {
    vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(in->GetBlock(i));
    assert("pre: grid block is nullptr" && (grid != nullptr));

    vtkInformation* info = in->GetMetaData(i);
    assert("pre: nullptr meta-data" && (info != nullptr));
    assert("pre: No piece meta-data" && info->Has(vtkDataObject::PIECE_EXTENT()));

    this->GridConnectivity->RegisterGrid(static_cast<int>(i),
      info->Get(vtkDataObject::PIECE_EXTENT()), grid->GetPointGhostArray(),
      grid->GetCellGhostArray(), grid->GetPointData(), grid->GetCellData(), grid->GetPoints());
  } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::CreateGhostedDataSet(
  vtkMultiBlockDataSet* in, vtkMultiBlockDataSet* out)
{
  assert("pre: Input multi-block is nullptr" && (in != nullptr));
  assert("pre: Output multi-block is nullptr" && (out != nullptr));
  assert("pre: Grid connectivity should not be nullptr" && (this->GridConnectivity != nullptr));

  out->SetNumberOfBlocks(in->GetNumberOfBlocks());
  int wholeExt[6];
  in->GetInformation()->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  vtkInformation* outInfo = out->GetInformation();
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt, 6);

  int ghostedExtent[6];
  for (unsigned int i = 0; i < out->GetNumberOfBlocks(); ++i)
  {
    // STEP 0: Get the computed ghosted grid extent
    this->GridConnectivity->GetGhostedGridExtent(i, ghostedExtent);

    // STEP 1: Construct the ghosted structured grid instance
    vtkStructuredGrid* ghostedGrid = vtkStructuredGrid::New();
    assert("pre: Cannot create ghosted grid instance" && (ghostedGrid != nullptr));
    ghostedGrid->SetExtent(ghostedExtent);

    vtkPoints* ghostedGridPoints = vtkPoints::New();
    ghostedGridPoints->DeepCopy(this->GridConnectivity->GetGhostedPoints(i));
    ghostedGrid->SetPoints(ghostedGridPoints);
    ghostedGridPoints->Delete();

    // STEP 2: Copy the node/cell data
    ghostedGrid->GetPointData()->DeepCopy(this->GridConnectivity->GetGhostedGridPointData(i));
    ghostedGrid->GetCellData()->DeepCopy(this->GridConnectivity->GetGhostedGridCellData(i));

    out->SetBlock(i, ghostedGrid);
    ghostedGrid->Delete();
  } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::GenerateGhostLayers(
  vtkMultiBlockDataSet* in, vtkMultiBlockDataSet* out)
{
  assert("pre: Input multi-block is nullptr" && (in != nullptr));
  assert("pre: Output multi-block is nullptr" && (out != nullptr));
  assert("pre: Grid connectivity should not be nullptr" && (this->GridConnectivity != nullptr));

  // STEP 0: Register the input grids
  this->RegisterGrids(in);

  // STEP 1: Computes the neighbors
  this->GridConnectivity->ComputeNeighbors();

  // STEP 2: Generate the ghost layers
  this->GridConnectivity->CreateGhostLayers(this->NumberOfGhostLayers);

  // STEP 3: Get the output dataset
  this->CreateGhostedDataSet(in, out);
}
