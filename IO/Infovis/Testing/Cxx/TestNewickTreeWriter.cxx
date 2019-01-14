/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNewickTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkNew.h"
#include "vtkNewickTreeReader.h"
#include "vtkNewickTreeWriter.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

int TestNewickTreeWriter(int argc, char* argv[])
{
  // get the full path to the input file
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                       "Data/Infovis/rep_set.tre");
  cout << "reading from a file: "<< file <<  endl;

  // read the input file into a vtkTree
  vtkNew<vtkNewickTreeReader> reader1;
  reader1->SetFileName(file);
  reader1->Update();
  vtkTree *tree1 = reader1->GetOutput();
  delete[] file;

  // write this vtkTree out to a string
  vtkNew<vtkNewickTreeWriter> writer;
  writer->WriteToOutputStringOn();
  writer->SetInputData(tree1);
  writer->Update();
  std::string treeString = writer->GetOutputStdString();

  // read this string back in, creating another vtkTree
  vtkNew<vtkNewickTreeReader> reader2;
  reader2->ReadFromInputStringOn();
  reader2->SetInputString(treeString);
  reader2->Update();
  vtkTree *tree2 = reader2->GetOutput();

  // compare these two trees.  This test fails if it detects any differences
  // between them.
  vtkIdType numVerticesTree1 = tree1->GetNumberOfVertices();
  vtkIdType numVerticesTree2 = tree2->GetNumberOfVertices();
  if (numVerticesTree1 != numVerticesTree2)
  {
    cout << "number of vertices is not equal: " << numVerticesTree1
         << " vs. " << numVerticesTree2 << endl;
    return EXIT_FAILURE;
  }

  vtkIdType numEdgesTree1 = tree1->GetNumberOfEdges();
  vtkIdType numEdgesTree2 = tree2->GetNumberOfEdges();
  if (numEdgesTree1 != numEdgesTree2)
  {
    cout << "number of edges is not equal: " << numEdgesTree1 <<
            " vs. " << numEdgesTree2 << endl;
    return EXIT_FAILURE;
  }

  for (vtkIdType vertex = 0; vertex < numVerticesTree1; ++vertex)
  {
    if (tree1->GetParent(vertex) != tree2->GetParent(vertex))
    {
      cout << "tree1 and tree2 do not agree on the parent of vertex " << vertex
           << endl;
      return EXIT_FAILURE;
    }
    if (tree1->GetNumberOfChildren(vertex) !=
        tree2->GetNumberOfChildren(vertex))
    {
      cout << "tree1 and tree2 do not agree on the number of children "
           << "for vertex " << vertex << endl;
      return EXIT_FAILURE;
    }
  }

  vtkAbstractArray *names1 =
    tree1->GetVertexData()->GetAbstractArray("node name");
  vtkAbstractArray *names2 =
    tree2->GetVertexData()->GetAbstractArray("node name");
  if (names1->GetNumberOfTuples() != names2->GetNumberOfTuples())
  {
    cout << "the names arrays are of different sizes" << endl;
    return EXIT_FAILURE;
  }
  for (vtkIdType v = 0; v < names1->GetNumberOfTuples(); v++)
  {
    if (names1->GetVariantValue(v) != names2->GetVariantValue(v))
    {
      cout << "tree1 and tree2 do not agree on the name of vertex " << v << endl;
      return EXIT_FAILURE;
    }
  }

  vtkAbstractArray *weights1 = tree1->GetEdgeData()->GetAbstractArray("weight");
  vtkAbstractArray *weights2 = tree2->GetEdgeData()->GetAbstractArray("weight");
  if (weights1->GetNumberOfTuples() != weights2->GetNumberOfTuples())
  {
    cout << "the weights arrays are of different sizes" << endl;
    return EXIT_FAILURE;
  }
  for (vtkIdType e = 0; e < weights1->GetNumberOfTuples(); e++)
  {
    if (weights1->GetVariantValue(e) != weights2->GetVariantValue(e))
    {
      cout << "tree1 and tree2 do not agree on the weight of edge " << e << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
