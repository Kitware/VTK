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
#include "vtkDoubleArray.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridWriter.h"
#include "vtkUnsignedIntArray.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"
#include "vtkAMRGridIndexEncoder.h"
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkAMRLink.h"

#include <string>
#include <sstream>
#include <cassert>
#include <vtkstd/map>
#include <algorithm>

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

  vtkstd::map<unsigned int,vtkPolyData*>::iterator it;
  for( it=this->ReceiverList.begin(); it != this->ReceiverList.end(); ++it )
      it->second->Delete();
  this->ReceiverList.erase(this->ReceiverList.begin(),this->ReceiverList.end());
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
bool vtkAMRDataTransferFilter::IsGhostCell( vtkUniformGrid *ug, int cellIdx )
{
  assert( "pre: input grid is NULL" && (ug != NULL) );
  assert( "pre: cell index out-of-bounds" &&
          ( (cellIdx >= 0) & (cellIdx < ug->GetNumberOfCells() ) )  );

  vtkCellData *CD = ug->GetCellData();
  assert( "pre: cell data is NULL!" && (CD != NULL) );

  if( CD->HasArray("GHOST") )
    {
      vtkIntArray *ghostArray =
       vtkIntArray::SafeDownCast( CD->GetArray( "GHOST" ) );
      assert( "pre: ghost array is NULL!" && (ghostArray != NULL) );

      if( ghostArray->GetValue( cellIdx ) == 0 )
        return true;
    }

  // Note: If the grid does not have any GHOST information, then
  // the cell is assumed to be a real cell of the grid.
  return false;
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::ComputeCellCenter(
             vtkUniformGrid *ug, int cellIdx, double center[3] )
{
  // Sanity checsk
  assert( "input grid is NULL" && (ug != NULL)  );
  assert( "center array is NULL" && (center != NULL) );

  vtkCell *myCell = ug->GetCell( cellIdx );
  assert( "post: cell is NULL" && (myCell != NULL) );

  double pCenter[3];
  double *weights = new double[ myCell->GetNumberOfPoints() ];
  int subId       = myCell->GetParametricCenter( pCenter );
  myCell->EvaluateLocation( subId,pCenter,center,weights );
  delete [] weights;
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::WriteReceivers( )
{
  std::map< unsigned int, vtkPolyData* >::iterator it=this->ReceiverList.begin();
  std::ostringstream oss;

  for( ; it != this->ReceiverList.end(); ++it )
    {
      int level = -1;
      int idx   = -1;
      vtkAMRGridIndexEncoder::decode( it->first,level,idx );

      oss.str( "" ); oss.clear();
      oss << "Receivers_" << level << "_" << idx << ".vtk";

      vtkPolyData *myData = it->second;
      if( myData != NULL )
        {
          vtkPolyDataWriter *myWriter = vtkPolyDataWriter::New();
          myWriter->SetFileName( oss.str().c_str() );
          myWriter->SetInput( it->second );
          myWriter->Write();
          myWriter->Delete();
        }

    }
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::AddReceiverInformation( vtkPolyData *receivers )
{
  assert( "pre: receiver data is NULL" && (receivers != NULL) );

  // Add array that will store the donor grid information
  vtkUnsignedIntArray *donorGrid = vtkUnsignedIntArray::New();
  donorGrid->SetName( "DonorGridIdx" );
  donorGrid->SetNumberOfComponents( 1 );
  donorGrid->SetNumberOfTuples( receivers->GetNumberOfPoints() );
  receivers->GetPointData()->AddArray( donorGrid );
  donorGrid->Delete();

  // Add array to hold the donor cell information
  vtkIntArray *donorCell = vtkIntArray::New();
  donorCell->SetName( "DonorCellIdx" );
  donorCell->SetNumberOfComponents( 1 ) ;
  donorCell->SetNumberOfTuples( receivers->GetNumberOfPoints() );
  donorCell->FillComponent(0,-1);
  receivers->GetPointData()->AddArray( donorCell );
  donorCell->Delete();

  // Add array to hold the donor level infomration
  vtkIntArray *donorLevel = vtkIntArray::New();
  donorLevel->SetName( "DonorLevel" );
  donorLevel->SetNumberOfComponents( 1 );
  donorLevel->SetNumberOfTuples( receivers->GetNumberOfPoints() );
  donorLevel->FillComponent(0,-1);
  receivers->GetPointData()->AddArray( donorLevel );
  donorLevel->Delete();

  // Add array to hold the donor cell center
  vtkDoubleArray *donorCellCenter = vtkDoubleArray::New();
  donorCellCenter->SetName( "DonorCentroid" );
  donorCellCenter->SetNumberOfComponents( 3 );
  donorCellCenter->SetNumberOfTuples( receivers->GetNumberOfPoints() );
  donorCellCenter->FillComponent(0,0.0);
  donorCellCenter->FillComponent(1,0.0);
  donorCellCenter->FillComponent(2,0.0);
  receivers->GetPointData()->AddArray( donorCellCenter );
  donorCellCenter->Delete();
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::GetReceivers( )
{
  assert( "pre: Extruded data" && (this->ExtrudedData != NULL) );

  unsigned int level = 0;
  for( ; level < this->ExtrudedData->GetNumberOfLevels(); ++level )
    {
      unsigned int idx = 0;
      for( ; idx < this->ExtrudedData->GetNumberOfDataSets( level ); ++idx )
        {

          vtkUniformGrid *gridPtr = this->ExtrudedData->GetDataSet(level,idx);
          if( gridPtr != NULL )
            {
              unsigned int gridIdx = vtkAMRGridIndexEncoder::encode(level,idx);
              if(this->ReceiverList.find( gridIdx )!=this->ReceiverList.end())
                {
                  this->ReceiverList[ gridIdx ]->Delete();
                  this->ReceiverList.erase( gridIdx );
                }

              vtkPolyData  *receivers   = vtkPolyData::New();
              vtkCellArray *vertexCells = vtkCellArray::New();
              vtkPoints *myPoints       = vtkPoints::New();

              // CellID maps the receiver point, i.e., the cell centroid back
              // to the corresponding ghost cell ID w.r.t. the extruded grid.
              vtkIntArray *meshIDData   = vtkIntArray::New();
              meshIDData->SetName( "CellID" );

              for( int i=0; i < gridPtr->GetNumberOfCells(); ++i )
                {
                  if( this->IsGhostCell( gridPtr, i) )
                    {
                      double center[3];
                      this->ComputeCellCenter( gridPtr, i, center );
                      myPoints->InsertNextPoint( center );

                      vtkIdType cidx =  myPoints->GetNumberOfPoints()-1;
                      vertexCells->InsertNextCell( 1, &cidx );
                      meshIDData->InsertNextValue( i );
                    } // END if ghost cell
                } // END for all cells

              receivers->SetPoints( myPoints );
              myPoints->Delete();

              receivers->SetVerts( vertexCells );
              vertexCells->Delete();

              receivers->GetPointData()->AddArray( meshIDData );
              meshIDData->Delete();

              // Prepare Receiver arrays
              this->AddReceiverInformation( receivers );

              this->ReceiverList[ gridIdx ] = receivers;
            } // END if grid is not NULL

        } // END for all grids
    } // END for all levels
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

  // STEP 4: Attach Ownership Information
  this->AttachPointOwnershipInfo( );

  // STEP 5: Synch processes
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
              vtkUniformGrid *ug = this->CloneGrid( myGrid );
              this->ExtrudedData->SetDataSet(currentLevel,dataIdx,myBox,ug);
              ug->Delete();
            }
          else
            {

              vtkUniformGrid *extrudedGrid = this->GetExtrudedGrid( myGrid );
              assert( "post: extrudedGrid != NULL" && (extrudedGrid != NULL) );

//              vtkAMRBox globalBox;
//              double h[3];
//              myBox.GetGridSpacing( h );
//              this->AMRDataSet->GetGlobalAMRBoxWithSpacing( globalBox, h );
//              myBox.GrowWithinBounds( this->NumberOfGhostLayers, globalBox );

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
void vtkAMRDataTransferFilter::FindDonors(
    const unsigned int receiverIdx,
    const int donorGridLevel,
    const int donorBlockIdx )
{
  if( this->ReceiverList.find(receiverIdx) == this->ReceiverList.end() )
    return;

  vtkPolyData *myReceivers = this->ReceiverList[ receiverIdx ];
  assert( "pre: receivers are NULL!" && (myReceivers != NULL) );

  if( myReceivers->GetNumberOfPoints() == 0 )
    return;

  unsigned int encodedDonorGridIdx=
   vtkAMRGridIndexEncoder::encode( donorGridLevel, donorBlockIdx );

  vtkUniformGrid *ug=
   this->AMRDataSet->GetDataSet( donorGridLevel,donorBlockIdx );
  assert( "pre: donor grid is NULL!" && (ug != NULL) );

  vtkPointData *PD = myReceivers->GetPointData();
  assert("pre: point data is NULL!" && (PD != NULL) );
  assert("pre: No DonorGridIdx attribute" && (PD->HasArray("DonorGridIdx")));
  assert("pre: No DonorCellIdx attribute" && (PD->HasArray("DonorCellIdx")));
  assert("pre: No DonorLevel attribute" && (PD->HasArray( "DonorLevel")));
  assert("pre: No DonorCentroid attribute" && (PD->HasArray("DonorCentroid")));

  vtkUnsignedIntArray *donorGridInfo=
   vtkUnsignedIntArray::SafeDownCast(PD->GetArray("DonorGridIdx"));
  vtkIntArray *donorCellInfo=
   vtkIntArray::SafeDownCast(PD->GetArray("DonorCellIdx"));
  vtkIntArray *donorLevelInfo=
   vtkIntArray::SafeDownCast(PD->GetArray("DonorLevel"));
  vtkDoubleArray *donorCentroid=
   vtkDoubleArray::SafeDownCast( PD->GetArray("DonorCentroid") );

  vtkIdType rcverIdx = 0;
  for( ; rcverIdx < myReceivers->GetNumberOfPoints(); ++rcverIdx )
    {
      double *rcver = myReceivers->GetPoint( rcverIdx );
      assert( "post: rcver array is NULL!" && (rcver != NULL) );

      int ijk[3];
      double pcoords[3];
      int status = ug->ComputeStructuredCoordinates( rcver, ijk, pcoords );
      if( status == 1 )
        {
          vtkIdType cellIdx=
            vtkStructuredData::ComputeCellId( ug->GetDimensions(),ijk );

          // Check if this data is at a higher resolution. Some ghost
          // cells can have both a lower & higher resolution donor
          // cells. In this case, preference is given to the highest
          // resolution data.
          if( donorLevelInfo->GetValue( rcverIdx ) < donorGridLevel )
            {
              donorLevelInfo->SetValue( rcverIdx, donorGridLevel );
              donorCellInfo->SetValue( rcverIdx, cellIdx );
              donorGridInfo->SetValue( rcverIdx, encodedDonorGridIdx );

              double dcentroid[3];
              this->ComputeCellCenter( ug, cellIdx, dcentroid );
              donorCentroid->SetComponent(rcverIdx, 0, dcentroid[0] );
              donorCentroid->SetComponent(rcverIdx, 1, dcentroid[1] );
              donorCentroid->SetComponent(rcverIdx, 2, dcentroid[2] );
            }

        } // END if the cell is found in this grid
      else if( donorLevelInfo->GetValue(rcverIdx) == -1 )
        {
          // -2 indicates that this node is orphaned, i.e., no donor cell
          // was found.
          donorLevelInfo->SetValue( rcverIdx, -2 );
          donorCellInfo->SetValue( rcverIdx, -2 );
          donorGridInfo->SetValue( rcverIdx, 0 );
        }

    } // END for all receivers

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::LocalDonorSearch()
{
  vtkUnsignedIntArray *cons = this->LocalConnectivity->GetEncodedGridKeys();

  unsigned int con = 0;
  for( ; con < cons->GetNumberOfTuples(); ++con )
    {
      unsigned int idx= cons->GetValue( con );
      int level       = -1;
      int blockIdx    = -1;
      vtkAMRGridIndexEncoder::decode(idx,level,blockIdx);

      int nCons=this->LocalConnectivity->GetNumberOfConnections(blockIdx,level);
      for( int i=0; i < nCons; ++i )
        {
          vtkAMRLink lnk=
           this->LocalConnectivity->GetConnection( blockIdx,level,i);
          this->FindDonors( idx, lnk.GetLevel(),lnk.GetBlockID() );
        } // END for all grid connections

    } // END for all grids with local connections

  cons->Delete();

}

//------------------------------------------------------------------------------
// TODO: Modularize and clean the function below
void vtkAMRDataTransferFilter::LocalDataTransfer()
{
  vtkstd::map<unsigned int,vtkPolyData* >::iterator it;
  it = this->ReceiverList.begin();
  for( ; it != this->ReceiverList.end(); ++it )
    {
      unsigned int rIdx  = it->first;
      vtkPolyData *receivers = it->second;
      assert( "pre: receivers is NULL" && (receivers != NULL) );

      int receiverLevel    = -1;
      int receiverBlockIdx = -1;
      vtkAMRGridIndexEncoder::decode( rIdx,receiverLevel,receiverBlockIdx );

      vtkUniformGrid *receiverGrid=
       this->ExtrudedData->GetDataSet(receiverLevel,receiverBlockIdx);
      assert( "pre: receiver grid is NULL" && (receiverGrid != NULL) );

      vtkCellData *receiverCD = receiverGrid->GetCellData();
      assert( "pre:Receiver grid cells is NULL" && (receiverCD != NULL) );
      assert( "pre:No DonorGridIdx attribute" &&
              (receiverCD->HasArray("DonorGridIdx")));
      assert( "pre:No DonorCellIdx attribute" &&
              (receiverCD->HasArray("DonorCellIdx")));
      assert( "pre:No DonorLevel attribute" &&
              (receiverCD->HasArray("DonorLevel")));
      assert( "pre: No DonorCentroid attribute" &&
              (receiverCD->HasArray("DonorCentroid")));

      vtkPointData *PD = receivers->GetPointData();
      assert( "pre:point data is NULL!" && (PD != NULL) );
      assert( "pre:No DonorGridIdx attribute" && (PD->HasArray("DonorGridIdx")));
      assert( "pre:No DonorCellIdx attribute" && (PD->HasArray("DonorCellIdx")));
      assert( "pre:No DonorLevel attribute" && (PD->HasArray("DonorLevel")));
      assert( "pre:No mesh CellId attribute" && (PD->HasArray("CellID")));
      assert( "pre:No DonorCentroid attribute" &&
              (PD->HasArray("DonorCentroid")));

      vtkUnsignedIntArray *donorGridInfo=
       vtkUnsignedIntArray::SafeDownCast(PD->GetArray("DonorGridIdx"));
      vtkIntArray *donorCellInfo=
       vtkIntArray::SafeDownCast(PD->GetArray("DonorCellIdx"));
      vtkIntArray *donorLevelInfo=
       vtkIntArray::SafeDownCast(PD->GetArray("DonorLevel"));
      vtkIntArray *meshCellInfo=
       vtkIntArray::SafeDownCast(PD->GetArray("CellID"));
      vtkDoubleArray *donorCentroid=
       vtkDoubleArray::SafeDownCast( PD->GetArray("DonorCentroid") );

      vtkIdType rcverIdx = 0;
      for( ; rcverIdx < receivers->GetNumberOfPoints(); ++rcverIdx )
        {
          int rcvCellIdx = meshCellInfo->GetValue(rcverIdx);
          assert( "post: rcver cell out-of-bounds" &&
           (rcvCellIdx >=0)&&(rcvCellIdx < receiverGrid->GetNumberOfCells()));

          int donorLevel            = donorLevelInfo->GetValue(rcverIdx);
          int donorCell             = donorCellInfo->GetValue(rcverIdx);
          unsigned int donorGridIdx = donorGridInfo->GetValue(rcverIdx);

          // Skip cells that are outside the boundary
          if( donorLevel == -2 )
            {
              // Setting the donor level on the recever grid side to -2
              // tells downstream filters, e.g., the dual-mesh-extractor
              // that this cell is probably out-of-bounds or more precisely
              // that no donor cell was found!
              vtkIntArray *dlevel =
               vtkIntArray::SafeDownCast(receiverCD->GetArray("DonorLevel"));
              dlevel->SetValue( rcvCellIdx, -2 );
              continue;
            }

          int donorGridLevel    = -1;
          int donorGridBlockIdx = -1;
          vtkAMRGridIndexEncoder::decode(
           donorGridIdx,donorGridLevel,donorGridBlockIdx);

         assert( "post: donor grid level mismatch!" &&
                  (donorGridLevel==donorLevel));

          double dcentroid[3];
          dcentroid[0] = donorCentroid->GetComponent(rcverIdx,0);
          dcentroid[1] = donorCentroid->GetComponent(rcverIdx,1);
          dcentroid[2] = donorCentroid->GetComponent(rcverIdx,2);

          vtkIntArray *rCellIdx=
           vtkIntArray::SafeDownCast(
            receiverCD->GetArray("DonorCellIdx") );
          vtkUnsignedIntArray *rGridIdx=
           vtkUnsignedIntArray::SafeDownCast(
            receiverCD->GetArray("DonorGridIdx") );
          vtkIntArray *rDonorLevel=
           vtkIntArray::SafeDownCast(
            receiverCD->GetArray( "DonorLevel") );
          vtkDoubleArray *rDonorCentroid=
            vtkDoubleArray::SafeDownCast(
              receiverCD->GetArray("DonorCentroid") );

          rCellIdx->SetValue(rcvCellIdx,donorCell);
          rGridIdx->SetValue(rcvCellIdx,donorGridIdx);
          rDonorLevel->SetValue(rcvCellIdx,donorLevel);

          rDonorCentroid->SetComponent( rcvCellIdx, 0, dcentroid[0] );
          rDonorCentroid->SetComponent( rcvCellIdx, 1, dcentroid[1] );
          rDonorCentroid->SetComponent( rcvCellIdx, 2, dcentroid[2] );

          vtkUniformGrid *donorGrid=
           this->AMRDataSet->GetDataSet(donorGridLevel,donorGridBlockIdx);
          assert( "pre: donor grid is NULL" && (donorGrid != NULL) );

          vtkCellData *donorCD = donorGrid->GetCellData();
          assert( "pre: donor grid cell data is NULL" && (donorCD != NULL) );

          vtkIdType arrayIdx = 0;
          for( ; arrayIdx < donorCD->GetNumberOfArrays(); ++arrayIdx )
            {
              vtkDataArray *cellData = donorCD->GetArray( arrayIdx );
              assert( "pre: cell data array is NULL" && (cellData != NULL) );

              if( receiverCD->HasArray(cellData->GetName() ) )
                {
                  vtkDataArray *rCellData =
                   receiverCD->GetArray( cellData->GetName() );
                  assert("pre: receiver cell data is NULL" && (rCellData!=NULL));
                  assert("pre: number of componenets mismatch" &&
                   (rCellData->GetNumberOfComponents()==
                    cellData->GetNumberOfComponents()) );

                  for(int k=0;k < cellData->GetNumberOfComponents();++k)
                    {
                      rCellData->SetComponent(rcvCellIdx,k,
                          cellData->GetComponent(donorCell,k));
                    }
                }

            } // END for all arrays

        } // END for all receiver nodes of the grid
    } // END for all receiving grids

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::CheckOwnershipAtSameLevel(
    vtkIntArray *ownership, vtkUniformGrid *myGrid, int level, int dataIdx )
{
  // Sanity check
  assert( "pre: Input grid is NULL" && (myGrid != NULL) );
  assert( "pre: Ownership info is NULL" && (ownership != NULL) );

  vtkCellData *myCellData = myGrid->GetCellData();
  assert( "pre: cell data is NULL" && (myCellData != NULL) );
  assert( "pre: donor level info not available" &&
          ( myCellData->HasArray("DonorLevel") ) );
  assert( "pre: donor grid info not available" &&
          ( myCellData->HasArray( "DonorGridIdx" ) ) );

  vtkIntArray *donorLevelInfo=
   vtkIntArray::SafeDownCast(
     myCellData->GetArray("DonorLevel") );

  vtkUnsignedIntArray *donorGridInfo=
   vtkUnsignedIntArray::SafeDownCast(
     myCellData->GetArray("DonorGridIdx") );

  vtkIdType cellIdx = 0;
  for( ;cellIdx < myGrid->GetNumberOfCells(); ++cellIdx )
    {

      if( this->IsGhostCell( myGrid, cellIdx ) &&
          (donorLevelInfo->GetValue(cellIdx) == level)  )
        {

          int donorGridBlock = -1;
          int donorLevel     = -1;
          vtkAMRGridIndexEncoder::decode(
           donorGridInfo->GetValue(cellIdx),donorLevel,donorGridBlock );
          assert("post: level mismatch" && (donorLevel==level) );

          vtkCell *myCell = myGrid->GetCell( cellIdx );
          assert("post: cell is NULL!" && (myCell != NULL) );

          if( dataIdx > donorGridBlock )
            {
              vtkIdList *nodes = myCell->GetPointIds();
              assert( "pre: cell nodes vtkIdList is null" &&
                      (nodes != NULL ) );

              vtkIdType nodeIdx = 0;
              for(; nodeIdx < nodes->GetNumberOfIds(); ++nodeIdx )
                ownership->SetValue( nodes->GetId( nodeIdx ), 0 );
            }

        }
    } // END for all grid cells
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::CheckOwnershipDownstream(
    vtkIntArray *ownership, vtkUniformGrid *grid,
    vtkHierarchicalBoxDataSet *amds, vtkIdType currentLevel )
{
  // Sanity check
  assert( "pre: Input grid is NULL" && (grid != NULL) );
  assert( "pre: Ownership info is NULL" && (ownership != NULL) );
  assert( "pre: AMR dataset is NULL" && (amds != NULL) );
  assert( "pre: level index out-of-bounds" &&
          (currentLevel >= 0) && (currentLevel < amds->GetNumberOfLevels() ) );

  vtkIdType nextLevel = currentLevel+1;
  if( nextLevel < amds->GetNumberOfLevels() )
    {

      unsigned int dataIdx = 0;
      for( ; dataIdx < amds->GetNumberOfDataSets( nextLevel ); ++dataIdx )
        {
          vtkUniformGrid *hiResGrid = amds->GetDataSet( nextLevel, dataIdx );
          if( hiResGrid == NULL )
            {
              // TODO: construct hiResGrid from AMR box.
            }

          assert("post: High resolution grid is NULL" && (hiResGrid != NULL));
          for( int node=0; node < grid->GetNumberOfPoints(); ++node )
            {

              if( ownership->GetValue( node ) == 1 )
                {
                  double pnt[3];
                  grid->GetPoint( node, pnt );

                  int ijk[3];
                  double pcoords[3];
                  int status=
                   hiResGrid->ComputeStructuredCoordinates(pnt,ijk,pcoords);
                  if( status == 1 )
                   ownership->SetValue( node,0 );
                }

            } // END for all nodes
        } // END for all data at next level

    }

}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::AttachPointOwnershipInfo()
{
  // Sanity checks
  assert( "pre: output data is NULL" && (this->ExtrudedData != NULL) );

  unsigned int level = 0;
  for( ; level < this->ExtrudedData->GetNumberOfLevels(); ++level )
    {
      unsigned int dataIdx = 0;
      for(;dataIdx < this->ExtrudedData->GetNumberOfDataSets(level);++dataIdx)
        {

          vtkIntArray *pntOwnership = vtkIntArray::New();
          vtkUniformGrid *myGrid= this->ExtrudedData->GetDataSet(level,dataIdx);
          if( myGrid != NULL )
            {
              pntOwnership->SetName( "PointOwnership" );
              pntOwnership->SetNumberOfComponents( 1 );
              pntOwnership->SetNumberOfTuples( myGrid->GetNumberOfPoints() );
              pntOwnership->FillComponent(0,1);
            }

          this->CheckOwnershipAtSameLevel(pntOwnership,myGrid,level,dataIdx);
//          this->CheckOwnershipDownstream(
//              pntOwnership, myGrid, this->ExtrudedData, level );
          this->CheckOwnershipDownstream(
              pntOwnership,myGrid,this->AMRDataSet,level );

          myGrid->GetPointData()->AddArray( pntOwnership );
          pntOwnership->Delete();

        } // END for all data
    } // END for all levels
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::DonorSearch()
{
  // Sanity checks
  assert("pre: ExtrudedData != NULL" && (this->ExtrudedData != NULL) );
  assert("pre: RemoteConnectivity != NULL" && (this->RemoteConnectivity!=NULL));
  assert("pre: LocalConnectivity != NULL" && (this->LocalConnectivity!=NULL));

  this->GetReceivers();

  this->LocalDonorSearch();
  this->WriteReceivers();

  // TODO:
  // Call RemoteDonorSearch()
}

//------------------------------------------------------------------------------
void vtkAMRDataTransferFilter::DataTransfer()
{
  // Sanity checks
  assert("pre: ExtrudedData != NULL" && (this->ExtrudedData != NULL) );

  this->LocalDataTransfer();

  // TODO:
  // Call RemoteDataTransfer()
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
vtkUniformGrid* vtkAMRDataTransferFilter::CloneGrid( vtkUniformGrid *ug)
{
  // Sanity check
  assert( "pre: input grid is NULL" && (ug != NULL) );

  // STEP 0: Construct the topology
  vtkUniformGrid *cloneGrid = vtkUniformGrid::New();
  cloneGrid->Initialize();
  cloneGrid->SetOrigin( ug->GetOrigin() );
  cloneGrid->SetDimensions( ug->GetDimensions() );
  cloneGrid->SetSpacing( ug->GetSpacing() );

  // STEP 1: Copy point & cell data from the original grid
  cloneGrid->GetPointData()->DeepCopy( ug->GetPointData() );
  cloneGrid->GetCellData()->DeepCopy( ug->GetCellData() );

  int numCells = cloneGrid->GetNumberOfCells();

  // STEP 2: Attach ghost array information
  vtkIntArray *ghostArray = vtkIntArray::New();
  ghostArray->SetName( "GHOST" );
  ghostArray->SetNumberOfTuples( numCells );
  ghostArray->SetNumberOfComponents( 1 );
  ghostArray->FillComponent(0,1);
  cloneGrid->GetCellData()->AddArray( ghostArray );
  ghostArray->Delete();

  // STEP 3: Attach donor grid information
  vtkUnsignedIntArray *donorGrid = vtkUnsignedIntArray::New();
  donorGrid->SetName( "DonorGridIdx" );
  donorGrid->SetNumberOfComponents( 1 );
  donorGrid->SetNumberOfTuples( numCells );
  cloneGrid->GetCellData()->AddArray( donorGrid );
  donorGrid->Delete();

  // STEP 4: Attach donor cell information
  vtkIntArray *donorCell = vtkIntArray::New();
  donorCell->SetName( "DonorCellIdx" );
  donorCell->SetNumberOfComponents( 1 ) ;
  donorCell->SetNumberOfTuples( numCells );
  donorCell->FillComponent(0,-1);
  cloneGrid->GetCellData()->AddArray( donorCell );
  donorCell->Delete();

  // STEP 5: Attach donor level information
  vtkIntArray *donorLevel = vtkIntArray::New();
  donorLevel->SetName( "DonorLevel" );
  donorLevel->SetNumberOfComponents( 1 );
  donorLevel->SetNumberOfTuples( numCells );
  donorLevel->FillComponent(0,-1);
  cloneGrid->GetCellData()->AddArray( donorLevel );
  donorLevel->Delete();

  // STEP 6: Attach donor centroid information
  vtkDoubleArray *donorCentroid = vtkDoubleArray::New();
  donorCentroid->SetName( "DonorCentroid" );
  donorCentroid->SetNumberOfComponents( 3 );
  donorCentroid->SetNumberOfTuples( numCells );
  donorCentroid->FillComponent(0,0.0);
  donorCentroid->FillComponent(1,0.0);
  donorCentroid->FillComponent(2,0.0);
  cloneGrid->GetCellData()->AddArray( donorCentroid );
  donorCentroid->Delete();
  return( cloneGrid );
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

  int numCells = extrudedGrid->GetNumberOfCells();

  // STEP 5: Attach donor grid information
  vtkUnsignedIntArray *donorGrid = vtkUnsignedIntArray::New();
  donorGrid->SetName( "DonorGridIdx" );
  donorGrid->SetNumberOfComponents( 1 );
  donorGrid->SetNumberOfTuples( numCells );
  extrudedGrid->GetCellData()->AddArray( donorGrid );
  donorGrid->Delete();

  // STEP 6: Attach donor cell information
  vtkIntArray *donorCell = vtkIntArray::New();
  donorCell->SetName( "DonorCellIdx" );
  donorCell->SetNumberOfComponents( 1 ) ;
  donorCell->SetNumberOfTuples( numCells );
  donorCell->FillComponent(0,-1);
  extrudedGrid->GetCellData()->AddArray( donorCell );
  donorCell->Delete();

  // STEP 7: Attach donor level information
  vtkIntArray *donorLevel = vtkIntArray::New();
  donorLevel->SetName( "DonorLevel" );
  donorLevel->SetNumberOfComponents( 1 );
  donorLevel->SetNumberOfTuples( numCells );
  donorLevel->FillComponent(0,-1);
  extrudedGrid->GetCellData()->AddArray( donorLevel );
  donorLevel->Delete();

  // STEP 6: Attach donor centroid information
  vtkDoubleArray *donorCentroid = vtkDoubleArray::New();
  donorCentroid->SetName( "DonorCentroid" );
  donorCentroid->SetNumberOfComponents( 3 );
  donorCentroid->SetNumberOfTuples( numCells );
  donorCentroid->FillComponent(0,0.0);
  donorCentroid->FillComponent(1,0.0);
  donorCentroid->FillComponent(2,0.0);
  extrudedGrid->GetCellData()->AddArray( donorCentroid );
  donorCentroid->Delete();
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
