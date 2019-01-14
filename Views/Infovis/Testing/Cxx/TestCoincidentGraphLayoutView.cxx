/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCoincidentGraphLayoutView.cxx

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
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkGraphLayoutView.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPoints.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkXMLTreeReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <sstream>

template<typename T> std::string ToString(const T& x)
{
  std::ostringstream oss;
  oss << x;
  return oss.str();
}

int TestCoincidentGraphLayoutView(int argc, char* argv[])
{
  VTK_CREATE(vtkMutableUndirectedGraph, graph);
  VTK_CREATE(vtkPoints, points);
  VTK_CREATE(vtkDoubleArray, pointData);
  pointData->SetNumberOfComponents(3);
  points->SetData(static_cast<vtkDataArray *>(pointData));
  graph->SetPoints(points);
  vtkIdType i = 0;

  for(i = 0; i < 10; i++)
  {
    graph->AddVertex();
    points->InsertNextPoint(0.0, 0.0, 0.0);
  }

  graph->AddVertex();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(3.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, 2.5, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(0.0, -2.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, -1.5, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(-1.0, 2.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(3.0, 0.0, 0.0);

  for(i = 1; i < 10; i++)
  {
    graph->AddEdge(0, i);
  }

  for(i = 10; i < 17; i++)
  {
    graph->AddEdge(i, i + 1);
  }
  graph->AddEdge(0, 10);


  VTK_CREATE(vtkStringArray, name);
  name->SetName("name");

  for (i = 0; i < graph->GetNumberOfVertices(); i++)
  {
    name->InsertNextValue("Vert" + ToString(i));
  }
  graph->GetVertexData()->AddArray(name);

  VTK_CREATE(vtkStringArray, label);
  label->SetName("edge label");
  VTK_CREATE(vtkIdTypeArray, dist);
  dist->SetName("distance");
  for (i = 0; i < graph->GetNumberOfEdges(); i++)
  {
    dist->InsertNextValue(i);
    switch (i % 4)
    {
      case 0:
        label->InsertNextValue("a");
        break;
      case 1:
        label->InsertNextValue("b");
        break;
      case 2:
        label->InsertNextValue("c");
        break;
      case 3:
        label->InsertNextValue("d");
        break;
    }
  }
  graph->GetEdgeData()->AddArray(dist);
  graph->GetEdgeData()->AddArray(label);

  // Graph layout view
  VTK_CREATE(vtkGraphLayoutView, view);
  view->DisplayHoverTextOff();
  view->SetLayoutStrategyToPassThrough();
  view->SetVertexLabelArrayName("name");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("size");
  view->ColorVerticesOn();
  view->SetEdgeColorArrayName("distance");
  view->ColorEdgesOn();
  view->SetEdgeLabelArrayName("edge label");
  view->EdgeLabelVisibilityOn();
  view->SetRepresentationFromInput(graph);

  view->ResetCamera();
  view->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
  }

  return !retVal;
}
