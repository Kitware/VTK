// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRandomHyperTreeGridSource.h"

#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include <cassert>
#include <sstream>

#include <numeric>

namespace
{

double colors[8][3] = { { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 1.0, 0.0 },
  { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.7, 0.3, 0.3 } };

bool ConstructScene(vtkRenderer* renderer, int numPieces)
{
  bool result = true;
  float maskedFraction = 1.0 / (2.0 * numPieces);
  for (int i = 0; i < numPieces; ++i)
  {
    vtkNew<vtkRandomHyperTreeGridSource> source;
    source->SetDimensions(5, 5, 2); // GridCell 4, 4, 1
    source->SetSeed(371399);
    source->SetSplitFraction(0.5);
    source->SetMaskedFraction(maskedFraction);
    source->Update();
    int nbChildrenPerNode = 8; // (branching factor = 2) ^ (dimension = 3)
    double errorMargin = 1.0 / nbChildrenPerNode;

    // fixed error tolerance
    if (source->GetActualMaskedCellFraction() > errorMargin + maskedFraction ||
      source->GetActualMaskedCellFraction() < maskedFraction - errorMargin)
    {
      std::cout << "The masked cell proportion is " << source->GetActualMaskedCellFraction()
                << " and it should around +/-" << errorMargin << " : " << maskedFraction
                << std::endl;
      result = false;
    }

    vtkNew<vtkHyperTreeGridGeometry> geom;
    geom->SetInputConnection(source->GetOutputPort());

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(geom->GetOutputPort());
    mapper->SetPiece(i);
    mapper->SetNumberOfPieces(numPieces);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetColor(colors[i]);

    renderer->AddActor(actor);
  }

  std::ostringstream labelStr;
  labelStr << "NumPieces: " << numPieces;

  vtkNew<vtkTextActor> label;
  label->SetInput(labelStr.str().c_str());
  label->GetTextProperty()->SetVerticalJustificationToBottom();
  label->GetTextProperty()->SetJustificationToCentered();
  label->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  label->GetPositionCoordinate()->SetValue(0.5, 0.);
  renderer->AddActor(label);
  return result;
}

// It turns out that intentionally in this test, we made sure that the spatial position
// (double) of the original cell of a HyperTree is the same as in the grid (unsigned int)
// of the HyperTreeGrid (cell scale 1 and originHTG).
// It is only in this particular context that it is possible to calculate the value of tree index
// from the coordinates.
vtkIdType ComputeTreeIndex(vtkHyperTreeGrid* htGrid, const std::vector<double>& originHTG,
  vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor, const unsigned int iC)
{
  vtkIdType crtTreeIndex = -1;
  const double* origin = supercursor->GetOrigin(iC);
  htGrid->GetIndexFromLevelZeroCoordinates(crtTreeIndex,
    static_cast<unsigned int>(origin[0] - originHTG[0]),
    static_cast<unsigned int>(origin[1] - originHTG[1]),
    static_cast<unsigned int>(origin[2] - originHTG[2]));
  return crtTreeIndex;
}

#ifndef NDEBUG
// An error has occurred. We list the current state here (in case the test is wrong).
bool GuruMeditation(const size_t& iTest, const std::string _comment, vtkHyperTreeGrid* htGrid,
  const std::vector<double>& originHTG, vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
{
  std::cerr << "GURU MEDITATION Test #" << iTest << "(" << _comment << "): Enexpected values"
            << std::endl;
  std::cerr << "  Expected values are ";
  for (unsigned int iC = 0; iC < supercursor->GetNumberOfCursors(); ++iC)
  {
    vtkHyperTree* ht = supercursor->GetTree(iC);
    if (ht != nullptr)
    {
      if (!supercursor->IsMasked(iC))
      {
        std::cerr << " " << ComputeTreeIndex(htGrid, originHTG, supercursor, iC) << ",";
      }
      else
      {
        std::cerr << " -1 (masked),";
      }
    }
    else
    {
      std::cerr << " -1,";
    }
  } // _c
  std::cerr << std::endl;

  return false;
}

bool GuruMeditation(const size_t& iTest, const std::string msg)
{
  std::cerr << "GURU MEDITATION Test #" << iTest << " : " << msg << std::endl;
  return false;
}

bool GuruMeditation(const std::string msg)
{
  std::cerr << "GURU MEDITATION " << msg << std::endl;
  return false;
}
#endif

// The source filter vtkRandomHyperTreeGridSource builds each hypertree in
// ascending order using implicit indexing. The assertion verifies this.
// This method set globalCellIndexByHT in order to record the global offset
// of each original cell index for each Hyper Tree.
// In addition, it sets the offset of the HyperTree to include the most cells.
void ComputeGlobalCellIndexByHTAndtreeIndexMaxNumberCell(vtkHyperTreeGrid* htGrid,
  std::vector<vtkIdType>& globalCellIndexByHT, vtkIdType& treeIndexMaxNumberCell)
{
  vtkIdType maxNumberCell = 0;
  vtkIdType indextree;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  htGrid->InitializeTreeIterator(it);
  while (vtkHyperTree* tree = it.GetNextTree(indextree))
  {
    globalCellIndexByHT.emplace_back(tree->GetGlobalIndexFromLocal(0));
    vtkIdType crtNumberCell = tree->GetNumberOfVertices();
    if (maxNumberCell <= crtNumberCell)
    {
      maxNumberCell = crtNumberCell;
      treeIndexMaxNumberCell = indextree;
    }
  } // it
}

void TripAll(vtkHyperTreeGrid* htGrid)
{
  // First pass across tree roots to evince cells intersected by contours
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  htGrid->InitializeTreeIterator(it);
  vtkIdType crtTreeIndex = 0;
  vtkIdType crtOffsetTree = 0;
  while (it.GetNextTree(crtTreeIndex)) // Masked or non
  {
    assert(crtTreeIndex == crtOffsetTree ||
      GuruMeditation("This missed cell " + std::to_string(crtOffsetTree) + " isn't possible."));
    ++crtOffsetTree;
  }
}

// Trip HTG tests
// htGrid : instance of HYperTreeGrid
// originHTG : coordinates origine of HyperTreeGrid
// inputTreeIndex :
//    particular case -1 : checked all index (iTest == 0)
//    0 to ... : checked neighbor index tree (iTest != 0)
// allTreeIndex : expected result neighbor index tree
// maskedRootCellsTree : indexs of cells masked
void Trip(vtkHyperTreeGrid* htGrid, const std::vector<double>& originHTG,
  const std::vector<vtkIdType>& inputTreeIndex,
#ifndef NDEBUG
  const std::vector<std::vector<vtkIdType>>& allTreeIndex,
  const std::vector<bool>& maskedRootCellsTree
#else
  const std::vector<std::vector<vtkIdType>>& vtkNotUsed(allTreeIndex),
  const std::vector<bool>& vtkNotUsed(maskedRootCellsTree)
#endif
)
{
  TripAll(htGrid);

  // Other pass across tree roots follow inputTreeIndex
  for (size_t iTest = 0; iTest < inputTreeIndex.size(); ++iTest)
  {
    vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> supercursor;

    htGrid->InitializeNonOrientedMooreSuperCursor(supercursor, inputTreeIndex[iTest]);

    if (supercursor->GetTree() != nullptr)
    {
      continue;
    }
    if (supercursor->IsMasked())
    {
#ifndef NDEBUG
      const vtkIdType crtTreeIndex =
#endif
        ComputeTreeIndex(htGrid, originHTG, supercursor, supercursor->GetIndiceCentralCursor());

      assert(maskedRootCellsTree[crtTreeIndex] ||
        GuruMeditation(iTest, "expected root cell masked", htGrid, originHTG, supercursor));
      continue;
    }
    for (unsigned int iC = 0; iC < supercursor->GetNumberOfCursors(); ++iC)
    {
      vtkHyperTree* ht = supercursor->GetTree(iC);
      if (ht != nullptr)
      {
        assert(supercursor->GetLevel(iC) == 0 ||
          GuruMeditation(iTest, "The level of origin cell of an HT is always 0 !"));
        assert(supercursor->GetGlobalNodeIndex(iC) <= htGrid->GetGlobalNodeIndexMax() ||
          GuruMeditation(iTest,
            "The global node index of origin cell of an HT is always lower and egal than global "
            "node index max !"));
        assert((htGrid->GetGlobalNodeIndexMax() + 1) == htGrid->GetNumberOfCells() ||
          GuruMeditation(
            iTest, "The global node index max more one is egal a number of cells, in this case !"));
#ifndef NDEBUG
        const vtkIdType crtTreeIndex =
#endif
          ComputeTreeIndex(htGrid, originHTG, supercursor, iC);
        if (!supercursor->IsMasked(iC))
        {
          assert(!maskedRootCellsTree[crtTreeIndex] ||
            GuruMeditation(iTest, "expected neighboor not masked", htGrid, originHTG, supercursor));

          assert(crtTreeIndex == allTreeIndex[iTest][iC] ||
            GuruMeditation(iTest, "expected neighboor value", htGrid, originHTG, supercursor));
        }
        else
        {
          assert(maskedRootCellsTree[crtTreeIndex] ||
            GuruMeditation(iTest, "expected neighboor masked", htGrid, originHTG, supercursor));
        }
      }
      else
      {
        assert(allTreeIndex[iTest][iC] == -1 ||
          GuruMeditation(iTest, "expected neighboor void", htGrid, originHTG, supercursor));
      }
    } // _c
  }
}

void TestHyperTreeGridGetShiftedLevelZeroIndex_3D()
{
  std::cout << "TestHyperTreeGridGetShiftedLevelZeroIndex_3D" << std::endl;
  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(5, 6, 7); // GridCell 4, 5, 6
  source->SetOutputBounds(1., 5., 1., 6., 1., 7.);
  source->SetSeed(42);
  source->SetSplitFraction(0.5);
  source->SetMaxDepth(2);

  const std::vector<double> originHTG{ 1., 1., 1. };

  source->Update();

  vtkHyperTreeGrid* const htGrid = vtkHyperTreeGrid::SafeDownCast(source->GetOutputDataObject(0));

  assert(htGrid->GetDimension() == 3 || GuruMeditation("Test for case 3D !"));

  vtkNew<vtkBitArray> mask;
  {
    const vtkIdType nbCells = htGrid->GetNumberOfCells();
    mask->SetNumberOfTuples(nbCells);
    for (vtkIdType iCell = 0; iCell < nbCells; ++iCell)
    {
      mask->SetValue(iCell, 0);
    }
    htGrid->SetMask(mask);
  }

  // Waited results
  vtkIdType nbTrees = 4 * 5 * 6;
  assert(htGrid->GetMaxNumberOfTrees() == nbTrees ||
    GuruMeditation("Invalid expected values nbTrees !"));

  std::vector<vtkIdType> inputTreeIndex;
  std::vector<std::vector<vtkIdType>> allTreeIndex;

  // Set global cell index by HyperTree
  std::vector<vtkIdType> globalCellIndexByHT;
  // Index of the HyperTree that has the largest number of cells
  vtkIdType treeIndexMaxNumberCell = 0;
  ComputeGlobalCellIndexByHTAndtreeIndexMaxNumberCell(
    htGrid, globalCellIndexByHT, treeIndexMaxNumberCell);
  assert(globalCellIndexByHT.size() == static_cast<size_t>(nbTrees) ||
    GuruMeditation("Valid size global index HT is this contexte !"));
  //
  std::vector<bool> maskedRootCellsTree(nbTrees, false);
  // iTest=0 Coin treeindex=0
  inputTreeIndex.emplace_back(0);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0, 1, -1, 4, 5, -1, -1, -1, -1, 20, 21, -1, 24, 25 });
  // iTest=1 treeindex=1
  inputTreeIndex.emplace_back(1);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 0, 1, 2, 4, 5, 6, -1, -1, -1, 20, 21, 22, 24, 25, 26 });
  // iTest=2 Coin treeindex=3
  inputTreeIndex.emplace_back(3);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 2, 3, -1, 6, 7, -1, -1, -1, -1, 22, 23, -1, 26, 27, -1 });
  // iTest=3 Coin treeindex=10
  inputTreeIndex.emplace_back(10);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 5,
    6, 7, 9, 10, 11, 13, 14, 15, 25, 26, 27, 29, 30, 31, 33, 34, 35 });
  // iTest=4 Coin treeindex=16
  inputTreeIndex.emplace_back(16);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 12, 13, -1, 16, 17, -1, -1, -1, -1, 32, 33, -1, 36, 37, -1, -1, -1 });
  // iTest=5 Coin treeindex=89
  inputTreeIndex.emplace_back(89);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ 64, 65, 66, 68, 69, 70, 72, 73, 74,
    84, 85, 86, 88, 89, 90, 92, 93, 94, 104, 105, 106, 108, 109, 110, 112, 113, 114 });
  // iTest=6 Coin treeindex=91
  inputTreeIndex.emplace_back(91);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ 66, 67, -1, 70, 71, -1, 74, 75, -1,
    86, 87, -1, 90, 91, -1, 94, 95, -1, 106, 107, -1, 110, 111, -1, 114, 115, -1 });
  // iTest=7 Coin treeindex=119
  inputTreeIndex.emplace_back(119);
  allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ 94, 95, -1, 98, 99, -1, -1, -1, -1,
    114, 115, -1, 118, 119, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 });
  // Call trip
  std::cout << "Trip all" << std::endl;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: " << treeIndexMaxNumberCell << " (treeIndexMaxNumberCell)"
            << std::endl;
  mask->SetValue(globalCellIndexByHT[treeIndexMaxNumberCell], 1);
  maskedRootCellsTree[treeIndexMaxNumberCell] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 0" << std::endl;
  mask->SetValue(globalCellIndexByHT[0], 1);
  maskedRootCellsTree[0] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 4" << std::endl;
  mask->SetValue(globalCellIndexByHT[4], 1);
  maskedRootCellsTree[4] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 15" << std::endl;
  mask->SetValue(globalCellIndexByHT[15], 1);
  maskedRootCellsTree[15] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 118" << std::endl;
  mask->SetValue(globalCellIndexByHT[118], 1);
  maskedRootCellsTree[118] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 119" << std::endl;
  mask->SetValue(globalCellIndexByHT[119], 1);
  maskedRootCellsTree[119] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);
}

