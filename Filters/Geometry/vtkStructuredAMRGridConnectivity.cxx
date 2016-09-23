/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredAMRGridConnectivity.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredAMRGridConnectivity.h"

// VTK Includes
#include "vtkObjectFactory.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGridConnectivity.h"
#include "vtkStructuredNeighbor.h"

// C++ includes
#include <cmath>
#include <cstdlib>

//------------------------------------------------------------------------------
#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]

namespace AMRBlockFace {
  enum
  {
    FRONT         = 0, // (+k diretion)
    BACK          = 1, // (-k direction)
    RIGHT         = 2, // (+i direction)
    LEFT          = 3, // (-i direction)
    TOP           = 4, // (+j direction)
    BOTTOM        = 5, // (-j direction)
    NOT_ON_BLOCK_FACE = 6
  };
}

vtkStandardNewMacro( vtkStructuredAMRGridConnectivity );

//-----------------------------------------------------------------------------
vtkStructuredAMRGridConnectivity::vtkStructuredAMRGridConnectivity()
{
  this->DataDimension      = 0;
  this->DataDescription    = VTK_EMPTY;
  this->NumberOfGrids      = 0;
  this->MaxLevel           = -1;
  this->RefinementRatio    = -1;
  this->NumberOfLevels     = 0;
  this->BalancedRefinement = true;
  this->CellCentered       = true;
  this->NodeCentered       = false;

  IMIN(this->WholeExtent) =
  JMIN(this->WholeExtent) =
  KMIN(this->WholeExtent) = VTK_INT_MAX;

  IMAX(this->WholeExtent) =
  JMAX(this->WholeExtent) =
  KMAX(this->WholeExtent) = VTK_INT_MIN;
}

