// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME TestAMRGhostLayerStripping.cxx -- Test for stripping ghost layers
//
// .SECTION Description
// A simple test for testing the functionality of stripping out ghost layers
// that partially cover lower resolution cells. The test constructs an AMR
// configuration using the vtkAMRGaussianPulseSource which has a known structure.
// Ghost layers are manually added to the hi-res grids and then stripped out.
// Tests cover also configurations with different refinement ratios and
// different numbers of ghost-layers.

// VTK includes
#include "vtkAMRBox.h"
#include "vtkAMRGaussianPulseSource.h"
#include "vtkAMRUtilities.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkMathUtilities.h"
#include "vtkOverlappingAMR.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRIterator.h"

// C/C++ includes
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

// #define DEBUG_ON

//------------------------------------------------------------------------------
// Debugging utilities. Must link vtkIOXML to work
#ifdef DEBUG_ON
#include "vtkXMLImageDataWriter.h"
void WriteUniformGrid(vtkUniformGrid* g, const std::string& prefix)
{
  assert("pre: Uniform grid (g) is nullptr!" && (g != nullptr));

  vtkXMLImageDataWriter* imgWriter = vtkXMLImageDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << imgWriter->GetDefaultFileExtension();
  imgWriter->SetFileName(oss.str().c_str());
  imgWriter->SetInputData(g);
  imgWriter->Write();

  imgWriter->Delete();
}
//------------------------------------------------------------------------------
void WriteUnGhostedGrids(const int dimension, vtkOverlappingAMR* amr)
{
  assert("pre: AMR dataset is nullptr!" && (amr != nullptr));

  std::ostringstream oss;
  oss.clear();
  unsigned int levelIdx = 0;
  for (; levelIdx < amr->GetNumberOfLevels(); ++levelIdx)
  {
    unsigned dataIdx = 0;
    for (; dataIdx < amr->GetNumberOfBlocks(levelIdx); ++dataIdx)
    {
      vtkUniformGrid* grid = amr->GetDataSet(levelIdx, dataIdx);
      if (grid != nullptr)
      {
        oss.str("");
        oss << dimension << "D_UNGHOSTED_GRID_" << levelIdx << "_" << dataIdx;
        WriteUniformGrid(grid, oss.str());
      }
    } // END for all data-sets
  }   // END for all levels
}

#endif

//------------------------------------------------------------------------------
double ComputePulse(const int dimension, double location[3], double pulseOrigin[3],
  double pulseWidth[3], double pulseAmplitude)
{
  double pulse = 0.0;

  double r = 0.0;
  for (int i = 0; i < dimension; ++i)
  {
    double d = location[i] - pulseOrigin[i];
    double d2 = d * d;
    double L2 = pulseWidth[i] * pulseWidth[i];
    r += d2 / L2;
  } // END for all dimensions
  pulse = pulseAmplitude * std::exp(-r);

  return (pulse);
}

//------------------------------------------------------------------------------
void GetCell(vtkUniformGrid* grid, vtkIdType cellIdx, vtkGenericCell* cell)
{
  const auto gridDims = grid->GetDataDimension();
  VTKCellType cellType;
  switch (gridDims)
  {
    case 3:
      cellType = VTK_VOXEL;
      break;
    case 2:
      cellType = VTK_PIXEL;
      break;
    case 1:
      cellType = VTK_LINE;
      break;
    default:
      cellType = VTK_VERTEX;
  }
  // vtkImageData not checks for visibility of cells, so we need to do it manually
  cell->SetCellType(cellType);
  grid->GetCellPoints(cellIdx, cell->PointIds);
  for (int i = 0; i < cell->PointIds->GetNumberOfIds(); ++i)
  {
    cell->Points->SetPoint(i, grid->GetPoint(cell->PointIds->GetId(i)));
  }
}

//------------------------------------------------------------------------------
void ComputeCellCenter(
  vtkUniformGrid* grid, vtkIdType cellIdx, vtkGenericCell* cell, double centroid[3])
{
  assert("pre: input grid instance is nullptr" && (grid != nullptr));
  assert(
    "pre: cell index is out-of-bounds!" && (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells()));

  // We want to get all cells including blanked cells.
  GetCell(grid, cellIdx, cell);

  double pcenter[3];
  std::vector<double> weights(cell->GetNumberOfPoints());
  int subId = cell->GetParametricCenter(pcenter);
  cell->EvaluateLocation(subId, pcenter, centroid, weights.data());
}