void TestHyperTreeGridGetShiftedLevelZeroIndex_2D(unsigned int _hiddenAxis, vtkIdType _nbTrees,
  const std::vector<double>& originHTG, vtkHyperTreeGrid* htGrid)
{
  assert(htGrid->GetDimension() == 2 || GuruMeditation("Test for case 2D !"));

  vtkNew<vtkBitArray> mask;
  {
    const vtkIdType nbCells = htGrid->GetNumberOfCells();
    mask->SetNumberOfTuples(nbCells);
    for (vtkIdType iCell = 0; iCell < nbCells; ++iCell)
    {
      mask->SetValue(iCell, 0);
    }
    htGrid->SetMask(mask);
  }

  // Expected results
  std::vector<vtkIdType> inputTreeIndex;
  std::vector<std::vector<vtkIdType>> allTreeIndex;

  // Set global cell index by HyperTree
  std::vector<vtkIdType> globalCellIndexByHT;
  // Index of the HyperTree that has the largest number of cells
  vtkIdType treeIndexMaxNumberCell = 0;
  ComputeGlobalCellIndexByHTAndtreeIndexMaxNumberCell(
    htGrid, globalCellIndexByHT, treeIndexMaxNumberCell);
  assert(globalCellIndexByHT.size() == static_cast<size_t>(_nbTrees) ||
    GuruMeditation("Valid size global index HT is this contexte !"));
  //
  std::vector<bool> maskedRootCellsTree(_nbTrees, false);
  // iTest=0 Coin treeindex=0
  inputTreeIndex.emplace_back(0);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, -1, 0, 1, -1, 4, 5 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, -1, 0, 4, -1, 1, 5 });
      break;
    }
  }

  // iTest=1 treeindex=1
  inputTreeIndex.emplace_back(1);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, 0, 1, 2, 4, 5, 6 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, 0, 4, -1, 1, 5, -1, 2, 6 });
      break;
    }
  }
  // iTest=2 Coin treeindex=3
  inputTreeIndex.emplace_back(3);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, -1, -1, 2, 3, -1, 6, 7, -1 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ -1, 2, 6, -1, 3, 7, -1, -1, -1 });
      break;
    }
  }
  // iTest=3 Coin treeindex=9
  inputTreeIndex.emplace_back(9);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ 4, 5, 6, 8, 9, 10, 12, 13, 14 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ 4, 8, 12, 5, 9, 13, 6, 10, 14 });
      break;
    }
  }
  // iTest=4 Coin treeindex=10
  inputTreeIndex.emplace_back(10);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ 5, 6, 7, 9, 10, 11, 13, 14, 15 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(std::initializer_list<vtkIdType>{ 5, 9, 13, 6, 10, 14, 7, 11, 15 });
      break;
    }
  }
  // iTest=5 Coin treeindex=13
  inputTreeIndex.emplace_back(13);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(
        std::initializer_list<vtkIdType>{ 8, 9, 10, 12, 13, 14, 16, 17, 18 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(
        std::initializer_list<vtkIdType>{ 8, 12, 16, 9, 13, 17, 10, 14, 18 });
      break;
    }
  }
  // iTest=6 Coin treeindex=16
  inputTreeIndex.emplace_back(16);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(
        std::initializer_list<vtkIdType>{ -1, 12, 13, -1, 16, 17, -1, -1, -1 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(
        std::initializer_list<vtkIdType>{ -1, -1, -1, 12, 16, -1, 13, 17, -1 });
      break;
    }
  }
  // iTest=7 Coin treeindex=19
  inputTreeIndex.emplace_back(19);
  switch (_hiddenAxis)
  {
    case 0:
    case 2:
    {
      allTreeIndex.emplace_back(
        std::initializer_list<vtkIdType>{ 14, 15, -1, 18, 19, -1, -1, -1, -1 });
      break;
    }
    case 1:
    {
      allTreeIndex.emplace_back(
        std::initializer_list<vtkIdType>{ 14, 18, -1, 15, 19, -1, -1, -1, -1 });
      break;
    }
  }

  std::cout << "Trip all" << std::endl;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: " << treeIndexMaxNumberCell << " (treeIndexMaxNumberCell)"
            << std::endl;
  mask->SetValue(globalCellIndexByHT[treeIndexMaxNumberCell], 1);
  maskedRootCellsTree[treeIndexMaxNumberCell] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 0" << std::endl;
  mask->SetValue(globalCellIndexByHT[0], 1);
  maskedRootCellsTree[0] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 4" << std::endl;
  mask->SetValue(globalCellIndexByHT[4], 1);
  maskedRootCellsTree[4] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 15" << std::endl;
  mask->SetValue(globalCellIndexByHT[15], 1);
  maskedRootCellsTree[15] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 17" << std::endl;
  mask->SetValue(globalCellIndexByHT[17], 1);
  maskedRootCellsTree[17] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);

  std::cout << "Trip masked cells: 19" << std::endl;
  mask->SetValue(globalCellIndexByHT[19], 1);
  maskedRootCellsTree[19] = true;
  Trip(htGrid, originHTG, inputTreeIndex, allTreeIndex, maskedRootCellsTree);
}

