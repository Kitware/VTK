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

  // Loop through all levels but, skip level 0
  for( int currentLevel=1; currentLevel < numLevels; ++currentLevel )
    {
      int numDataSets = this->AMRDataSet->GetNumberOfDataSets( currentLevel );
      for( int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
        {

          vtkAMRBox myBox;
          vtkUniformGrid *myGrid =
           this->AMRDataSet->GetDataSet(currentLevel,dataIdx,myBox);

//          myBox.Grow( this->NumberOfGhostLayers );
////          myBox.ExtrudeGhostCells( this->NumberOfGhostLayers );
//          myGrid = (myGrid != NULL)? this->GetExtrudedGrid(myBox,myGrid) : NULL;
//
//          if( myGrid != NULL )
//            {
//              std::ostringstream oss;
//              oss << "EXTRUDED_GRID_" << myBox.GetBlockId( );
//              oss << "_" << myBox.GetLevel( );
//              this->WriteGrid( myGrid, oss.str( ).c_str( )  );
//            }

          this->ExtrudedData->SetDataSet(currentLevel,dataIdx,myBox,myGrid);

        } // END for all data at this level
    } // END for all levels

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
vtkUniformGrid* vtkAMRDataTransferFilter::GetExtrudedGrid(
    vtkUniformGrid* srcGrid )
{
  // Sanity check
  assert( "pre: SourceGrid != NULL" && (srcGrid != NULL) );
  vtkUniformGrid *extrudedGrid = vtkUniformGrid::New();

  int    ndim[3];
  double origin[3];
  double h[3];

// TODO: implement this
//  extrudedGrid->Initialize();
//  extrudedGrid->SetDimensions( ndim );
//  extrudedGrid->SetSpacing( h );
//  extrudedGrid->SetOrigin( origin );

  // Deep-Copy Data from source Grid to Extruded Grid
//  vtkPointData *pntData = srcGrid->GetPointData();
//  for( int idx=0; idx < pntData->GetNumberOfArrays(); ++idx )
//    {
//      vtkDataArray *a = pntData->GetArray( idx );
//      switch( a->GetDataType( ) )
//        {
//
//        } // END switch
//
//    } // END for all point arrays

//  vtkCellData *cellData = srcGrid->GetCellData();
//  for( int idx=0; idx < cellData->GetNumberOfArrays(); ++idx )
//    {
//      vtkDataArray *a = cellData->GetArray( idx );
//    } // END for all cell arrays
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

