/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRDataTransferFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRDataTransferFilter.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkAMRBox.h"
#include "vtkAMRInterBlockConnectivity.h"
#include "vtkMultiProcessController.h"
#include "vtkUniformGrid.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridWriter.h"
#include "vtkUnsignedIntArray.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"

#include <string>
#include <sstream>
#include <cassert>

//
// Standard Functions
//
vtkStandardNewMacro(vtkAMRDataTransferFilter);

vtkAMRDataTransferFilter::vtkAMRDataTransferFilter()
{
  this->Controller          = NULL;
  this->AMRDataSet          = NULL;
  this->RemoteConnectivity  = NULL;
  this->LocalConnectivity   = NULL;
  this->ExtrudedData        = NULL;
  this->NumberOfGhostLayers = 1;
}

//------------------------------------------------------------------------------
vtkAMRDataTransferFilter::~vtkAMRDataTransferFilter()
{
  this->ExtrudedData->Delete();
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  // Not implemented
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::Transfer( )
{
  // Sanity Checks
  assert("pre:ghost layers >= 1" && (this->NumberOfGhostLayers>=1));
  assert("pre:AMRDataSet != NULL" && (this->AMRDataSet != NULL));
  assert("pre:Controller != NULL" && (this->Controller != NULL));
  assert("pre:RemoteConnectivity != NULL" && (this->RemoteConnectivity!=NULL));
  assert("pre:LocalConnectivity != NULL" && (this->LocalConnectivity!=NULL));

  // STEP 0: Construct the extruded ghost data
  this->ExtrudeGhostLayers();
  assert("post: ExtrudedData != NULL" && (this->ExtrudedData != NULL ) );

  // STEP 1: Donor-Receiver search
  this->DonorSearch();

  // STEP 3: DataTransfer
  this->DataTransfer( );

  // STEP 4: Synch processes
  this->Controller->Barrier( );
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::ExtrudeGhostLayers( )
{
  // Sanity Checks
  assert("pre:AMRDataSet != NULL" && (this->AMRDataSet != NULL) );

  this->WriteData( this->AMRDataSet,"INITIAL");

  this->ExtrudedData = vtkHierarchicalBoxDataSet::New();
  int numLevels      = this->AMRDataSet->GetNumberOfLevels();

  for( int currentLevel=0; currentLevel < numLevels; ++currentLevel )
    {
      int numDataSets = this->AMRDataSet->GetNumberOfDataSets( currentLevel );
      for( int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
        {
          // Get metadata of the AMR grid
          vtkAMRBox myBox;
          int rc = this->AMRDataSet->GetMetaData(currentLevel,dataIdx, myBox);
          assert( "post: No metadata found!" && (rc==1) );

          // Get the AMR grid instance
          vtkUniformGrid *myGrid =
           this->AMRDataSet->GetDataSet(currentLevel,dataIdx);

          if( currentLevel == 0)
            {
              // Grids at level 0 are not extruded
              this->ExtrudedData->SetDataSet(currentLevel,dataIdx,myBox,myGrid);
            }
          else
            {

              std::ostringstream oss;
              oss.str(""); oss.clear();
              oss << "InitialGrid_" << currentLevel << "_" << dataIdx;
              this->WriteGrid( myGrid, oss.str() );

              vtkUniformGrid *extrudedGrid = this->GetExtrudedGrid( myGrid );
              assert( "post: extrudedGrid != NULL" && (extrudedGrid != NULL) );

              oss.str(""); oss.clear();
              oss << "ExtrudedGrid_" << currentLevel << "_" << dataIdx;
              this->WriteGrid( extrudedGrid, oss.str() );

              myBox.Grow(this->NumberOfGhostLayers);
              this->ExtrudedData->SetDataSet(
               currentLevel,dataIdx,myBox,extrudedGrid);
              extrudedGrid->Delete();
            }
        } // END for all data at this level

        this->ExtrudedData->SetRefinementRatio(
         currentLevel,this->AMRDataSet->GetRefinementRatio(currentLevel));

    } // END for all levels

  this->ExtrudedData->GenerateVisibilityArrays();
  this->WriteData( this->ExtrudedData, "EXTRUDED" );

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::DonorSearch()
{
  // Sanity checks
  assert("pre: ExtrudedData != NULL" && (this->ExtrudedData != NULL) );
  assert("pre: RemoteConnectivity != NULL" && (this->RemoteConnectivity!=NULL));
  assert("pre: LocalConnectivity != NULL" && (this->LocalConnectivity!=NULL));

  // TODO:
  // Call LocalDonorSearch()

  // TODO:
  // Call RemoteDonorSearch()
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::DataTransfer()
{
  // TODO: implement this
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::CopyPointData(
    vtkUniformGrid *src, vtkUniformGrid *t, int *re )
{
  // Sanity check
  assert( "pre: source grid is NULL" && (src != NULL) );
  assert( "pre: target grid is NULL" && (t != NULL) );
  assert( "pre: real extent is NULL" && (re != NULL) );
  assert( "pre: source node-data is NULL" && (src->GetPointData() != NULL) );

  if( src->GetPointData()->GetNumberOfArrays() == 0 )
    return;

  for( int array=0; array < src->GetPointData()->GetNumberOfArrays(); ++array )
    {
      // TODO: implement this
    } // END for all node arrays

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::CopyCellData(
    vtkUniformGrid *src, vtkUniformGrid *t, int *re )
{
  // Sanity check
  assert( "pre: source grid is NULL" && (src != NULL) );
  assert( "pre: target grid is NULL" && (t != NULL) );
  assert( "pre: real extent is NULL" && (re != NULL) );
  assert( "pre: source cell-data is NULL" && (src->GetCellData() != NULL) );

  if( src->GetCellData()->GetNumberOfArrays() == 0)
    return;

  for( int array=0; array < src->GetCellData()->GetNumberOfArrays(); ++array )
    {
      vtkDataArray *arrayPtr = src->GetCellData()->GetArray( array );
      assert( "post: arrayPtr != NULL" && (arrayPtr != NULL) );


      vtkDataArray *newArray =
       vtkDataArray::CreateDataArray( arrayPtr->GetDataType() );
      newArray->SetName( arrayPtr->GetName() );
      newArray->SetNumberOfComponents( arrayPtr->GetNumberOfComponents() );
      newArray->SetNumberOfTuples(t->GetNumberOfCells() );

      // Loop through the real extent and copy the cell data from the source
      // grid to the target grid
      int tijk[3]; /* target grid ijk */
      int sijk[3]; /* source grid ijk */
      for( sijk[0]=0,tijk[0]=re[0]; tijk[0]<=re[1]; ++tijk[0],++sijk[0] )
      {
        for( sijk[1]=0,tijk[1]=re[2]; tijk[1]<=re[3]; ++tijk[1],++sijk[1] )
          {
            for( sijk[2]=0,tijk[2]=re[4]; tijk[2]<=re[5]; ++tijk[2],++sijk[2] )
              {
                // Get the source cell index w.r.t. the source grid
                int sIdx=
                vtkStructuredData::ComputeCellId( src->GetDimensions(), sijk);
                assert( "post: source cell index out-of-bounds!" &&
                         (sIdx >= 0) && (sIdx < src->GetNumberOfCells() ) );

                // Get the target cell index w.r.t. the target grid
                int tIdx=
                vtkStructuredData::ComputeCellId( t->GetDimensions(), tijk );
                assert( "post: target cell index out-of-bounds!" &&
                        (tIdx >= 0) && (tIdx < t->GetNumberOfCells() ) );

                int component = 0;
                for( ;component<newArray->GetNumberOfComponents(); ++component)
                  {
                      newArray->SetComponent(
                       tIdx,component,arrayPtr->GetComponent(sIdx, component));
                  } // END for all array components

              } // END for all k
          } // END for all j
      } // END for all i


      t->GetCellData()->AddArray( newArray );
      newArray->Delete();

    } // END for all cell arrays


}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::AttachCellGhostInformation(
    vtkUniformGrid *extrudedGrid, int *realCellExtent)
{
  // Sanity Check
  assert( "pre: Input grid is NULL!" && (extrudedGrid != NULL) );
  assert( "pre: real extent array is NULL!" && (realCellExtent != NULL) );

  vtkIntArray *ghostArray = vtkIntArray::New();
  ghostArray->SetName( "GHOST" );
  ghostArray->SetNumberOfTuples( extrudedGrid->GetNumberOfCells() );
  ghostArray->SetNumberOfComponents( 1 );

  int celldims[3];
  extrudedGrid->GetDimensions(celldims);
  celldims[0]--; celldims[1]--; celldims[2]--;
  celldims[0] = (celldims[0] < 1)? 1 : celldims[0];
  celldims[1] = (celldims[1] < 1)? 1 : celldims[1];
  celldims[2] = (celldims[2] < 1)? 1 : celldims[2];

  for( int i=0; i < celldims[0]; ++i )
    {
      for( int j=0; j < celldims[1]; ++j )
        {
          for( int k=0; k < celldims[2]; ++k )
            {
              int ijk[3];
              ijk[0]=i; ijk[1]=j; ijk[2]=k;

              // Since celldims consists of the cell dimensions, ComputePointId
              // is sufficient to get the corresponding linear cell index!
              int cellIdx = vtkStructuredData::ComputePointId( celldims, ijk );

              assert(
               "Cell Index Out-of-range" &&
               (cellIdx >= 0) && (cellIdx < extrudedGrid->GetNumberOfCells()));

              if( (i >= realCellExtent[0]) && (i <= realCellExtent[1]) &&
                  (j >= realCellExtent[2]) && (j <= realCellExtent[3]) &&
                  (k >= realCellExtent[4]) && (k <= realCellExtent[5]) )
                  ghostArray->InsertValue( cellIdx, 1 );
              else
                  ghostArray->InsertValue( cellIdx, 0 );

            } // END for all k
        } // END for all j
    } // END for all i

  extrudedGrid->GetCellData()->AddArray( ghostArray );
  ghostArray->Delete();
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRDataTransferFilter::GetExtrudedGrid(
    vtkUniformGrid* srcGrid )
{
  // Sanity check
  assert( "pre: SourceGrid != NULL" && (srcGrid != NULL) );
  vtkUniformGrid *extrudedGrid = vtkUniformGrid::New();

  int    realCellExtent[6];
  int    ndim[3];
  double origin[3];
  double h[3];

  // STEP 0: Initialize
  for( int i=0; i < 6; ++i )
    {
      realCellExtent[i] = ndim[i%3] = 0;
      origin[i%3]       = h[i%3]    = 0.0;
    }

  // STEP 1: Constructed extruded grid
  srcGrid->GetDimensions( ndim );
  srcGrid->GetOrigin( origin );
  srcGrid->GetSpacing( h );

  for( int i=0; i < srcGrid->GetDataDimension(); ++i )
    {
      ndim[i]              += 2*this->NumberOfGhostLayers;
      origin[ i ]          -= h[i]*this->NumberOfGhostLayers;
      realCellExtent[i*2]   = this->NumberOfGhostLayers;
      realCellExtent[i*2+1] = ndim[i] - 2*this->NumberOfGhostLayers - 1;
    }
  extrudedGrid->Initialize();
  extrudedGrid->SetDimensions( ndim );
  extrudedGrid->SetSpacing( h );
  extrudedGrid->SetOrigin( origin );

  // STEP 2: Compute ghost cell information
  this->AttachCellGhostInformation( extrudedGrid, realCellExtent );

  // STEP 3: Copy PointData
  this->CopyPointData( srcGrid, extrudedGrid, realCellExtent );

  // STEP 4: Copy CellData
  this->CopyCellData( srcGrid, extrudedGrid, realCellExtent );

  return( extrudedGrid );
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::WriteData( vtkHierarchicalBoxDataSet* amrData,
                                          std::string prefix)
{
  assert( "pre: AMRData != NULL" && (amrData != NULL) );

  vtkXMLHierarchicalBoxDataWriter *myAMRWriter =
        vtkXMLHierarchicalBoxDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << myAMRWriter->GetDefaultFileExtension();

  myAMRWriter->SetFileName( oss.str().c_str() );
  myAMRWriter->SetInput( amrData );
  myAMRWriter->Write();
  myAMRWriter->Delete();
  if( this->Controller != NULL )
    this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::WriteGrid(
    vtkUniformGrid *grid, std::string prefix)
{
  // Sanity check
  assert( "pre: grid != NULL" && (grid != NULL) );

  // STEP 0: Convert the uniform grid to a structured grid
  vtkImageToStructuredGrid *image2sgrid = vtkImageToStructuredGrid::New();
  image2sgrid->SetInput( grid );
  image2sgrid->Update();
  vtkStructuredGrid *sgrid = image2sgrid->GetOutput();
  assert( "post: sgrid != NULL" && (sgrid != NULL)  );

  // STEP 1: Write structured grid
  std::ostringstream oss;
  oss.str( "" ); oss.clear();
  oss << prefix << ".vtk";

  vtkStructuredGridWriter *sgridWriter = vtkStructuredGridWriter::New();
  sgridWriter->SetFileName( oss.str().c_str() );
  sgridWriter->SetInput( sgrid );
  sgridWriter->Write();

  // STEP 2: CleanUp
  image2sgrid->Delete();
  sgridWriter->Delete();
}
