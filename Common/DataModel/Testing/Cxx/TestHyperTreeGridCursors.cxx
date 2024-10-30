// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"
#include "vtkHyperTreeGridOrientedCursor.h"
#include "vtkUniformHyperTreeGrid.h"

#include <array>
#include <iostream>
#include <string>

namespace
{

template <typename CursorType>
struct is_unlimited : public std::false_type
{
};

// register here unlimited cursor types
template <>
struct is_unlimited<vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor> : public std::true_type
{
};
template <>
struct is_unlimited<vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor> : public std::true_type
{
};

//------------------------------------------------------------------------------
class TestCursor
{

public:
  template <typename t, typename std::enable_if<!is_unlimited<t>::value>::type* = nullptr>
  int doTest(t* mcur)
  {
    if (mcur->IsLeaf())
    {
      return 1;
    }
    int res = 1;
    const unsigned char nbChilds = mcur->GetNumberOfChildren();
    for (int i = 0; i < nbChilds; i++)
    {
      mcur->ToChild(i);
      res += this->doTest(mcur);
      mcur->ToParent();
    }
    return res;
  }

  template <typename t, typename std::enable_if<is_unlimited<t>::value>::type* = nullptr>
  int doTest(t* mcur)
  {
    int status = 0;
    // For unlimited type, we descend 10 times independently of the tree shape
    // this goes deeper than the lowest child anyway
    for (int i = 0; i < 10; i++)
    {
      mcur->ToChild(0);
      double b[6];
      mcur->GetBounds(b);
      std::cout << "bounds:" << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << " " << b[4]
                << " " << b[5] << std::endl;
      double* orig = mcur->GetOrigin();
      std::cout << "orig: " << orig[0] << " " << orig[1] << " " << orig[2] << " " << std::endl;

      const vtkIdType vertex_id = mcur->GetVertexId();
      std::cout << "vertex id: " << vertex_id << std::endl;
      if (vertex_id == std::numeric_limits<unsigned int>::max())
      {
        // value used in elder child for invalid childs
        std::cerr << "Error: invalid vertex_id encountered while going down in doTest" << std::endl;
        status = 1;
      }

      if (mcur->IsRoot())
      {
        std::cerr << "Error: a child can never be the root" << std::endl;
        status = 1;
      }
    }
    for (int i = 0; i < 10; i++)
    {
      const vtkIdType vertex_id = mcur->GetVertexId();
      std::cout << "vertex id: " << vertex_id << std::endl;
      if (vertex_id == std::numeric_limits<unsigned int>::max())
      {
        std::cerr << "Error: invalid vertex_id encountered while going up in doTest" << std::endl;
        status = 1;
      }
      mcur->ToParent();
    }

    std::cout << "second descent" << std::endl;

    // second descent
    for (int i = 0; i < 10; i++)
    {
      mcur->ToChild(1);
      double b[6];
      mcur->GetBounds(b);
      std::cout << "bounds:" << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << " " << b[4]
                << " " << b[5] << std::endl;
      double* orig = mcur->GetOrigin();
      std::cout << "orig: " << orig[0] << " " << orig[1] << " " << orig[2] << " " << std::endl;

      const vtkIdType vertex_id = mcur->GetVertexId();
      std::cout << "vertex id: " << vertex_id << std::endl;
      if (vertex_id == std::numeric_limits<unsigned int>::max())
      {
        std::cerr << "Error: invalid vertex_id encountered while going down in doTest" << std::endl;
        status = 1;
      }
    }
    for (int i = 0; i < 10; i++)
    {
      const vtkIdType vertex_id = mcur->GetVertexId();
      std::cout << "vertex id: " << vertex_id << std::endl;
      if (vertex_id == std::numeric_limits<unsigned int>::max())
      {
        std::cerr << "Error: invalid vertex_id encountered while going up in doTest" << std::endl;
        status = 1;
      }
      mcur->ToParent();
    }

    return status;
  }

