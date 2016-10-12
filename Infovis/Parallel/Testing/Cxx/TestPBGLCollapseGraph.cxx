/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBGLCollapseGraph.cxx

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

#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPBGLCollapseGraph.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphSQLReader.h"
#include "vtkSmartPointer.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkVertexListIterator.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <sstream>
#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>

namespace
{

#define myassert(Cond)                                  \
  if (!(Cond))                                          \
  {                                                   \
      cerr << "error (" __FILE__ ":" << dec << __LINE__ \
           << ") assertion \"" #Cond "\" failed."       \
           << endl;                                     \
    MPI_Abort(MPI_COMM_WORLD, -1);                      \
  }

void TestPSQLGraphReader()
{
  std::ostringstream oss;
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
    return;
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

  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper =
    vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  int total = num_processes(helper->GetProcessGroup());
  int rank = process_id(helper->GetProcessGroup());

  // Update the pipeline
  collapse->Update(rank, total, 0);

  // Display the output
  vtkGraph* output = collapse->GetOutput();
  vtkAbstractArray* colorArr = vtkArrayDownCast<vtkAbstractArray>(output->GetVertexData()->GetAbstractArray("color"));
  vtkSmartPointer<vtkVertexListIterator> vit =
    vtkSmartPointer<vtkVertexListIterator>::New();
  output->GetVertices(vit);
  while (vit->HasNext())
  {
    vtkIdType v = vit->Next();
    int ind = output->GetDistributedGraphHelper()->GetVertexIndex(v);
    int owner = output->GetDistributedGraphHelper()->GetVertexOwner(v);
    int color = colorArr->GetVariantValue(ind).ToInt();
    cerr << "PROCESS " << rank << " vertex: " << hex << v
      << "," << color << endl;
  }
  vtkSmartPointer<vtkEdgeListIterator> it =
    vtkSmartPointer<vtkEdgeListIterator>::New();
  output->GetEdges(it);
  while (it->HasNext())
  {
    vtkEdgeType e = it->Next();
    vtkIdType ind = output->GetDistributedGraphHelper()->GetEdgeIndex(e.Id);
    cerr << "PROCESS " << rank << " edge: " << hex << e.Id
      << " (" << e.Source << "," << e.Target << ")"
      << " index: " << ind << endl;
  }
}

}

//----------------------------------------------------------------------------
int TestPBGLCollapseGraph(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  TestPSQLGraphReader();
  MPI_Finalize();
  return 0;
}
