/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNewickTreeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNewickTreeReader.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

int TestNewickTreeReader(int argc, char* argv[])
{
  // reading from a file
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                       "Data/Infovis/rep_set.tre");

  cout << "reading from a file: "<< file <<  endl;

  vtkSmartPointer<vtkNewickTreeReader> reader1 =
      vtkSmartPointer<vtkNewickTreeReader>::New();
  reader1->SetFileName(file);
  delete[] file;
  reader1->Update();
  vtkTree *tree1 = reader1->GetOutput();


  if (tree1->GetNumberOfVertices() != 836)
  {
    cerr << "Wrong number of Vertices: "<< tree1->GetNumberOfVertices() << endl;
    return 1;
  }

  if (tree1->GetNumberOfEdges() != 835)
  {
    cerr << "Wrong number of Edges: "<< tree1->GetNumberOfEdges() << endl;
    return 1;
  }


  //reading from a string
  cout << "reading from a  string"<< endl;
  char inputStr[] =  "(((A:0.1,B:0.2,(C:0.3,D:0.4)E:0.5)F:0.6,G:0.7)H:0.8,I:0.9);";

  vtkSmartPointer<vtkNewickTreeReader> reader2 =
      vtkSmartPointer<vtkNewickTreeReader>::New();
  reader2->SetReadFromInputString(1);
  reader2->SetInputString(inputStr);
  reader2->Update();
  vtkTree *tree2 = reader2->GetOutput();

  if (tree2->GetNumberOfVertices() != 10)
  {
    cerr << "Wrong number of Vertices: "<< tree2->GetNumberOfVertices() << endl;
    return 1;
  }

  if (tree2->GetNumberOfEdges() != 9)
  {
    cerr << "Wrong number of Edges: "<< tree2->GetNumberOfEdges() << endl;
    return 1;
  }

  return 0;
}
