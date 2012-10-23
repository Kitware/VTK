/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRGhostLayerStripping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestAMRGhostLayerStripping.cxx -- Test for stripping ghost layers
//
// .SECTION Description
// A simple test for testing the functionality of stripping out ghost layers
// that partially cover lower resolution cells. The test constructs an AMR
// configuration using the vtkAMRGaussianPulseSource which has a known structure.
// Ghost layers are manually added to the hi-res grids and then stripped out.
// Tests cover also configurations with different refinement ratios and
// different numbers of ghost-layers.

// C/C++ includes
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

// VTK includes
#include "vtkAMRGaussianPulseSource.h"
#include "vtkAMRUtilities.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMathUtilities.h"
#include "vtkOverlappingAMR.h"
#include "vtkAMRInformation.h"
#include "vtkUniformGrid.h"
//#define DEBUG_ON

//------------------------------------------------------------------------------
// Debugging utilites. Must link vtkIOXML to work
#ifdef DEBUG_ON
#include "vtkXMLImageDataWriter.h"
void WriteUniformGrid( vtkUniformGrid *g, std::string prefix )
{
  assert( "pre: Uniform grid (g) is NULL!" && (g != NULL) );

  vtkXMLImageDataWriter *imgWriter = vtkXMLImageDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << imgWriter->GetDefaultFileExtension();
  imgWriter->SetFileName( oss.str().c_str() );
  imgWriter->SetInputData( g );
  imgWriter->Write();

  imgWriter->Delete();
}
//------------------------------------------------------------------------------
void WriteUnGhostedGrids(
    const int dimension, vtkOverlappingAMR *amr)
{
  assert("pre: AMR dataset is NULL!" && (amr != NULL) );

  std::ostringstream oss;
  oss.clear();
  unsigned int levelIdx = 0;
  for(;levelIdx < amr->GetNumberOfLevels(); ++levelIdx )
    {
    unsigned dataIdx = 0;
    for(;dataIdx < amr->GetNumberOfDataSets(levelIdx); ++dataIdx )
      {
      vtkUniformGrid *grid = amr->GetDataSet(levelIdx,dataIdx);
      if( grid != NULL )
        {
        oss.str("");
        oss << dimension << "D_UNGHOSTED_GRID_" << levelIdx << "_" << dataIdx;
        WriteUniformGrid(grid,oss.str());
        }
      } // END for all data-sets
    } // END for all levels
}

#endif

//------------------------------------------------------------------------------
double ComputePulse(
    const int dimension,
    double location[3],
    double pulseOrigin[3],
    double pulseWidth[3],
    double pulseAmplitude)
{
  double pulse = 0.0;

  double r = 0.0;
  for( int i=0; i < dimension; ++i )
    {
    double d = location[i]-pulseOrigin[i];
    double d2 = d*d;
    double L2 = pulseWidth[i]*pulseWidth[i];
    r += d2/L2;
    } // END for all dimensions
  pulse = pulseAmplitude * std::exp( -r );

  return( pulse );
}

//------------------------------------------------------------------------------
void ComputeCellCenter(
    vtkUniformGrid *grid, vtkIdType cellIdx, double centroid[3])
{
  assert("pre: input grid instance is NULL" && (grid != NULL));
  assert("pre: cell index is out-of-bounds!" &&
         (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells()));

  vtkCell *myCell = grid->GetCell( cellIdx );
  assert( "ERROR: Cell is NULL" && (myCell != NULL) );

  // Work around for blanked cells. For blanked cells GetCell returns a
  // VTK_EMPTY_CELL! Therefore, to get the cell, we have to temporarily
  // unblank it and then blank it again.
  bool isCellBlanked = false;
  if( (myCell->GetCellType() == VTK_EMPTY_CELL) &&
      (!grid->IsCellVisible(cellIdx)) )
    {
    grid->UnBlankCell( cellIdx );
    myCell = grid->GetCell(cellIdx);
    isCellBlanked = true;
    }

  double pcenter[3];
  double *weights = new double[ myCell->GetNumberOfPoints() ];
  int subId = myCell->GetParametricCenter( pcenter );
  myCell->EvaluateLocation( subId, pcenter, centroid, weights );
  delete [] weights;

  if( isCellBlanked )
    {
    grid->BlankCell( cellIdx );
    }
}