void TestHyperTreeGridGetShiftedLevelZeroIndex_2D_XY()
{
  std::cout << "TestHyperTreeGridGetShiftedLevelZeroIndex_2D_XY" << std::endl;

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(5, 6, 1); // GridCell 4, 5, 0
  source->SetOutputBounds(1., 5., 1., 6., 1., 1.);
  source->SetSeed(42);
  source->SetSplitFraction(0.5);
  source->SetMaxDepth(2);

  source->Update();

  vtkHyperTreeGrid* htGrid = vtkHyperTreeGrid::SafeDownCast(source->GetOutputDataObject(0));

  assert(htGrid->GetDimension() == 2 || GuruMeditation("Test for case 2D !"));
  assert(htGrid->GetAxes()[0] == 0 || GuruMeditation("2D with first axe X (0) !"));
  assert(htGrid->GetAxes()[1] == 1 || GuruMeditation("2D with second axe Y (1) !"));

  vtkIdType nbTrees = 4 * 5 * 1;
  std::vector<double> originHTG{ 1., 1., 0. };

  assert(htGrid->GetMaxNumberOfTrees() == nbTrees ||
    GuruMeditation("Invalid expected values nbTrees !"));

  TestHyperTreeGridGetShiftedLevelZeroIndex_2D(2, nbTrees, originHTG, htGrid);
}

