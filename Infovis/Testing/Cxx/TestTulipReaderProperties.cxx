/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTulipReaderProperties.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkTulipReader.h"
#include "vtkVariantArray.h"

template<typename value_t>
void TestValue(const value_t& Value, const value_t& ExpectedValue,
               const vtkStdString& ValueDescription, int& ErrorCount)
{
  if(Value == ExpectedValue)
    {
    return;
    }

  cerr << ValueDescription << " is [" << Value << "] - expected ["
       << ExpectedValue << "]" << endl;

  ++ErrorCount;
}

int TestTulipReaderProperties(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                       "Data/Infovis/clustered-graph.tlp");

  cerr << "file: " << file << endl;

  vtkSmartPointer<vtkTulipReader> reader =
      vtkSmartPointer<vtkTulipReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  vtkGraph* const graph = reader->GetOutput();

  int error_count = 0;

  // Test a sample of the node pedigree id property
  vtkVariantArray *nodePedigree= vtkVariantArray::SafeDownCast(
      graph->GetVertexData()->GetPedigreeIds());
  if (nodePedigree)
    {
    TestValue(nodePedigree->GetValue(0), vtkVariant(0),
              "Node 0 pedigree id property", error_count);
    TestValue(nodePedigree->GetValue(5), vtkVariant(5),
              "Node 5 pedigree id property", error_count);
    TestValue(nodePedigree->GetValue(11), vtkVariant(11),
              "Node 11 pedigree id property", error_count);
    }
  else
    {
    cerr << "Node pedigree id property not found." << endl;
    ++error_count;
    }

  // Test a sample of the node string property
  vtkStringArray *nodeProperty1 = vtkStringArray::SafeDownCast(
      graph->GetVertexData()->GetAbstractArray("Node Name"));
  if (nodeProperty1)
    {
    TestValue(nodeProperty1->GetValue(0), vtkStdString("Node A"),
              "Node 0 string property", error_count);
    TestValue(nodeProperty1->GetValue(5), vtkStdString("Node F"),
              "Node 5 string property", error_count);
    TestValue(nodeProperty1->GetValue(11), vtkStdString("Node L"),
              "Node 11 string property", error_count);
    }
  else
    {
    cerr << "Node string property 'Node Name' not found." << endl;
    ++error_count;
    }

  // Test a sample of the node int property
  vtkIntArray *nodeProperty2 = vtkIntArray::SafeDownCast(
      graph->GetVertexData()->GetAbstractArray("Weight"));
  if (nodeProperty2)
    {
    TestValue(nodeProperty2->GetValue(0), 100, "Node 0 int property",
              error_count);
    TestValue(nodeProperty2->GetValue(5), 105, "Node 5 int property",
              error_count);
    TestValue(nodeProperty2->GetValue(11), 111, "Node 11 int property",
              error_count);
    }
  else
    {
    cerr << "Node int property 'Weight' not found." << endl;
    ++error_count;
    }

  // Test a sample of the node double property
  vtkDoubleArray *nodeProperty3 = vtkDoubleArray::SafeDownCast(
      graph->GetVertexData()->GetAbstractArray("Betweenness Centrality"));
  if (nodeProperty3)
    {
    TestValue(nodeProperty3->GetValue(0), 0.0306061, "Node 0 double property",
              error_count);
    TestValue(nodeProperty3->GetValue(5), 0.309697, "Node 5 double property",
              error_count);
    TestValue(nodeProperty3->GetValue(11), 0.0306061, "Node 11 double property",
              error_count);
    }
  else
    {
    cerr << "Node double property 'Betweenness Centrality' not found." << endl;
    ++error_count;
    }

  // Test a sample of the edge string property
  vtkStringArray *edgeProperty1 = vtkStringArray::SafeDownCast(
      graph->GetEdgeData()->GetAbstractArray("Edge Name"));
  if (edgeProperty1)
    {
    TestValue(edgeProperty1->GetValue(0), vtkStdString("Edge A"),
              "Edge 0 string property", error_count);
    TestValue(edgeProperty1->GetValue(7), vtkStdString("Edge H"),
              "Edge 7 string property", error_count);
    TestValue(edgeProperty1->GetValue(16), vtkStdString("Edge Q"),
              "Edge 16 string property", error_count);
    }
  else
    {
    cerr << "Edge string property 'Edge Name' not found." << endl;
    ++error_count;
    }

  // Test a sample of the edge int property
  vtkIntArray *edgeProperty2 = vtkIntArray::SafeDownCast(
      graph->GetEdgeData()->GetAbstractArray("Weight"));
  if (edgeProperty2)
    {
    TestValue(edgeProperty2->GetValue(0), 100, "Edge 0 int property",
              error_count);
    TestValue(edgeProperty2->GetValue(7), 107, "Edge 7 int property",
              error_count);
    TestValue(edgeProperty2->GetValue(16), 116, "Edge 16 int property",
              error_count);
    }
  else
    {
    cerr << "Edge int property 'Weight' not found." << endl;
    ++error_count;
    }

  // Test a sample of the edge pedigree id property
  vtkVariantArray *edgePedigree= vtkVariantArray::SafeDownCast(
      graph->GetEdgeData()->GetPedigreeIds());
  if (edgePedigree)
    {
    TestValue(edgePedigree->GetValue(0), vtkVariant(0),
              "Edge 0 pedigree id property", error_count);
    TestValue(edgePedigree->GetValue(7), vtkVariant(7),
              "Edge 7 pedigree id property", error_count);
    TestValue(edgePedigree->GetValue(16), vtkVariant(16),
              "Edge 16 pedigree id property", error_count);
    }
  else
    {
    cerr << "Edge pedigree id property not found." << endl;
    ++error_count;
    }

  cerr << error_count << " errors" << endl;
  return error_count;
}
