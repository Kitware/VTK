// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationStringKey.h"
#include "vtkNew.h"
#include "vtkPhyloXMLTreeReader.h"
#include "vtkPhyloXMLTreeWriter.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"
#include "vtkUnsignedCharArray.h"

#include <iostream>

//------------------------------------------------------------------------------
bool VerifyArrayValue(vtkTree* tree, vtkIdType index, const char* arrayName, const char* baseline)
{
  vtkAbstractArray* array = tree->GetVertexData()->GetAbstractArray(arrayName);
  if (!array)
  {
    std::cout << "could not find " << arrayName << std::endl;
    return false;
  }
  std::string value = array->GetVariantValue(index).ToString();
  if (value != baseline)
  {
    std::cout << "value for " << arrayName << " is " << value << ", should be " << baseline
              << std::endl;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool VerifyArrayAttribute(
  vtkTree* tree, const char* arrayName, const char* attributeName, const char* baseline)
{
  vtkAbstractArray* array = tree->GetVertexData()->GetAbstractArray(arrayName);
  if (!array)
  {
    std::cout << "could not find " << arrayName << std::endl;
    return false;
  }
  vtkInformation* info = array->GetInformation();
  vtkNew<vtkInformationIterator> infoItr;
  infoItr->SetInformation(info);
  for (infoItr->InitTraversal(); !infoItr->IsDoneWithTraversal(); infoItr->GoToNextItem())
  {
    vtkInformationStringKey* key = vtkInformationStringKey::SafeDownCast(infoItr->GetCurrentKey());
    if (strcmp(key->GetName(), attributeName) == 0)
    {
      std::string value = info->Get(key);
      if (value == baseline)
      {
        return true;
      }
      else
      {
        std::cout << "found " << value << " for " << arrayName << "'s " << attributeName
                  << " attribute.  Expected " << baseline << std::endl;
        return false;
      }
    }
  }
  std::cout << "could not find " << attributeName << " for " << arrayName << std::endl;
  return false;
}

//------------------------------------------------------------------------------
bool VerifyColor(vtkTree* tree, vtkIdType vertex, unsigned char r, unsigned char g, unsigned char b)
{
  vtkUnsignedCharArray* array =
    vtkArrayDownCast<vtkUnsignedCharArray>(tree->GetVertexData()->GetAbstractArray("color"));
  if (!array)
  {
    std::cout << "could not find color array" << std::endl;
    return false;
  }
  if (array->GetNumberOfComponents() != 3)
  {
    std::cout << "color array does not have 3 components" << std::endl;
  }
  double* color = array->GetTuple3(vertex);
  if (color[0] != r)
  {
    std::cout << "red value " << color[0] << " found for vertex " << vertex << ".  Should be " << r
              << std::endl;
    return false;
  }
  if (color[1] != g)
  {
    std::cout << "green value " << color[1] << " found for vertex " << vertex << ".  Should be "
              << g << std::endl;
    return false;
  }
  if (color[2] != b)
  {
    std::cout << "blue value " << color[2] << " found for vertex " << vertex << ".  Should be " << b
              << std::endl;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
int TestPhyloXMLTreeReadWrite(int argc, char* argv[])
{
  // get the full path to the input file
  char* inputFile =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/XML/example_phylo.xml");
  std::cout << "reading from a file: " << inputFile << std::endl;

  // read the input file into a vtkTree
  vtkNew<vtkPhyloXMLTreeReader> reader;
  reader->SetFileName(inputFile);
  reader->Update();
  vtkTree* tree = reader->GetOutput();
  delete[] inputFile;

  // time to verify that the tree was read correctly.
  // 1: it exists
  if (!tree)
  {
    std::cout << "No output tree" << std::endl;
    return EXIT_FAILURE;
  }

  // 2: it has the right number of vertices
  vtkIdType numVertices = tree->GetNumberOfVertices();
  if (numVertices != 6)
  {
    std::cout << "tree has " << numVertices << " vertices (should be 6)." << std::endl;
    return EXIT_FAILURE;
  }

  // 3: its topology seems correct
  int numChildren[6] = { 1, 2, 2, 0, 0, 0 };
  for (vtkIdType vertex = 0; vertex < 6; ++vertex)
  {
    if (tree->GetNumberOfChildren(vertex) != numChildren[vertex])
    {
      std::cout << "incorrect number of children for vertex " << vertex << "should be "
                << numChildren[vertex] << ", found " << tree->GetNumberOfChildren(vertex)
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  // 4: verify vertex data

  // tree-level data
  if (!VerifyArrayValue(tree, 0, "phylogeny.name", "example tree"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(
        tree, 0, "phylogeny.description", "example tree to test PhyloXML reader and writer"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 0, "phylogeny.confidence", "0.99"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.confidence", "type", "probability"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayValue(tree, 0, "phylogeny.property.length", "1"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.property.length", "authority", "NOAA"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.property.length", "applies_to", "phylogeny"))
  {
    return EXIT_FAILURE;
  }
  if (!VerifyArrayAttribute(tree, "phylogeny.property.length", "unit", "METRIC:m"))
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
  double weights[5] = { 1.0, 2.0, 1.0, 1.0, 3.0 };
  vtkAbstractArray* weightArray = tree->GetEdgeData()->GetAbstractArray("weight");
  if (!weightArray)
  {
    std::cout << "could not find weight array" << std::endl;
    return EXIT_FAILURE;
  }
  for (vtkIdType edge = 0; edge < tree->GetNumberOfEdges(); ++edge)
  {
    double value = weightArray->GetVariantValue(edge).ToDouble();
    if (value != weights[edge])
    {
      std::cout << "weight " << value << " found for edge #" << edge << ", expected "
                << weights[edge] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // end of tree verification.
  // next step:
  // write this vtkTree out to to a string in PhyloXML format
  vtkNew<vtkPhyloXMLTreeWriter> writer;
  writer->SetInputData(tree);
  writer->SetWriteToOutputString(true);
  writer->IgnoreArray("node weight");
  writer->Update();
  std::string phyloXML = writer->GetOutputString();

  // recreate a vtkTree from this PhyloXML string.
  vtkNew<vtkPhyloXMLTreeReader> reader2;
  reader2->SetReadFromInputString(1);
  reader2->SetInputString(phyloXML);
  reader2->Update();
  vtkTree* tree2 = reader2->GetOutput();

  // write it back out to PhyloXML again and verify that it is
  // identical to our previous PhyloXML string.
  vtkNew<vtkPhyloXMLTreeWriter> writer2;
  writer2->SetInputData(tree2);
  writer2->SetWriteToOutputString(true);
  writer2->IgnoreArray("node weight");
  writer2->Update();
  std::string phyloXML2 = writer2->GetOutputString();
  if (phyloXML != phyloXML2)
  {
    std::cout << "output strings do not match." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
