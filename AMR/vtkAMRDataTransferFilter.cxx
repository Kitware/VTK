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
#include "vtkAssertUtils.hpp"
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

#include <string>
#include <sstream>

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
  vtkAssertUtils::assertTrue( (this->NumberOfGhostLayers>=1),__FILE__,__LINE__);
  vtkAssertUtils::assertNotNull(this->AMRDataSet,__FILE__,__LINE__);
  vtkAssertUtils::assertNotNull(this->Controller,__FILE__,__LINE__);
  vtkAssertUtils::assertNotNull(this->RemoteConnectivity,__FILE__,__LINE__);
  vtkAssertUtils::assertNotNull(this->LocalConnectivity,__FILE__,__LINE__);

  // STEP 0: Construct the extruded ghost data
  this->ExtrudeGhostLayers();
  vtkAssertUtils::assertNotNull(this->ExtrudedData,__FILE__,__LINE__);

  // STEP 1: Donor-Receiver search
  this->DonorSearch();
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::ExtrudeGhostLayers( )
{
  vtkAssertUtils::assertNotNull(this->AMRDataSet,__FILE__,__LINE__);

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

          myBox.ExtrudeGhostCells( this->NumberOfGhostLayers );
          myGrid = (myGrid != NULL)? this->GetExtrudedGrid(myBox,myGrid) : NULL;

          if( myGrid != NULL )
            {
              std::ostringstream oss;
              oss << "EXTRUDED_GRID_" << myBox.GetBlockId( );
              oss << "_" << myBox.GetLevel( );
              this->WriteGrid( myGrid, oss.str( ).c_str( )  );
            }

          this->ExtrudedData->SetDataSet(currentLevel,dataIdx,myBox,myGrid);

        } // END for all data at this level
    } // END for all levels

  this->WriteData( this->ExtrudedData, "EXTRUDED" );

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::DonorSearch()
{
  vtkAssertUtils::assertNotNull(this->ExtrudedData,__FILE__,__LINE__);

  // TODO: implement this
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::DataTransfer()
{
  // TODO: implement this
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRDataTransferFilter::GetExtrudedGrid( vtkAMRBox &ebox,
                                                      vtkUniformGrid* srcGrid )
{
  vtkAssertUtils::assertNotNull( srcGrid,__FILE__,__LINE__ );

  vtkUniformGrid *extrudedGrid = vtkUniformGrid::New();

  int    ndim[3];
  double origin[3];
  double h[3];

  ebox.GetNumberOfNodes( ndim );
  ebox.GetGridSpacing( h );
  ebox.GetBoxOrigin( origin );

  extrudedGrid->Initialize();
  extrudedGrid->SetDimensions( ndim );
  extrudedGrid->SetSpacing( h );
  extrudedGrid->SetOrigin( origin );

  return( extrudedGrid );
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::WriteData( vtkHierarchicalBoxDataSet* amrData,
                                          std::string prefix)
{
  vtkAssertUtils::assertNotNull(amrData,__FILE__,__LINE__);

  std::ostringstream oss;
  int numLevels = amrData->GetNumberOfLevels();
  for( int level=0; level < numLevels; ++level )
    {
      int numData = amrData->GetNumberOfDataSets( level );
      for( int data=0; data < numData; ++data )
        {
          oss.str( "" );
          oss << prefix << "_BLOCK_" << data << "_LEVEL_" << level << ".vtk";

          vtkAMRBox myBox;
          vtkUniformGrid *myGrid = amrData->GetDataSet(level,data,myBox);

          if( myGrid != NULL )
            this->WriteGrid( myGrid, oss.str()  );
        } // END for all data
    } // END for all levels

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::WriteGrid( vtkUniformGrid* grid,
                                          std::string prefix )
{
  vtkAssertUtils::assertNotNull( grid, __FILE__, __LINE__ );

  // STEP 0: Setup the file name to use
  std::string fileName = prefix + ".vtk";

  // STEP 1: Convert to a structured grid
  vtkImageToStructuredGrid *img2sgrid = vtkImageToStructuredGrid::New( );
  vtkAssertUtils::assertNotNull( img2sgrid,__FILE__,__LINE__);

  img2sgrid->SetInput( grid );
  img2sgrid->Update();
  vtkStructuredGrid *myGrid = img2sgrid->GetOutput();
  vtkAssertUtils::assertNotNull(myGrid,__FILE__,__LINE__);

  // STEP 2: Use structured grid writer to write the grid object
  vtkStructuredGridWriter *myWriter = vtkStructuredGridWriter::New();
  vtkAssertUtils::assertNotNull( myWriter,__FILE__,__LINE__);

  std::cout << "Writing grid @" << fileName << std::endl;
  std::cout.flush();

  myWriter->SetInput(0,myGrid);
  myWriter->SetFileName( fileName.c_str( ) );
  myWriter->Update();
  myWriter->Delete();

  img2sgrid->Delete();
}
