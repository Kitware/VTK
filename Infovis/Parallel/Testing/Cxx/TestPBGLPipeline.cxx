/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBGLPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */

#include <mpi.h>

#include "vtkActor.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphLayout.h"
#include "vtkGraphLayoutView.h"
#include "vtkGraphToPolyData.h"
#include "vtkInformation.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPBGLCollapseGraph.h"
#include "vtkPBGLCollapseParallelEdges.h"
#include "vtkPBGLCollectGraph.h"
#include "vtkPBGLConnectedComponents.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphSQLReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSQLQuery.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStringToCategory.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkVertexListIterator.h"
#include "vtkViewTheme.h"

#include <vtksys/ios/sstream>

#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void RenderGraph(vtkAlgorithm* alg, vtkRenderer* ren, double r, double g, double
 b, double z, float size)
{
  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(alg->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  VTK_CREATE(vtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(r, g, b);
  edgeActor->GetProperty()->SetLineWidth(size/2);
  edgeActor->SetPosition(0, 0, z);
  VTK_CREATE(vtkGlyphSource2D, vertex);
  vertex->SetGlyphTypeToVertex();
  VTK_CREATE(vtkGlyph3D, glyph);
  glyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  glyph->SetInputConnection(1, vertex->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, vertMapper);
  vertMapper->SetInputConnection(glyph->GetOutputPort());
  VTK_CREATE(vtkActor, vertActor);
  vertActor->SetMapper(vertMapper);
  vertActor->GetProperty()->SetColor(r, g, b);
  vertActor->GetProperty()->SetPointSize(size);
  vertActor->SetPosition(0, 0, z);
  ren->AddActor(edgeActor);
  ren->AddActor(vertActor);
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  bool DoReplicate = false;
  vtkStdString url;
  vtkStdString password;
  vtkStdString vertexTable = "vertices";
  vtkStdString edgeTable = "edges";
  vtkStdString vertexId = "id";
  vtkStdString source = "source";
  vtkStdString target = "target";
  vtkStdString collapseField = "color";
  bool debugWait = false;
  for (int i = 1; i < argc; ++ i)
    {
    if (vtkStdString(argv[i]) == "-replicate")
      {
      DoReplicate = true;
      }
    else if (vtkStdString(argv[i]) == "-debug")
      {
      debugWait = true;
      }
    else if (vtkStdString(argv[i]) == "-db")
      {
      ++i;
      url = vtkStdString(argv[i]);
      }
    else if (vtkStdString(argv[i]) == "-password")
      {
      ++i;
      password = vtkStdString(argv[i]);
      }
    else if (vtkStdString(argv[i]) == "-vertextable")
      {
      ++i;
      vertexTable = vtkStdString(argv[i]);
      }
    else if (vtkStdString(argv[i]) == "-edgetable")
      {
      ++i;
      edgeTable = vtkStdString(argv[i]);
      }
    else if (vtkStdString(argv[i]) == "-id")
      {
      ++i;
      vertexId = vtkStdString(argv[i]);
      }
    else if (vtkStdString(argv[i]) == "-source")
      {
      ++i;
      source = vtkStdString(argv[i]);
      }
    else if (vtkStdString(argv[i]) == "-target")
      {
      ++i;
      target = vtkStdString(argv[i]);
      }
    else if (vtkStdString(argv[i]) == "-collapse")
      {
      ++i;
      collapseField = vtkStdString(argv[i]);
      }
    }

  while (debugWait) ;

  vtkSmartPointer<vtkSQLDatabase> db;
  if (url.length() > 0)
    {
    db.TakeReference(vtkSQLDatabase::CreateFromURL(url));
    if (!db.GetPointer())
      {
      cerr << "Could not create database instance for URL." << endl;
      MPI_Finalize();
      return 1;
      }
    if (!db->Open(password))
      {
      cerr << "Could not open database." << endl;
      MPI_Finalize();
      return 1;
      }
    }
  else
    {
    // Create an in-memory database containing a cycle graph.
    vtksys_ios::ostringstream oss;
    int vertices = 10000;
    db = vtkSmartPointer<vtkSQLiteDatabase>::New();
    vtkSQLiteDatabase::SafeDownCast(db)->SetDatabaseFileName(":memory:");
    bool ok = db->Open("");
    if (!ok)
      {
      cerr << "Could not open database!" << endl;
      cerr << db->GetLastErrorText() << endl;
      MPI_Finalize();
      return 1;
      }
    vtkSmartPointer<vtkSQLQuery> query;
    query.TakeReference(db->GetQueryInstance());
    query->SetQuery("create table vertices (id INTEGER, name VARCHAR(10), color INTEGER)");
    query->Execute();
    for (int i = 0; i < vertices; ++i)
      {
      oss.str("");
      oss << "insert into vertices values(" << i << ","
        << vtkVariant(i).ToString() << "," << i % 2 << ")" << endl;
      query->SetQuery(oss.str().c_str());
      query->Execute();
      }
    query->SetQuery("create table edges (source INTEGER, target INTEGER, name VARCHAR(10))");
    query->Execute();
    for (int i = 0; i < vertices; ++i)
      {
      oss.str("");
      oss << "insert into edges values(" << i << ", "
        << (i+1)%vertices << ", "
        << vtkVariant(i).ToString() << ")" << endl;
      query->SetQuery(oss.str().c_str());
      query->Execute();
      }
    }

  // Create the reader
  vtkSmartPointer<vtkPBGLGraphSQLReader> reader =
    vtkSmartPointer<vtkPBGLGraphSQLReader>::New();
  reader->SetDatabase(db);
  reader->SetVertexTable(vertexTable);
  reader->SetEdgeTable(edgeTable);
  reader->SetVertexIdField(vertexId);
  reader->SetSourceField(source);
  reader->SetTargetField(target);

#if 0

  // Create the connected components filter
  vtkSmartPointer<vtkPBGLConnectedComponents> conn =
    vtkSmartPointer<vtkPBGLConnectedComponents>::New();
  conn->SetInputConnection(category->GetOutputPort());
  conn->SetComponentArrayName("component");
#endif

  // Create the collapse vertices filter
  vtkSmartPointer<vtkPBGLCollapseGraph> collapse =
    vtkSmartPointer<vtkPBGLCollapseGraph>::New();
  collapse->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, collapseField);
  collapse->SetInputConnection(reader->GetOutputPort());

  // Create the collapse parallel edges filter
  vtkSmartPointer<vtkPBGLCollapseParallelEdges> collapseParallel =
    vtkSmartPointer<vtkPBGLCollapseParallelEdges>::New();
  collapseParallel->SetInputConnection(collapse->GetOutputPort());

  // Setup the filter to collect the graph onto rank 0
  // Set up the graph collector
  VTK_CREATE(vtkPBGLCollectGraph, collect);
  collect->SetInputConnection(0, collapseParallel->GetOutputPort());
  collect->SetReplicateGraph(false);
  //collect->CopyEdgeDataOff();
  //collect->CopyVertexDataOff();

  // Setup the parallel executive
  vtkAlgorithm* lastFilter = collect;
  vtkStreamingDemandDrivenPipeline* exec =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(lastFilter->GetExecutive());
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper =
    vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  int total = num_processes(helper->GetProcessGroup());
  int rank = process_id(helper->GetProcessGroup());
  lastFilter->UpdateInformation();
  exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), total);
  exec->SetUpdatePiece(exec->GetOutputInformation(0), rank);
  lastFilter->Update();
  vtkSmartPointer<vtkDirectedGraph> output = vtkSmartPointer<vtkDirectedGraph>::New();
  output->ShallowCopy(lastFilter->GetOutputDataObject(0));