void TestHyperTreeGridGetShiftedLevelZeroIndex_2D_XZ()
{
  std::cout << "TestHyperTreeGridGetShiftedLevelZeroIndex_2D_XZ" << std::endl;

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(5, 1, 6); // GridCell 4, 0, 5
  source->SetOutputBounds(1., 5., 1., 1., 1., 6.);
  source->SetSeed(42);
  source->SetSplitFraction(0.5);
  source->SetMaxDepth(2);

  source->Update();

  vtkHyperTreeGrid* htGrid = vtkHyperTreeGrid::SafeDownCast(source->GetOutputDataObject(0));

  assert(htGrid->GetDimension() == 2 || GuruMeditation("Test for case 2D !"));
  assert(htGrid->GetAxes()[0] == 2 || GuruMeditation("2D with first axe Z (2) !"));
  assert(htGrid->GetAxes()[1] == 0 || GuruMeditation("2D with second axe X (0) !"));

  vtkIdType nbTrees = 4 * 1 * 5;
  std::vector<double> originHTG{ 1., 0., 1. };

  assert(htGrid->GetMaxNumberOfTrees() == nbTrees ||
    GuruMeditation("Invalid expected values nbTrees !"));

  TestHyperTreeGridGetShiftedLevelZeroIndex_2D(1, nbTrees, originHTG, htGrid);
}

