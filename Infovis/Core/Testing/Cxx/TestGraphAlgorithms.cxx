/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraphAlgorithms.cxx

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
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphToPolyData.h"
#include "vtkGraphWriter.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkVertexDegree.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void PerformAlgorithm(vtkRenderer* ren, vtkAlgorithm* alg,
  double xoffset, double yoffset,
  const char* vertColorArray, double vertMin, double vertMax,
  const char* edgeColorArray = 0, double edgeMin = 0, double edgeMax = 0)
{
  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(alg->GetOutputPort());

  VTK_CREATE(vtkGlyphSource2D, glyph);
  glyph->SetGlyphTypeToVertex();
  VTK_CREATE(vtkGlyph3D, vertexGlyph);
  vertexGlyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  vertexGlyph->SetInputConnection(1, glyph->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, vertexMapper);
  vertexMapper->SetInputConnection(vertexGlyph->GetOutputPort());
  vertexMapper->SetScalarModeToUsePointFieldData();
  if (vertColorArray)
  {
    vertexMapper->SelectColorArray(vertColorArray);
    vertexMapper->SetScalarRange(vertMin, vertMax);
  }
  VTK_CREATE(vtkActor, vertexActor);
  vertexActor->SetMapper(vertexMapper);
  vertexActor->GetProperty()->SetPointSize(10.0);
  vertexActor->SetPosition(xoffset, yoffset, 0.001);

  VTK_CREATE(vtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeMapper->SetScalarModeToUseCellFieldData();
  if (edgeColorArray)
  {
    edgeMapper->SelectColorArray(edgeColorArray);
    edgeMapper->SetScalarRange(edgeMin, edgeMax);
  }
  VTK_CREATE(vtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->SetPosition(xoffset, yoffset, 0);

  ren->AddActor(vertexActor);
  ren->AddActor(edgeActor);
}

int TestGraphAlgorithms(int argc, char* argv[])
{
  // Create the test graph
  VTK_CREATE(vtkMutableUndirectedGraph, g);

  VTK_CREATE(vtkPoints, pts);
  g->AddVertex();
  pts->InsertNextPoint(0, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.5, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.25, 0.5, 0);
  g->AddVertex();
  pts->InsertNextPoint(0, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.5, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(1, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.75, 0.5, 0);
  g->SetPoints(pts);

  g->AddEdge(0, 1);
  g->AddEdge(0, 2);
  g->AddEdge(1, 2);
  g->AddEdge(2, 3);
  g->AddEdge(2, 4);
  g->AddEdge(3, 4);

  VTK_CREATE(vtkRenderer, ren);

  // Test vertex degree
  VTK_CREATE(vtkVertexDegree, degree);
  degree->SetInputData(g);
  PerformAlgorithm(ren, degree, 0, 0, "VertexDegree", 0, 4);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return !retVal;
}

