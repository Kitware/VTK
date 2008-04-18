/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStringToCategory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkActor.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkDataSetAttributes.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringToCategory.h"
#include "vtkTestUtilities.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestStringToCategory(int argc, char* argv[])
{
  VTK_CREATE(vtkMutableDirectedGraph, graph);
  VTK_CREATE(vtkStringArray, vertString);
  vertString->SetName("vertex string");
  for (vtkIdType i = 0; i < 10; ++i)
    {
    graph->AddVertex();
    if (i % 2)
      {
      vertString->InsertNextValue("vertex type 1");
      }
    else
      {
      vertString->InsertNextValue("vertex type 2");
      }
    }
  graph->GetVertexData()->AddArray(vertString);
  VTK_CREATE(vtkStringArray, edgeString);
  edgeString->SetName("edge string");
  for (vtkIdType i = 0; i < 10; ++i)
    {
    graph->AddEdge(i, (i+1)%10);
    graph->AddEdge(i, (i+3)%10);
    if (i % 2)
      {
      edgeString->InsertNextValue("edge type 1");
      edgeString->InsertNextValue("edge type 3");
      }
    else
      {
      edgeString->InsertNextValue("edge type 2");
      edgeString->InsertNextValue("edge type 4");
      }
    }
  graph->GetEdgeData()->AddArray(edgeString);

  VTK_CREATE(vtkStringToCategory, vertexCategory);
  vertexCategory->SetInput(graph);
  vertexCategory->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "vertex string");
  vertexCategory->SetCategoryArrayName("vertex category");

  VTK_CREATE(vtkStringToCategory, edgeCategory);
  edgeCategory->SetInputConnection(vertexCategory->GetOutputPort());
  edgeCategory->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_EDGES, "edge string");
  edgeCategory->SetCategoryArrayName("edge category");

  VTK_CREATE(vtkCircularLayoutStrategy, strategy);
  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(edgeCategory->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection(layout->GetOutputPort());
  mapper->SetEdgeColorArrayName("edge category");
  mapper->ColorEdgesOn();
  mapper->SetVertexColorArrayName("vertex category");
  mapper->ColorVerticesOn();
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}