void TestHyperTreeGridGetShiftedLevelZeroIndex_2D_YZ()
{
  std::cout << "TestHyperTreeGridGetShiftedLevelZeroIndex_2D_YZ" << std::endl;

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(1, 5, 6); // GridCell 0, 4, 5
  source->SetOutputBounds(1., 1., 1., 5., 1., 6.);
  source->SetSeed(42);
  source->SetSplitFraction(0.5);
  source->SetMaxDepth(2);

  source->Update();

  vtkHyperTreeGrid* htGrid = vtkHyperTreeGrid::SafeDownCast(source->GetOutputDataObject(0));

  vtkIdType nbTrees = 1 * 4 * 5;
  std::vector<double> originHTG{ 0., 1., 1. };

  assert(htGrid->GetMaxNumberOfTrees() == nbTrees ||
    GuruMeditation("Invalid expected values nbTrees !"));

  assert(htGrid->GetDimension() == 2 || GuruMeditation("Test for case 2D !"));
  assert(htGrid->GetAxes()[0] == 1 || GuruMeditation("2D with first axe Y (1) !"));
  assert(htGrid->GetAxes()[1] == 2 || GuruMeditation("2D with second axe Z (2) !"));

  TestHyperTreeGridGetShiftedLevelZeroIndex_2D(0, nbTrees, originHTG, htGrid);
}

