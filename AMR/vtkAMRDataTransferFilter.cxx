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
  // TODO: implement this
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

  std::cout << "ndim: " << ndim[0] << " " << ndim[1] << " " << ndim[2] << "\n";

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
            myBox.WriteToVtkFile( oss.str().c_str() );
        } // END for all data
    } // END for all levels

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::WriteGrid( vtkUniformGrid* grid,
                                          std::string prefix )
{
  vtkAssertUtils::assertNotNull( grid, __FILE__, __LINE__ );

   std::ostringstream oss;
   oss.str( "" );
   oss << prefix << ".vtk";

   std::ofstream ofs;
   ofs.open( oss.str( ).c_str( ) );
   vtkAssertUtils::assertTrue( ofs.is_open(), __FILE__, __LINE__ );

   ofs << "# vtk DataFile Version 3.0\n";
   ofs <<  prefix << "\n";
   ofs << "ASCII\n";
   ofs << "DATASET STRUCTURED_GRID\n";

   double pnt[3];
   int ijk[3];
   int N[3];
   grid->GetDimensions( N );
   ofs << "DIMENSIONS " << N[0] << " " << N[1] << " " << N[2] << std::endl;
   ofs << "POINTS " << grid->GetNumberOfPoints( ) <<  " double\n";

   for( int k=0; k < N[2]; ++k )
     {
       for( int j=0; j < N[1]; ++j )
         {
           for( int i=0; i < N[0]; ++i )
             {
               ijk[0] = i; ijk[1] = j; ijk[2] = k;
               int pntIdx = vtkStructuredData::ComputePointId( N, ijk );
               grid->GetPoint( pntIdx, pnt );

               ofs << pnt[ 0 ] << " " << pnt[ 1 ] << " " << pnt[ 2 ] << "\n";
             } // END for all k
         } // END for all j
     } // END for all i

   vtkPointData *pntData = grid->GetPointData();
   if( (pntData != NULL) && (pntData->GetNumberOfArrays() > 0) )
     {
       ofs << "POINT_DATA " << grid->GetNumberOfPoints() << std::endl;
       for(int dataArray=0;dataArray < pntData->GetNumberOfArrays();++dataArray)
         {
           vtkDataArray *array = pntData->GetArray( dataArray );
           vtkAssertUtils::assertNotNull( array, __FILE__, __LINE__ );

           ofs << this->GetDataArrayVTKString( array );
         } // END for all data arrays

     } // if there are point data

   vtkCellData *cellData = grid->GetCellData( );
   if( (cellData != NULL) && ( cellData->GetNumberOfArrays() > 0) )
     {
       ofs << "CELL_DATA " << grid->GetNumberOfCells() << std::endl;
       for( int arrayidx=0; arrayidx<cellData->GetNumberOfArrays(); ++arrayidx)
         {
           vtkDataArray *array = cellData->GetArray( arrayidx );
           vtkAssertUtils::assertNotNull( array, __FILE__, __LINE__ );

           ofs << this->GetDataArrayVTKString( array );
         } // END for all arrays
     }
   ofs.close( );

}

//------------------------------------------------------------------------------
std::string vtkAMRDataTransferFilter::GetDataArrayVTKString(vtkDataArray *d )
{
  vtkAssertUtils::assertNotNull( d, __FILE__, __LINE__ );
  std::ostringstream oss;
  oss.str( "" );

  int i=0;
  switch( d->GetNumberOfComponents()  )
  {
    case 1:
      // Write as scalars
      oss << "SCALARS " << d->GetName() << " " << d->GetDataTypeAsString();
      oss << " 1\n";
      oss << "LOOKUP_TABLE default\n";
      for( i=0; i < d->GetNumberOfTuples(); ++i )
       oss << d->GetComponent(i,1) << std::endl;
      break;
    default:
      // Write as Vector
      // TODO: implement this
      break;
  }
  return( oss.str( ) );
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::WriteGrids( )
{
  vtkAssertUtils::assertNotNull( this->AMRDataSet,__FILE__,__LINE__);

  std::ostringstream oss;

  for( int l=0; l < this->AMRDataSet->GetNumberOfLevels(); ++l )
    {
      for( int idx=0; idx < this->AMRDataSet->GetNumberOfDataSets(l); ++idx )
        {
          vtkAMRBox myBox;
          vtkUniformGrid* myGrid=this->AMRDataSet->GetDataSet( l, idx, myBox );
          if( myGrid != NULL )
            {
              oss.str( "" );
              oss << "INITIAL_" << myBox.GetBlockId() << "_";
              oss << myBox.GetLevel();
              this->WriteGrid( myGrid, oss.str( ) );
            }
        } // END for all data
    } // END for all levels
}
