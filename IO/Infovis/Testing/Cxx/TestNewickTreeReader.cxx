// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNewickTreeReader.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

#include <iostream>

int TestNewickTreeReader(int argc, char* argv[])
{
  // reading from a file
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/rep_set.tre");

  std::cout << "reading from a file: " << file << std::endl;

  vtkSmartPointer<vtkNewickTreeReader> reader1 = vtkSmartPointer<vtkNewickTreeReader>::New();
  reader1->SetFileName(file);
  delete[] file;
  reader1->Update();
  vtkTree* tree1 = reader1->GetOutput();

  if (tree1->GetNumberOfVertices() != 836)
  {
    std::cerr << "Wrong number of Vertices: " << tree1->GetNumberOfVertices() << std::endl;
    return 1;
  }

  if (tree1->GetNumberOfEdges() != 835)
  {
    std::cerr << "Wrong number of Edges: " << tree1->GetNumberOfEdges() << std::endl;
    return 1;
  }

  // reading from a string
  std::cout << "reading from a string" << std::endl;
  char inputStr[] = "(((A:0.1,B:0.2,(C:0.3,D:0.4)E:0.5)F:0.6,G:0.7)H:0.8,I:0.9);";

  vtkSmartPointer<vtkNewickTreeReader> reader2 = vtkSmartPointer<vtkNewickTreeReader>::New();
  reader2->SetReadFromInputString(1);
  reader2->SetInputString(inputStr);
  reader2->Update();
  vtkTree* tree2 = reader2->GetOutput();

  if (tree2->GetNumberOfVertices() != 10)
  {
    std::cerr << "Wrong number of Vertices: " << tree2->GetNumberOfVertices() << std::endl;
    return 1;
  }

  if (tree2->GetNumberOfEdges() != 9)
  {
    std::cerr << "Wrong number of Edges: " << tree2->GetNumberOfEdges() << std::endl;
    return 1;
  }

  return 0;
}
