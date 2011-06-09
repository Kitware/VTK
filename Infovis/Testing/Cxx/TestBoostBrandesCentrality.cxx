/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoostAlgorithms.cxx

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
#include "vtkBoostBrandesCentrality.h"
#include "vtkBoostBreadthFirstSearch.h"
#include "vtkBoostBreadthFirstSearchTree.h"
#include "vtkBoostConnectedComponents.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraphLayoutView.h"
#include "vtkGraphToPolyData.h"
#include "vtkGraphWriter.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#include <boost/version.hpp>
#include "vtkBoostBiconnectedComponents.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestBoostBrandesCentrality(int argc, char* argv[])
{
  // Create the test graph
  VTK_CREATE(vtkMutableUndirectedGraph, g);

  VTK_CREATE(vtkMatrix4x4, mat1);
  mat1->SetElement(1,3, 5);
  VTK_CREATE(vtkTransform, transform1);
  transform1->SetMatrix(mat1);

  VTK_CREATE(vtkMatrix4x4, mat2);
  mat2->SetElement(1,3, 0);
  VTK_CREATE(vtkTransform, transform2);
  transform2->SetMatrix(mat2);

  VTK_CREATE(vtkFloatArray, weights);
  weights->SetName("weights");
  g->GetEdgeData()->AddArray(weights);

  VTK_CREATE(vtkPoints, pts);
  g->AddVertex();
  pts->InsertNextPoint(1, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(1, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(1, -1, 0);
  g->AddVertex();
  pts->InsertNextPoint(2, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(3, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(2.5, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(4, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(4, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(4, -1, 0);

  g->SetPoints(pts);

  vtkEdgeType e = g->AddEdge(0, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(1, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(2, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(3, 4);
  weights->InsertTuple1(e.Id, 1);

  e = g->AddEdge(3, 5);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(5, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(6, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(7, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(8, 4);
  weights->InsertTuple1(e.Id, 10);

  // Test centrality
  VTK_CREATE(vtkBoostBrandesCentrality, centrality);
  centrality->SetInputData(g);
  centrality->SetEdgeWeightArrayName("weights");
  centrality->SetInvertEdgeWeightArray(1);
  centrality->UseEdgeWeightArrayOn();

  VTK_CREATE(vtkGraphLayoutView, view);
  view->SetLayoutStrategyToPassThrough();
  view->SetRepresentationFromInputConnection(centrality->GetOutputPort());
  view->ResetCamera();
  view->SetColorVertices(1);
  view->SetVertexColorArrayName("centrality");
  view->SetColorEdges(1);
  view->SetEdgeColorArrayName("centrality");

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}
