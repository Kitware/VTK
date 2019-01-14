/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPhyloXMLTreeReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationStringKey.h"
#include "vtkNew.h"
#include "vtkPhyloXMLTreeReader.h"
#include "vtkPhyloXMLTreeWriter.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"
#include "vtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
bool VerifyArrayValue(vtkTree *tree, vtkIdType index, const char* arrayName,
                      const char* baseline)
{
  vtkAbstractArray *array =
    tree->GetVertexData()->GetAbstractArray(arrayName);
  if (!array)
  {
    cout << "could not find " << arrayName << endl;
    return false;
  }
  std::string value  = array->GetVariantValue(index).ToString();
  if (value.compare(baseline) != 0)
  {
    cout << "value for " << arrayName << " is " << value << ", should be "
         << baseline << endl;
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool VerifyArrayAttribute(vtkTree *tree, const char *arrayName,
                          const char *attributeName, const char *baseline)
{
  vtkAbstractArray *array =
    tree->GetVertexData()->GetAbstractArray(arrayName);
  if (!array)
  {
    cout << "could not find " << arrayName << endl;
    return false;
  }
  vtkInformation *info = array->GetInformation();
  vtkNew<vtkInformationIterator> infoItr;
  infoItr->SetInformation(info);
  for (infoItr->InitTraversal(); !infoItr->IsDoneWithTraversal();
    infoItr->GoToNextItem())
  {
    vtkInformationStringKey* key =
      vtkInformationStringKey::SafeDownCast(infoItr->GetCurrentKey());
    if (strcmp(key->GetName(), attributeName) == 0)
    {
      std::string value = info->Get(key);
      if (value.compare(baseline) == 0)
      {
        return true;
      }
      else
      {
        cout << "found " << value << " for " << arrayName << "'s "
             << attributeName << " attribute.  Expected " << baseline << endl;
        return false;
      }
    }
  }
  cout << "could not find " << attributeName << " for " << arrayName << endl;
  return false;
}

//----------------------------------------------------------------------------
bool VerifyColor(vtkTree *tree, vtkIdType vertex, unsigned char r, unsigned char g, unsigned char b)
{
  vtkUnsignedCharArray *array = vtkArrayDownCast<vtkUnsignedCharArray>(
    tree->GetVertexData()->GetAbstractArray("color"));
  if (!array)
  {
    cout << "could not find color array" << endl;
    return false;
  }
  if (array->GetNumberOfComponents() != 3)
  {
    cout << "color array does not have 3 components" << endl;
  }
  double *color = array->GetTuple3(vertex);
  if (color[0] != r)
  {
    cout << "red value " << color[0] << " found for vertex " << vertex
         << ".  Should be " << r << endl;
    return false;
  }
  if (color[1] != g)
  {
    cout << "green value " << color[1] << " found for vertex " << vertex
         << ".  Should be " << g << endl;
    return false;
  }
  if (color[2] != b)
  {
    cout << "blue value " << color[2] << " found for vertex " << vertex
         << ".  Should be " << b << endl;
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
int TestPhyloXMLTreeReadWrite(int argc, char* argv[])
{
  // get the full path to the input file
  char* inputFile = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/Infovis/XML/example_phylo.xml");
  cout << "reading from a file: "<< inputFile <<  endl;

  // read the input file into a vtkTree
  vtkNew<vtkPhyloXMLTreeReader> reader;
  reader->SetFileName(inputFile);
  reader->Update();
  vtkTree *tree = reader->GetOutput();
  delete[] inputFile;

  // time to verify that the tree was read correctly.
  // 1: it exists
  if (!tree)
  {
    cout << "No output tree" << endl;
    return EXIT_FAILURE;
  }

  // 2: it has the right number of vertices
  vtkIdType numVertices = tree->GetNumberOfVertices();
  if (numVertices != 6)
  {
    cout << "tree has " << numVertices << " vertices (should be 6)." << endl;
    return EXIT_FAILURE;
  }

 // 3: its topology seems correct
 int numChildren[6] = {1, 2, 2, 0, 0, 0};
 for (vtkIdType vertex = 0; vertex < 6; ++vertex)
 {
   if (tree->GetNumberOfChildren(vertex) != numChildren[vertex])
   {
     cout << "incorrect number of children for vertex " << vertex
          << "should be " << numChildren[vertex] << ", found "
          << tree->GetNumberOfChildren(vertex) << endl;
     return EXIT_FAILURE;
   }
 }

  // 4: verify vertex data

  // tree-level data
  if (!VerifyArrayValue(tree, 0, "phylogeny.name", "example tree"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 0, "phylogeny.description",
      "example tree to test PhyloXML reader and writer"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 0, "phylogeny.confidence", "0.99"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.confidence", "type",
      "probability"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 0, "phylogeny.property.length", "1"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.property.length", "authority",
      "NOAA"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.property.length", "applies_to",
      "phylogeny"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.property.length", "unit",
      "METRIC:m"))
  {
    return EXIT_FAILURE;
  }

  // vertex names
  if (!VerifyArrayValue(tree, 0, "node name", "root"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 1, "node name", "internalOne"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 2, "node name", "internalTwo"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 3, "node name", "a"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 4, "node name", "b"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 5, "node name", "c"))
  {
    return EXIT_FAILURE;
  }

  // vertex confidence
  if (!VerifyArrayValue(tree, 0, "confidence", "0.95"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 1, "confidence", "0.9"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 2, "confidence", "0.85"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 3, "confidence", "0.8"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 4, "confidence", "0.75"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 5, "confidence", "0.85"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "confidence", "type", "probability"))
  {
    return EXIT_FAILURE;
  }

  // vertex length (custom property)
  if (!VerifyArrayValue(tree, 0, "property.length", "0"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 1, "property.length", "2"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 2, "property.length", "3"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 3, "property.length", "4"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 4, "property.length", "5"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 5, "property.length", "6"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "property.length", "authority", "NOAA"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "property.length", "applies_to", "clade"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "property.length", "unit", "METRIC:m"))
  {
    return EXIT_FAILURE;
  }

  // color
  if (!VerifyColor(tree, 0, 0, 0, 0))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyColor(tree, 1, 0, 0, 0))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyColor(tree, 2, 255, 0, 0))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyColor(tree, 3, 255, 0, 0))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyColor(tree, 4, 0, 255, 0))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyColor(tree, 5, 0, 0, 255))
  {
    return EXIT_FAILURE;
  }

 // 5: EdgeData (just weights for now)
 double weights[5] = {1.0, 2.0, 1.0, 1.0, 3.0};
 vtkAbstractArray *weightArray =
   tree->GetEdgeData()->GetAbstractArray("weight");
 if (!weightArray)
 {
   cout << "could not find weight array" << endl;
   return EXIT_FAILURE;
 }
 for (vtkIdType edge = 0; edge < tree->GetNumberOfEdges(); ++edge)
 {
   double value = weightArray->GetVariantValue(edge).ToDouble();
   if (value != weights[edge])
   {
     cout << "weight " << value << " found for edge #" << edge
          << ", expected " << weights[edge] << endl;
     return EXIT_FAILURE;
   }
 }

  // end of tree verification.
  // next step:
  // write this vtkTree out to to a string in PhyloXML format
  vtkNew<vtkPhyloXMLTreeWriter> writer;
  writer->SetInputData(tree);
  writer->SetWriteToOutputString(1);
  writer->IgnoreArray("node weight");
  writer->Update();
  std::string phyloXML = writer->GetOutputString();

  // recreate a vtkTree from this PhyloXML string.
  vtkNew<vtkPhyloXMLTreeReader> reader2;
  reader2->SetReadFromInputString(1);
  reader2->SetInputString(phyloXML);
  reader2->Update();
  vtkTree *tree2 = reader2->GetOutput();

  // write it back out to PhyloXML again and verify that it is
  // identical to our previous PhyloXML string.
  vtkNew<vtkPhyloXMLTreeWriter> writer2;
  writer2->SetInputData(tree2);
  writer2->SetWriteToOutputString(1);
  writer2->IgnoreArray("node weight");
  writer2->Update();
  std::string phyloXML2 = writer2->GetOutputString();
  if (phyloXML.compare(phyloXML2) != 0)
  {
    cout << "output strings do not match." << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
