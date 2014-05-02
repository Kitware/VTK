/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredAMRGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestStructuredAMRGridConnectivity.cxx -- Test AMR grid connectivity
//
// .SECTION Description
//  Serial test for structured AMR grid connectivity/nesting

// VTK includes
#include "vtkAMRInformation.h"
#include "vtkCell.h"
#include "vtkDoubleArray.h"
#include "vtkGhostArray.h"
#include "vtkIntArray.h"
#include "vtkOverlappingAMR.h"
#include "vtkStructuredAMRGridConnectivity.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLImageDataWriter.h"

// C++ includes
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

//#define ENABLE_IO

#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]

namespace
{

//------------------------------------------------------------------------------
//  GLOBAL DATA
//------------------------------------------------------------------------------
const int NumPatches = 4;

const int NumLevels = 2;

const int BlocksPerLevel[2] = {2,2};

// AMR patches are defined as a 7-tuple consisting of the following:
// (level,imin,imax,jmin,jmax,kmin,kmax)
// Where imin,imax,jmin,jmax,kmin,kmax are defined w.r.t. a virtual grid
// that covers the entire domain at level 0.
static int Patches[4][7] = {
    {0,0,2,0,5,0,5},
    {0,2,5,0,5,0,5},
    {1,1,4,2,4,0,5},
    {1,2,4,0,2,0,5}
};

// Define the number of dimensions for the root level virtual grid
// The domain is assumed to be square [NDIM x NDIM x NDIM]
const int NDIM = 6;

// Define uniform grid spacing at level 0
const double h0 = 1.0;

// Global origin
const double origin[3] = {0.0,0.0,0.0};

#ifdef ENABLE_IO
//------------------------------------------------------------------------------
void WriteGrid(vtkUniformGrid *grid, std::string prefix)
{
  assert("pre: grid is NULL" && (grid != NULL) );

  vtkXMLImageDataWriter *writer = vtkXMLImageDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str().c_str() );
  writer->SetInputData( grid );
  writer->Write();
  writer->Delete();
}
#endif

//------------------------------------------------------------------------------
void GetPoint(
  const int i, const int j, const int k, double h[3], double pnt[3])
{
  pnt[0] = origin[0] + i*h[0];
  pnt[1] = origin[1] + j*h[1];
  pnt[2] = origin[2] + k*h[2];
}

//------------------------------------------------------------------------------
void GetGridExtent(
    const int blockIdx, const int dim, const int ratio, int ext[6])
{
  assert("pre: block index is out-of-bounds" &&
         (blockIdx >= 0) && (blockIdx < NumPatches) );
  assert("pre: dimensino is out-of-bounds" &&
         (dim >= 2) && (dim <= 3) );


  // STEP 0: Initialize the extent
  for(int i=0; i < 6; ++i)
    {
    ext[i]=0;
    }

  int level      = Patches[blockIdx][0];
  int *patchBase = &Patches[blockIdx][1];
  if( level == 0 )
    {
    for(int i=0; i < dim; ++i)
      {
      ext[i*2]   = patchBase[i*2];
      ext[i*2+1] = patchBase[i*2+1];
      } // END for all dimensions
    } // END if level is zero
  else
    {
    for(int i=0; i < dim; ++i)
      {
      ext[i*2]   = (level*ratio) * patchBase[i*2];
      ext[i*2+1] = (level*ratio) * patchBase[i*2+1];
      } // END for all dimensions
    } // END else
}

//------------------------------------------------------------------------------
void WriteAMR(vtkOverlappingAMR *amr, std::string prefix)
{
#ifdef ENABLE_IO
  std::ostringstream oss;
  oss.clear();
  unsigned int levelIdx = 0;
  for( ;levelIdx < amr->GetNumberOfLevels(); ++levelIdx)
    {
    unsigned int dataIdx = 0;
    for( ;dataIdx < amr->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      oss.str("");
      oss << prefix << "-L" << levelIdx << "-G" << dataIdx;
      if( amr->GetDataSet(levelIdx,dataIdx) != NULL )
        {
        WriteGrid(amr->GetDataSet(levelIdx,dataIdx), oss.str() );
        } // END if grid is not NULL
      } // END for all data
    } // END for all levels
#else
  /* silences some compiler warnings */
  static_cast<void>(amr);
  static_cast<void>(prefix);
#endif
}