//------------------------------------------------------------------------------
void GeneratePulseField(const int dimension,vtkUniformGrid* grid)
{
  assert("pre: grid is NULL!" && (grid != NULL));
  assert("pre: grid is empty!" && (grid->GetNumberOfCells() >= 1) );

  double pulseOrigin[3];
  double pulseWidth[3];
  double pulseAmplitude;

  vtkAMRGaussianPulseSource *pulseSource = vtkAMRGaussianPulseSource::New();
  pulseSource->GetPulseOrigin( pulseOrigin );
  pulseSource->GetPulseWidth( pulseWidth );
  pulseAmplitude = pulseSource->GetPulseAmplitude();
  pulseSource->Delete();

  vtkDoubleArray *centroidArray = vtkDoubleArray::New();
  centroidArray->SetName("Centroid");
  centroidArray->SetNumberOfComponents( 3 );
  centroidArray->SetNumberOfTuples( grid->GetNumberOfCells() );

  vtkDoubleArray *pulseField = vtkDoubleArray::New();
  pulseField->SetName( "Gaussian-Pulse" );
  pulseField->SetNumberOfComponents( 1 );
  pulseField->SetNumberOfTuples( grid->GetNumberOfCells() );

  double centroid[3];
  vtkIdType cellIdx = 0;
  for(; cellIdx < grid->GetNumberOfCells(); ++cellIdx )
    {
    ComputeCellCenter(grid,cellIdx,centroid);
    centroidArray->SetComponent(cellIdx,0,centroid[0]);
    centroidArray->SetComponent(cellIdx,1,centroid[1]);
    centroidArray->SetComponent(cellIdx,2,centroid[2]);

    double pulse = ComputePulse(
        dimension,centroid,pulseOrigin,pulseWidth,pulseAmplitude);
    pulseField->SetComponent(cellIdx,0,pulse);
    } // END for all cells

  grid->GetCellData()->AddArray( centroidArray );
  centroidArray->Delete();
  grid->GetCellData()->AddArray( pulseField );
  pulseField->Delete();
}

//------------------------------------------------------------------------------
vtkUniformGrid* GetGhostedGrid(
    const int dimension,vtkUniformGrid *refGrid, int ghost[6], const int NG)
{
  assert("pre: NG >= 1" && (NG >= 1) );

  // STEP 0: If the reference grid is NULL just return
  if( refGrid == NULL )
    {
    return NULL;
    }

  // STEP 1: Acquire reference grid origin,spacing, dims
  int dims[3];
  double origin[3];
  double spacing[3];
  refGrid->GetOrigin(origin);
  refGrid->GetSpacing(spacing);
  refGrid->GetDimensions(dims);

  // STEP 2: Adjust origin and dimensions for ghost cells along each dimension
  for( int i=0; i < 3; ++i )
    {
    if( ghost[i*2]==1 )
      {
      // Grow along min of dimension i
      dims[i]   += NG;
      origin[i] -= NG*spacing[i];
      }
    if( ghost[i*2+1]==1 )
      {
      // Grow along max of dimension i
      dims[i] += NG;
      }
    } // END for all dimensions

  // STEP 3: Construt ghosted grid
  vtkUniformGrid *grid = vtkUniformGrid::New();
  grid->Initialize();
  grid->SetOrigin( origin );
  grid->SetSpacing( spacing );
  grid->SetDimensions( dims );

  // STEP 4: Construct field data, i.e., Centroid and Gaussian-Pulse. The
  // data is recomputed here, since we know how to compute it.
  GeneratePulseField(dimension,grid);

  return( grid );
}

