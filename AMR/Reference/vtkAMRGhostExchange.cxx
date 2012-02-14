/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRGhostExchange.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRGhostExchange.h"
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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkAMRGhostCellExtruder.h"

#include <string>
#include <sstream>
#include <cassert>
#include <vtkstd/map>
#include <algorithm>

//
// Standard Functions
//
vtkStandardNewMacro(vtkAMRGhostExchange);

vtkAMRGhostExchange::vtkAMRGhostExchange()
{
  this->Controller          = NULL;
  this->AMRDataSet          = NULL;
  this->RemoteConnectivity  = NULL;
  this->LocalConnectivity   = NULL;
  this->ExtrudedData        = NULL;
  this->NumberOfGhostLayers = 1;

  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
}

//------------------------------------------------------------------------------
vtkAMRGhostExchange::~vtkAMRGhostExchange()
{
  vtkstd::map<unsigned int,vtkPolyData*>::iterator it;
  for( it=this->ReceiverList.begin(); it != this->ReceiverList.end(); ++it )
      it->second->Delete();
  this->ReceiverList.erase(this->ReceiverList.begin(),this->ReceiverList.end());
}

//------------------------------------------------------------------------------
void vtkAMRGhostExchange::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkAMRGhostExchange::FillInputPortInformation(
      int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRGhostExchange::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRGhostExchange::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // Sanity check
  assert( "pre: Input vector is NULL"  && (inputVector != NULL) );
  assert( "pre: output vector is NULL" && (outputVector != NULL) );

  // STEP 0: Get input & output objects
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: Null input information object!" && (input != NULL) );
  this->AMRDataSet = vtkHierarchicalBoxDataSet::SafeDownCast(
   input->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input AMR dataset is NULL" && (this->AMRDataSet != NULL) );

  vtkInformation *output= outputVector->GetInformationObject(0);
  assert( "pre: Null output information object!" && (output != NULL) );
  this->ExtrudedData = vtkHierarchicalBoxDataSet::SafeDownCast(
   output->Get(vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: ouput AMR dataset is NULL" && (this->ExtrudedData != NULL) );

  // STEP 1: Transfer solution to ghosts
  this->Transfer( );

  // STEP 2: Synchronize
  if( this->Controller != NULL )
    this->Controller->Barrier();

  return 1;
}

//------------------------------------------------------------------------------
bool vtkAMRGhostExchange::IsGhostCell( vtkUniformGrid *ug, int cellIdx )
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
void vtkAMRGhostExchange::ComputeCellCenter(
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
void vtkAMRGhostExchange::WriteReceivers( )
{
  std::map< unsigned int, vtkPolyData* >::iterator it=this->ReceiverList.begin();
  std::ostringstream oss;

  for( ; it != this->ReceiverList.end(); ++it )
    {
      int level = -1;
      int idx   = -1;
      vtkAMRGridIndexEncoder::Decode( it->first,level,idx );

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
void vtkAMRGhostExchange::AddReceiverInformation( vtkPolyData *receivers )
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
void vtkAMRGhostExchange::GetReceivers( )
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
              unsigned int gridIdx = vtkAMRGridIndexEncoder::Encode(level,idx);
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
void vtkAMRGhostExchange::Transfer( )
{
  // Sanity Checks
  assert("pre:ghost layers >= 1" && (this->NumberOfGhostLayers>=1));
  assert("pre:AMRDataSet != NULL" && (this->AMRDataSet != NULL));
  assert("pre:Controller != NULL" && (this->Controller != NULL));
  assert("pre:RemoteConnectivity != NULL" && (this->RemoteConnectivity!=NULL));
  assert("pre:LocalConnectivity != NULL" && (this->LocalConnectivity!=NULL));

  // STEP 0: Construct the extruded ghost data
  vtkAMRGhostCellExtruder *cellExtruder = vtkAMRGhostCellExtruder::New();
  cellExtruder->SetInput( this->AMRDataSet );
  cellExtruder->SetNumberOfGhostLayers( this->NumberOfGhostLayers );
  cellExtruder->Update();
  this->ExtrudedData = cellExtruder->GetOutput();
  assert( "Extruded AMR data-set is NULL" &&
          (this->ExtrudedData != NULL) );

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
void vtkAMRGhostExchange::FindDonors(
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
   vtkAMRGridIndexEncoder::Encode( donorGridLevel, donorBlockIdx );

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
void vtkAMRGhostExchange::LocalDonorSearch()
{
  vtkUnsignedIntArray *cons = this->LocalConnectivity->GetEncodedGridKeys();

  unsigned int con = 0;
  for( ; con < cons->GetNumberOfTuples(); ++con )
    {
      unsigned int idx= cons->GetValue( con );
      int level       = -1;
      int blockIdx    = -1;
      vtkAMRGridIndexEncoder::Decode(idx,level,blockIdx);

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
void vtkAMRGhostExchange::LocalDataTransfer()
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
      vtkAMRGridIndexEncoder::Decode( rIdx,receiverLevel,receiverBlockIdx );

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
          vtkAMRGridIndexEncoder::Decode(
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
void vtkAMRGhostExchange::CheckOwnershipAtSameLevel(
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
          vtkAMRGridIndexEncoder::Decode(
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
void vtkAMRGhostExchange::CheckOwnershipDownstream(
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
void vtkAMRGhostExchange::AttachPointOwnershipInfo()
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
void vtkAMRGhostExchange::DonorSearch()
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
void vtkAMRGhostExchange::DataTransfer()
{
  // Sanity checks
  assert("pre: ExtrudedData != NULL" && (this->ExtrudedData != NULL) );

  this->LocalDataTransfer();

  // TODO:
  // Call RemoteDataTransfer()
}
