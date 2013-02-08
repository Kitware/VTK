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
#include "vtkObjectFactory.h"
#include "vtkIndent.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkDoubleArray.h"

#include <cassert>

vtkStandardNewMacro( vtkRectilinearGridPartitioner );

//------------------------------------------------------------------------------
vtkRectilinearGridPartitioner::vtkRectilinearGridPartitioner()
{
  this->NumberOfPartitions  = 2;
  this->NumberOfGhostLayers = 0;
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

  int dims[3];
  vtkStructuredData::GetDimensionsFromExtent(subext,dims,dataDescription );

  int i,j,k;
  int lidx;    // local linear index
  double p[3];
  switch( dataDescription )
    {
    case VTK_XY_PLANE:
      // Sanity checks
      assert("pre: dimension should be greater than 0" && (dims[0] > 0) );
      assert("pre: dimension should be greater than 0" && (dims[1] > 0) );

      // Allocate coordinate arrays
      xcoords->SetNumberOfComponents(1);
      xcoords->SetNumberOfTuples(dims[0]);
      ycoords->SetNumberOfComponents(1);
      ycoords->SetNumberOfTuples(dims[1]);

      zcoords->SetNumberOfComponents(1);
      zcoords->SetNumberOfTuples(0);

      // Copy coordinates from the whole grid
      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; i <= subext[1]; ++i )
        {
        lidx = i-subext[0];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[0]) );
        grd->GetPoint( i,j,k,p );
        xcoords->SetComponent(lidx,0, p[0] );
        } // END for all i

      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; j <= subext[3]; ++j )
        {
        lidx = j-subext[2];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[1]) );
        grd->GetPoint( i,j,k,p );
        ycoords->SetComponent(lidx,0,p[1]);
        } // END for all j
      break;
    case VTK_XZ_PLANE:
      // Sanity checks
      assert("pre: dimension should be greater than 0" && (dims[0] > 0) );
      assert("pre: dimension should be greater than 0" && (dims[2] > 0) );

      // Allocate coordinate arrays
      xcoords->SetNumberOfComponents(1);
      xcoords->SetNumberOfTuples(dims[0]);
      zcoords->SetNumberOfComponents(1);
      zcoords->SetNumberOfTuples(dims[2]);

      ycoords->SetNumberOfComponents(1);
      ycoords->SetNumberOfTuples(0);

      // Copy coordinates from the whole grid
      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; i <= subext[1]; ++i )
        {
        lidx = i-subext[0];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[0]) );
        grd->GetPoint( i,j,k,p );
        xcoords->SetComponent(lidx,0, p[0] );
        } // END for all i

      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; k <= subext[5]; ++k )
        {
        lidx = k-subext[4];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[2]) );
        grd->GetPoint( i,j,k,p );
        zcoords->SetComponent( lidx,0,p[2] );
        } // END for all k
      break;
    case VTK_YZ_PLANE:
      // Sanity checks
      assert("pre: dimension should be greater than 0" && (dims[1] > 0) );
      assert("pre: dimension should be greater than 0" && (dims[2] > 0) );

      // Allocate coordinate arrays
      ycoords->SetNumberOfComponents(1);
      ycoords->SetNumberOfTuples(dims[1]);
      zcoords->SetNumberOfComponents(1);
      zcoords->SetNumberOfTuples(dims[2]);

      xcoords->SetNumberOfComponents(1);
      xcoords->SetNumberOfTuples(dims[0]);

      // Copy coordinates from the whole grid
      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; j <= subext[3]; ++j )
        {
        lidx = j-subext[2];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[1]) );
        grd->GetPoint( i,j,k,p );
        ycoords->SetComponent(lidx,0,p[1]);
        } // END for all j

      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; k <= subext[5]; ++k )
        {
        lidx = k-subext[4];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[2]) );
        grd->GetPoint( i,j,k,p );
        zcoords->SetComponent( lidx,0,p[2] );
        } // END for all k
      break;
    case VTK_XYZ_GRID:
      // Sanity checks
      assert("pre: dimension should be greater than 0" && (dims[0] > 0) );
      assert("pre: dimension should be greater than 0" && (dims[1] > 0) );
      assert("pre: dimension should be greater than 0" && (dims[2] > 0) );

      // Allocate coordinate arrays
      xcoords->SetNumberOfComponents(1);
      xcoords->SetNumberOfTuples( dims[0] );
      ycoords->SetNumberOfComponents(1);
      ycoords->SetNumberOfTuples( dims[1] );

      zcoords->SetNumberOfComponents(1);
      zcoords->SetNumberOfTuples( dims[2] );

      // Copy coordinates from the whole grid
      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; i <= subext[1]; ++i )
        {
        lidx = i-subext[0];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[0]) );
        grd->GetPoint( i,j,k,p );
        xcoords->SetComponent(lidx,0, p[0] );
        } // END for all i

      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; j <= subext[3]; ++j )
        {
        lidx = j-subext[2];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[1]) );
        grd->GetPoint( i,j,k,p );
        ycoords->SetComponent(lidx,0,p[1]);
        } // END for all j

      i=subext[0]; k=subext[4]; j=subext[2];
      for( ; k <= subext[5]; ++k )
        {
        lidx = k-subext[4];
        assert("pre: local linear index is out-of-bounds!" &&
               (lidx >= 0) && (lidx < dims[2]) );
        grd->GetPoint( i,j,k,p );
        zcoords->SetComponent( lidx,0,p[2] );
        } // END for all k
      break;
    default:
      vtkErrorMacro("Cannot handle structured data!");
    }

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

  // STEP 4: Partition
  extentPartitioner->Partition();

  // STEP 5: Extract partition in a multi-block
  multiblock->SetNumberOfBlocks( extentPartitioner->GetNumExtents( ) );

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

    subgrid->SetXCoordinates( xcoords );
    subgrid->SetYCoordinates( ycoords );
    subgrid->SetZCoordinates( zcoords );
    xcoords->Delete();
    ycoords->Delete();
    zcoords->Delete();
    } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