//------------------------------------------------------------------------------
void GeneratePulseField(const int dimension, vtkUniformGrid* grid)
{
  assert("pre: grid is nullptr!" && (grid != nullptr));
  assert("pre: grid is empty!" && (grid->GetNumberOfCells() >= 1));

  double pulseOrigin[3];
  double pulseWidth[3];
  double pulseAmplitude;

  vtkAMRGaussianPulseSource* pulseSource = vtkAMRGaussianPulseSource::New();
  pulseSource->GetPulseOrigin(pulseOrigin);
  pulseSource->GetPulseWidth(pulseWidth);
  pulseAmplitude = pulseSource->GetPulseAmplitude();
  pulseSource->Delete();

  vtkDoubleArray* centroidArray = vtkDoubleArray::New();
  centroidArray->SetName("Centroid");
  centroidArray->SetNumberOfComponents(3);
  centroidArray->SetNumberOfTuples(grid->GetNumberOfCells());

  vtkDoubleArray* pulseField = vtkDoubleArray::New();
  pulseField->SetName("Gaussian-Pulse");
  pulseField->SetNumberOfComponents(1);
  pulseField->SetNumberOfTuples(grid->GetNumberOfCells());

  double centroid[3];
  vtkIdType cellIdx = 0;
  vtkNew<vtkGenericCell> cell;
  for (; cellIdx < grid->GetNumberOfCells(); ++cellIdx)
  {
    ComputeCellCenter(grid, cellIdx, cell, centroid);
    centroidArray->SetComponent(cellIdx, 0, centroid[0]);
    centroidArray->SetComponent(cellIdx, 1, centroid[1]);
    centroidArray->SetComponent(cellIdx, 2, centroid[2]);

    double pulse = ComputePulse(dimension, centroid, pulseOrigin, pulseWidth, pulseAmplitude);
    pulseField->SetComponent(cellIdx, 0, pulse);
  } // END for all cells

  grid->GetCellData()->AddArray(centroidArray);
  centroidArray->Delete();
  grid->GetCellData()->AddArray(pulseField);
  pulseField->Delete();
}

//------------------------------------------------------------------------------
vtkUniformGrid* GetGhostedGrid(
  const int dimension, vtkUniformGrid* refGrid, int ghost[6], const int NG)
{
  assert("pre: NG >= 1" && (NG >= 1));

  // STEP 0: If the reference grid is nullptr just return
  if (refGrid == nullptr)
  {
    return nullptr;
  }

  // STEP 1: Acquire reference grid origin,spacing, dims
  int dims[3];
  double origin[3];
  double spacing[3];
  refGrid->GetOrigin(origin);
  refGrid->GetSpacing(spacing);
  refGrid->GetDimensions(dims);

  // STEP 2: Adjust origin and dimensions for ghost cells along each dimension
  for (int i = 0; i < 3; ++i)
  {
    if (ghost[i * 2] == 1)
    {
      // Grow along min of dimension i
      dims[i] += NG;
      origin[i] -= NG * spacing[i];
    }
    if (ghost[i * 2 + 1] == 1)
    {
      // Grow along max of dimension i
      dims[i] += NG;
    }
  } // END for all dimensions

  // STEP 3: Construct ghosted grid
  vtkUniformGrid* grid = vtkUniformGrid::New();
  grid->Initialize();
  grid->SetOrigin(origin);
  grid->SetSpacing(spacing);
  grid->SetDimensions(dims);

  // STEP 4: Construct field data, i.e., Centroid and Gaussian-Pulse. The
  // data is recomputed here, since we know how to compute it.
  GeneratePulseField(dimension, grid);

  return (grid);
}