//------------------------------------------------------------------------------
void AttachCellBlanking(vtkOverlappingAMR *amr)
{
  assert("pre: input amr dataset is NULL" && (amr != NULL) );

  unsigned int levelIdx = 0;
  for( ;levelIdx < amr->GetNumberOfLevels(); ++levelIdx )
    {
    unsigned int dataIdx = 0;
    for( ;dataIdx < amr->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      vtkUniformGrid *grid = amr->GetDataSet(levelIdx,dataIdx);
      if( grid != NULL )
        {
        vtkIdType numCells = grid->GetNumberOfCells();

        vtkUnsignedCharArray *ghostArray = grid->GetCellVisibilityArray();
        assert("pre: cell visibility is NULL" && (ghostArray != NULL) );
        unsigned char *ghostptr = ghostArray->GetPointer(0);
        assert("pre: ghostptr is NULL!" && (ghostptr != NULL) );

        vtkIntArray *blanking = vtkIntArray::New();
        blanking->SetName( "BLANKING" );
        blanking->SetNumberOfComponents(1);
        blanking->SetNumberOfTuples(numCells);

        int *iblank = blanking->GetPointer(0);
        assert("pre: iblank array pointer is NULL" && (iblank != NULL) );

        for(vtkIdType cellIdx=0; cellIdx < numCells; ++cellIdx)
          {
          if(vtkGhostArray::IsPropertySet(
              ghostptr[cellIdx],vtkGhostArray::BLANK))
            {
            iblank[cellIdx] = 0;
            }
          else
            {
            iblank[cellIdx] = 1;
            }
          } // END for all cells

        grid->GetCellData()->AddArray( blanking );
        blanking->Delete();
        } // END if grid != NULL
      } // END for all data
    } // END for all levels

}

//------------------------------------------------------------------------------
void ApplyXYZFieldToGrid( vtkUniformGrid *grd, std::string prefix )
{
  assert( "pre: grd should not be NULL" && (grd != NULL)  );

  // Get the grid's point-data and cell-data data-structures
  vtkCellData  *CD = grd->GetCellData();
  assert( "pre: Cell data is NULL" && (CD != NULL) );

  std::ostringstream oss;

  // Allocate arrays
  oss.str("");
  oss << prefix << "-CellXYZ";
  vtkDoubleArray *cellXYZArray = vtkDoubleArray::New();
  cellXYZArray->SetName( oss.str().c_str() );
  cellXYZArray->SetNumberOfComponents( 3 );
  cellXYZArray->SetNumberOfTuples( grd->GetNumberOfCells() );


  oss.str("");
  oss << prefix << "-NodeXYZ";
  vtkDoubleArray *nodeXYZArray = vtkDoubleArray::New();
  nodeXYZArray->SetName( oss.str().c_str() );
  nodeXYZArray->SetNumberOfComponents( 3 );
  nodeXYZArray->SetNumberOfTuples( grd->GetNumberOfPoints() );

  // Compute field arrays
  std::set< vtkIdType > visited;
  for( vtkIdType cellIdx=0; cellIdx < grd->GetNumberOfCells(); ++cellIdx )
    {
    vtkCell *c = grd->GetCell( cellIdx );
    assert( "pre: cell is not NULL" && (c != NULL) );

    double centroid[3];
    double xsum = 0.0;
    double ysum = 0.0;
    double zsum = 0.0;

    for( vtkIdType node=0; node < c->GetNumberOfPoints(); ++node )
      {
      double xyz[3];

      vtkIdType meshPntIdx = c->GetPointId( node );
      grd->GetPoint(  meshPntIdx, xyz );
      xsum += xyz[0];
      ysum += xyz[1];
      zsum += xyz[2];

      if( visited.find( meshPntIdx ) == visited.end() )
        {
        visited.insert( meshPntIdx );


        nodeXYZArray->SetComponent( meshPntIdx, 0, xyz[0] );
        nodeXYZArray->SetComponent( meshPntIdx, 1, xyz[1] );
        nodeXYZArray->SetComponent( meshPntIdx, 2, xyz[2] );
        } // END if
      } // END for all nodes

    centroid[0] = xsum / c->GetNumberOfPoints();
    centroid[1] = ysum / c->GetNumberOfPoints();
    centroid[2] = zsum / c->GetNumberOfPoints();

    cellXYZArray->SetComponent( cellIdx, 0, centroid[0] );
    cellXYZArray->SetComponent( cellIdx, 1, centroid[1] );
    cellXYZArray->SetComponent( cellIdx, 2, centroid[2] );
    } // END for all cells

  // Insert field arrays to grid point/cell data
  CD->AddArray( cellXYZArray );
  cellXYZArray->Delete();

// For now we are dealing only with cell-centered AMR
//  PD->AddArray( nodeXYZArray );
  nodeXYZArray->Delete();
}