  ~TestCursor() = default;
};

//------------------------------------------------------------------------------
void generateSingleCellTreeHTG(vtkUniformHyperTreeGrid* uhtg, unsigned int treeId)
{
  std::cout << "Initializing single cell tree " << treeId << "\n";
  std::cout.flush();
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  uhtg->InitializeNonOrientedCursor(cursor, treeId, true);
  cursor->SetGlobalIndexStart(uhtg->GetNumberOfCells());
}

//------------------------------------------------------------------------------
void generateQuadTreeHTG(vtkUniformHyperTreeGrid* uhtg, unsigned int treeId)
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

//------------------------------------------------------------------------------
void generateOctreeHTG(vtkUniformHyperTreeGrid* uhtg, unsigned int treeId)
{
  std::cout << "Initializing octree " << treeId << "\n";
  std::cout.flush();

  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  uhtg->InitializeNonOrientedCursor(cursor, treeId, true);
  cursor->SetGlobalIndexStart(uhtg->GetNumberOfCells());

  // level 0
  // CursorIndex: 0
  // level 0, 0
  cursor->SubdivideLeaf();
  // CursorIndex: 1
  cursor->ToChild(3);
  // level 1, 0.3
  cursor->SubdivideLeaf();
  // CursorIndex: 1
  cursor->ToChild(1);
  // level 2, 0.3.1
  cursor->ToParent();
  // level 1, 0.3
  cursor->ToParent();
  // level 0, 0
  cursor->ToChild(0);
  // level 1, 0.0
  cursor->SubdivideLeaf();
  // CursorIndex: 1 17
  cursor->ToChild(7);
}

//------------------------------------------------------------------------------
void initSingleCellTreeHTG(vtkUniformHyperTreeGrid* uhtg)
{
  std::cout << "Initializing Uniform Single Cell Tree Grid\n";
  std::cout.flush();
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetExtent(0, 1, 0, 1, 0, 0);

  generateSingleCellTreeHTG(uhtg, 0);
}
//------------------------------------------------------------------------------
void initQuadTreeHTG(vtkUniformHyperTreeGrid* uhtg)
{
  std::cout << "Initializing Uniform QuadTree Grid\n";
  std::cout.flush();
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetDimensions(5, 2, 1);

  generateQuadTreeHTG(uhtg, 1);
  generateQuadTreeHTG(uhtg, 3);
}
//------------------------------------------------------------------------------
void initOctreeHTG(vtkUniformHyperTreeGrid* uhtg)
{
  std::cout << "Initializing Uniform OcTree Grid\n";
  std::cout.flush();
  uhtg->SetBranchFactor(2);
  uhtg->SetGridScale(1.1);
  uhtg->SetOrigin(0., 0., 0.);
  uhtg->SetDimensions(3, 3, 2);
  generateOctreeHTG(uhtg, 0);
  generateOctreeHTG(uhtg, 1);
}

/**
 * Test the Tree deletion function, using cursors
 */
int TestTreeDeletion()
{
  // Setup a HTG with a few trees at given ids
  vtkNew<vtkHyperTreeGrid> htg;
  vtkHyperTree* tree = vtkHyperTree::CreateInstance(2, 2);
  std::array<vtkIdType, 6> ids{ 0, 1, 3, 5, 8, 12 };
  for (auto id : ids)
  {
    htg->SetTree(id, tree);
  }
  tree->Delete();

  // Delete some trees
  vtkIdType totalTreesRemoved = 0;
  vtkIdType expectedTreesRemoved = 3;
  totalTreesRemoved += htg->RemoveTree(3);
  totalTreesRemoved += htg->RemoveTree(5);
  totalTreesRemoved += htg->RemoveTree(12);
  totalTreesRemoved += htg->RemoveTree(12);
  if (totalTreesRemoved != expectedTreesRemoved)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Expected to have " << expectedTreesRemoved << " trees removed but got " << totalTreesRemoved
                          << " instead.");
    return 1;
  }

  vtkIdType inIndex = 0;
  std::size_t offset = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  htg->InitializeTreeIterator(it);
  std::array<vtkIdType, 3> expectedIds{ 0, 1, 8 };
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(inIndex))
  {
    if (inIndex != expectedIds[offset++])
    {
      vtkErrorWithObjectMacro(nullptr,
        "Expected to get tree id " << expectedIds[offset] << " at position " << offset
                                   << " but got " << inIndex << " instead.");
      return 1;
    }
  }

  if (offset != expectedIds.size())
  {
    vtkErrorWithObjectMacro(nullptr,
      "Expected to have " << expectedIds.size() << " trees left in the HTG but got " << offset
                          << " instead.");
    return 1;
  }

  return 0;
}

} // anonymous namespace