//------------------------------------------------------------------------------
vtkOverlappingAMR *GetGhostedDataSet(
    const int dimension, const int NG, vtkOverlappingAMR *inputAMR)
{
  vtkOverlappingAMR *ghostedAMR = vtkOverlappingAMR::New();
  std::vector<int> blocksPerLevel(2);
  blocksPerLevel[0]=1;
  blocksPerLevel[1]=2;

  ghostedAMR->Initialize(static_cast<int>(blocksPerLevel.size()), &blocksPerLevel[0]);
  ghostedAMR->SetGridDescription(inputAMR->GetGridDescription());
  ghostedAMR->SetOrigin(inputAMR->GetOrigin());

  for(unsigned int i=0; i<inputAMR->GetNumberOfLevels();i++)
    {
    double spacing[3];
    inputAMR->GetSpacing(i,spacing);
    ghostedAMR->SetSpacing(i,spacing);
    }

  assert( "pre: Expected number of levels is 2" &&
          (ghostedAMR->GetNumberOfLevels()==2));

  // Copy the root grid
  vtkUniformGrid *rootGrid = vtkUniformGrid::New();
  rootGrid->DeepCopy( inputAMR->GetDataSet(0,0) );
  vtkAMRBox box(rootGrid->GetOrigin(), rootGrid->GetDimensions(), rootGrid->GetSpacing(), ghostedAMR->GetOrigin(), rootGrid->GetGridDescription());
  ghostedAMR->SetAMRBox(0,0,box);
  ghostedAMR->SetDataSet(0,0,rootGrid);
  rootGrid->Delete();

  // Knowing the AMR configuration returned by vtkAMRGaussingPulseSource
  // we manually pad ghost-layers to the grids at level 1 (hi-res). How
  // ghost layers are created is encoded to a ghost vector for each grid,
  // {imin,imax,jmin,jmax,kmin,kmax}, where a value of "1" indicates that ghost
  // cells are created in that direction or a "0" to indicate that ghost cells
  // are not created in the given direction.
  int ghost[2][6] = {
      {0,1,0,1,0,0},  // ghost vector for grid (1,0) -- grow at imax,jmax
      {1,0,1,0,0,0}   // ghost vector for grid (1,1) -- grow at imin,jmin
  };

  for( int i=0; i < 2; ++i )
    {
    vtkUniformGrid *grid = inputAMR->GetDataSet(1,i);
    vtkUniformGrid *ghostedGrid = GetGhostedGrid(dimension,grid,ghost[i],NG);
    box = vtkAMRBox(ghostedGrid->GetOrigin(), ghostedGrid->GetDimensions(), ghostedGrid->GetSpacing(), ghostedAMR->GetOrigin(), ghostedGrid->GetGridDescription());

    ghostedAMR->SetAMRBox(1,i,box);
    ghostedAMR->SetDataSet(1,i,ghostedGrid);

#ifdef DEBUG_ON
    std::ostringstream oss;
    oss.clear();
    oss.str("");
    oss << dimension << "D_GHOSTED_GRID_1_" << i;
    WriteUniformGrid( ghostedGrid, oss.str() );
#endif

    ghostedGrid->Delete();
    } // END for all grids
  return( ghostedAMR );
}

//------------------------------------------------------------------------------
vtkOverlappingAMR *GetAMRDataSet(
    const int dimension, const int refinementRatio)
{
  vtkAMRGaussianPulseSource *amrGPSource = vtkAMRGaussianPulseSource::New();
  amrGPSource->SetDimension( dimension );
  amrGPSource->SetRefinementRatio( refinementRatio );
  amrGPSource->Update();
  vtkOverlappingAMR *myAMR = vtkOverlappingAMR::New();
  myAMR->ShallowCopy( amrGPSource->GetOutput() );
  amrGPSource->Delete();
  return( myAMR );
}

