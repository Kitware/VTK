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
#include "vtkEdgeListIterator.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkInformation.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPBGLCollapseGraph.h"
#include "vtkPBGLCollectGraph.h"
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
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkVertexListIterator.h"

#include <vtksys/ios/sstream>
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
  for (int i = 1; i < argc; ++ i)
    {
    if (vtkStdString(argv[i]) == "-replicate")
      {
      DoReplicate = true;
      }
    }

  vtksys_ios::ostringstream oss;
  // Make a database containing a cycle.
  int vertices = 11;
  vtkSmartPointer<vtkSQLiteDatabase> db =
    vtkSmartPointer<vtkSQLiteDatabase>::New();
  db->SetDatabaseFileName(":memory:");
  bool ok = db->Open("");
  if (!ok)
    {
    cerr << "Could not open database!" << endl;
    cerr << db->GetLastErrorText() << endl;
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

  // Create the reader
  vtkSmartPointer<vtkPBGLGraphSQLReader> reader =
    vtkSmartPointer<vtkPBGLGraphSQLReader>::New();
  reader->SetDatabase(db);
  reader->SetVertexTable("vertices");
  reader->SetEdgeTable("edges");
  reader->SetVertexIdField("id");
  reader->SetSourceField("source");
  reader->SetTargetField("target");

  // Create the collapse filter
  vtkSmartPointer<vtkPBGLCollapseGraph> collapse =
    vtkSmartPointer<vtkPBGLCollapseGraph>::New();
  collapse->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "color");
  collapse->SetInputConnection(reader->GetOutputPort());

  // Setup the parallel executive
  vtkStreamingDemandDrivenPipeline* exec =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(collapse->GetExecutive());
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper =
    vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  int total = num_processes(helper->GetProcessGroup());
  int rank = process_id(helper->GetProcessGroup());
  collapse->UpdateInformation();
  exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), total);
  exec->SetUpdatePiece(exec->GetOutputInformation(0), rank);

  // Setup the filter to collect the graph onto rank 0
  // Set up the graph collector
  VTK_CREATE(vtkPBGLCollectGraph, collect);
  collect->SetInputConnection(0, collapse->GetOutputPort());
  collect->SetReplicateGraph(DoReplicate);

  // Layout the graph
  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(collect->GetOutputPort());
  VTK_CREATE(vtkForceDirectedLayoutStrategy, force);
  layout->SetLayoutStrategy(force);

  // Display the output
  VTK_CREATE(vtkRenderer, ren);
  RenderGraph(layout, ren, 1, 1, 1, 0.01, 2.0f);
  
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  win->Render();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }
  MPI_Finalize();
  return !retVal;
}
