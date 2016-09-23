/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkRectilinearGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkRectilinearGridPartitioner.h"

// VTK includes
#include "vtkDoubleArray.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"

#include <cassert>

vtkStandardNewMacro( vtkRectilinearGridPartitioner );

//------------------------------------------------------------------------------
vtkRectilinearGridPartitioner::vtkRectilinearGridPartitioner()
{
  this->NumberOfPartitions  = 2;
  this->NumberOfGhostLayers = 0;
  this->DuplicateNodes = 1;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkRectilinearGridPartitioner::~vtkRectilinearGridPartitioner()
{

}

//------------------------------------------------------------------------------
void vtkRectilinearGridPartitioner::PrintSelf(
    std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
  oss << "NumberOfPartitions: " << this->NumberOfPartitions << std::endl;
  oss << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}

//------------------------------------------------------------------------------
int vtkRectilinearGridPartitioner::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkRectilinearGridPartitioner::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkRectilinearGridPartitioner::ExtractGridCoordinates(
    vtkRectilinearGrid *grd, int subext[6],
    vtkDoubleArray *xcoords, vtkDoubleArray *ycoords, vtkDoubleArray *zcoords )
{
  assert("pre: NULL rectilinear grid" && (grd != NULL) );
  assert("pre: NULL xcoords" && (xcoords != NULL) );
  assert("pre: NULL ycoords" && (ycoords != NULL) );
  assert("pre: NULL zcoords" && (zcoords != NULL) );

  int dataDescription = vtkStructuredData::GetDataDescriptionFromExtent(subext);

  int ndims[3];
  vtkStructuredData::GetDimensionsFromExtent(subext,ndims,dataDescription );

  vtkDoubleArray* coords[3];
  coords[0] = xcoords;
  coords[1] = ycoords;
  coords[2] = zcoords;

  vtkDataArray* src_coords[3];
  src_coords[0] = grd->GetXCoordinates();
  src_coords[1] = grd->GetYCoordinates();
  src_coords[2] = grd->GetZCoordinates();

  for(int dim=0; dim < 3; ++dim)
  {
    coords[ dim ]->SetNumberOfComponents( 1 );
    coords[ dim ]->SetNumberOfTuples( ndims[ dim ] );

    for(int idx=subext[dim*2]; idx <= subext[dim*2+1]; ++idx)
    {
      vtkIdType lidx = idx-subext[dim*2];
      coords[ dim ]->SetTuple1(lidx,src_coords[dim]->GetTuple1(idx));
    } // END for all ids

  } // END for all dimensions
}

//------------------------------------------------------------------------------
int vtkRectilinearGridPartitioner::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,vtkInformationVector* outputVector )
{
  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert("pre: input information object is NULL" && (input != NULL) );
  vtkRectilinearGrid *grd =
     vtkRectilinearGrid::SafeDownCast(input->Get(vtkDataObject::DATA_OBJECT()));

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert("pre: output information object is NULL" && (output != NULL) );
  vtkMultiBlockDataSet *multiblock =
      vtkMultiBlockDataSet::SafeDownCast(
          output->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: multi-block grid is NULL" && (multiblock != NULL) );

  // STEP 2: Get the global extent
  int extent[6];
  grd->GetExtent( extent );

  // STEP 3: Setup extent partitioner
  vtkExtentRCBPartitioner *extentPartitioner = vtkExtentRCBPartitioner::New();
  assert("pre: extent partitioner is NULL" && (extentPartitioner != NULL) );
  extentPartitioner->SetGlobalExtent( extent );
  extentPartitioner->SetNumberOfPartitions( this->NumberOfPartitions );
  extentPartitioner->SetNumberOfGhostLayers( this->NumberOfGhostLayers );
  if( this->DuplicateNodes == 1 )
  {
    extentPartitioner->DuplicateNodesOn();
  }
  else
  {
    extentPartitioner->DuplicateNodesOff();
  }

  // STEP 4: Partition
  extentPartitioner->Partition();

  // STEP 5: Extract partition in a multi-block
  multiblock->SetNumberOfBlocks( extentPartitioner->GetNumExtents( ) );

  // Set the whole extent of the grid
  multiblock->GetInformation()->Set(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);

  int subext[6];
  unsigned int blockIdx = 0;
  for( ; blockIdx < multiblock->GetNumberOfBlocks(); ++blockIdx )
  {
    extentPartitioner->GetPartitionExtent( blockIdx, subext );
    vtkRectilinearGrid *subgrid = vtkRectilinearGrid::New();
    subgrid->SetExtent( subext );

    vtkDoubleArray *xcoords = vtkDoubleArray::New();
    vtkDoubleArray *ycoords = vtkDoubleArray::New();
    vtkDoubleArray *zcoords = vtkDoubleArray::New();

    this->ExtractGridCoordinates(grd,subext,xcoords,ycoords,zcoords);

    subgrid->SetXCoordinates( xcoords );
    subgrid->SetYCoordinates( ycoords );
    subgrid->SetZCoordinates( zcoords );
    xcoords->Delete();
    ycoords->Delete();
    zcoords->Delete();

    vtkInformation *metadata = multiblock->GetMetaData( blockIdx );
    assert( "pre: metadata is NULL" && (metadata != NULL) );
    metadata->Set( vtkDataObject::PIECE_EXTENT(), subext, 6);

    multiblock->SetBlock(blockIdx, subgrid);
    subgrid->Delete();
  } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