#if 0
  // Create the string to category filter
  vtkSmartPointer<vtkStringToCategory> category =
    vtkSmartPointer<vtkStringToCategory>::New();
  category->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, collapseField);
  category->SetCategoryArrayName("category");
  category->SetInput(output);
  //category->SetInputConnection(reader->GetOutputPort());
#endif

  int retVal = vtkRegressionTester::PASSED;
  if (rank == 0)
    {
    // Display the output
    VTK_CREATE(vtkGraphLayoutView, view);
//    VTK_CREATE(vtkRenderWindow, win);
//    view->SetupRenderWindow(win);
    //view->AddRepresentationFromInputConnection(category->GetOutputPort());
//    view->AddRepresentationFromInput(output);
    view->SetRepresentationFromInput(output);
    //view->SetVertexLabelArrayName("email");
    //view->VertexLabelVisibilityOn();
    //view->SetVertexColorArrayName("category");
    view->SetVertexColorArrayName(collapseField);
    view->ColorVerticesOn();
    //view->SetEdgeLabelArrayName("weight");
    //view->EdgeLabelVisibilityOn();
    //view->SetEdgeColorArrayName("weight");
    //view->ColorEdgesOn();
    view->SetEdgeLayoutStrategyToPassThrough();
    view->SetLayoutStrategyToFast2D();
    //view->SetLayoutStrategyToSimple2D();
    view->ResetCamera();
    vtkSmartPointer<vtkViewTheme> theme;
    theme.TakeReference(vtkViewTheme::CreateMellowTheme());
    //theme->SetCellOpacity(0.02);
    view->ApplyViewTheme(theme);
    view->Update();

    view->Render();
//    view->GetInteractor()->Start();


    /*
    int retVal = vtkRegressionTestImage(win);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      win->GetInteractor()->Start();
      retVal = vtkRegressionTester::PASSED;
      }
    */
    }

  MPI_Finalize();
  return !retVal;
}