void TestEmptyHyperTreeGrid()
{
  std::cout << "TestEmptyHyperTreeGrid" << std::endl;

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(1, 1, 1); // HTG dimension is 0, should not output any tree
  source->Update();

  assert(vtkHyperTreeGrid::SafeDownCast(source->GetOutputDataObject(0))->GetDimension() == 0 ||
    GuruMeditation("Empty HTG Has dimension != 0"));
}

} // end anon namespace

int TestRandomHyperTreeGridSource(int, char*[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);
  bool result = true;
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    result &= ConstructScene(renderer, 1);
    renWin->AddRenderer(renderer.Get());
  }
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.5, 1.0, 1.0);
    result &= ConstructScene(renderer, 2);
    renWin->AddRenderer(renderer.Get());
  }
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.0, 0.5, 0.5);
    result &= ConstructScene(renderer, 4);
    renWin->AddRenderer(renderer.Get());
  }

  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.0, 1.0, 0.5);
    result &= ConstructScene(renderer, 8);
    renWin->AddRenderer(renderer.Get());
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  renWin->Render();
  iren->Start();

  TestHyperTreeGridGetShiftedLevelZeroIndex_3D();
  TestHyperTreeGridGetShiftedLevelZeroIndex_2D_XY();
  TestHyperTreeGridGetShiftedLevelZeroIndex_2D_XZ();
  TestHyperTreeGridGetShiftedLevelZeroIndex_2D_YZ();
  TestEmptyHyperTreeGrid();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
