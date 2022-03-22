/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridBitmask.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkUniformHyperTreeGrid.h"

#include <iostream>
#include <string>
#include <vector>
// exist in ElderChildIndex test
void SubdivideAndInitMaskChildrenTrue(vtkHyperTreeGridNonOrientedCursor* cursor);
void CheckTestStatusHTG(int rc, const std::string& TestName);
//------------------------------------------------------------------------------
bool compareMask(vtkBitArray* bm, const std::vector<bool>& ref_bm)
{
  const size_t nbElementsBm = bm->GetNumberOfValues();
  const size_t nbElementsRef = ref_bm.size();
  if (nbElementsRef != nbElementsBm)
  {
    std::cout << "Not same amount of bits in expected and actual mask: [REF]" << nbElementsRef
              << " vs [Actual]" << nbElementsBm << std::endl;
    return false;
  }
  for (size_t i = 0; i < nbElementsRef; ++i)
  {
    const bool ref_val = ref_bm.at(i);
    const vtkIdType vtkIdx = static_cast<vtkIdType>(i);
    const bool act_val = bm->GetValue(vtkIdx) != 0;
    if (ref_val != act_val)
    {
      std::cout << "Mask value different for idx " << i << ": " << ref_val << "[REF] vs " << act_val
                << "[Actual]" << std::endl;
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------------------
void displayMask(const std::string& msg, vtkHyperTreeGrid* htg)
{
  if (!msg.empty())
  {
    std::cout << msg << " ";
    std::cout.flush();
  }
  auto* bm = htg->GetMask();
  if (!bm)
  {
    std::cout << "No Mask" << std::endl;
    std::cout.flush();
    return;
  }
  std::cout << "Mask: ";
  std::cout.flush();
  const vtkIdType nbElements = bm->GetNumberOfValues();
  for (vtkIdType i = 0; i < nbElements; ++i)
  {
    std::cout << bm->GetValue(i);
  }
  std::cout << std::endl;
  std::cout.flush();
}
//------------------------------------------------------------------------------
void initUniformHyperTreeOneRootCell(vtkUniformHyperTreeGrid* uhtg)
{
  std::cout << "Initializing Uniform Grid\n";
  std::cout.flush();
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetDimensions(3, 3, 2);
  vtkNew<vtkBitArray> mask;
  uhtg->SetMask(mask);

  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  const unsigned int treeId = 1;
  const vtkIdType nbElementsInHTG = uhtg->GetNumberOfCells();
  uhtg->InitializeNonOrientedCursor(cursor, treeId, true);
  cursor->SetGlobalIndexStart(nbElementsInHTG);
  cursor->SetMask(false);
}
//------------------------------------------------------------------------------
int TestUniformHyperTreeOneRootCell()
{
  vtkNew<vtkUniformHyperTreeGrid> uhtg;
  initUniformHyperTreeOneRootCell(uhtg);
  auto* bm = uhtg->GetMask();
  //                       0
  return !compareMask(bm, { false });
}
void subdivide(vtkHyperTreeGridNonOrientedCursor* cursor, const std::vector<int>& sub)
{
  for (size_t i = 0; i < sub.size(); ++i)
  {
    const int subIdx = sub[i];
    if (subIdx == -1)
    {
      cursor->ToParent();
      continue;
    }
    if (cursor->IsLeaf())
    {
      SubdivideAndInitMaskChildrenTrue(cursor);
    }
    cursor->ToChild(subIdx);
    cursor->SetMask(false);
  }
}
//------------------------------------------------------------------------------
int TestUniformHyperTreeOneRootCellSubdivided()
{
  vtkNew<vtkUniformHyperTreeGrid> uhtg;
  initUniformHyperTreeOneRootCell(uhtg);
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  uhtg->InitializeTreeIterator(it);
  vtkIdType treeId = -1;
  it.GetNextTree(treeId);
  if (treeId == -1)
  {
    std::cout << "Impossible to retrieve either of the trees" << std::endl;
    std::cout.flush();
    return 1;
  }
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  uhtg->InitializeNonOrientedCursor(cursor, treeId);
  cursor->SetMask(false);
  subdivide(cursor, { 7, 2, -1, -1, 2, 7 });
  // 0 11011110 11011111 11111110
  auto* bm = uhtg->GetMask();
  displayMask("OneRootCellSubdivided", uhtg);
  // 0 11011110 11011111 11111110
  std::vector<bool> exp_bm(25, true);
  for (unsigned int i : { 0, 3, 8, 11, 24 })
  {
    exp_bm[i] = false;
  }
  return !compareMask(bm, exp_bm);
}
//------------------------------------------------------------------------------
void initUniformHyperTreeSeveralRootCells(
  vtkUniformHyperTreeGrid* uhtg, const std::vector<std::vector<int>>& sub = {})
{
  std::cout << "Init Uniform Grid several root cells\n";
  std::cout.flush();
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetDimensions(3, 4, 2);
  vtkNew<vtkBitArray> mask;
  uhtg->SetMask(mask);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  const std::vector<unsigned int> treeIds = { 3, 1, 5, 4 };
  unsigned int treeIndex = 0;
  for (unsigned int treeId : treeIds)
  {
    const vtkIdType nbElementsInHTG = uhtg->GetNumberOfCells();
    uhtg->InitializeNonOrientedCursor(cursor, treeId, true);
    cursor->SetGlobalIndexStart(nbElementsInHTG);
    // std::cout << "GlobalIndexStart of HT " << treeId << " : " << nbElementsInHTG << std::endl;
    cursor->SetMask(false);
    if (!sub.empty())
    {
      subdivide(cursor, sub.at(treeIndex));
    }
    // displayMask("Inserted 1 root cell (tree)", uhtg);
    ++treeIndex;
  }
  displayMask("Inserted 4 root cells (trees)", uhtg);
}
//------------------------------------------------------------------------------
int TestUniformHyperTreeSeveralRootCells()
{
  std::cout << "Test Uniform Grid several root cells\n";
  std::cout.flush();
  vtkNew<vtkUniformHyperTreeGrid> uhtg;
  initUniformHyperTreeSeveralRootCells(uhtg);
  auto* bm = uhtg->GetMask();
  //                        0      0      0      0
  const std::vector<bool> exp_bm(4, false);
  return !compareMask(bm, exp_bm);
}
//------------------------------------------------------------------------------
int TestUniformHyperTreeSeveralRootCellsSubdivided()
{
  std::cout << "Test Uniform Grid several root cells subdivided\n";
  std::cout.flush();
  vtkNew<vtkUniformHyperTreeGrid> uhtg;
  initUniformHyperTreeSeveralRootCells(uhtg, { { 5, 0, -1, 2 }, { 7, 7, 7 }, { 4, -1, 6 }, { 4 } });
  auto* bm = uhtg->GetMask();
  //(HTa - 3) 0 11111011 01011111
  //(HTb - 1) 0 11111110 11111110 11111110
  //(HTc - 5) 0 11110101
  //(HTd - 4) 0 11101111
  // 0      6   9 10     17       25       33       41 42   4749 51    56
  // 0 11111011 01011111 0 11111110 11111110 11111110 0 11110101 0 11110111
  std::vector<bool> exp_bm(60, true);
  for (unsigned int i : { 0, 6, 9, 11, 17, 25, 33, 41, 42, 47, 49, 51, 56 })
  {
    exp_bm[i] = false;
  }
  displayMask("TestUniformHyperTreeSeveralRootCellsSubdivided", uhtg);
  return !compareMask(bm, exp_bm);
}
//------------------------------------------------------------------------------
int TestUniformHyperTreeEmpty()
{
  std::cout << "Initializing Empty Uniform Grid\n";
  std::cout.flush();
  vtkNew<vtkUniformHyperTreeGrid> uhtg;
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetDimensions(3, 3, 2);
  vtkNew<vtkBitArray> mask;
  uhtg->SetMask(mask);
  displayMask("Empty HTG", uhtg);
  auto* bm = uhtg->GetMask();
  return !(bm->GetNumberOfValues() == 0);
}

//------------------------------------------------------------------------------
int TestHyperTreeGridBitmask(int, char*[])
{
  std::cout << "Starting tests" << std::endl;
  std::cout.flush();
  int rc = 0;
  rc += TestUniformHyperTreeEmpty();
  CheckTestStatusHTG(rc, "TestUniformHyperTreeEmpty");
  rc += TestUniformHyperTreeOneRootCell();
  CheckTestStatusHTG(rc, "TestUniformHyperTreeOneRootCell");
  rc += TestUniformHyperTreeOneRootCellSubdivided();
  CheckTestStatusHTG(rc, "TestUniformHyperTreeOneRootCellSubdivided");
  rc += TestUniformHyperTreeSeveralRootCells();
  CheckTestStatusHTG(rc, "TestUniformHyperTreeSeveralRootCells");
  rc += TestUniformHyperTreeSeveralRootCellsSubdivided();
  CheckTestStatusHTG(rc, "TestUniformHyperTreeSeveralRootCellsSubdivided");

  return (rc);
}
