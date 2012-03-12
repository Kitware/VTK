/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPStructuredGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPStructuredGridGhostDataGenerator.h"
#include "vtkObjectFactory.h"
#include "vtkPStructuredGridConnectivity.h"
#include "vtkStructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>

vtkStandardNewMacro(vtkPStructuredGridGhostDataGenerator);

//------------------------------------------------------------------------------
vtkPStructuredGridGhostDataGenerator::vtkPStructuredGridGhostDataGenerator()
{
  this->GridConnectivity = vtkPStructuredGridConnectivity::New();
}

//------------------------------------------------------------------------------
vtkPStructuredGridGhostDataGenerator::~vtkPStructuredGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridGhostDataGenerator::PrintSelf(
    ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridGhostDataGenerator::RegisterGrids(
    vtkMultiBlockDataSet *in)
{
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: grid connectivity is NULL" && (this->GridConnectivity != NULL) );

  this->GridConnectivity->SetController( this->Controller );
  this->GridConnectivity->SetNumberOfGrids( in->GetNumberOfBlocks() );
  this->GridConnectivity->SetNumberOfGhostLayers(0);
  this->GridConnectivity->SetWholeExtent(
      in->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  this->GridConnectivity->Initialize();

  for( unsigned int i=0; i < in->GetNumberOfBlocks(); ++i )
    {
    vtkStructuredGrid *grid = vtkStructuredGrid::SafeDownCast(in->GetBlock(i));
    if( grid != NULL )
      {
      vtkInformation *info = in->GetMetaData( i );
      assert("pre: NULL meta-data" && (info != NULL) );
      assert("pre: No piece meta-data" &&
              info->Has(vtkDataObject::PIECE_EXTENT()));

      this->GridConnectivity->RegisterGrid(
          static_cast<int>(i),info->Get(vtkDataObject::PIECE_EXTENT() ),
          grid->GetPointVisibilityArray(),
          grid->GetCellVisibilityArray(),
          grid->GetPointData(),
          grid->GetCellData(),
          grid->GetPoints() );
      } // END if the grid is not NULL
    } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkPStructuredGridGhostDataGenerator::CreateGhostedDataSet(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out )
{
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: output multi-block is NULL" && (out != NULL) );

  out->SetNumberOfBlocks( in->GetNumberOfBlocks() );
  int wholeExt[6];
  in->GetInformation()->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt);
  out->GetInformation()->Set(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt,6);

  int ghostedExtent[6];
  for( unsigned int i=0; i < out->GetNumberOfBlocks(); ++i )
    {
    if( in->GetBlock(i) != NULL )
      {
      // STEP 0: Get the computed ghosted grid extent
      this->GridConnectivity->GetGhostedGridExtent( i, ghostedExtent );

      // STEP 1: Construct the ghosted structured grid instance
      vtkStructuredGrid *ghostedGrid = vtkStructuredGrid::New();
      assert("pre: Cannot create ghosted grid instance" && (ghostedGrid != NULL));
      ghostedGrid->SetExtent( ghostedExtent );

      vtkPoints *ghostedGridPoints = vtkPoints::New();
      ghostedGridPoints->DeepCopy(this->GridConnectivity->GetGhostedPoints(i));
      ghostedGrid->SetPoints(ghostedGridPoints);
      ghostedGridPoints->Delete();

      // STEP 2: Copy the node/cell data
      ghostedGrid->GetPointData()->DeepCopy(
         this->GridConnectivity->GetGhostedGridPointData(i) );
      ghostedGrid->GetCellData()->DeepCopy(
         this->GridConnectivity->GetGhostedGridCellData(i) );

      // STEP 3: Copy the ghost arrays
      ghostedGrid->SetPointVisibilityArray(
          this->GridConnectivity->GetGhostedPointGhostArray(i) );
      ghostedGrid->SetPointVisibilityArray(
         this->GridConnectivity->GetGhostedCellGhostArray(i) );

      out->SetBlock(i,ghostedGrid);
      ghostedGrid->Delete();
      }
    else
      {
      out->SetBlock( i, NULL );
      }
    } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkPStructuredGridGhostDataGenerator::GenerateGhostLayers(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out )
{
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: output multi-block is NULL" && (out != NULL) );
  assert("pre: grid connectivity is NULL" && (this->GridConnectivity != NULL) );
  assert("pre: controller should not be NULL" && (this->Controller != NULL) );

  // STEP 0: Register grids
  this->RegisterGrids( in );
  this->Barrier();

  // STEP 1: Compute neighboring topology
  this->GridConnectivity->ComputeNeighbors();

  // STEP 2: Create ghost layers
  this->GridConnectivity->CreateGhostLayers( this->NumberOfGhostLayers );

  // STEP 3: Create the ghosted data-set
  this->CreateGhostedDataSet(in,out);
  this->Barrier();
}