//------------------------------------------------------------------------------
bool CheckFields(vtkUniformGrid *grid)
{
  // Since we know exactly what the fields are, i.e., gaussian-pulse and
  // centroid, we manually check the grid for correctness.
  assert("pre: grid is NULL" && (grid != NULL) );
  vtkCellData *CD = grid->GetCellData();
  if( !CD->HasArray("Centroid") || !CD->HasArray("Gaussian-Pulse") )
    {
    return false;
    }

  vtkDoubleArray *centroidArray =
      vtkDoubleArray::SafeDownCast(CD->GetArray("Centroid"));
  assert("pre: centroid arrays is NULL!" && (centroidArray != NULL) );
  if( centroidArray->GetNumberOfComponents() != 3 )
    {
    return false;
    }
  double *centers = static_cast<double*>(centroidArray->GetVoidPointer(0));

  vtkDoubleArray *pulseArray =
      vtkDoubleArray::SafeDownCast(CD->GetArray("Gaussian-Pulse"));
  assert("pre: pulse array is NULL!" && (pulseArray != NULL) );
  if( pulseArray->GetNumberOfComponents() != 1)
    {
    return false;
    }
  double *pulses = static_cast<double*>(pulseArray->GetVoidPointer(0));

  // Get default pulse parameters
  double pulseOrigin[3];
  double pulseWidth[3];
  double pulseAmplitude;

  vtkAMRGaussianPulseSource *pulseSource = vtkAMRGaussianPulseSource::New();
  pulseSource->GetPulseOrigin( pulseOrigin );
  pulseSource->GetPulseWidth( pulseWidth );
  pulseAmplitude = pulseSource->GetPulseAmplitude();
  pulseSource->Delete();

  double centroid[3];
  int dim = grid->GetDataDimension();
  vtkIdType cellIdx = 0;
    for(; cellIdx < grid->GetNumberOfCells(); ++cellIdx)
    {
    ComputeCellCenter(grid,cellIdx,centroid);
    double val = ComputePulse(
        dim,centroid,pulseOrigin,pulseWidth,pulseAmplitude);

    if( !vtkMathUtilities::FuzzyCompare(val,pulses[cellIdx],1e-9) )
      {
      std::cerr << "ERROR: pulse data mismatch!\n";
      std::cerr << "expected=" << val << " computed=" << pulses[cellIdx];
      std::cerr << std::endl;
      return false;
      }
    if( !vtkMathUtilities::FuzzyCompare(centroid[0],centers[cellIdx*3])  ||
        !vtkMathUtilities::FuzzyCompare(centroid[1],centers[cellIdx*3+1]) ||
        !vtkMathUtilities::FuzzyCompare(centroid[2],centers[cellIdx*3+2])  )
      {
      std::cerr << "ERROR: centroid data mismatch!\n";
      return false;
      }
    }// END for all cells
  return true;
}

//------------------------------------------------------------------------------
bool AMRDataSetsAreEqual(
    vtkOverlappingAMR *computed, vtkOverlappingAMR *expected)
{
  assert("pre: computed AMR dataset is NULL" && (computed != NULL) );
  assert("pre: expected AMR dataset is NULL" && (expected != NULL) );

  if( computed == expected )
    {
    return true;
    }

  if( computed->GetNumberOfLevels() != expected->GetNumberOfLevels() )
    {
    return false;
    }

  if (! (*computed->GetAMRInfo()==*expected->GetAMRInfo()))
    {
    std::cerr << "ERROR: AMR data mismatch!\n";
    return false;
    }

  unsigned int levelIdx = 0;
  for(; levelIdx < computed->GetNumberOfLevels(); ++levelIdx )
    {
    if( computed->GetNumberOfDataSets(levelIdx) !=
         expected->GetNumberOfDataSets(levelIdx))
      {
      return false;
      }

    unsigned int dataIdx = 0;
    for(;dataIdx < computed->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      vtkUniformGrid *computedGrid = computed->GetDataSet(levelIdx,dataIdx);
      vtkUniformGrid *expectedGrid = expected->GetDataSet(levelIdx,dataIdx);

      for( int i=0; i < 3; ++i )
        {
        if( !vtkMathUtilities::FuzzyCompare(
              computedGrid->GetOrigin()[i],
              expectedGrid->GetOrigin()[i]) )
          {
          std::cerr << "ERROR: grid origin mismathc!\n";
          return false;
          }
        }// END for all dimensions

      if(!CheckFields(computedGrid) )
        {
        std::cerr << "ERROR: grid fields were not as expected!\n";
        return false;
        }
      }// END for all data
    } // END for all levels

  return true;
}