//------------------------------------------------------------------------------
vtkUniformGrid* GetGrid(
    double gridOrigin[3], double h[3], int ndim[3])
{
  vtkUniformGrid *grid = vtkUniformGrid::New();
  grid->Initialize();
  grid->SetOrigin(gridOrigin);
  grid->SetSpacing(h);
  grid->SetDimensions(ndim);
  ApplyXYZFieldToGrid(grid,"INITIAL");
  return(grid);
}

//------------------------------------------------------------------------------
vtkUniformGrid* RefinePatch(
    vtkUniformGrid* vgrid,int level, int dim, int patchExtent[6], int ratio)
{
  assert("pre: parent is NULL" && (vgrid != NULL) );

  int ext[6];
  vgrid->GetExtent(ext);
  assert("pre: patchExtent must be within the parent extent!" &&
         vtkStructuredExtent::Smaller(patchExtent,ext));

  double min[3];
  double max[3];
  double h[3];
  int ndim[3];

  // Set some nominal values to ensure proper initialization
  ndim[0] = ndim[1] = ndim[2] = 1;
  min[0]  = min[1]  = min[2]  =
  max[0]  = max[1]  = max[2]  = 0.0;
  h[0]    = h[1]    = h[2]    = 0.5;

  // STEP 0: Get min
  int minIJK[3];
  minIJK[0] = patchExtent[0];
  minIJK[1] = patchExtent[2];
  minIJK[2] = patchExtent[4];
  vtkIdType minIdx = vtkStructuredData::ComputePointIdForExtent(ext,minIJK);
  vgrid->GetPoint( minIdx, min );

  // STEP 1: Get max
  int maxIJK[3];
  maxIJK[0] = patchExtent[1];
  maxIJK[1] = patchExtent[3];
  maxIJK[2] = patchExtent[5];
  vtkIdType maxIdx = vtkStructuredData::ComputePointIdForExtent(ext,maxIJK);
  vgrid->GetPoint( maxIdx, max );

  int patchdims[3];
  patchdims[0] = patchExtent[1]-patchExtent[0]+1;
  patchdims[1] = patchExtent[3]-patchExtent[2]+1;
  patchdims[2] = patchExtent[5]-patchExtent[4]+1;

  // STEP 2: Compute the spacing of the refined patch and its dimensions
  if( level == 0 )
    {
    for( int i=0; i < dim; ++i )
      {
      h[i]    = h0;
      ndim[i] = patchdims[i];
      } // END for all dimensions
    } // END if
  else
    {
    for( int i=0; i < dim; ++i )
      {
      int r   = level*ratio;
      h[i]    = h0/static_cast<double>(r);
      ndim[i] = (level*r)*patchdims[i]-(r-1);
      } // END for all dimensions
    } // END else


  // STEP 3: Construct uniform grid for requested patch
  vtkUniformGrid *grid = vtkUniformGrid::New();
  grid->Initialize();
  grid->SetOrigin( min );
  grid->SetSpacing( h );
  grid->SetDimensions(ndim);

  // STEP 4: Compute cell/node field on patch
  ApplyXYZFieldToGrid(grid,"INITIAL");
  return(grid);
}