//------------------------------------------------------------------------------
vtkOverlappingAMR* GetGhostedDataSet(const int dimension, const int NG, vtkOverlappingAMR* inputAMR)
{
  vtkOverlappingAMR* ghostedAMR = vtkOverlappingAMR::New();
  std::vector<unsigned int> blocksPerLevel(2);
  blocksPerLevel[0] = 1;
  blocksPerLevel[1] = 2;

  ghostedAMR->Initialize(blocksPerLevel);
  ghostedAMR->SetGridDescription(inputAMR->GetGridDescription());
  ghostedAMR->SetOrigin(inputAMR->GetOrigin());

  for (unsigned int i = 0; i < inputAMR->GetNumberOfLevels(); i++)
  {
    double spacing[3];
    inputAMR->GetSpacing(i, spacing);
    ghostedAMR->SetSpacing(i, spacing);
  }

  assert("pre: Expected number of levels is 2" && (ghostedAMR->GetNumberOfLevels() == 2));

  // Copy the root grid
  vtkUniformGrid* rootGrid = vtkUniformGrid::New();
  rootGrid->DeepCopy(vtkUniformGrid::SafeDownCast(inputAMR->GetDataSetAsCartesianGrid(0, 0)));
  vtkAMRBox box(rootGrid->GetOrigin(), rootGrid->GetDimensions(), rootGrid->GetSpacing(),
    ghostedAMR->GetOrigin(), rootGrid->GetDataDescription());
  ghostedAMR->SetAMRBox(0, 0, box);
  ghostedAMR->SetDataSet(0, 0, rootGrid);
  rootGrid->Delete();

  // Knowing the AMR configuration returned by vtkAMRGaussingPulseSource
  // we manually pad ghost-layers to the grids at level 1 (hi-res). How
  // ghost layers are created is encoded to a ghost vector for each grid,
  // {imin,imax,jmin,jmax,kmin,kmax}, where a value of "1" indicates that ghost
  // cells are created in that direction or a "0" to indicate that ghost cells
  // are not created in the given direction.
  int ghost[2][6] = {
    { 0, 1, 0, 1, 0, 0 }, // ghost vector for grid (1,0) -- grow at imax,jmax
    { 1, 0, 1, 0, 0, 0 }  // ghost vector for grid (1,1) -- grow at imin,jmin
  };

  for (int i = 0; i < 2; ++i)
  {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(inputAMR->GetDataSetAsCartesianGrid(1, i));
    vtkUniformGrid* ghostedGrid = GetGhostedGrid(dimension, grid, ghost[i], NG);
    box = vtkAMRBox(ghostedGrid->GetOrigin(), ghostedGrid->GetDimensions(),
      ghostedGrid->GetSpacing(), ghostedAMR->GetOrigin(), ghostedGrid->GetDataDescription());

    ghostedAMR->SetAMRBox(1, i, box);
    ghostedAMR->SetDataSet(1, i, ghostedGrid);

#ifdef DEBUG_ON
    std::ostringstream oss;
    oss.clear();
    oss.str("");
    oss << dimension << "D_GHOSTED_GRID_1_" << i;
    WriteUniformGrid(ghostedGrid, oss.str());
#endif

    ghostedGrid->Delete();
  } // END for all grids
  return (ghostedAMR);
}

//------------------------------------------------------------------------------
vtkOverlappingAMR* GetAMRDataSet(const int dimension, const int refinementRatio)
{
  vtkAMRGaussianPulseSource* amrGPSource = vtkAMRGaussianPulseSource::New();
  amrGPSource->SetDimension(dimension);
  amrGPSource->SetRefinementRatio(refinementRatio);
  amrGPSource->Update();
  vtkOverlappingAMR* myAMR = vtkOverlappingAMR::New();
  myAMR->CompositeShallowCopy(amrGPSource->GetOutput());
  amrGPSource->Delete();

  // Manually remove ghost array for easier comparison
  vtkSmartPointer<vtkUniformGridAMRIterator> iter =
    vtkSmartPointer<vtkUniformGridAMRIterator>::New();
  iter->SetDataSet(myAMR);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkImageData::SafeDownCast(iter->GetCurrentDataObject())
      ->GetCellData()
      ->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  }

  return (myAMR);
}