//------------------------------------------------------------------------------
int TestGhostStripping(
    const int dimension, const int refinementRatio, const int NG)
{
  int rc = 0;
  std::cout << "====\n";
  std::cout << "Checking AMR data dim=" << dimension
            << " r=" << refinementRatio
            << " NG=" << NG << std::endl;
  std::cout.flush();

  // Get the non-ghosted dataset
  vtkOverlappingAMR *amrData = GetAMRDataSet(dimension,refinementRatio);
  assert("pre: amrData should not be NULL!" && (amrData != NULL) );
  if(vtkAMRUtilities::HasPartiallyOverlappingGhostCells(amrData))
   {
   ++rc;
   std::cerr << "ERROR: erroneously detected partially overlapping "
             << "ghost cells on non-ghosted grid!\n";
   }

  // Get the ghosted dataset
  vtkOverlappingAMR *ghostedAMRData=GetGhostedDataSet(dimension,NG,amrData);
  assert("pre: ghosted AMR data is NULL!" && (ghostedAMRData != NULL) );

  if( NG == refinementRatio  )
    {
    // There are no partially overlapping ghost cells
    if(vtkAMRUtilities::HasPartiallyOverlappingGhostCells(
        ghostedAMRData))
      {
      ++rc;
      std::cerr << "ERROR: detected partially overlapping "
                << "ghost cells when there shouldn't be any!\n";
      }
    }
  else
    {
    if(!vtkAMRUtilities::HasPartiallyOverlappingGhostCells(
       ghostedAMRData))
      {
      ++rc;
      std::cerr << "ERROR: failed detection of partially overlapping "
                << "ghost cells!\n";
      }
    }

  vtkOverlappingAMR *strippedAMRData = vtkOverlappingAMR::New();
  vtkAMRUtilities::StripGhostLayers( ghostedAMRData, strippedAMRData );
#ifdef DEBUG_ON
  WriteUnGhostedGrids(dimension,strippedAMRData);
#endif

  // The strippedAMRData is expected to be exactly the same as the initial
  // unghosted AMR dataset
  if(!AMRDataSetsAreEqual(strippedAMRData,amrData) )
    {
    ++rc;
    std::cerr << "ERROR: AMR data did not match expected data!\n";
    }

  amrData->Delete();
  ghostedAMRData->Delete();
  strippedAMRData->Delete();
  return( rc );
}
//------------------------------------------------------------------------------
int TestAMRGhostLayerStripping(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int rc   = 0;
  int DIM0 = 2;
  int NDIM = 3;

  int NumberOfRefinmentRatios = 3;
  int rRatios[3] = { 2,3,4 };

  for( int dim=DIM0; dim <= NDIM; ++dim )
    {
    for( int r=0; r < NumberOfRefinmentRatios; ++r )
      {
      for( int ng=1; ng <= rRatios[r]-1; ++ng )
        {
        rc += TestGhostStripping(dim,rRatios[r],ng);
        } // END for all ghost-layer tests
      } // END for all refinementRatios to test
    } // END for all dimensions to test

  return rc;
}