//------------------------------------------------------------------------------
void Get2DAMRData(vtkOverlappingAMR* amrData, int ratio)
{
  assert("pre: input AMR Data is NULL" && (amrData != NULL) );
  assert("pre: input AMR Data is NULL" && (ratio >= 2) );

  amrData->Initialize(NumLevels, const_cast<int*>(BlocksPerLevel));

  // Root virtual block at level 0
  double h[3] = {h0,h0,h0};
  int vdim[3]  = {NDIM,NDIM,NDIM};
  vtkUniformGrid *vgrid = GetGrid(
      const_cast<double*>(origin), const_cast<double*>(h), vdim);
  assert("pre: virtual grid is NULL" && (vgrid != NULL) );

  vtkUniformGrid *refinedPatch = NULL;
  int idxAtLevel[NumLevels] = {0,0};
  for( int patchIdx=0; patchIdx < NumPatches; ++patchIdx )
    {
    int patchLevel = Patches[patchIdx][0];
    int *patch     = &Patches[patchIdx][1];
    refinedPatch   = RefinePatch(vgrid,patchLevel,2,patch,ratio);
    assert("pre: refined grid is NULL" && (refinedPatch != NULL) );
    amrData->SetDataSet(patchLevel,idxAtLevel[patchLevel],refinedPatch);
    idxAtLevel[patchLevel]++;
    refinedPatch->Delete();
    refinedPatch = NULL;
    }
  vgrid->Delete();
}

//------------------------------------------------------------------------------
void Get3DAMRData(vtkOverlappingAMR* amrData, int ratio)
{
  assert("pre: input AMR Data is NULL" && (amrData != NULL) );
  assert("pre: input AMR Data is NULL" && (ratio >= 2) );

  amrData->Initialize(NumLevels, const_cast<int*>(BlocksPerLevel));

  // Root virtual block at level 0
  double h[3] = {h0,h0,h0};
  int vdim[3]  = {NDIM,NDIM,NDIM};
  vtkUniformGrid *vgrid = GetGrid(
      const_cast<double*>(origin), const_cast<double*>(h), vdim);
  assert("pre: virtual grid is NULL" && (vgrid != NULL) );

  vtkUniformGrid *refinedPatch = NULL;
  int idxAtLevel[NumLevels] = {0,0};
  for( int patchIdx=0; patchIdx < NumPatches; ++patchIdx )
    {
    int patchLevel = Patches[patchIdx][0];
    int *patch     = &Patches[patchIdx][1];
    refinedPatch   = RefinePatch(vgrid,patchLevel,3,patch,ratio);
    assert("pre: refined grid is NULL" && (refinedPatch != NULL) );
    amrData->SetDataSet(patchLevel,idxAtLevel[patchLevel],refinedPatch);
    idxAtLevel[patchLevel]++;
    refinedPatch->Delete();
    refinedPatch = NULL;
    }
  vgrid->Delete();
}

//------------------------------------------------------------------------------
void RegisterGrids(
    vtkOverlappingAMR *amr,
    int dim, int ratio,
    vtkStructuredAMRGridConnectivity *gridConnectivity)
{
  assert( "pre: input AMR data should not be NULL" && (amr != NULL) );
  assert( "pre: gridConnectivity object should not be NULL" &&
          (gridConnectivity != NULL) );
  assert( "pre: dimension should be 2 or 3" && (dim >= 2) && (dim <= 3)  );
  assert( "pre: refinement ratio should be >= 2" && (ratio >= 2) );

  gridConnectivity->SetNodeCentered(false);
  gridConnectivity->SetCellCentered(true);
  gridConnectivity->Initialize(
      amr->GetNumberOfLevels(),amr->GetTotalNumberOfBlocks(),ratio);

  unsigned int levelIdx = 0;
  int ext[6];
  for(;levelIdx < amr->GetNumberOfLevels(); ++levelIdx)
    {
    unsigned int dataIdx=0;
    for(;dataIdx < amr->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      int idx = amr->GetCompositeIndex(levelIdx,dataIdx);
      vtkUniformGrid *grid = amr->GetDataSet(levelIdx,dataIdx);
      if( grid != NULL )
        {
        GetGridExtent(idx,dim,ratio,ext);
        gridConnectivity->RegisterGrid(
            idx,levelIdx,ext,
            grid->GetPointVisibilityArray(),
            grid->GetCellVisibilityArray(),
            grid->GetPointData(),
            grid->GetCellData(),
            NULL);
        }

      } // END for all data
    } // END for all levels
}