//------------------------------------------------------------------------------
bool AMRDataSetsAreEqual(vtkOverlappingAMR* computed, vtkOverlappingAMR* expected)
{
  assert("pre: computed AMR dataset is nullptr" && (computed != nullptr));
  assert("pre: expected AMR dataset is nullptr" && (expected != nullptr));

  if (computed == expected)
  {
    return true;
  }

  if (computed->GetNumberOfLevels() != expected->GetNumberOfLevels())
  {
    return false;
  }

  if (!(*computed->GetOverlappingAMRMetaData() == *expected->GetOverlappingAMRMetaData()))
  {

    std::cerr << "ERROR: AMR meta data mismatch!\n";
    return false;
  }

  unsigned int levelIdx = 0;
  for (; levelIdx < computed->GetNumberOfLevels(); ++levelIdx)
  {
    if (computed->GetNumberOfBlocks(levelIdx) != expected->GetNumberOfBlocks(levelIdx))
    {
      return false;
    }

    unsigned int dataIdx = 0;
    for (; dataIdx < computed->GetNumberOfBlocks(levelIdx); ++dataIdx)
    {
      vtkImageData* dataset = computed->GetDataSetAsImageData(levelIdx, dataIdx);
      vtkImageData* expectedDataset = expected->GetDataSetAsImageData(levelIdx, dataIdx);
      if (!vtkTestUtilities::CompareDataObjects(dataset, expectedDataset))
      {
        std::cerr << "Datasets does not match for level " << levelIdx << " dataset " << dataIdx
                  << std::endl;
        return EXIT_FAILURE;
      }
    } // END for all data
  }   // END for all levels

  return true;
}

//------------------------------------------------------------------------------
int TestGhostStripping(const int dimension, const int refinementRatio, const int NG)
{
  int rc = 0;
  std::cout << "====\n";
  std::cout << "Checking AMR data dim=" << dimension << " r=" << refinementRatio << " NG=" << NG
            << std::endl;
  std::cout.flush();

  // Get the non-ghosted dataset
  vtkOverlappingAMR* amrData = GetAMRDataSet(dimension, refinementRatio);
  assert("pre: amrData should not be nullptr!" && (amrData != nullptr));
  if (vtkAMRUtilities::HasPartiallyOverlappingGhostCells(amrData))
  {
    ++rc;
    std::cerr << "ERROR: erroneously detected partially overlapping "
              << "ghost cells on non-ghosted grid!\n";
  }

  // Get the ghosted dataset
  vtkOverlappingAMR* ghostedAMRData = GetGhostedDataSet(dimension, NG, amrData);
  assert("pre: ghosted AMR data is nullptr!" && (ghostedAMRData != nullptr));

  if (NG == refinementRatio)
  {
    // There are no partially overlapping ghost cells
    if (vtkAMRUtilities::HasPartiallyOverlappingGhostCells(ghostedAMRData))
    {
      ++rc;
      std::cerr << "ERROR: detected partially overlapping "
                << "ghost cells when there shouldn't be any!\n";
    }
  }
  else
  {
    if (!vtkAMRUtilities::HasPartiallyOverlappingGhostCells(ghostedAMRData))
    {
      ++rc;
      std::cerr << "ERROR: failed detection of partially overlapping "
                << "ghost cells!\n";
    }
  }

  vtkOverlappingAMR* strippedAMRData = vtkOverlappingAMR::New();
  vtkAMRUtilities::StripGhostLayers(ghostedAMRData, strippedAMRData);
#ifdef DEBUG_ON
  WriteUnGhostedGrids(dimension, strippedAMRData);
#endif

  // The strippedAMRData is expected to be exactly the same as the initial
  // unghosted AMR dataset
  if (!AMRDataSetsAreEqual(strippedAMRData, amrData))
  {
    ++rc;
    std::cerr << "ERROR: AMR data did not match expected data!\n";
  }

  amrData->Delete();
  ghostedAMRData->Delete();
  strippedAMRData->Delete();
  return (rc);
}
//------------------------------------------------------------------------------
int TestAMRGhostLayerStripping(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int rc = 0;
  int DIM0 = 2;
  int NDIM = 3;

  int NumberOfRefinmentRatios = 3;
  int rRatios[3] = { 2, 3, 4 };

  for (int dim = DIM0; dim <= NDIM; ++dim)
  {
    for (int r = 0; r < NumberOfRefinmentRatios; ++r)
    {
      for (int ng = 1; ng <= rRatios[r] - 1; ++ng)
      {
        rc += TestGhostStripping(dim, rRatios[r], ng);
      } // END for all ghost-layer tests
    }   // END for all refinementRatios to test
  }     // END for all dimensions to test

  return rc;
}
