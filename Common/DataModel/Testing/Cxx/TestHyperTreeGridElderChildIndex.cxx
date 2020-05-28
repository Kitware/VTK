/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridElderChildIndex.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTree.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkUniformHyperTreeGrid.h"

#include <iostream>
#include <string>
//------------------------------------------------------------------------------
int TestElderChildIndexOK(vtkUniformHyperTreeGrid* htg)
{
  std::cout << "Starting Elder Child indices comparison\n";
  std::cout.flush();
  int rc = 0;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  auto* ht1 = htg->GetTree(1);
  auto* ht3 = htg->GetTree(3);
  size_t nbElements = 0;
  const unsigned int* ElderChildHT1 = ht1->GetElderChildIndexArray(nbElements);
  const unsigned int* ElderChildHT3 = ht3->GetElderChildIndexArray(nbElements);
  for (unsigned int i = 0; i < nbElements; ++i)
  {
    std::cout << "index " << i << ": " << ElderChildHT1[i] << "\n";
    std::cout.flush();
    if (ElderChildHT1[i] != ElderChildHT3[i])
    {
      rc = 1;
      break;
    }
  }
  return rc;
}

void generateTree(vtkUniformHyperTreeGrid* uhtg, unsigned int treeId)
{
  std::cout << "Initializing tree " << treeId << "\n";
  std::cout.flush();
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  uhtg->InitializeNonOrientedCursor(cursor, treeId, true);
  cursor->SetGlobalIndexStart(uhtg->GetNumberOfVertices());
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

void initializeUniformHyperTreeGrid(vtkUniformHyperTreeGrid* uhtg)
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
  vtkNew<vtkUniformHyperTreeGrid> uhtg1;
  initializeUniformHyperTreeGrid(uhtg1);

  rc += TestElderChildIndexOK(uhtg1);
  CheckTestStatusHTG(rc, "TestElderChildIndexOK");

  return (rc);
}
