/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimple3DCirclesStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkEdgeLayout.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPassThroughEdgeStrategy.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRandomGraphSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSimple3DCirclesStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkVertexGlyphFilter.h"

int TestSimple3DCirclesStrategy(int argc, char *argv[])
  {
  // graph
  vtkSmartPointer<vtkMutableDirectedGraph> graph = vtkSmartPointer<vtkMutableDirectedGraph>::New();
  // mapper
  vtkSmartPointer<vtkPolyDataMapper> edgeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkPolyDataMapper> vertMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkPassThroughEdgeStrategy> edgeStrategy = vtkSmartPointer<vtkPassThroughEdgeStrategy>::New();
  vtkSmartPointer<vtkSimple3DCirclesStrategy> strategy = vtkSmartPointer<vtkSimple3DCirclesStrategy>::New();
  vtkSmartPointer<vtkGraphLayout> layout = vtkSmartPointer<vtkGraphLayout>::New();
  vtkSmartPointer<vtkEdgeLayout> edgeLayout = vtkSmartPointer<vtkEdgeLayout>::New();
  vtkSmartPointer<vtkGraphToPolyData> graphToPoly = vtkSmartPointer<vtkGraphToPolyData>::New();
  vtkSmartPointer<vtkVertexGlyphFilter> vertGlyph = vtkSmartPointer<vtkVertexGlyphFilter>::New();
  vtkSmartPointer<vtkActor> edgeActor = vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkActor> vertActor = vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renwin = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renwin);
  renwin->SetMultiSamples(0);

// Vertices:
// layer 0: 0,1,2
// layer 1: 3,4,5,6
// layer 2: 7,8,9
// standalone: 10,11
  for ( int i = 0; i < 12; ++i )
    graph->AddVertex();

// Edges:
// layer 0 -> 1
  graph->AddEdge( 0, 4 );
  graph->AddEdge( 0, 6 );
  graph->AddEdge( 1, 5 );
  graph->AddEdge( 1, 6 );
  graph->AddEdge( 2, 3 );
  graph->AddEdge( 2, 4 );
  graph->AddEdge( 2, 5 );
// layer 1 -> 2
  graph->AddEdge( 3, 8 );
  graph->AddEdge( 3, 7 );
  graph->AddEdge( 4, 9 );
  graph->AddEdge( 4, 8 );
  graph->AddEdge( 5, 7 );
// layer 0 -> 2
  graph->AddEdge( 0, 9 );

  strategy->SetMethod( vtkSimple3DCirclesStrategy::FixedDistanceMethod );
  strategy->AutoHeightOn();
  strategy->SetDirection(0.0,-1.0,0.0);
  strategy->SetMinimumDegree( 45.0 );
  layout->SetInput( graph );
  layout->SetLayoutStrategy( strategy );

  // Uncomment the following for a more interesting result!
#if 0
  vtkSmartPointer<vtkRandomGraphSource> src = vtkSmartPointer<vtkRandomGraphSource>::New();
  src->SetNumberOfVertices(1000);
  src->SetNumberOfEdges(0);
  src->SetDirected(true);
  src->SetStartWithTree(true);
  layout->SetInputConnection( src->GetOutputPort() );
#endif

  edgeLayout->SetInputConnection( layout->GetOutputPort() );
  edgeLayout->SetLayoutStrategy( edgeStrategy );
  edgeLayout->Update();

  graphToPoly->EdgeGlyphOutputOn();
  graphToPoly->SetInput( edgeLayout->GetOutput() );
  vertGlyph->SetInput( edgeLayout->GetOutput() );

  edgeMapper->ScalarVisibilityOff();
  edgeMapper->ImmediateModeRenderingOn();
  edgeMapper->SetInputConnection( graphToPoly->GetOutputPort() );
  edgeActor->GetProperty()->SetColor( 0.75, 0.75, 0.75 );
  edgeActor->GetProperty()->SetOpacity(1.0);
  edgeActor->GetProperty()->SetLineWidth(2);
  edgeActor->PickableOff();
  edgeActor->SetMapper( edgeMapper );
  ren->AddActor( edgeActor );


  vertMapper->ScalarVisibilityOff();
  vertMapper->ImmediateModeRenderingOn();
  vertMapper->SetInputConnection( vertGlyph->GetOutputPort() );
  vertActor->GetProperty()->SetColor( 0.5, 0.5, 0.5 );
  vertActor->GetProperty()->SetOpacity(1.0);
  vertActor->GetProperty()->SetPointSize(7);
  vertActor->PickableOff();
  vertActor->SetMapper( vertMapper );
  ren->AddActor( vertActor );

  renwin->SetSize(800,600);

  renwin->AddRenderer(ren);
  renwin->Render();

  int retVal = vtkRegressionTestImage( renwin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
  }