//-----------------------------------------------------------------------------
vtkStructuredAMRGridConnectivity::~vtkStructuredAMRGridConnectivity()
{
  this->AMRHierarchy.clear();
  this->GridExtents.clear();
  this->GridLevels.clear();
  this->Neighbors.clear();
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::PrintSelf(
    std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
  os << "=====================\n";
  os << "DATA DIMENSION: " << this->DataDimension << std::endl;
  os << "WHOLE EXTENT: [";
  for( int i=0; i < 6; ++i )
  {
    os << this->WholeExtent[i] << " ";
  }
  os << "]\n";
  os << "TOTAL NUMBER OF LEVELS:" << this->NumberOfLevels << std::endl;
  os << "TOTAL NUMBER OF GRIDS:"  << this->NumberOfGrids  << std::endl;
  if( this->HasConstantRefinementRatio() )
  {
    os << "CONSTANT REFINEMENT RATIO: " << this->RefinementRatio << std::endl;
  }
  else
  {
    os << "VARIABLE REFINEMENT RATIO\n";
  }

  int gridExtent[6];
  int neiExtent[6];
  for( unsigned int gridId=0; gridId < this->NumberOfGrids; ++gridId )
  {
    os << "=====================\n";
    os << "GRID["  << gridId << "] ";
    os << "LEVEL=" << this->GetGridLevel( gridId ) << " ";
    os << "EXTENT: ";
    this->GetGridExtent(gridId,gridExtent);
    this->PrintExtent(os,gridExtent);
    os << std::endl;
    if( this->GhostedExtents.size() != 0 )
    {
      assert("pre: ghosted extents vector is not properly allocated" &&
             (this->GhostedExtents.size()/6 == this->NumberOfGrids) );
      os << "GHOSTED EXTENT: ";
      int ghostedExt[6];
      this->GetGhostedExtent( gridId, ghostedExt );
      this->PrintExtent(os,ghostedExt);
      os << std::endl;
    }

    os << std::endl;
    os << "Connecting faces: "
       << this->GetNumberOfConnectingBlockFaces( gridId ) << " ";

    os << "[ ";
    if( this->HasBlockConnection( gridId, AMRBlockFace::FRONT ) )
    {
      os << "FRONT(+k) ";
    }
    if( this->HasBlockConnection( gridId, AMRBlockFace::BACK ) )
    {
      os << "BACK(-k) ";
    }
    if( this->HasBlockConnection( gridId, AMRBlockFace::RIGHT ) )
    {
      os << "RIGHT(+i) ";
    }
    if( this->HasBlockConnection( gridId, AMRBlockFace::LEFT ) )
    {
      os << "LEFT(-i) ";
    }
    if( this->HasBlockConnection( gridId, AMRBlockFace::TOP) )
    {
      os << "TOP(+j) ";
    }
    if( this->HasBlockConnection( gridId, AMRBlockFace::BOTTOM) )
    {
      os << "BOTTOM(-j) ";
    }
    os << "] ";
    os << std::endl;

    os << "NUMBER OF NEIGHBORS: " << this->Neighbors[gridId].size();
    os << std::endl << std::endl;

    for( unsigned int nei=0; nei < this->Neighbors[gridId].size(); ++nei)
    {
      os << "\t-----------------------------" << std::endl;
      os << "\tNEIGHBOR[" << nei << "] ";
      os << "ID=" << this->Neighbors[gridId][nei].NeighborID << " ";
      os << "LEVEL=" << this->Neighbors[gridId][nei].NeighborLevel << " ";
      os << "EXTENT=";
      this->GetGridExtent(this->Neighbors[gridId][nei].NeighborID,neiExtent);
      this->PrintExtent(os,neiExtent);
      os << " RELATIONSHIP=";
      os << this->Neighbors[gridId][nei].GetRelationShipString();
      os << std::endl;

      os << "\tGRID OVERLAP EXTENT=";
      this->PrintExtent(os,this->Neighbors[gridId][nei].GridOverlapExtent);
      os << "NEI OVERLAP EXTENT=";
      this->PrintExtent(os, this->Neighbors[gridId][nei].OverlapExtent);
      os << std::endl;

      os << "\tORIENTATION: (";
      os << this->Neighbors[gridId][nei].Orientation[0];
      os << ", ";
      os << this->Neighbors[gridId][nei].Orientation[1];
      os << ", ";
      os << this->Neighbors[gridId][nei].Orientation[2];
      os << ")\n";
      os << std::endl << std::endl;

      os << "\tRCVEXTENT=";
      this->PrintExtent(os,this->Neighbors[gridId][nei].RcvExtent);
      os << std::endl;
      os << "\tSNDEXTENT=";
      this->PrintExtent(os,this->Neighbors[gridId][nei].SendExtent);
      os << std::endl << std::endl;
    } // END for all neighbors

  } // END for all grids
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::Initialize(
    const unsigned int NumLevels,
    const unsigned int N, const int rr)
{
  this->NumberOfLevels = NumLevels;
  this->RefinementRatio = rr;
  this->SetNumberOfGrids( N );
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::SetNumberOfGrids(
    const unsigned int N )
{
  if (N == 0)
  {
    vtkErrorMacro("Number of grids cannot be 0.");
    return;
  }
  this->NumberOfGrids = N;
  this->AllocateUserRegisterDataStructures();

  this->GridExtents.resize( 6*N );
  this->GridLevels.resize( N );
  this->Neighbors.resize( N );
  this->BlockTopology.resize( N );

  if( !this->HasConstantRefinementRatio() )
  {
    this->RefinementRatios.resize( this->NumberOfLevels,-1 );
  }
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::SetBlockTopology(
    const int gridID)
{
  assert("pre: gridID is out-of-bounds!" &&
          (gridID >= 0) &&
          (gridID < static_cast<int>(this->NumberOfGrids)));

  int gridExtent[6];
  this->GetCoarsenedExtent(gridID,this->GridLevels[gridID],0,gridExtent);

  // Check in IMIN
  if( gridExtent[0] > this->WholeExtent[0] )
  {
    this->AddBlockConnection( gridID, AMRBlockFace::LEFT);
  }

  // Check in IMAX
  if( gridExtent[1] < this->WholeExtent[1] )
  {
    this->AddBlockConnection( gridID, AMRBlockFace::RIGHT);
  }

  // Check in JMIN
  if( gridExtent[2] > this->WholeExtent[2] )
  {
    this->AddBlockConnection( gridID, AMRBlockFace::BOTTOM );
  }

  // Check in JMAX
  if( gridExtent[3] < this->WholeExtent[3] )
  {
    this->AddBlockConnection( gridID, AMRBlockFace::TOP );
  }

  // Check in KMIN
  if( gridExtent[4] > this->WholeExtent[4] )
  {
    this->AddBlockConnection( gridID, AMRBlockFace::BACK );
  }

  // Check in KMAX
  if( gridExtent[5] < this->WholeExtent[5] )
  {
    this->AddBlockConnection( gridID, AMRBlockFace::FRONT );
  }
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::ComputeNeighbors()
{
  // STEP 0: Compute the whole extent w.r.t. level 0 which also computes the
  // data-description and dimension of the data.
  this->ComputeWholeExtent();

  // STEP 1: Establish neighbor relation between grids in the AMR hierarchy
  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
  {
    this->SetBlockTopology( i );

    for( unsigned int j=i+1; j < this->NumberOfGrids; ++j )
    {
      this->EstablishNeighbors(i,j);
    } // END for all j

    this->FillGhostArrays(
        i,this->GridPointGhostArrays[i],this->GridCellGhostArrays[i]);
  } // END for all i
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::GetGhostedExtent(
        const int gridID, int ext[6])
{
  assert("pre: grid ID is out-of-bounds!" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: ghosted-extents vector has not been allocated" &&
       (this->NumberOfGrids==this->GhostedExtents.size()/6));
  assert("pre: Number of ghost layers should not be 0" &&
       (this->NumberOfGhostLayers > 0) );

  for( int i=0; i < 6; ++i )
  {
    ext[ i ] = this->GhostedExtents[gridID*6+i];
  }
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::SetGhostedExtent(
        const int gridID, int ext[6])
{
  assert("pre: grid ID is out-of-bounds!" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: ghosted-extents vector has not been allocated" &&
       (this->NumberOfGrids==this->GhostedExtents.size()/6));
  assert("pre: Number of ghost layers should not be 0" &&
       (this->NumberOfGhostLayers > 0) );

  for( int i=0; i < 6; ++i )
  {
    this->GhostedExtents[gridID*6+i] = ext[ i ] ;
  }
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::CreateGhostLayers(const int N)
{
  if( N==0 )
  {
    vtkWarningMacro(
        "N=0 ghost layers requested! No ghost layers will be created");
    return;
  }

  this->NumberOfGhostLayers += N;
  this->AllocateInternalDataStructures();
  this->GhostedExtents.resize( 6*this->NumberOfGrids );

  for(unsigned int i=0; i < this->NumberOfGrids; ++i)
  {
    this->CreateGhostedExtent( i, N );
    this->CreateGhostedMaskArrays( i );
    this->ComputeNeighborSendAndRcvExtent(i,N);
    this->InitializeGhostData( i );
    this->TransferRegisteredDataToGhostedData( i );
    this->TransferGhostDataFromNeighbors( i );
  } // END for all grids
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::InitializeGhostData(
    const int gridID)
{
  assert( "pre: gridID is out-of-bounds!" &&
          (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: Grid has no registered point data!" &&
          (this->GridPointData[gridID] != NULL) );
  assert( "pre: Grid has no registered cell data!" &&
          (this->GridCellData[gridID] != NULL) );

  // STEP 0: Get the ghosted grid extent
  int ghostedExtent[6];
  this->GetGhostedExtent( gridID, ghostedExtent );

  // STEP 1: Get the number of nodes/cells in the ghosted extent
  int numNodes =
    vtkStructuredData::GetNumberOfPoints(ghostedExtent,this->DataDescription);
  int numCells =
    vtkStructuredData::GetNumberOfCells(ghostedExtent,this->DataDescription);

  // NOTE: For AMR we currently only support uniform AMR, so there is no need
  // to allocate the GhostedGridPoints

  // STEP 2: Allocate point data, if node-centered is true
  if( this->GetNodeCentered() )
  {
    assert( "pre: GhostedPointData vector has not been properly allocated!" &&
         (this->NumberOfGrids==this->GhostedGridPointData.size() ) );

    this->GhostedGridPointData[ gridID ] = vtkPointData::New();
    vtkPointData *PD = this->GridPointData[gridID];
    for(int array=0; array < PD->GetNumberOfArrays(); ++array)
    {
      int dataType = PD->GetArray( array )->GetDataType();
      vtkDataArray *dataArray = vtkDataArray::CreateDataArray( dataType );
      assert( "Cannot create data array" && (dataArray != NULL) );

      dataArray->SetName(PD->GetArray(array)->GetName());
      dataArray->SetNumberOfComponents(
          PD->GetArray(array)->GetNumberOfComponents());
      dataArray->SetNumberOfTuples( numNodes );

      this->GhostedGridPointData[ gridID ]->AddArray( dataArray );
      dataArray->Delete();
    } // END for all node arrays
  } // END if node-centered data-set

  // STEP 3: Allocate cell data
  if( this->GetCellCentered() )
  {
    assert("pre: GhostedCellData vector has not been properly allocated!" &&
           (this->NumberOfGrids==this->GhostedGridCellData.size() ) );
    this->GhostedGridCellData[ gridID ] = vtkCellData::New();
    vtkCellData *CD = this->GridCellData[gridID];
    for(int array=0; array < CD->GetNumberOfArrays(); ++array)
    {
      int dataType = CD->GetArray( array )->GetDataType();
      vtkDataArray *dataArray = vtkDataArray::CreateDataArray( dataType );
      assert( "Cannot create data array" && (dataArray != NULL) );

      dataArray->SetName(CD->GetArray(array)->GetName());
      dataArray->SetNumberOfComponents(
          CD->GetArray(array)->GetNumberOfComponents());
      dataArray->SetNumberOfTuples(numCells);

      this->GhostedGridCellData[ gridID ]->AddArray(dataArray);
      dataArray->Delete();
    } // END for all cell arrays
  } // END if cell-centered data-set
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::TransferRegisteredDataToGhostedData(
    const int gridID)
{
  assert("pre: grid ID is out-of-bounds!" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));

  // NOTE: For AMR we only support uniform grids, so we only transfer fields,
  // i.e., PointData and CellData here.

  // STEP 0: Get the registered grid extent
  int registeredExtent[6];
  this->GetGridExtent(gridID,registeredExtent);

  // STEP 1: Get the ghosted grid extent
  int ghostedExtent[6];
  this->GetGhostedExtent(gridID,ghostedExtent);

  // STEP 2: Get corresponding registered and ghosted cell extents
  int registeredCellExtent[6];
  int ghostedCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      registeredExtent,registeredCellExtent,this->DataDescription);
  vtkStructuredData::GetCellExtentFromPointExtent(
      ghostedExtent,ghostedCellExtent,this->DataDescription);

  // STEP 3: Loop over registered grid extent
  int ijk[3];
  for(int i=IMIN(registeredExtent); i <= IMAX(registeredExtent); ++i)
  {
    for(int j=JMIN(registeredExtent); j <= JMAX(registeredExtent); ++j)
    {
      for(int k=KMIN(registeredExtent); k <= KMAX(registeredExtent); ++k)
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;

        if( this->GetNodeCentered() )
        {
          // Compute the source index to the registered data
          vtkIdType sourcePntIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  registeredExtent,ijk,this->DataDescription);

          // Compute the target index to the ghosted data
          vtkIdType targetPntIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  ghostedExtent,ijk,this->DataDescription);

          this->CopyFieldData(
              this->GridPointData[gridID],sourcePntIdx,
              this->GhostedGridPointData[gridID],targetPntIdx);
        }

        if( this->IsNodeWithinExtent(i,j,k,registeredCellExtent) )
        {
          // Compute the source cell idx. Note, since we are passing to
          // ComputePointIdForExtent a cell extent, this is a cell id, not
          // a point id.
          vtkIdType sourceCellIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  registeredCellExtent, ijk, this->DataDescription );

          // Compute the target cell idx. Note, since we are passing to
          // ComputePointIdForExtent a cell extent, this is a cell id, not
          // a point id.
          vtkIdType targetCellIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  ghostedCellExtent, ijk, this->DataDescription );

          // Transfer cell data from the registered grid to the ghosted grid
          this->CopyFieldData(
              this->GridCellData[gridID], sourceCellIdx,
              this->GhostedGridCellData[gridID], targetCellIdx );
        } // END if within the cell extent

      } // END for all k
    } // END for all j
  } // END for all i
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::TransferLocalNodeCenteredNeighborData(
    const int vtkNotUsed(gridID),
    vtkStructuredAMRNeighbor& vtkNotUsed(nei))
{
  vtkErrorMacro("Node-centered AMR datasets are currently not supported!");
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::GetLocalCellCentersAtSameLevel(
      const int gridID, vtkStructuredAMRNeighbor &nei)
{
  // STEP 0: Get the grid's extent and cell extent
  int RegisteredGridExtent[6];
  this->GetGridExtent( gridID, RegisteredGridExtent );
  int RegisteredGridCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      RegisteredGridExtent,RegisteredGridCellExtent,this->DataDescription);

  // STEP 1: Get the grid's ghosted extent and cell extent
  int GhostedGridExtent[6];
  this->GetGhostedExtent( gridID, GhostedGridExtent );
  int GhostedCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      GhostedGridExtent,GhostedCellExtent,this->DataDescription);


  // STEP 2: Get the neighbor's extent and cell extent
  int NeighborExtent[6];
  this->GetGridExtent( nei.NeighborID, NeighborExtent );
  int NeighborCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      NeighborExtent,NeighborCellExtent,this->DataDescription);

  // STEP 3: Get RcvCell extent
  int RcvCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      const_cast<int*>(nei.RcvExtent),RcvCellExtent);

  // STEP 4: Loop through the RcvCellExtent and copy values iff a higher res
  // value does not exist.
  int ijk[3];
  for (int i=IMIN(RcvCellExtent); i <= IMAX(RcvCellExtent); ++i)
  {
    for (int j=JMIN(RcvCellExtent); j <= JMAX(RcvCellExtent); ++j)
    {
      for (int k=KMIN(RcvCellExtent); k <= KMAX(RcvCellExtent); ++k)
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;

        if( this->IsNodeWithinExtent(i,j,k, NeighborCellExtent)
            && !this->IsNodeWithinExtent(i,j,k, RegisteredGridCellExtent))
        {
          // Sanity check!
          assert("pre: RcvExtent is outside the GhostExtent!" &&
              this->IsNodeWithinExtent(i,j,k,GhostedGridExtent));
          assert("pre: RcvExtent is outside the NeighborExtent" &&
              this->IsNodeWithinExtent(i,j,k,NeighborExtent));

          // Compute the source & target index
          // Note: Since these indices are computed from a cell extent they
          // correspond to a cell index.
          vtkIdType sourceIdx = vtkStructuredData::ComputePointIdForExtent(
              NeighborCellExtent, ijk, this->DataDescription);

          vtkIdType targetIdx = vtkStructuredData::ComputePointIdForExtent(
              GhostedCellExtent, ijk, this->DataDescription);

          if(this->CellCenteredDonorLevel[gridID][targetIdx]< nei.NeighborLevel)
          {
            this->CopyFieldData(
                this->GridCellData[nei.NeighborID],sourceIdx,
                this->GhostedGridCellData[gridID],targetIdx);
            this->CellCenteredDonorLevel[gridID][targetIdx]=nei.NeighborLevel;
          } // END if this is a finer solution
        } // END if this is a ghost cell

      } // END for all k
    } // END for all j
  } // END for all i

}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::GetLocalCellCentersFromCoarserLevel(
    const int gridID, vtkStructuredAMRNeighbor &nei)
{
  assert( "pre: Expected a coarser neighbor" &&
          (nei.NeighborLevel < nei.GridLevel) );

  // STEP 0: Get the grid's extent and cell extent
  int RegisteredGridExtent[6];
  this->GetGridExtent( gridID, RegisteredGridExtent );
  int RegisteredGridCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      RegisteredGridExtent,RegisteredGridCellExtent,this->DataDescription);

  // STEP 1: Get the grid's ghosted extent and cell extent
  int GhostedGridExtent[6];
  this->GetGhostedExtent( gridID, GhostedGridExtent );
  int GhostedCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      GhostedGridExtent,GhostedCellExtent,this->DataDescription);


  // STEP 3: Get the neighbor's extent and cell extent
  int NeighborExtent[6];
  this->GetGridExtent( nei.NeighborID, NeighborExtent );
  int NeighborCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      NeighborExtent,NeighborCellExtent,this->DataDescription);

  // STEP 4: Get RcvCell extent
  int rcvDataDescription = vtkStructuredData::GetDataDescriptionFromExtent(
                              const_cast<int*>(nei.RcvExtent));
  int RcvCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      const_cast<int*>(nei.RcvExtent),RcvCellExtent);

  // STEP 5: Loop through the rcv cell extent and fill ghost regions
  int ijk[3];
  for(int i=IMIN(RcvCellExtent); i <= IMAX(RcvCellExtent); ++i)
  {
    for(int j=JMIN(RcvCellExtent); j <= JMAX(RcvCellExtent); ++j)
    {
      for(int k=KMIN(RcvCellExtent); k <= KMAX(RcvCellExtent); ++k)
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;

        int orient[3];
        int ndim = -1;
        this->GetOrientationVector(rcvDataDescription,orient,ndim);

        int range[6];
        this->GetCellRefinedExtent(
              orient,ndim,i,j,k,nei.NeighborLevel,
              this->GetGridLevel(gridID),range);

        // Loop through the range of fine grid cells.
        int myIJK[3];
        for(int ii=IMIN(range); ii <= IMAX(range); ++ii)
        {
          for(int jj=JMIN(range); jj <= JMAX(range); ++jj)
          {
            for(int kk=KMIN(range); kk <= KMAX(range); ++kk)
            {
              myIJK[0]=ii; myIJK[1]=jj; myIJK[2]=kk;
              if( !this->IsNodeWithinExtent(ii,jj,kk,GhostedCellExtent))
              {
                continue;
              }

              if(this->IsNodeWithinExtent(i,j,k, NeighborCellExtent) &&
                 !this->IsNodeWithinExtent(ii,jj,kk,RegisteredGridCellExtent))
              {
                 // Sanity check!
                 assert("pre: RcvExtent is outside the GhostExtent!" &&
                    this->IsNodeWithinExtent(ii,jj,kk,GhostedGridExtent));
                 assert("pre: RcvExtent is outside the NeighborExtent" &&
                    this->IsNodeWithinExtent(i,j,k,NeighborExtent));

                 // Compute the source & target index
                 // Note: Since these indices are computed from a cell extent
                 // they correspond to a cell index.
                 vtkIdType sourceIdx =
                     vtkStructuredData::ComputePointIdForExtent(
                          NeighborCellExtent, ijk, this->DataDescription);

                 vtkIdType targetIdx =
                     vtkStructuredData::ComputePointIdForExtent(
                         GhostedCellExtent, myIJK, this->DataDescription);

                 if(this->CellCenteredDonorLevel[gridID][targetIdx] <
                     nei.NeighborLevel)
                 {
                   this->CopyFieldData(
                      this->GridCellData[nei.NeighborID],sourceIdx,
                      this->GhostedGridCellData[gridID],targetIdx);
                   this->CellCenteredDonorLevel[gridID][targetIdx]=
                       nei.NeighborLevel;
                 } // END if this is a finer solution
              } // END if

            } // END for all kk
          } // END for all jj
        } // END for all ii

      } // END for all k
    } // END for all j
  } // END for all i
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::GetLocalCellCentersFromFinerLevel(
    const int gridID, vtkStructuredAMRNeighbor &nei)
{
  assert( "pre: Expected a finer neighbor" &&
          (nei.NeighborLevel > nei.GridLevel) );

  // STEP 0: Get the grid's extent and cell extent
  int RegisteredGridExtent[6];
  this->GetGridExtent( gridID, RegisteredGridExtent );
  int RegisteredGridCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      RegisteredGridExtent,RegisteredGridCellExtent,this->DataDescription);

  // STEP 1: Get the grid's ghosted extent and cell extent
  int GhostedGridExtent[6];
  this->GetGhostedExtent( gridID, GhostedGridExtent );
  int GhostedCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      GhostedGridExtent,GhostedCellExtent,this->DataDescription);


  // STEP 3: Get the neighbor's extent and cell extent
  int NeighborExtent[6];
  this->GetGridExtent( nei.NeighborID, NeighborExtent );
  int NeighborCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      NeighborExtent,NeighborCellExtent,this->DataDescription);

  // STEP 4: Get RcvCell extent
