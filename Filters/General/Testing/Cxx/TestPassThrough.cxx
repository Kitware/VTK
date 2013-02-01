/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBooleanOperationPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPassThrough.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

bool CompareData(vtkGraph* Output, vtkGraph* Input)
{
  bool inputDirected = (vtkDirectedGraph::SafeDownCast(Input) != 0);
  bool outputDirected = (vtkDirectedGraph::SafeDownCast(Output) != 0);
  if(inputDirected != outputDirected)
    {
    std::cerr << "Directedness not the same" << std::endl;
    return false;
    }

  if(Input->GetNumberOfVertices() != Output->GetNumberOfVertices())
    {
    std::cerr << "GetNumberOfVertices not the same" << std::endl;
    return false;
    }

  if(Input->GetNumberOfEdges() != Output->GetNumberOfEdges())
    {
    std::cerr << "GetNumberOfEdges not the same" << std::endl;
    return false;
    }

  if(Input->GetVertexData()->GetNumberOfArrays() != Output->GetVertexData()->GetNumberOfArrays())
    {
    std::cerr << "GetVertexData()->GetNumberOfArrays() not the same" << std::endl;
    return false;
    }

  if(Input->GetEdgeData()->GetNumberOfArrays() != Output->GetEdgeData()->GetNumberOfArrays())
    {
    std::cerr << "GetEdgeData()->GetNumberOfArrays() not the same" << std::endl;
    return false;
    }

  vtkEdgeListIterator *inputEdges = vtkEdgeListIterator::New();
  vtkEdgeListIterator *outputEdges = vtkEdgeListIterator::New();
  while(inputEdges->HasNext())
    {
    vtkEdgeType inputEdge = inputEdges->Next();
    vtkEdgeType outputEdge = outputEdges->Next();
    if(inputEdge.Source != outputEdge.Source)
      {
      std::cerr << "Input source != output source" << std::endl;
      return false;
      }

    if(inputEdge.Target != outputEdge.Target)
      {
      std::cerr << "Input target != output target" << std::endl;
      return false;
      }
    }
  inputEdges->Delete();
  outputEdges->Delete();

  return true;
}

int TestPassThrough(int , char* [])
{
  std::cerr << "Generating graph ..." << std::endl;
  VTK_CREATE(vtkMutableDirectedGraph, g);
  VTK_CREATE(vtkDoubleArray, x);
  x->SetName("x");
  VTK_CREATE(vtkDoubleArray, y);
  y->SetName("y");
  VTK_CREATE(vtkDoubleArray, z);
  z->SetName("z");
  for (vtkIdType i = 0; i < 10; ++i)
    {
    for (vtkIdType j = 0; j < 10; ++j)
      {
      g->AddVertex();
      x->InsertNextValue(i);
      y->InsertNextValue(j);
      z->InsertNextValue(1);
      }
    }
  g->GetVertexData()->AddArray(x);
  g->GetVertexData()->AddArray(y);
  g->GetVertexData()->AddArray(z);
  std::cerr << "... done" << std::endl;

  VTK_CREATE(vtkPassThrough, pass);
  pass->SetInputData(g);
  pass->Update();
  vtkGraph *output = vtkGraph::SafeDownCast(pass->GetOutput());

  if (!CompareData(g, output))
    {
    std::cerr << "ERROR: Graphs not identical!" << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
