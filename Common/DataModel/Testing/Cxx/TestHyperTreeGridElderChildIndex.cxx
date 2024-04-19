// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkUniformHyperTreeGrid.h"

#include <iostream>
#include <string>
//------------------------------------------------------------------------------
void SubdivideAndInitMaskChildrenTrue(vtkHyperTreeGridNonOrientedCursor* cursor)
{
  cursor->SubdivideLeaf();
  for (unsigned char i = 0; i < cursor->GetNumberOfChildren(); ++i)
  {
    cursor->ToChild(i);
    cursor->SetMask(true);
    cursor->ToParent();
  }
}
//------------------------------------------------------------------------------
void generateTree2(vtkUniformHyperTreeGrid* uhtg, unsigned int treeId)
{
  std::cout << "Initializing octree " << treeId << "\n";
  std::cout.flush();
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  uhtg->InitializeNonOrientedCursor(cursor, treeId, true);
  cursor->SetGlobalIndexStart(uhtg->GetNumberOfCells());

  // level 0
  cursor->SetMask(false);
  // ElderChildIndex: a
  // bitmask: 0
  // level 0, 0
  SubdivideAndInitMaskChildrenTrue(cursor);
  // ElderChildIndex: 1
  // bitmask: 0 11111111
  cursor->ToChild(3);
  // level 1, 0.3
  cursor->SetMask(false);
  // bitmask: 0 11101111
  SubdivideAndInitMaskChildrenTrue(cursor);
  // ElderChildIndex: 1 aaa9
  // bitmask: 0 11101111 11111111
  cursor->ToChild(1);
  // level 2, 0.3.1
  cursor->SetMask(false);
  // bitmask: 0 11101111 10111111
  cursor->ToParent();
  // level 1, 0.3
  cursor->ToParent();
  // level 0, 0
  cursor->ToChild(0);
  // level 1, 0.0
  cursor->SetMask(false);
  // bitmask: 0 01101111 10111111
  SubdivideAndInitMaskChildrenTrue(cursor);
  // ElderChildIndex: 1 17-aa9
  // bitmask: 0 01101111 10111111 11111111
  cursor->ToChild(7);
  cursor->SetMask(false);
  // bitmask: 0 01101111 10111111 11111110
}
//------------------------------------------------------------------------------
int TestElderChildIndexOK(vtkUniformHyperTreeGrid* htg)
{
  std::cout << "Starting Elder Child indices comparison\n";
  std::cout.flush();
  int rc = 0;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  htg->InitializeTreeIterator(it);
  vtkHyperTree* ht1 = it.GetNextTree();
  vtkHyperTree* ht2 = it.GetNextTree();
  size_t nbElements = 0;
  const unsigned int* ElderChildHT1 = (ht1) ? ht1->GetElderChildIndexArray(nbElements) : nullptr;
  const unsigned int* ElderChildHT2 = (ht2) ? ht2->GetElderChildIndexArray(nbElements) : nullptr;
  if (ht1 == nullptr || ht2 == nullptr)
  {
    nbElements = 0;
    std::cout << "Impossible to retrieve either of the trees" << std::endl;
    std::cout.flush();
  }
  for (unsigned int i = 0; i < nbElements; ++i)
  {
    std::cout << "index " << i << ": " << ElderChildHT1[i] << "\n";
    std::cout.flush();
    if (ElderChildHT1[i] != ElderChildHT2[i])
    {
      rc = 1;
      break;
    }
  }
  vtkBitArray* mask = htg->GetMask();
  if (mask)
  {
    std::cout << "Mask: ";
    for (vtkIdType i = 0; i < mask->GetNumberOfValues(); ++i)
    {
      std::cout << mask->GetValue(i);
    }
    std::cout << std::endl;
  }
  else
  {
    std::cout << "No mask" << std::endl;
  }
  return rc;
}

void generateTree(vtkUniformHyperTreeGrid* uhtg, unsigned int treeId)
{
  std::cout << "Initializing quadtree " << treeId << "\n";
  std::cout.flush();
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  uhtg->InitializeNonOrientedCursor(cursor, treeId, true);
  cursor->SetGlobalIndexStart(uhtg->GetNumberOfCells());
  // level 0
  cursor->SubdivideLeaf();
  cursor->ToChild(0);
  // level 1.0
  cursor->SubdivideLeaf();
  cursor->ToParent();
  // level 0
  cursor->ToChild(1);
  // level 1.1
  cursor->SubdivideLeaf();
  cursor->ToParent();
  // level 0
  cursor->ToChild(0);
  // level 1.0
  cursor->ToChild(0);
  // level 2.0
  cursor->SubdivideLeaf();
  cursor->ToChild(2);
  // level 3.2
  cursor->SubdivideLeaf();
  cursor->ToChild(0);
  // level 4.0
  cursor->SubdivideLeaf();
}

void initializeUniformHyperTreeGridQuadTree(vtkUniformHyperTreeGrid* uhtg)
{
  std::cout << "Initializing Uniform Grid\n";
  std::cout.flush();
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetDimensions(5, 2, 1);

  generateTree(uhtg, 1);
  generateTree(uhtg, 3);
}
//------------------------------------------------------------------------------
void initializeUniformHyperTreeGridOcTree(vtkUniformHyperTreeGrid* uhtg)
{
  std::cout << "Initializing Uniform Grid\n";
  std::cout.flush();
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetDimensions(3, 3, 2);
  vtkNew<vtkBitArray> mask;
  uhtg->SetMask(mask);
  generateTree2(uhtg, 0);
  generateTree2(uhtg, 1);
}

//------------------------------------------------------------------------------
void CheckTestStatusHTG(int rc, const std::string& TestName)
{
  std::cout << "Test " << TestName << "...";
  std::cout.flush();
  if (rc == 0)
  {
    std::cout << "PASSED!\n";
    std::cout.flush();
  }
  else
  {
    std::cout << "FAILED!\n";
    std::cout.flush();
  }
}

int TestHyperTreeGridElderChildIndex(int, char*[])
{
  std::cout << "Starting test 1\n";
  std::cout.flush();
  int rc = 0;
  {
    vtkNew<vtkUniformHyperTreeGrid> uhtg1;
    initializeUniformHyperTreeGridQuadTree(uhtg1);

    rc += TestElderChildIndexOK(uhtg1);
    CheckTestStatusHTG(rc, "TestElderChildIndexOKQuadTree");
  }
  {
    vtkNew<vtkUniformHyperTreeGrid> uhtg1;
    initializeUniformHyperTreeGridOcTree(uhtg1);

    rc += TestElderChildIndexOK(uhtg1);
    CheckTestStatusHTG(rc, "TestElderChildIndexOKOctree");
  }

  return (rc);
}