//  int rcvDataDescription =
//      vtkStructuredData::GetDataDescriptionFromExtent(
//                   const_cast<int*>(nei.RcvExtent));

  int RcvCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      const_cast<int*>(nei.RcvExtent),RcvCellExtent);

  // STEP 5: Get receive node/cell extent w.r.t. this grid
  int GridRcvExtent[6];
 nei.GetReceiveExtentOnGrid(
     this->NumberOfGhostLayers,GhostedGridExtent,GridRcvExtent);
  int GridRcvDataDescription =
      vtkStructuredData::GetDataDescriptionFromExtent(GridRcvExtent);
//  assert("pre: mismatching data description" &&
//          (rcvDataDescription == GridRcvDataDescription) );
  int GridRcvCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      GridRcvExtent,GridRcvCellExtent);

  int ijk[3];
  for(int i=IMIN(GridRcvCellExtent); i <= IMAX(GridRcvCellExtent); ++i)
  {
    for(int j=JMIN(GridRcvCellExtent); j <= JMAX(GridRcvCellExtent); ++j)
    {
      for(int k=KMIN(GridRcvCellExtent); k <= KMAX(GridRcvCellExtent); ++k)
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;
        if(!this->IsNodeWithinExtent(i,j,k,RegisteredGridCellExtent) &&
            this->IsNodeWithinExtent(i,j,k,GhostedCellExtent))
        {
          // Compute target cell index. Note since a cell extent is given to
          // ComputePointIdForExtent, a cell index is returned.
          vtkIdType targetIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  GhostedCellExtent,ijk,this->DataDescription);

          if(this->CellCenteredDonorLevel[gridID][targetIdx] <
             nei.NeighborLevel)
          {
            std::vector<vtkIdType> sourceIds;

            int range[6];
            int orient[3];
            int ndim;
            this->GetOrientationVector(GridRcvDataDescription,orient,ndim);
            this->GetCellRefinedExtent(
                orient,ndim,i,j,k,nei.GridLevel,nei.NeighborLevel,range);

            int rcvIJK[3];
            for(int ii=IMIN(range); ii <= IMAX(range); ++ii )
            {
              for(int jj=JMIN(range); jj <= JMAX(range); ++jj)
              {
                for(int kk=KMIN(range); kk <= KMAX(range); ++kk)
                {
                  rcvIJK[0]=ii; rcvIJK[1]=jj; rcvIJK[2]=kk;
                  if(this->IsNodeWithinExtent(ii,jj,kk,RcvCellExtent))
                  {
                    vtkIdType sourceIdx =
                        vtkStructuredData::ComputePointIdForExtent(
                            NeighborCellExtent,rcvIJK,this->DataDescription);
                    sourceIds.push_back(sourceIdx);
                  } // END if
                } // END for all kk
              } // END for all jj
            } // END for all ii

            if( sourceIds.size() > 0 )
            {
              this->AverageFieldData(
                  this->GridCellData[nei.NeighborID],&sourceIds[0],
                  static_cast<int>(sourceIds.size()),
                  this->GhostedGridCellData[gridID],targetIdx);

              this->CellCenteredDonorLevel[gridID][targetIdx]=
                  nei.NeighborLevel;
            }
            else
            {
              vtkWarningMacro("Empty list of sources!")
            }

          } // END if nei has finer resolution

        } // END if it is ghost cell

      } // END for all k
    } // END for all j
  } // END for all i
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::TransferLocalCellCenteredNeighborData(
    const int gridID, vtkStructuredAMRNeighbor &nei)
{
  int gridLevel = this->GetGridLevel( gridID );
  assert("pre: grid level mismatch!" && (gridLevel == nei.GridLevel) );

  // STEP 0: Check if the neighbor is strictly a child
  if( nei.RelationShip == vtkStructuredAMRNeighbor::CHILD )
  {
    // A child that is completely covered by this grid does not contribute to
    // its ghost-layers.
    return;
  }

  // STEP 1: Initialize the donor-level array if the array has not been
  // initialized before
  int GhostedGridExtent[6];
  this->GetGhostedExtent(gridID,GhostedGridExtent);
  int numCells = vtkStructuredData::GetNumberOfCells(
                       GhostedGridExtent,this->DataDescription);
  if( static_cast<int>(this->CellCenteredDonorLevel[gridID].size())
      != numCells)
  {
    this->CellCenteredDonorLevel[ gridID ].resize(numCells,-1);
  }

  // STEP 2: Fill data in the ghost levels
  if(gridLevel == nei.NeighborLevel)
  {
    this->GetLocalCellCentersAtSameLevel(gridID,nei);
  } // END if grids are at the same level
  else if(gridLevel < nei.NeighborLevel)
  {
    this->GetLocalCellCentersFromFinerLevel(gridID,nei);
  } // END if grid is coarser than the neighbor
  else
  {
    this->GetLocalCellCentersFromCoarserLevel(gridID,nei);
  } // END else grid is finer than the neighbor
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::TransferLocalNeighborData(
      const int gridID, vtkStructuredAMRNeighbor &nei)
{
  if( this->GetNodeCentered() )
  {
    this->TransferLocalNodeCenteredNeighborData(gridID,nei);
  }

  if( this->GetCellCentered() )
  {
    this->TransferLocalCellCenteredNeighborData(gridID,nei);
  }
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::TransferGhostDataFromNeighbors(
    const int gridID)
{
  // Sanity check
  assert("pre: gridID is out-of-bounds!" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: Neigbors is not propertly allocated" &&
       (this->NumberOfGrids==this->Neighbors.size() ) );

  this->CellCenteredDonorLevel.resize( this->NumberOfGrids );
  int NumNeis = static_cast<int>(this->Neighbors[gridID].size());
  for(int nei=0; nei < NumNeis; ++nei)
  {
    this->TransferLocalNeighborData(gridID,this->Neighbors[gridID][nei]);
  } // END for all neighbors
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::AverageFieldData(
    vtkFieldData *source, vtkIdType *sourceIds, const int N,
    vtkFieldData *target, vtkIdType targetIdx)
{
  assert( "pre: source field data is NULL!" && (source != NULL) );
  assert( "pre: target field data is NULL!" && (target != NULL) );
  assert( "pre: sourceIds is NULL!" && (sourceIds != NULL) );
  assert( "pre: N > 0" && (N > 0) );
  assert( "pre: source number of arrays does not match target!" &&
           source->GetNumberOfArrays()==target->GetNumberOfArrays() );

  int arrayIdx = 0;
  for( ; arrayIdx < source->GetNumberOfArrays(); ++arrayIdx )
  {
    // Get source array
    vtkDataArray *sourceArray = source->GetArray( arrayIdx );
    assert( "ERROR: encountered NULL source array" && (sourceArray != NULL) );

    // Get target array
    vtkDataArray *targetArray = target->GetArray( arrayIdx );
    assert( "ERROR: encountered NULL target array" && (targetArray != NULL) );

    // Sanity checks
    assert( "ERROR: target/source array name mismatch!" &&
      (strcmp( sourceArray->GetName(), targetArray->GetName() ) == 0 ) );
    assert( "ERROR: target/source array num components mismatch!" &&
      (sourceArray->GetNumberOfComponents()==
       targetArray->GetNumberOfComponents()));
    assert( "ERROR: targetIdx out-of-bounds!" &&
      (targetIdx >= 0) && (targetIdx < targetArray->GetNumberOfTuples() ) );

    int numComponents = sourceArray->GetNumberOfComponents();

    std::vector< double > averageTuple;
    averageTuple.resize(numComponents,0);
    for( int comp=0; comp < numComponents; ++comp)
    {
      for( int src=0; src < N; ++src )
      {
        vtkIdType sourceIdx = sourceIds[ src ];
        assert( "ERROR: sourceIdx out-of-bounds!" &&
         (sourceIdx >= 0) && (sourceIdx < sourceArray->GetNumberOfTuples()));

        averageTuple[comp] += sourceArray->GetComponent(sourceIdx,comp);
      }
      averageTuple[comp] /= N;
      targetArray->SetComponent(targetIdx,comp,averageTuple[comp]);
    } // END for all components

  } // END for all arrays
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::CopyFieldData(
    vtkFieldData *source, vtkIdType sourceIdx,
    vtkFieldData *target, vtkIdType targetIdx)
{
  assert( "pre: source field data is NULL!" && (source != NULL) );
  assert( "pre: target field data is NULL!" && (target != NULL) );
  assert( "pre: source number of arrays does not match target!" &&
          source->GetNumberOfArrays()==target->GetNumberOfArrays() );

  int arrayIdx = 0;
  for( ; arrayIdx < source->GetNumberOfArrays(); ++arrayIdx )
  {
    // Get source array
    vtkDataArray *sourceArray = source->GetArray( arrayIdx );
    assert( "ERROR: encountered NULL source array" && (sourceArray != NULL) );

    // Get target array
    vtkDataArray *targetArray = target->GetArray( arrayIdx );
    assert( "ERROR: encountered NULL target array" && (targetArray != NULL) );

    // Sanity checks
    assert( "ERROR: target/source array name mismatch!" &&
      (strcmp( sourceArray->GetName(), targetArray->GetName() ) == 0 ) );
    assert( "ERROR: target/source array num components mismatch!" &&
      (sourceArray->GetNumberOfComponents()==
       targetArray->GetNumberOfComponents()));
    assert( "ERROR: sourceIdx out-of-bounds!" &&
      (sourceIdx >= 0) && (sourceIdx < sourceArray->GetNumberOfTuples() ) );
    assert( "ERROR: targetIdx out-of-bounds!" &&
      (targetIdx >= 0) && (targetIdx < targetArray->GetNumberOfTuples() ) );

    // Copy the tuple
    targetArray->SetTuple(targetIdx, sourceIdx, sourceArray);
  } // END for all arrays
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::ComputeNeighborSendAndRcvExtent(
    const int gridID, const int N)
{
  // Sanity check
  assert( "pre: gridID is out-of-bounds!" &&
          (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: Neigbors is not propertly allocated" &&
          (this->NumberOfGrids==this->Neighbors.size() ) );

  int gridRealExtent[6];
  this->GetGridExtent(gridID,gridRealExtent);

  int gridGhostedExtent[6];
  this->GetGhostedExtent(gridID,gridGhostedExtent);

  int neiRealExtent[6];

  int NumNeis = static_cast<int>(this->Neighbors[ gridID ].size());
  for(int nei=0; nei < NumNeis; ++nei)
  {
    this->GetGridExtent(
        this->Neighbors[gridID][nei].NeighborID,neiRealExtent);

    this->Neighbors[gridID][nei].ComputeSendAndReceiveExtent(
        gridRealExtent,gridGhostedExtent,neiRealExtent,this->WholeExtent,N);
  } // END for all neighbors
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::CreateGhostedMaskArrays(
        const int gridID)
{
  assert( "pre: gridID is out-of-bounds!" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: GhostedPointGhostArray has not been allocated" &&
       (this->NumberOfGrids == this->GhostedPointGhostArray.size()));
  assert( "pre: GhostedCellGhostArray has not been allocated" &&
       (this->NumberOfGrids == this->GhostedCellGhostArray.size()));

  // STEP 0: Initialize ghosted node and cell arrays
  if( this->GhostedPointGhostArray[gridID] == NULL )
  {
    this->GhostedPointGhostArray[ gridID ] = vtkUnsignedCharArray::New();
  }
  else
  {
    this->GhostedPointGhostArray[ gridID ]->Reset();
  }

  if( this->GhostedCellGhostArray[ gridID ] == NULL )
  {
    this->GhostedCellGhostArray[ gridID ] = vtkUnsignedCharArray::New();
  }
  else
  {
    this->GhostedCellGhostArray[ gridID ]->Reset();
  }

  // STEP 1: Get the ghosted extent
  int ghostExtent[6];
  this->GetGhostedExtent( gridID, ghostExtent );

  // STEP 2: Compute numNodes/numCells on the ghosted grid
  int numNodes =
    vtkStructuredData::GetNumberOfPoints(ghostExtent,this->DataDescription);
  int numCells =
    vtkStructuredData::GetNumberOfCells(ghostExtent,this->DataDescription);

  // STEP 3: Allocate the ghosted node and cell arrays
  this->GhostedPointGhostArray[ gridID ]->Allocate( numNodes );
  this->GhostedCellGhostArray[ gridID ]->Allocate( numCells );

  // STEP 4: Get the registered extent of the grid
  int registeredGridExtent[6];
  this->GetGridExtent(gridID,registeredGridExtent);

  // STEP 5: Get normalized whole extent w.r.t. the level of this grid
  int normalizedWholeExt[6];
  this->GetWholeExtentAtLevel(this->GetGridLevel(gridID),normalizedWholeExt);

  // STEP 6: Fill ghosted points ghost array
  int ijk[3];
  for(int i=IMIN(ghostExtent); i <= IMAX(ghostExtent); ++i)
  {
    for(int j=JMIN(ghostExtent); j <= JMAX(ghostExtent); ++j)
    {
      for(int k=KMIN(ghostExtent); k <= KMAX(ghostExtent); ++k)
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;
        vtkIdType pntIdx =
           vtkStructuredData::ComputePointIdForExtent(
                   ghostExtent,ijk,this->DataDescription);

        if( this->IsNodeWithinExtent(i,j,k,registeredGridExtent))
        {
          // Copy data from registered grid
          vtkIdType srcIdx =
             vtkStructuredData::ComputePointIdForExtent(
                 registeredGridExtent,ijk,this->DataDescription);
          unsigned char p = 0;
          if(this->GridPointGhostArrays[gridID])
          {
            p = this->GridPointGhostArrays[gridID]->GetValue(srcIdx);
          }
          this->GhostedPointGhostArray[ gridID ]->SetValue(pntIdx, p);
        } // END if node within the registered extent
        else
        {
          // The node is a ghost node
          unsigned char p = 0;
          p |= vtkDataSetAttributes::DUPLICATEPOINT;
          if( this->IsNodeOnBoundaryOfExtent(i,j,k,normalizedWholeExt) )
          {
            // We don't have BOUNDARY now but we might add
            // it in the future.
            //vtkGhostArray::SetProperty(p,vtkGhostArray::BOUNDARY);
          }

          this->GhostedPointGhostArray[ gridID ]->SetValue(pntIdx,p);
        } // END else

      } // END for all k
    } // END for all j
  } // END for all i

  // STEP 7: Fill ghosted cells ghost array
  int ghostCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      ghostExtent,ghostCellExtent,this->DataDescription);

  int registeredCellExtent[6];
  vtkStructuredData::GetCellExtentFromPointExtent(
      registeredGridExtent,registeredCellExtent,this->DataDescription);

  for(int i=IMIN(ghostCellExtent); i <= IMAX(ghostCellExtent); ++i)
  {
    for(int j=JMIN(ghostCellExtent); j <= JMAX(ghostCellExtent); ++j)
    {
      for( int k=KMIN(ghostCellExtent); k <= KMAX(ghostCellExtent); ++k)
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;
        vtkIdType cellIdx =
            vtkStructuredData::ComputePointIdForExtent(
                ghostCellExtent,ijk,this->DataDescription);

        if( this->IsNodeWithinExtent(i,j,k,registeredCellExtent))
        {
          vtkIdType srcCellIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  registeredCellExtent,ijk,this->DataDescription);
          unsigned char p = 0;
          if(this->GridCellGhostArrays[gridID])
          {
            p = this->GridCellGhostArrays[gridID]->GetValue(srcCellIdx);
          }
          this->GhostedCellGhostArray[ gridID ]->SetValue(cellIdx, p);
        }
        else
        {
          unsigned char p = 0;
          p |= vtkDataSetAttributes::DUPLICATECELL;
          this->GhostedCellGhostArray[ gridID ]->SetValue(cellIdx,p);
        }
      } // END for all k
    } // END for all j
  } // END for all i
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::CreateGhostedExtent(
        const int gridId, const int N)
{
  assert("pre: gridId is out-of-bounds!" &&
         (gridId >= 0) && (gridId < static_cast<int>(this->NumberOfGrids)));
  assert("pre: ghosted extents vector has not been allocated!" &&
          (this->NumberOfGrids == this->GhostedExtents.size()/6 ) );
  assert("pre: number of ghost-layers requested should not be 0" &&
          (this->NumberOfGhostLayers > 0) );

  int ext[6];
  this->GetGridExtent( gridId, ext);

  switch( this->DataDescription )
  {
    case VTK_X_LINE:
      IMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::LEFT))?N:0;
      IMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::RIGHT))?N:0;
      break;
    case VTK_Y_LINE:
      JMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BOTTOM))?N:0;
      JMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::TOP))?N:0;
      break;
    case VTK_Z_LINE:
      KMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BACK))?N:0;
      KMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::FRONT))?N:0;
      break;
    case VTK_XY_PLANE:
      IMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::LEFT))?N:0;
      IMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::RIGHT))?N:0;
      JMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BOTTOM))?N:0;
      JMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::TOP))?N:0;
      break;
    case VTK_YZ_PLANE:
      JMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BOTTOM))?N:0;
      JMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::TOP))?N:0;
      KMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BACK))?N:0;
      KMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::FRONT))?N:0;
      break;
    case VTK_XZ_PLANE:
      IMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::LEFT))?N:0;
      IMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::RIGHT))?N:0;
      KMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BACK))?N:0;
      KMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::FRONT))?N:0;
      break;
    case VTK_XYZ_GRID:
      IMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::LEFT))?N:0;
      IMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::RIGHT))?N:0;
      JMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BOTTOM))?N:0;
      JMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::TOP))?N:0;
      KMIN(ext)-=(this->HasBlockConnection(gridId,AMRBlockFace::BACK))?N:0;
      KMAX(ext)+=(this->HasBlockConnection(gridId,AMRBlockFace::FRONT))?N:0;
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
  } // END switch

  this->SetGhostedExtent( gridId, ext );
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::FillCellsGhostArray(
      const int gridId, vtkUnsignedCharArray *cellsArray)
{
  assert("pre: grid index is out-of-bounds" &&
      (gridId >= 0) && (gridId < static_cast<int>(this->NumberOfGrids)));

  if( cellsArray == NULL )
  {
    return;
  }

  // STEP 0: Get the node extent & grid data description
  int ext[6];
  this->GetGridExtent(gridId,ext);
  int dataDescription =
      vtkStructuredData::GetDataDescription( ext );
  int numCells = vtkStructuredData::GetNumberOfCells(ext,dataDescription);
  if( numCells != cellsArray->GetNumberOfTuples() )
  {
    vtkErrorMacro("CellsArray may not be allocated properly!");
    return;
  }

  // STEP 1: Get the cell extent
  int cellext[6];
  vtkStructuredData::GetCellExtentFromPointExtent(ext,cellext,dataDescription);

  // STEP 2: Mark all cells as internal
  unsigned char *ghostArrayPtr = cellsArray->GetPointer(0);
  assert("pre: ghost array ptr is NULL" && (ghostArrayPtr != NULL) );

  int ijk[3];
  for(int i=IMIN(cellext); i <= IMAX(cellext); ++i)
  {
    for(int j=JMIN(cellext); j <= JMAX(cellext); ++j)
    {
      for(int k=KMIN(cellext); k <= KMAX(cellext); ++k)
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;

        vtkIdType idx =
            vtkStructuredData::ComputePointIdForExtent(
                cellext,ijk,dataDescription);
        assert("pre: cell index is out-of-bounds!" && (idx < numCells) );
        ghostArrayPtr[idx] = 0;
      } // END for all k
    } // END for all j
  } // END for all i

  // STEP 3: Loop through the neighbors of this grid, and mark all cells that
  // are covered by hi-res cells.
  int numNeis = static_cast<int>(this->Neighbors[ gridId ].size());
  for( int nei=0; nei < numNeis; ++nei )
  {
    int rel = this->Neighbors[ gridId ][ nei ].RelationShip;
    if( (rel == vtkStructuredAMRNeighbor::CHILD) ||
        (rel == vtkStructuredAMRNeighbor::PARTIALLY_OVERLAPPING_CHILD) )
    {

      // Get the cell overlap extent
      int overlapCellExt[6];
      vtkStructuredData::GetCellExtentFromPointExtent(
          this->Neighbors[ gridId ][ nei ].GridOverlapExtent,
          overlapCellExt,
          dataDescription);

      for( int i=IMIN(overlapCellExt); i <= IMAX(overlapCellExt); ++i )
      {
        for( int j=JMIN(overlapCellExt); j <= JMAX(overlapCellExt); ++j )
        {
          for( int k=KMIN(overlapCellExt); k <= KMAX(overlapCellExt); ++k )
          {
            ijk[0]=i; ijk[1]=j; ijk[2]=k;
            vtkIdType idx =
                vtkStructuredData::ComputePointIdForExtent(
                    cellext,ijk,dataDescription);
            assert("pre: cell index is out-of-bounds!" && (idx < numCells) );
            ghostArrayPtr[idx] |= vtkDataSetAttributes::REFINEDCELL;
          } // END for all k
        } // END for all j
      } // END for all i

    } // END if
  } // END for all neighbors
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::MarkNodeProperty(
    const int gridId, const int i, const int j, const int k,
    int gridExt[6], int wholeExt[6], unsigned char &p)
{
  p = 0;

  if( !this->IsNodeInterior(i,j,k,gridExt) )
  {
    if( this->IsNodeOnBoundaryOfExtent(i,j,k,wholeExt))
    {
      //We might use BOUNDARY in the future
      //vtkGhostArray::SetProperty( p, vtkGhostArray::BOUNDARY );
    }

    if( this->IsNodeOnSharedBoundary(i,j,k, gridId, gridExt) )
    {
      // NOTE: for AMR grids, all the grids own all of ther points so we don't
      // ignore any of the points.
      // We might use SHARED in the future
      //vtkGhostArray::SetProperty(p,vtkGhostArray::SHARED);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::FillNodesGhostArray(
    const int gridId, vtkUnsignedCharArray* nodesArray )
{
  assert("pre: grid index is out-of-bounds" &&
      (gridId >= 0) && (gridId < static_cast<int>(this->NumberOfGrids)));

  // STEP 0: If the nodes array is NULL, return immediately
  if( nodesArray == NULL )
  {
    return;
  }

  // STEP 1: Get normalized whole extent at the level of the given grid.
  int gridLevel = this->GetGridLevel( gridId );
  int NormalizedWholeExtent[6];
  this->GetWholeExtentAtLevel(gridLevel,NormalizedWholeExtent);

  // STEP 2: Loop through the grid points and mark them accordingly
  int ext[6];
  this->GetGridExtent(gridId,ext);
  int gridDataDescription =
     vtkStructuredData::GetDataDescriptionFromExtent(ext);
  assert("pre:grid data-description does not match whole extent description" &&
         (gridDataDescription == this->DataDescription) );

  // STEP 3: Mark nodes
  int ijk[3];
  for( int i=IMIN(ext); i <= IMAX(ext); ++i )
  {
    for( int j=JMIN(ext); j <= JMAX(ext); ++j )
    {
      for( int k=KMIN(ext); k <= KMAX(ext); ++k )
      {
        ijk[0]=i; ijk[1]=j; ijk[2]=k;
        vtkIdType idx = vtkStructuredData::ComputePointIdForExtent(
            ext,ijk,gridDataDescription);

        this->MarkNodeProperty(
            gridId, i, j, k, ext, NormalizedWholeExtent,
            *nodesArray->GetPointer(idx) );
      } // END for all k
    } // END for all j
  } // END for all i
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::GetNodeOrientation(
    const int i, const int j, const int k, int ext[6], int orientation[3])
{
  orientation[0]=orientation[1]=orientation[2]=AMRBlockFace::NOT_ON_BLOCK_FACE;
  switch( this->DataDescription )
  {
    case VTK_X_LINE:
      orientation[0] =
          this->Get1DOrientation(
              i, ext[0], ext[1],
              AMRBlockFace::LEFT, AMRBlockFace::RIGHT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      break;
    case VTK_Y_LINE:
      orientation[1] =
          this->Get1DOrientation(
              j, ext[2], ext[3],
              AMRBlockFace::BOTTOM, AMRBlockFace::TOP,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      break;
    case VTK_Z_LINE:
      orientation[2] =
          this->Get1DOrientation(
              k, ext[4], ext[5],
              AMRBlockFace::BACK, AMRBlockFace::FRONT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      break;
    case VTK_XY_PLANE:
      orientation[0] =
          this->Get1DOrientation(
              i, ext[0], ext[1],
              AMRBlockFace::LEFT, AMRBlockFace::RIGHT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      orientation[1] =
          this->Get1DOrientation(
              j, ext[2], ext[3], AMRBlockFace::BOTTOM, AMRBlockFace::TOP,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      break;
    case VTK_YZ_PLANE:
      orientation[1] =
            this->Get1DOrientation(
                j, ext[2], ext[3],
                AMRBlockFace::BOTTOM, AMRBlockFace::TOP,
                AMRBlockFace::NOT_ON_BLOCK_FACE);
      orientation[2] =
          this->Get1DOrientation(
              k, ext[4], ext[5],
              AMRBlockFace::BACK, AMRBlockFace::FRONT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      break;
    case VTK_XZ_PLANE:
      orientation[0] =
          this->Get1DOrientation(
              i, ext[0], ext[1],
              AMRBlockFace::LEFT, AMRBlockFace::RIGHT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      orientation[2] =
          this->Get1DOrientation(
              k, ext[4], ext[5],
              AMRBlockFace::BACK, AMRBlockFace::FRONT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      break;
    case VTK_XYZ_GRID:
      orientation[0] =
          this->Get1DOrientation(
              i, ext[0], ext[1],
              AMRBlockFace::LEFT, AMRBlockFace::RIGHT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      orientation[1] =
          this->Get1DOrientation(
              j, ext[2], ext[3],
              AMRBlockFace::BOTTOM, AMRBlockFace::TOP,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      orientation[2] =
          this->Get1DOrientation(
              k, ext[4], ext[5],
              AMRBlockFace::BACK, AMRBlockFace::FRONT,
              AMRBlockFace::NOT_ON_BLOCK_FACE);
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
  }
}

//-----------------------------------------------------------------------------
bool vtkStructuredAMRGridConnectivity::IsNodeWithinExtent(
      const int i, const int j, const int k,
      int GridExtent[6])
{
  bool status = false;
  switch( this->DataDescription )
  {
    case VTK_X_LINE:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1]) )
      {
        status = true;
      }
      break;
    case VTK_Y_LINE:
      if( (GridExtent[2] <= j) && (j <= GridExtent[3] ) )
      {
        status = true;
      }
      break;
    case VTK_Z_LINE:
      if( (GridExtent[4] <= k) && (k <= GridExtent[5] ) )
      {
        status = true;
      }
      break;
    case VTK_XY_PLANE:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1]) &&
           (GridExtent[2] <= j) && (j <= GridExtent[3])  )
      {
        status = true;
      }
      break;
    case VTK_YZ_PLANE:
      if( (GridExtent[2] <= j) && (j <= GridExtent[3] ) &&
           (GridExtent[4] <= k) && (k <= GridExtent[5] ) )
      {
        status = true;
      }
      break;
    case VTK_XZ_PLANE:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1] ) &&
           (GridExtent[4] <= k) && (k <= GridExtent[5] ) )
      {
        status = true;
      }
      break;
    case VTK_XYZ_GRID:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1]) &&
           (GridExtent[2] <= j) && (j <= GridExtent[3]) &&
           (GridExtent[4] <= k) && (k <= GridExtent[5]) )
      {
        status = true;
      }
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
  } // END switch
  return( status );
}

//-----------------------------------------------------------------------------
bool vtkStructuredAMRGridConnectivity::IsNodeOnSharedBoundary(
        const int i, const int j, const int k,
        const int gridId, int gridExt[6])
{
  bool status = false;

  if( this->IsNodeOnBoundaryOfExtent(i,j,k, gridExt) )
  {
    // For the vertex corresponding to i,j,k, the node orientation tuple
    // encodes on which face the node lies.
    int nodeOrientation[3];
    this->GetNodeOrientation(i,j,k,gridExt,nodeOrientation);
    for( int ii=0; ii < 3; ++ii )
    {
      if( (nodeOrientation[ii] != AMRBlockFace::NOT_ON_BLOCK_FACE) &&
          this->HasBlockConnection(gridId,nodeOrientation[ii]) )
      {
        status = true;
        break;
      }
    } // END for all dimensions
  } // END if node on boundary
  return( status );
}

//-----------------------------------------------------------------------------
bool vtkStructuredAMRGridConnectivity::IsNodeOnBoundaryOfExtent(
        const int i, const int j, const int k, int ext[6] )
{
  bool status = false;
  switch( this->DataDescription )
  {
    case VTK_X_LINE:
     if( i==ext[0] || i==ext[1] )
     {
       status = true;
     }
     break;
   case VTK_Y_LINE:
     if( j==ext[2] || j==ext[3] )
     {
       status = true;
     }
     break;
   case VTK_Z_LINE:
     if( k==ext[4] || k==ext[5] )
     {
       status = true;
     }
     break;
   case VTK_XY_PLANE:
     if( (i==ext[0] || i==ext[1]) ||
         (j==ext[2] || j==ext[3]) )
     {
       status = true;
     }
     break;
   case VTK_YZ_PLANE:
     if( (j==ext[2] || j==ext[3]) ||
         (k==ext[4] || k==ext[5]) )
     {
       status = true;
     }
     break;
   case VTK_XZ_PLANE:
     if( (i==ext[0] || i==ext[1]) ||
         (k==ext[4] || k==ext[5]) )
     {
       status = true;
     }
     break;
   case VTK_XYZ_GRID:
     if( (i==ext[0] || i==ext[1]) ||
         (j==ext[2] || j==ext[3]) ||
         (k==ext[4] || k==ext[5]) )
     {
       status = true;
     }
     break;
   default:
     std::cout << "Data description is: " << this->DataDescription << "\n";
     std::cout.flush();
     assert( "pre: Undefined data-description!" && false );
  } // END switch
  return( status );
}

//-----------------------------------------------------------------------------
bool vtkStructuredAMRGridConnectivity::IsNodeInterior(
      const int i, const int j, const int k, int GridExtent[6])
{
  bool status = false;
  switch( this->DataDescription )
  {
    case VTK_X_LINE:
      if( (GridExtent[0] < i) && (i < GridExtent[1]) )
      {
        status = true;
      }
      break;
    case VTK_Y_LINE:
      if( (GridExtent[2] < j) && (j < GridExtent[3] ) )
      {
        status = true;
      }
      break;
    case VTK_Z_LINE:
      if( (GridExtent[4] < k) && (k < GridExtent[5] ) )
      {
        status = true;
      }
      break;
    case VTK_XY_PLANE:
      if( (GridExtent[0] < i) && (i < GridExtent[1]) &&
          (GridExtent[2] < j) && (j < GridExtent[3])  )
      {
        status = true;
      }
      break;
    case VTK_YZ_PLANE:
      if( (GridExtent[2] < j) && (j < GridExtent[3] ) &&
          (GridExtent[4] < k) && (k < GridExtent[5] ) )
      {
        status = true;
      }
      break;
    case VTK_XZ_PLANE:
      if( (GridExtent[0] < i) && (i < GridExtent[1] ) &&
          (GridExtent[4] < k) && (k < GridExtent[5] ) )
      {
        status = true;
      }
      break;
    case VTK_XYZ_GRID:
      if( (GridExtent[0] < i) && (i < GridExtent[1]) &&
          (GridExtent[2] < j) && (j < GridExtent[3]) &&
          (GridExtent[4] < k) && (k < GridExtent[5]) )
      {
        status = true;
      }
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
  } // END switch
  return( status );
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::FillGhostArrays(
    const int gridId,
    vtkUnsignedCharArray* nodesArray,
    vtkUnsignedCharArray* cellsArray)
{
  this->FillNodesGhostArray(gridId,nodesArray);
  this->FillCellsGhostArray(gridId,cellsArray);
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::RegisterGrid(
      const int gridIdx,
      const int level,
      int extents[6],
      vtkUnsignedCharArray* nodesGhostArray,
      vtkUnsignedCharArray* cellGhostArray,
      vtkPointData* pointData,
      vtkCellData* cellData,
      vtkPoints* gridNodes)
{
  assert( "pre: level should be >= 0" && (level >= 0) );
  assert( "pre: Grid index is out-of-bounds!" &&
          ( (gridIdx >= 0) &&
            (gridIdx < static_cast<int>(this->NumberOfGrids) ) ) );

  if( level > this->MaxLevel )
  {
    this->MaxLevel = level;
  }

  this->GridLevels[ gridIdx ] = level;
  this->InsertGridAtLevel( level, gridIdx );

  for( int i=0; i < 6; ++i )
  {
    this->GridExtents[ gridIdx*6+i ] = extents[ i ];
  }

  this->RegisterGridGhostArrays(gridIdx,nodesGhostArray,cellGhostArray);
  this->RegisterFieldData(gridIdx,pointData,cellData);
  this->RegisterGridNodes(gridIdx,gridNodes);
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::RegisterGrid(
    const int gridIdx, const int level, const int refinementRatio,
    int extents[6],
    vtkUnsignedCharArray* nodesGhostArray,
    vtkUnsignedCharArray* cellGhostArray,
    vtkPointData* pointData,
    vtkCellData* cellData,
    vtkPoints* gridNodes)
{
  assert(
   "pre: This method should only be called if there is varying ref. ratio!" &&
   !this->HasConstantRefinementRatio() );
  assert("pre: Refinement ratios have not been allocated!" &&
         (this->RefinementRatios.size()==this->NumberOfGrids));

  this->RefinementRatios[ gridIdx ] = refinementRatio;

  this->RegisterGrid(
        gridIdx,level,extents,
        nodesGhostArray,
        cellGhostArray,
        pointData,
        cellData,
        gridNodes);
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::EstablishNeighbors(
      const int i, const int j)
{
  int ext1[6]; /* extent for grid i */
  int ext2[6]; /* extent for grid j */

  // STEP 0: Compute the level difference between the two grids
  int levelDiff = std::abs((this->GridLevels[j]-this->GridLevels[i]));

  // STEP 1: If this is a balanced refinement, check the level difference
  if( this->BalancedRefinement )
  {
    if( levelDiff > 1 )
    {
      // If the refinement is balanced, adjacent grids are guaranteed to have
      // a level difference of 1, hence, we skip grids that have bigger level
      // difference.
      return;
    }
  } // END if balanced refinement

  // NOTE: To establish neighboring connectivity, the grids must be coarsened
  // or refined at the same level. By convention, we always normalize to the
  // level of grid j.
  int normalizedLevel = this->GridLevels[j];

  // STEP 2: Get normalized extents, i.e., extents at the same level
  if( this->GridLevels[i] == this->GridLevels[j] )
  {
    // Grids at the same level, connectivity can be determined directly
    // by acquiring each grid extent.
    this->GetGridExtent(i,ext1);
    this->GetGridExtent(j,ext2);
  } // END if the grids are at the same level
  else if( this->GridLevels[i] < this->GridLevels[j] )
  {
    // Grid "i" is coarser than grid "j"
    this->GetRefinedExtent(i,this->GridLevels[i],this->GridLevels[j],ext1);
    this->GetGridExtent(j,ext2);
  }
  else if( this->GridLevels[i] > this->GridLevels[j] )
  {
    // Grid "i" is finer than grid "j"
    this->GetCoarsenedExtent(i,this->GridLevels[i],this->GridLevels[j],ext2);
    this->GetGridExtent(j,ext2);
  }
  else
  {
    // Code should not reach here!
    vtkErrorMacro("Code should not reach here!");
  }

  // STEP 3: Get the whole extent at the normalized level
  int myWholeExtent[6];
  this->GetWholeExtentAtLevel(normalizedLevel,myWholeExtent);

  // STEP 4: Use vtkStructuredGridConnectivity to establish neighbors
  vtkStructuredGridConnectivity *gridConnectivity =
      vtkStructuredGridConnectivity::New();
  gridConnectivity->SetWholeExtent( myWholeExtent );
  gridConnectivity->SetNumberOfGrids(2);
  gridConnectivity->RegisterGrid(0,ext1,NULL,NULL,NULL,NULL,NULL);
  gridConnectivity->RegisterGrid(1,ext2,NULL,NULL,NULL,NULL,NULL);
  gridConnectivity->ComputeNeighbors();

  if( gridConnectivity->GetNumberOfNeighbors(0) != 0)
  {
    assert(gridConnectivity->GetNumberOfNeighbors(0)==1);
    assert(gridConnectivity->GetNumberOfNeighbors(1)==1);
    vtkStructuredNeighbor nei0 = gridConnectivity->GetGridNeighbor(0,0);
    vtkStructuredNeighbor nei1 = gridConnectivity->GetGridNeighbor(1,0);

    vtkStructuredAMRNeighbor amrNei0 =
        this->GetAMRNeighbor(
            i,this->GridLevels[i], ext1,
            j,this->GridLevels[j], ext2,
            normalizedLevel,
            levelDiff,
            nei0);
    this->Neighbors[i].push_back( amrNei0 );

    vtkStructuredAMRNeighbor amrNei1 =
        this->GetAMRNeighbor(
            j,this->GridLevels[j], ext2,
            i,this->GridLevels[i], ext1,
            normalizedLevel,
            levelDiff,
            nei1);
    this->Neighbors[j].push_back( amrNei1 );
  }

  gridConnectivity->Delete();
}

//-----------------------------------------------------------------------------
void
vtkStructuredAMRGridConnectivity::ComputeAMRNeighborOverlapExtents(
    const int iLevel, const int jLevel, const int normalizedLevel,
    const vtkStructuredNeighbor &nei,
    int orient[3], int ndim,
    int gridOverlapExtent[6],
    int neiOverlapExtent[6])
{
  for( int i=0; i < 6; ++i )
  {
    gridOverlapExtent[ i ] =
    neiOverlapExtent[ i ]  =
    nei.OverlapExtent[ i ];
  }

  if( iLevel != normalizedLevel )
  {
    assert( "pre: level is not equal to the normalized level!" &&
            (jLevel==normalizedLevel) );

    // Change the gridOverlapExtent
    if( iLevel < normalizedLevel )
    {
      this->CoarsenExtent(
          orient,ndim,normalizedLevel,iLevel,gridOverlapExtent);
    }
    else
    {
      this->RefineExtent(
          orient,ndim,normalizedLevel,iLevel,gridOverlapExtent);
    }
  } // END if iLevel is not the same as the normalized level
  else if( jLevel != normalizedLevel )
  {
    assert( "pre: level is not equal to the normalized level!" &&
            (iLevel==normalizedLevel) );

    // Change the neiOverlapExtent
    if( jLevel < normalizedLevel )
    {
      this->CoarsenExtent(
          orient,ndim,normalizedLevel,jLevel,neiOverlapExtent);
    }
    else
    {
      this->RefineExtent(
          orient,ndim,normalizedLevel,jLevel,neiOverlapExtent);
    }
  } // END else if
  else
  {
    /* grids are at the same level */
    return;
  }
}

//-----------------------------------------------------------------------------
vtkStructuredAMRNeighbor
vtkStructuredAMRGridConnectivity::GetAMRNeighbor(
      const int vtkNotUsed(i), const int iLevel, int next1[6],
      const int j, const int jLevel, int next2[6],
      const int normalizedLevel,
      const int levelDiff,
      vtkStructuredNeighbor &nei)
{
  // STEP 0: Get the overlap extent data-description & dimension
  int overlapDim = vtkStructuredData::GetDataDimension(nei.OverlapExtent);

  // STEP 1: Get orientation vector and ndim for the domain which is used to
  // determine which dimensions of the overlap extent to refine/coarsen as
  // necessary.
  int ndim;
  int orient[3];
  this->GetOrientationVector(this->DataDescription,orient,ndim);

  // STEP 2: Compute grid overlap extent (grid i) and nei overlap extent, i.e.,
  // grid j.
  int gridOverlap[6];
  int neiOverlap[6];
  this->ComputeAMRNeighborOverlapExtents(
      iLevel, jLevel,normalizedLevel,nei,
      orient,ndim,
      gridOverlap,neiOverlap);

  // STEP 3: Detect relationship type
  int relationShip = vtkStructuredAMRNeighbor::UNDEFINED;
  if( iLevel == jLevel )
  {
    // If the grids are at the same level, the AMR hierarchy is valid iff they
    // are siblings. Hence, the grids should not be overlapping. A necessary
    // and sufficient condition for non-overlapping grids at the same level is
    // that their interface, i.e., overlap extent, will be a geometric object
    // whose dimensionality is one less the dimensionality of the domain.
    // For example, in 2-D the interface will be a 1-D line and in 3-D the
    // the interface will be a 2-D plane.
    assert( "pre: Detected overlapping grids at the same level" &&
            (overlapDim == this->DataDimension-1) );
    relationShip = vtkStructuredAMRNeighbor::SAME_LEVEL_SIBLING;
  } // END if
  else if( iLevel < jLevel )
  {
    if( overlapDim == this->DataDimension-1 )
    {
      // Grid i is adjacent with a finder grid
      relationShip = vtkStructuredAMRNeighbor::COARSE_TO_FINE_SIBLING;
    } // END if
    else
    {
      // Grid j is a child of i
      // NOTE: child relationships can only differ by one level!
      if( levelDiff <= 1)
      {
        if( this->AreExtentsEqual(nei.OverlapExtent,next2) )
        {
          relationShip = vtkStructuredAMRNeighbor::CHILD;
        }
        else
        {
          relationShip = vtkStructuredAMRNeighbor::PARTIALLY_OVERLAPPING_CHILD;
        }
      } // END if levelDiff
    } // END else
  } // END else if
  else
  {
    if( overlapDim == this->DataDimension-1 )
    {
      // Grid i is adjacent with a coarser grid
      relationShip = vtkStructuredAMRNeighbor::FINE_TO_COARSE_SIBLING;
    } // END if
    else
    {
      // Grid j is a parent of i
      // NOTE: parent relationships can only differ by one level!
      if( levelDiff <= 1 )
      {
        if( this->AreExtentsEqual(nei.OverlapExtent,next1) )
        {
          relationShip = vtkStructuredAMRNeighbor::PARENT;
        }
        else
        {
          relationShip = vtkStructuredAMRNeighbor::PARTIALLY_OVERLAPPING_PARENT;
        }
      } // END if levelDiff
    } // END else
  } // END else

  // STEP 4: Construct AMR neighbor
  vtkStructuredAMRNeighbor amrNei(
      iLevel,j,jLevel,
      gridOverlap,
      neiOverlap,
      nei.Orientation,
      relationShip);

  return( amrNei );
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::GetOrientationVector(
      const int dataDescription, int orient[3], int &ndim )
{
  switch( dataDescription )
  {
    case VTK_X_LINE:
      ndim        =  1;
      orient[ 0 ] =  0;
      orient[ 1 ] = -1;
      orient[ 2 ] = -1;
      break;
    case VTK_Y_LINE:
      ndim        =  1;
      orient[ 0 ] =  1;
      orient[ 1 ] = -1;
      orient[ 2 ] = -1;
      break;
    case VTK_Z_LINE:
      ndim        =  1;
      orient[ 0 ] =  2;
      orient[ 1 ] = -1;
      orient[ 2 ] = -1;
      break;
    case VTK_XY_PLANE:
      ndim        =  2;
      orient[ 0 ] =  0;
      orient[ 1 ] =  1;
      orient[ 2 ] = -1;
      break;
    case VTK_YZ_PLANE:
      ndim        =  2;
      orient[ 0 ] =  1;
      orient[ 1 ] =  2;
      orient[ 2 ] = -1;
      break;
    case VTK_XZ_PLANE:
      ndim        =  2;
      orient[ 0 ] =  0;
      orient[ 1 ] =  2;
      orient[ 2 ] = -1;
      break;
    case VTK_XYZ_GRID:
      ndim        = 3;
      orient[ 0 ] = 0;
      orient[ 1 ] = 1;
      orient[ 2 ] = 2;
      break;
    default:
      vtkErrorMacro("Undefined data-description!");
  }
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::GetCoarsenedExtent(
        const int gridIdx, int fromLevel, int toLevel, int ext[6])
{
  assert("pre: grid index is out-of-bounds!" &&
          (gridIdx >= 0) &&
          (gridIdx < static_cast<int>(this->NumberOfGrids)) );
  assert("pre: toLevel must be >= 0" && (toLevel >= 0) );

  // STEP 0: Acquire the grid extent corresponding to the given grid.
  this->GetGridExtent(gridIdx, ext);

  // STEP 1: If we are at the same level, we need to do nothing
  if( fromLevel == toLevel )
  {
    return;
  }

  // STEP 2: Get the orientation vector and dimension
  int orient[3];
  int ndim = -1;
  this->GetOrientationVector(this->DataDescription,orient,ndim);

  // STEP 3: Coarsen the extent
  this->CoarsenExtent(orient,ndim,fromLevel,toLevel,ext);
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::CoarsenExtent(
    int orient[3], int ndim, int fromLevel, int toLevel, int ext[6])
{
  assert("pre: ndim must be either 1, 2 or 3" && (ndim > 0 && ndim <= 3) );

  if( this->HasConstantRefinementRatio() )
  {
    assert("pre: invalid constant refinement ratio" &&
            (this->RefinementRatio >= 2) );
    int levelDifference = std::abs(fromLevel-toLevel);
    int ratio = levelDifference * this->RefinementRatio;
    for( int i=0; i < ndim; ++i )
    {
      int dimIdx        = orient[i];
      ext[ dimIdx*2  ] /= ratio;
      ext[ dimIdx*2+1] /= ratio;
    } // END for all dimensions
  } // END if
  else
  {
    assert("pre: refinement ratios has not been allocated" &&
            this->RefinementRatios.size()==this->NumberOfGrids);

    for( int l=fromLevel-1; l >= toLevel; --l)
    {
      int ratio = this->GetRefinementRatioAtLevel( l );
      for( int i=0; i < ndim; ++i )
      {
        int dimIdx        = orient[ i ];
        ext[ dimIdx*2  ] /= ratio;
        ext[ dimIdx*2+1] /= ratio;
      } // END for all dimensions
    } // END for each level
  } // END else
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::GetRefinedExtent(
        const int gridIdx, int fromLevel, int toLevel, int ext[6])
{
  assert("pre: grid index is out-of-bounds!" &&
           (gridIdx >= 0) &&
           (gridIdx < static_cast<int>(this->NumberOfGrids)) );
  assert("pre: toLevel <= MaxLevel" && (toLevel <= this->MaxLevel) );

  // STEP 0: Acquire the grid extent corresponding to the given grid.
  this->GetGridExtent(gridIdx, ext);

  // STEP 1: If we are at the same level, we need to do nothing
  if( fromLevel == toLevel )
  {
    return;
  }

  // STEP 2: Get the orientation vector and dimension
  int orient[3];
  int ndim = -1;
  this->GetOrientationVector(this->DataDescription,orient,ndim);

  // STEP 3: Refine the extent
  this->RefineExtent(orient,ndim,fromLevel,toLevel,ext);
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::RefineExtent(
    int orient[3], int ndim, int fromLevel, int toLevel, int ext[6])
{
  assert("pre: ndim must be either 1, 2 or 3" && (ndim > 0 && ndim <= 3) );

  if( this->HasConstantRefinementRatio() )
  {
    assert("pre: invalid constant refinement ratio" &&
            (this->RefinementRatio >= 2) );
    int levelDifference = std::abs(fromLevel-toLevel);
    int ratio = levelDifference * this->RefinementRatio;
    for( int i=0; i < ndim; ++i )
    {
      int dimIdx       = orient[i];
      ext[ dimIdx*2 ]  *= ratio;
      ext[ dimIdx*2+1] *= ratio;
    }
  } // END if constant refinement ratio
  else
  {
    assert("pre: refinement ratios has not been allocated" &&
           this->RefinementRatios.size()==this->NumberOfGrids);

    for( int l=fromLevel; l < toLevel; ++l )
    {
      int ratio = this->GetRefinementRatioAtLevel( l );
      for( int i=0; i < ndim; ++i )
      {
        int dimIdx        = orient[ i ];
        ext[ dimIdx*2  ] *= ratio;
        ext[ dimIdx*2+1] *= ratio;
      } // END for all dimensions
    } // END for each level
  } // END else varying refinement ratio
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::GetCellRefinedExtent(
        int orient[3], int ndim,
        const int i, const int j, const int k,
        const int fromLevel, const int toLevel,
        int ext[6])
{
  // STEP 0: Initialize ext
  IMIN(ext) = IMAX(ext) = i;
  JMIN(ext) = JMAX(ext) = j;
  KMIN(ext) = KMAX(ext) = k;

  // STEP 1: Compute refined cell extent
  if( this->HasConstantRefinementRatio() )
  {
    assert("pre: invalid constant refinement ratio" &&
           this->RefinementRatio >= 2);
    int levelDifference = std::abs(fromLevel-toLevel);
    int ratio           = levelDifference * this->RefinementRatio;
    for( int dim=0; dim < ndim; ++dim)
    {
      int dimIdx        = orient[ dim ];
      ext[ dimIdx*2 ]  *= ratio;
      ext[ dimIdx*2+1 ] = (ext[dimIdx*2]  + (ratio-1) );
    } // END for all dimensions
  } // END if constant refinement ratio
  else
  {
    for( int l=fromLevel; l < toLevel; ++l )
    {
      int ratio = this->GetRefinementRatioAtLevel( l );
      for(int dim=0; dim < ndim; ++dim)
      {
        int dimIdx        = orient[ dim ];
        ext[ dimIdx*2 ]  *= ratio;
        ext[ dimIdx*2+1 ] = (ext[dimIdx*2] + (ratio-1));
      } // END for all dimensions
    } // END for each level
  } // END else varying refinement ratio
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::GetWholeExtentAtLevel(
    const int level, int ext[6])
{
  assert( "pre: level index is out-of-bounds!" &&
          (level >= 0) && (level <= this->MaxLevel) );

  for( int i=0; i < 6; ++i )
  {
    ext[i] = this->WholeExtent[i];
  }

  if( level > 0 )
  {
    int orient[3];
    int ndim = -1;
    this->GetOrientationVector(this->DataDescription,orient,ndim);
    this->RefineExtent(orient,ndim,0,level,ext);
  }
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRGridConnectivity::ComputeWholeExtent()
{
  if( !this->LevelExists( 0 ) )
  {
    vtkErrorMacro("AMR dataset has no level 0!\n");
    return;
  }

  std::set<int>::iterator iter;
  std::set<int> rootLevelGrids = this->AMRHierarchy[0];

  int ext[6];
  bool initialPass = true;
  for( iter=rootLevelGrids.begin(); iter != rootLevelGrids.end(); ++iter)
  {
    int gridIdx = *iter;
    this->GetGridExtent(gridIdx, ext);

    if( initialPass )
    {
      for( int i=0; i < 6; ++i )
      {
        this->WholeExtent[i] = ext[i];
      }
      initialPass = false;
    } // END if
    else
    {
      for(int dim=0; dim < 3; ++dim)
      {

        if(this->WholeExtent[dim*2] > ext[dim*2] )
        {
          this->WholeExtent[dim*2] = ext[dim*2];
        }

        if(this->WholeExtent[dim*2+1] < ext[dim*2+1])
        {
          this->WholeExtent[dim*2+1] = ext[dim*2+1];
        }

      } // END for all dimensions
    } // END else
  } // END for all grids at level L0

    this->DataDescription =
        vtkStructuredData::GetDataDescriptionFromExtent(this->WholeExtent);
    this->DataDimension =
        vtkStructuredData::GetDataDimension(this->DataDescription);
}
