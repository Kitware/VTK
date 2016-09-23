/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestKdTreeBoxSelection.cxx

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
#include "vtkAreaPicker.h"
#include "vtkBSPCuts.h"
#include "vtkCubeSource.h"
#include "vtkFloatArray.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGlyph3D.h"
#include "vtkGraph.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkKdNode.h"
#include "vtkKdTree.h"
#include "vtkLookupTable.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRandomGraphSource.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkTree.h"
#include "vtkTreeLevelsFilter.h"
#include "vtkTreeMapToPolyData.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//
// Make a vtkTree from a kd-tree
//
void BuildTree(vtkIdType parent, vtkKdNode *parentVertex, vtkMutableDirectedGraph *tree, vtkFloatArray *rectArray)
{
  double bounds[6];
  parentVertex->GetBounds(bounds);
  rectArray->InsertTuple(parent, bounds);
  if (parentVertex->GetLeft() != NULL)
  {
    vtkIdType curIndex = tree->AddChild(parent);
    BuildTree(curIndex, parentVertex->GetLeft(), tree, rectArray);
    curIndex = tree->AddChild(parent);
    BuildTree(curIndex, parentVertex->GetRight(), tree, rectArray);
  }
}

int TestKdTreeBoxSelection(int argc, char *argv[])
{
  bool interactive = false;
  bool threedim = false;
  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-I"))
    {
      interactive = true;
      continue;
    }
    if (!strcmp(argv[i], "-d"))
    {
      threedim = true;
      continue;
    }

    cerr << argv[0] << " options:\n"
      << "  -I run interactively\n"
      << "  -d three-dimensional\n";
    return 0;
  }

  //
  // Create a random graph and perform layout
  //

  VTK_CREATE(vtkRandomGraphSource, source);
  source->SetStartWithTree(true);
  source->SetNumberOfVertices(100);
  source->SetNumberOfEdges(15);

  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(source->GetOutputPort());
  if (threedim)
  {
    VTK_CREATE(vtkForceDirectedLayoutStrategy, forceLayout);
    forceLayout->SetGraphBounds(-3, 3, -3, 3, -3, 3);
    layout->SetLayoutStrategy(forceLayout);
  }
  else
  {
    VTK_CREATE(vtkSimple2DLayoutStrategy, simpleLayout);
    simpleLayout->SetJitter(true);
    layout->SetLayoutStrategy(simpleLayout);
  }

  layout->Update();
  vtkGraph* g = vtkGraph::SafeDownCast(layout->GetOutput());

  //
  // Create the kd-tree
  //

  VTK_CREATE(vtkKdTree, kdTree);
  kdTree->OmitZPartitioning();
  kdTree->SetMinCells(1);
  kdTree->BuildLocatorFromPoints(g->GetPoints());

  //
  // Perform an area selection
  //

  VTK_CREATE(vtkIdTypeArray, selection);
  double bounds[6] =
    {-2, 2, -0.5, 3, -1, 1};
    //{-1, 1, -1, 1, -1, 1};
  kdTree->FindPointsInArea(bounds, selection);

  //
  // Create selected vertex glyphs
  //

  double glyphSize = 0.05;

  VTK_CREATE(vtkPolyData, selectPoly);
  VTK_CREATE(vtkPoints, selectPoints);
  double pt[3];
  for (vtkIdType i = 0; i < selection->GetNumberOfTuples(); i++)
  {
    g->GetPoint(selection->GetValue(i), pt);
    selectPoints->InsertNextPoint(pt);
  }
  selectPoly->SetPoints(selectPoints);

  VTK_CREATE(vtkSphereSource, selectSphere);
  selectSphere->SetRadius(1.1*glyphSize);

  VTK_CREATE(vtkGlyph3D, selectGlyph);
  selectGlyph->SetInputData(0, selectPoly);
  selectGlyph->SetInputConnection(1, selectSphere->GetOutputPort());

  VTK_CREATE(vtkPolyDataMapper, selectMapper);
  selectMapper->SetInputConnection(selectGlyph->GetOutputPort());

  VTK_CREATE(vtkActor, selectActor);
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  //
  // Create selection box actor
  //

  VTK_CREATE(vtkCubeSource, cubeSource);
  cubeSource->SetBounds(bounds);

  VTK_CREATE(vtkPolyDataMapper, cubeMapper);
  cubeMapper->SetInputConnection(cubeSource->GetOutputPort());

  VTK_CREATE(vtkActor, cubeActor);
  cubeActor->SetMapper(cubeMapper);
  cubeActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
  cubeActor->GetProperty()->SetOpacity(0.5);

  //
  // Create kd-tree actor
  //

  VTK_CREATE(vtkMutableDirectedGraph, tree);
  VTK_CREATE(vtkFloatArray, rectArray);
  rectArray->SetName("rectangles");
  rectArray->SetNumberOfComponents(4);
  tree->GetVertexData()->AddArray(rectArray);
  vtkKdNode* top = kdTree->GetCuts()->GetKdNodeTree();
  BuildTree(tree->AddVertex(), top, tree, rectArray);

  VTK_CREATE(vtkTree, realTree);
  if (!realTree->CheckedShallowCopy(tree))
  {
    cerr << "Invalid tree structure." << endl;
  }

  VTK_CREATE(vtkTreeLevelsFilter, treeLevels);
  treeLevels->SetInputData(realTree);

  VTK_CREATE(vtkTreeMapToPolyData, treePoly);
  treePoly->SetInputConnection(treeLevels->GetOutputPort());

  VTK_CREATE(vtkLookupTable, lut);

  VTK_CREATE(vtkPolyDataMapper, treeMapper);
  treeMapper->SetInputConnection(treePoly->GetOutputPort());
  treeMapper->SetScalarRange(0, 10);
  treeMapper->SetLookupTable(lut);

  VTK_CREATE(vtkActor, treeActor);
  treeActor->SetMapper(treeMapper);
  //treeActor->GetProperty()->SetRepresentationToWireframe();
  //treeActor->GetProperty()->SetOpacity(0.2);

  //
  // Create graph actor
  //

  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputData(g);

  VTK_CREATE(vtkTransform, transform);
  if (threedim)
  {
    transform->Translate(0, 0, 0);
  }
  else
  {
    transform->Translate(0, 0, glyphSize);
  }

  VTK_CREATE(vtkTransformFilter, transFilter);
  transFilter->SetInputConnection(graphToPoly->GetOutputPort());
  transFilter->SetTransform(transform);

  VTK_CREATE(vtkPolyDataMapper, graphMapper);
  graphMapper->SetInputConnection(transFilter->GetOutputPort());

  VTK_CREATE(vtkActor, graphActor);
  graphActor->SetMapper(graphMapper);

  //
  // Create vertex glyphs
  //

  VTK_CREATE(vtkSphereSource, sphere);
  sphere->SetRadius(glyphSize);

  VTK_CREATE(vtkGlyph3D, glyph);
  glyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  glyph->SetInputConnection(1, sphere->GetOutputPort());

  VTK_CREATE(vtkPolyDataMapper, glyphMapper);
  glyphMapper->SetInputConnection(glyph->GetOutputPort());

  VTK_CREATE(vtkActor, glyphActor);
  glyphActor->SetMapper(glyphMapper);

  //
  // Set up render window
  //

  VTK_CREATE(vtkRenderer, ren);
  if (!threedim)
  {
    ren->AddActor(treeActor);
  }
  ren->AddActor(graphActor);
  ren->AddActor(glyphActor);
  ren->AddActor(cubeActor);
  ren->AddActor(selectActor);

  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);

  VTK_CREATE(vtkAreaPicker, picker);
  iren->SetPicker(picker);

  VTK_CREATE(vtkInteractorStyleRubberBandPick, interact);
  iren->SetInteractorStyle(interact);

  if (interactive)
  {
    iren->Initialize();
    iren->Start();
  }

  return 0;
}

