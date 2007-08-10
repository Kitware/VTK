/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoostTreeLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkActor.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkGraphWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTreeLayoutStrategy.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void RenderGraph(vtkRenderer* ren, vtkAbstractGraphAlgorithm* alg, 
  double xoffset, double yoffset, 
  const char* vertColorArray, double vertMin, double vertMax, 
  const char* edgeColorArray, double edgeMin, double edgeMax)
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

int TestBoostTreeLayoutStrategy(int argc, char* argv[])
{
  // Create the test graph
  VTK_CREATE(vtkGraph, g);
  g->SetDirected(false);

  VTK_CREATE(vtkPoints, pts);
  g->AddVertex();
  g->AddVertex();
  g->AddVertex();
  g->AddVertex();
  g->AddVertex();
  g->AddVertex();
  g->AddVertex();

  g->AddEdge(0, 1);
  g->AddEdge(0, 2);
  g->AddEdge(1, 2);
  g->AddEdge(2, 3);
  g->AddEdge(2, 4);
  g->AddEdge(3, 4);
  g->AddEdge(4, 5);
  g->AddEdge(4, 6);
  g->AddEdge(5, 6);

  VTK_CREATE(vtkRenderer, ren);

  // Test breadth first search
  VTK_CREATE(vtkGraphLayout, layout);
  VTK_CREATE(vtkTreeLayoutStrategy, strategy);
  layout->SetLayoutStrategy(strategy);
  layout->SetInput(g);
  RenderGraph(ren, layout, 0, 0, NULL, 0, 0, NULL, 0, 0);

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