//------------------------------------------------------------------------------
int TestHyperTreeGridCursors(int, char*[])
{
  std::cout << "Starting test 1\n";
  std::cout.flush();

  int status = 0;

  // Single cell tree
  {
    vtkNew<vtkUniformHyperTreeGrid> uhtg0;
    initSingleCellTreeHTG(uhtg0);

    const int expectedResult = 1;

    vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> mooreSC;
    uhtg0->InitializeNonOrientedMooreSuperCursor(mooreSC, 0);
    TestCursor test1;
    const int res1 = test1.doTest(mooreSC.Get());
    if (res1 != expectedResult)
    {
      std::cerr << "ERROR non oriented moore supercursor visited " << res1 << " leaves instead of "
                << expectedResult << std::endl;
      ++status;
    }

    vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursor> vonNeumannSC;
    uhtg0->InitializeNonOrientedVonNeumannSuperCursor(vonNeumannSC, 0);
    TestCursor test2;
    const int res2 = test2.doTest(vonNeumannSC.Get());
    if (res2 != expectedResult)
    {
      std::cerr << "ERROR non oriented von neumann supercursor visited " << res2
                << " leaves instead of " << expectedResult << std::endl;
      ++status;
    }

    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor> unlimitedGeo;
    uhtg0->InitializeNonOrientedUnlimitedGeometryCursor(unlimitedGeo, 0);
    TestCursor test3;
    status += test3.doTest(unlimitedGeo.Get());
    // no result for unlimited

    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor> unlimtedMooreSC;
    uhtg0->InitializeNonOrientedUnlimitedMooreSuperCursor(unlimtedMooreSC, 0);
    TestCursor test4;
    status += test4.doTest(unlimtedMooreSC.Get());
    // no result for unlimited
  }

  // Quad tree
  {
    vtkNew<vtkUniformHyperTreeGrid> uhtg1;
    initQuadTreeHTG(uhtg1);

    const int expectedResult = 25;

    vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> mooreSC;
    uhtg1->InitializeNonOrientedMooreSuperCursor(mooreSC, 1);
    TestCursor test1;
    const int res1 = test1.doTest(mooreSC.Get());
    if (res1 != expectedResult)
    {
      std::cerr << "ERROR non oriented moore supercursor visited " << res1 << " leaves instead of "
                << expectedResult << std::endl;
      ++status;
    }

    vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursor> vonNeumannSC;
    uhtg1->InitializeNonOrientedVonNeumannSuperCursor(vonNeumannSC, 1);
    TestCursor test2;
    const int res2 = test2.doTest(vonNeumannSC.Get());
    if (res2 != expectedResult)
    {
      std::cerr << "ERROR non oriented von neumann supercursor visited " << res2
                << " leaves instead of " << expectedResult << std::endl;
      ++status;
    }

    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor> unlimitedGeo;
    uhtg1->InitializeNonOrientedUnlimitedGeometryCursor(unlimitedGeo, 1);
    TestCursor test3;
    status += test3.doTest(unlimitedGeo.Get());
    // no result for unlimited

    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor> unlimtedMooreSC;
    uhtg1->InitializeNonOrientedUnlimitedMooreSuperCursor(unlimtedMooreSC, 1);
    TestCursor test4;
    status += test4.doTest(unlimtedMooreSC.Get());
    // no result for unlimited
  }

  // Octree
  {
    vtkNew<vtkUniformHyperTreeGrid> uhtg2;
    initOctreeHTG(uhtg2);

    const int expectedResult = 25;

    vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> mooreSC;
    uhtg2->InitializeNonOrientedMooreSuperCursor(mooreSC, 0);
    TestCursor test1;
    const int res1 = test1.doTest(mooreSC.Get());
    if (res1 != expectedResult)
    {
      std::cerr << "ERROR non oriented moore supercursor visited " << res1 << " leaves instead of "
                << expectedResult << std::endl;
      ++status;
    }

    vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursor> vonNeumannSC;
    uhtg2->InitializeNonOrientedVonNeumannSuperCursor(vonNeumannSC, 0);
    TestCursor test2;
    const int res2 = test2.doTest(vonNeumannSC.Get());
    if (res2 != expectedResult)
    {
      std::cerr << "ERROR non oriented von neumann supercursor visited " << res2
                << " leaves instead of " << expectedResult << std::endl;
      ++status;
    }

    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor> unlimitedGeo;
    uhtg2->InitializeNonOrientedUnlimitedGeometryCursor(unlimitedGeo, 1);
    TestCursor test3;
    status += test3.doTest(unlimitedGeo.Get());
    // no result for unlimited

    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor> unlimtedMooreSC;
    uhtg2->InitializeNonOrientedUnlimitedMooreSuperCursor(unlimtedMooreSC, 1);
    TestCursor test4;
    status += test4.doTest(unlimtedMooreSC.Get());
    // no result for unlimited
  }

  {
    status += TestTreeDeletion();
  }

  return status;
}