//------------------------------------------------------------------------------
void GetGhostedAMRData(
    vtkOverlappingAMR *amr,
    vtkStructuredAMRGridConnectivity *amrConnectivity,
    vtkOverlappingAMR *ghostedAMR)
{
  assert("pre: AMR is NULL" && (amr != NULL) );
  assert("pre: AMR grid connectivity is NULL" && (amrConnectivity != NULL) );
  assert("pre: Ghosted AMR is NULL" && (ghostedAMR != NULL) );
  std::vector<int> blocksPerLevel;
  for(unsigned int i=0; i<amr->GetNumberOfLevels();i++)
    {
    blocksPerLevel.push_back(amr->GetNumberOfDataSets(i));
    }
  ghostedAMR->Initialize(
      static_cast<int>(blocksPerLevel.size()), &blocksPerLevel[0]);

  unsigned int levelIdx=0;
  for( ;levelIdx < amr->GetNumberOfLevels(); ++levelIdx)
    {
    unsigned int dataIdx=0;
    for(;dataIdx < amr->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      int linearIdx        = amr->GetCompositeIndex(levelIdx,dataIdx);
      vtkUniformGrid *grid = amr->GetDataSet(levelIdx,dataIdx);

      if( grid != NULL )
        {
        int ghostedExtent[6];
        amrConnectivity->GetGhostedExtent(linearIdx,ghostedExtent);

        double gridOrigin[3];
        GetPoint(
            IMIN(ghostedExtent),JMIN(ghostedExtent),KMIN(ghostedExtent),
            grid->GetSpacing(),gridOrigin);

        int dims[3];
        vtkStructuredData::GetDimensionsFromExtent(ghostedExtent,dims);

        vtkUniformGrid *ghostedGrid = vtkUniformGrid::New();
        ghostedGrid->Initialize();
        ghostedGrid->SetOrigin(gridOrigin);
        ghostedGrid->SetSpacing( grid->GetSpacing() );
        ghostedGrid->SetDimensions(dims);
        ghostedGrid->GetCellData()->ShallowCopy(
            amrConnectivity->GetGhostedGridCellData(linearIdx));

        ghostedAMR->SetDataSet(levelIdx,dataIdx,ghostedGrid);
        ghostedGrid->Delete();
        } // END if grid is not null
      else
        {
        ghostedAMR->SetDataSet(levelIdx,dataIdx,NULL);
        } // END else

      } // END for all data
    } // END for all levels
}

//------------------------------------------------------------------------------
int Test2DAMR(const int ratio)
{
  std::cout << "==========================================\n";
  std::cout << "TESTING 2-D AMR  REFINEMENT RATIO=" << ratio << std::endl;
  std::cout.flush();

  int rc = 0;

  // STEP 0: Get the AMR data
  vtkOverlappingAMR *amr = vtkOverlappingAMR::New();
  Get2DAMRData(amr,ratio);
  assert("post Total number of blocks mismatch!" &&
         (static_cast<int>(amr->GetTotalNumberOfBlocks())==NumPatches));
  WriteAMR(amr, "AMR2D-INITIAL");

  // STEP 1: Register grids
  vtkStructuredAMRGridConnectivity *amrGridConnectivity =
       vtkStructuredAMRGridConnectivity::New();
  RegisterGrids(amr,2,ratio,amrGridConnectivity);

  // STEP 2: Compute Neighbors
  amrGridConnectivity->ComputeNeighbors();
  amrGridConnectivity->Print( std::cout );

  // STEP 3: Attach blank cell arrays
  AttachCellBlanking(amr);
  WriteAMR(amr,"AMR2D-BLANKED");

  // STEP 4: Create ghost-layers
  std::cout << "Ghosting...\n";
  std::cout.flush();
  amrGridConnectivity->CreateGhostLayers( 1 );
  amrGridConnectivity->Print(std::cout);
  std::cout << "[DONE]\n";
  std::cout.flush();

  // STEP 5: Get & Write ghosted grids
  vtkOverlappingAMR *ghostedAMR = vtkOverlappingAMR::New();
  GetGhostedAMRData(amr,amrGridConnectivity,ghostedAMR);
  WriteAMR(ghostedAMR,"AMR2D-GHOSTED");

  // STEP 6: De-allocate
  amrGridConnectivity->Delete();
  amr->Delete();
  ghostedAMR->Delete();
  return( rc );
}

//------------------------------------------------------------------------------
int Test3DAMR(const int ratio)
{
  std::cout << "==========================================\n";
  std::cout << "TESTING 3-D AMR  REFINEMENT RATIO=" << ratio << std::endl;
  std::cout.flush();

  int rc = 0;

  // STEP 0: Get the AMR data
  vtkOverlappingAMR *amr = vtkOverlappingAMR::New();
  Get3DAMRData(amr,ratio);
  assert("post Total number of blocks mismatch!" &&
         (static_cast<int>(amr->GetTotalNumberOfBlocks())==NumPatches));
  WriteAMR(amr, "AMR3D-INITIAL");

  // STEP 1: Register grids
  vtkStructuredAMRGridConnectivity *amrGridConnectivity =
      vtkStructuredAMRGridConnectivity::New();
  RegisterGrids(amr,3,ratio,amrGridConnectivity);

  // STEP 2: Compute Neighbors
  amrGridConnectivity->ComputeNeighbors();
  amrGridConnectivity->Print( std::cout );

  // STEP 3: Attach blank cell arrays
  AttachCellBlanking(amr);
  WriteAMR(amr,"AMR3D-BLANKED");

  // STEP 4: Create ghost-layers
  std::cout << "Ghosting...\n";
  std::cout.flush();
  amrGridConnectivity->CreateGhostLayers( 1 );
  amrGridConnectivity->Print(std::cout);
  std::cout << "[DONE]\n";
  std::cout.flush();

  // STEP 5: Get & Write ghosted grids
  vtkOverlappingAMR *ghostedAMR = vtkOverlappingAMR::New();
  GetGhostedAMRData(amr,amrGridConnectivity,ghostedAMR);
  WriteAMR(ghostedAMR,"AMR3D-GHOSTED");

  // STEP 6: De-allocate
  amrGridConnectivity->Delete();
  amr->Delete();
  ghostedAMR->Delete();
  return( rc );
}

//------------------------------------------------------------------------------
int TestStructuredAMRGridConnectivity_internal(int argc, char *argv[])
{
  // STEP 0: Silence some compiler warnings here
  static_cast<void>(argc);
  static_cast<void>(argv);

  int nRatios  = 3;
  int ratios[] = {2, 3, 4};
  int rc       = 0;

  // STEP 1: Loop through are refinement ratios
  for( int i=0; i < nRatios; ++i )
    {
    rc += Test2DAMR(ratios[i]);
    rc += Test3DAMR(ratios[i]);
    } // END for all refinement ratios

  return( rc );
}

//------------------------------------------------------------------------------
int TestSimpleAMRGridConnectivity(int vtkNotUsed(argc), char *argv[])
{
  int rc    = 0;
  int dim   = atoi(argv[1]);
  int ratio = atoi(argv[2]);

  switch( dim )
    {
    case 2:
      rc += Test2DAMR( ratio );
      break;
    case 3:
      rc += Test3DAMR( ratio );
      break;
    default:
      std::cerr << "ERROR: Only dimension of 2 and 3 is handled" << std::endl;
      rc = -1;
    }
  return( rc );
}

}

//------------------------------------------------------------------------------
int TestStructuredAMRGridConnectivity(int argc, char *argv[])
{
  int rc = 0;
  if( argc > 1 )
    {
    rc += TestSimpleAMRGridConnectivity(argc,argv);
    }
  else
    {
    rc += TestStructuredAMRGridConnectivity_internal(argc, argv);
    }

  return( rc );
}
