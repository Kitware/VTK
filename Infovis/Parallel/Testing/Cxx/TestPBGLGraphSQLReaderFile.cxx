/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBGLGraphSQLReaderFile.cxx

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
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedGraph.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkGraphLayoutView.h"
#include "vtkMPIController.h"
#include "vtkMySQLDatabase.h"
#include "vtkPBGLBreadthFirstSearch.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphSQLReader.h"
#include "vtkPBGLShortestPaths.h"
#include "vtkPBGLRandomGraphSource.h"
#include "vtkPBGLCollectGraph.h"
#include "vtkSmartPointer.h"
#include "vtkVertexListIterator.h"
#include "vtkGraph.h"
#include "vtkGraphLayoutView.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLiteDatabase.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUndirectedGraph.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkViewTheme.h"
#include "vtkIntArray.h"

#include <vtksys/stl/functional>
#include <vtksys/stl/string>

#include <boost/mpi/collectives.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/timer.hpp>
#include <boost/lexical_cast.hpp>

#include <cassert>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
int ConnectToDb(int argc, char* argv[], vtkSQLiteDatabase * db);
int loadGraphFromSQL(vtkSQLDatabase * db,
                     vtkPBGLGraphSQLReader * sqlSrc,
                     int testNum,
                     bool directed);
int collectDistributedGraphToSingleNode(vtkAlgorithmOutput * inGraph,
                                        vtkPBGLCollectGraph * PBGLCollect);

int validateByCounting(vtkIdType testCase, vtkIdType numVerts, vtkIdType numEdges);

int executeTestCase(vtkIdType testCase, vtkSQLiteDatabase * db);

void printDistributedGraph(vtkGraph * G);



//------------------------------------------------------------------------------
int
TestPBGLGraphSQLReaderFile(int argc, char* argv[])
{
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;

  VTK_CREATE(vtkMPIController, controller);
  controller->Initialize( &argc, &argv, 1);

  // =====[ Read MySQL Database Into Graph ]====================================
  VTK_CREATE(vtkSQLiteDatabase, db);
  if( ConnectToDb(argc,argv,db) )
    {
    cout << "Failed to connect to database!" << endl;
    return 1;
    }

  for(vtkIdType testCase=1; testCase<=5; testCase++)
    {
    if(executeTestCase(testCase, db))
      {
      cerr << "Failed test case: " << testCase << endl;
      return 1;
      }
    }
  return 0;
}


//=====[ Execute Test Case ]====================================================
int executeTestCase(vtkIdType testCase, vtkSQLiteDatabase * db)
{
  boost::mpi::communicator world;

  // =====[ Read MySQL Database Into Graph ]====================================
  VTK_CREATE(vtkPBGLGraphSQLReader, sqlSrc);
  if( loadGraphFromSQL(db, sqlSrc, testCase, false))
    {
    cout << "Failed to load graph" << endl;
    return 1;
    }

  // =====[ Collect Graph to One Node ]=========================================
  // Note: if there's a problem this will often crash as it touches most of the
  //       graph.  For now, we'll just let correctness be based on whether this
  //       step passes or not.
  VTK_CREATE(vtkPBGLCollectGraph, collectedGraph);
  collectDistributedGraphToSingleNode(sqlSrc->GetOutputPort(),
                                    collectedGraph);

  // =====[ Do some validation ]================================================
  vtkIdType numVerts = collectedGraph->GetOutput()->GetNumberOfVertices();
  vtkIdType numEdges = collectedGraph->GetOutput()->GetNumberOfEdges();

  if( validateByCounting(testCase, numVerts, numEdges) )
    {
    return 1;
    }

  for(int i=0; i<world.size(); i++)
    {
    world.barrier();
    if(i==world.rank())
      {
      cout << "Process " << world.rank() << " passes test #"<< testCase << endl;
      fflush(stdout);
      }
    }
  world.barrier();
  return 0;
}


//=====[ Validate node and edge counts ]========================================
int validateByCounting(vtkIdType testCase, vtkIdType numVerts, vtkIdType numEdges)
{
  boost::mpi::communicator world;

  // print out a bit of debugging info (useful for ctest -V).

  for(int i=0; i<world.size(); i++)
    {
    world.barrier();
    if(i==world.rank())
      {
      cout << "Process " << world.rank() << " has " <<  numVerts << " vertices and "
           << numEdges << " edges." << endl;
      fflush(stdout);
      }
    }
  world.barrier();

  if(world.rank()==0)
    {
    switch(testCase)
      {
      case 1:
        if(numVerts != 8 || numEdges != 13)
          {
          cout << "Test failed, there should be 8 verts and 13 edges, test found "
               << numVerts << "vertices and " << numEdges << " edges." << endl;
          return 1;
          }
        break;
      case 2:
        if(numVerts != 3 || numEdges != 3)
          {
          cout << "Test failed, there should be 3 verts and 3 edges, test found "
               << numVerts << "vertices and " << numEdges << " edges." << endl;
          return 1;
          }
        break;
      case 3:
        if(numVerts != 4 || numEdges != 5)
          {
          cout << "Test failed, there should be 4 verts and 5 edges, test found "
               << numVerts << "vertices and " << numEdges << " edges." << endl;
          return 1;
          }
        break;
      case 4:
        if(numVerts != 3 || numEdges != 3)
          {
          cout << "Test failed, there should be 8 verts and 13 edges, test found "
               << numVerts << "vertices and " << numEdges << " edges." << endl;
          return 1;
          }
        break;
      case 5:
        if(numVerts != 10 || numEdges != 11)
          {
          cout << "Test failed, there should be 8 verts and 13 edges, test found "
               << numVerts << "vertices and " << numEdges << " edges." << endl;
          return 1;
          }
        break;
      default:
        return 1;
      };
    }
  else
    {
    switch(testCase)
      {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        if(numVerts != 0 || numEdges != 0)
          {
          cout << "Test failed, there should be 0 verts and 0 edges, test found "
               << numVerts << "vertices and " << numEdges << " edges." << endl;
          return 1;
          }
        break;
      default:
        return 1;
      };
    }
  return 0;
}



//=====[ Connect to SQLite Database ]===========================================
int ConnectToDb(int argc, char* argv[], vtkSQLiteDatabase * db)
  {
  boost::mpi::communicator world;

  char * filename = NULL;

  filename = vtkTestUtilities::ExpandDataFileName(argc, argv,
      "Data/Infovis/SQLite/SmallTestGraphs.db");

  cout << "Filename:" << filename << endl;

  db->SetDatabaseFileName(filename);
  delete[] filename;

  db->Open("");
  if(!db->IsOpen())
    {
    cerr << "Could not open database!" << endl;
    cerr << db->GetLastErrorText() << endl;
    return 1;
    }

#ifdef DEBUG
  vtkStringArray * tablesList = db->GetTables();
  int numTables = tablesList->GetNumberOfValues();
  cout << "# of tables = " << numTables << endl;
  for(int i=0; i<numTables; i++)
    {
    cout << "\t" << tablesList->GetValue(i) << endl;
    }
#endif
  return 0;
  }



//=====[ Load the Graph from the database ]=====================================
int loadGraphFromSQL(vtkSQLDatabase * db,
                     vtkPBGLGraphSQLReader * sqlSrc,
                     int testNum,
                     bool directed=false)
{
  boost::mpi::communicator world;

  if(world.rank()==0)
    {
    cout << ">>>\tLoad SQL Graph" << endl;
    cout << "\t-\tLoading test graph #" << testNum << endl;
    fflush(stdout);
    }
  world.barrier();

  sqlSrc->SetDatabase(db);
  sqlSrc->SetSourceField("sendID");
  sqlSrc->SetTargetField("recvID");

  switch(testNum)
    {
    case 1:
      sqlSrc->SetVertexTable("verts1");
      sqlSrc->SetEdgeTable("edges1");
      sqlSrc->SetVertexIdField("eid");
      break;
    case 2:
      sqlSrc->SetVertexTable("verts2");
      sqlSrc->SetEdgeTable("edges2");
      sqlSrc->SetVertexIdField("eid");
      break;
    case 3:
      sqlSrc->SetVertexTable("verts3");
      sqlSrc->SetEdgeTable("edges3");
      sqlSrc->SetVertexIdField("eid");
      break;
    case 4:
      sqlSrc->SetVertexTable("verts4");
      sqlSrc->SetEdgeTable("edges4");
      sqlSrc->SetVertexIdField("eid");
      break;
    case 5:
      sqlSrc->SetVertexTable("verts5");
      sqlSrc->SetEdgeTable("edges5");
      sqlSrc->SetVertexIdField("eid");
      break;
    default:
      cout << "ERROR: Invalid graph selection in test loader" << endl;
      return 1;
    };

  sqlSrc->SetDirected(directed);

  vtkStreamingDemandDrivenPipeline * execSqlSrc =
         vtkStreamingDemandDrivenPipeline::SafeDownCast(sqlSrc->GetExecutive());

  sqlSrc->UpdateInformation();

  execSqlSrc->SetUpdateNumberOfPieces(execSqlSrc->GetOutputInformation(0), world.size());
  execSqlSrc->SetUpdatePiece(execSqlSrc->GetOutputInformation(0), world.rank());
  sqlSrc->Update();

  fflush(stdout);
  world.barrier();
  if(world.rank()==0)
    {
    cout << "<<<\tLoad SQL Graph" << endl;
    fflush(stdout);
    }
  world.barrier();
  return 0;
}



//=====[ Collect Graph to One Processor ]=======================================
int collectDistributedGraphToSingleNode(vtkAlgorithmOutput * inGraph,
                                        vtkPBGLCollectGraph * PBGLCollect)
{
  boost::mpi::communicator world;
  if(world.rank()==0)
    {
    cout << ">>>\tCollect graph to single node." << endl;
    }
  world.barrier();

  PBGLCollect->SetInputConnection( inGraph );
  PBGLCollect->SetTargetProcessor(0);
  PBGLCollect->SetReplicateGraph(false);
  PBGLCollect->CopyVertexDataOn();
  PBGLCollect->CopyEdgeDataOn();
  PBGLCollect->CreateOriginProcessArrayOn();
  PBGLCollect->UpdateInformation();

  vtkStreamingDemandDrivenPipeline * exec2 = NULL;
  exec2 = vtkStreamingDemandDrivenPipeline::SafeDownCast(
                                                  PBGLCollect->GetExecutive());
  exec2->SetUpdateNumberOfPieces( exec2->GetOutputInformation(0), world.size());
  exec2->SetUpdatePiece( exec2->GetOutputInformation(0), world.rank());
  PBGLCollect->Update();

  if(world.rank()==0)
    {
    cout << "<<<\tCollect graph to single node." << endl;
    }
  world.barrier();

  return 0;  // success
}



// =====[printDistributedGraph]=================================================
// Just a helper...
void printDistributedGraph(vtkGraph * G)
{
  boost::mpi::communicator world;
  int rank = world.rank();
  int nump = world.size();

  // Print vertices
  int numVertices = G->GetNumberOfVertices();
  printf("[%d]\tG.NumberOfVertices = %d\n", rank, numVertices);
  fflush(stdout);
  world.barrier();

  vtkDataSetAttributes *distribVertexData = G->GetVertexData();
  int numVertexArrays = distribVertexData->GetNumberOfArrays();
  printf("[%d]\tG.NumberOfVertexArrays = %d\n", rank, numVertexArrays);
  fflush(stdout);
  world.barrier();

  for(int p=0; p<nump; p++)
    {
    world.barrier();
    if(p==rank)
      for(int i=0; i<numVertices; i++)
        {
        cout << "[" << rank << "]\tvIndx=" << i;
        for(int j=0; j<numVertexArrays; j++)
          {
          vtkAbstractArray * A = distribVertexData->GetAbstractArray(j);
          cout << "\ta"<<j<<"='" <<  A->GetVariantValue(i).ToString() << "'";
          }
        cout << endl;
        }
    fflush(stdout);
    }
  world.barrier();

  // Print out the Edges
  int numEdges = G->GetNumberOfEdges();
  printf("[%d]\tG.NumberOfEdges = %d\n", rank, numEdges);
  fflush(stdout);
  world.barrier();

  vtkDataSetAttributes *distribEdgeData = G->GetEdgeData();
  int numEdgeArrays = distribEdgeData->GetNumberOfArrays();
  printf("[%d]\tG.NumberOfEdgeArrays = %d\n", rank, numEdgeArrays);
  fflush(stdout);
  world.barrier();

#if 0
  // might be helpful... so not deleting it atm.
  vtkDistributedGraphHelper * helper = G->GetDistributedGraphHelper();

  for(int p=0; p<nump; p++)
    {
    if(p==rank)
      for(int ei=0; ei<numEdges; ei++)
        {
        cout << "[" << rank << "]\teIndx=" << ei
             << "\t(" << helper->GetVertexOwner(G->GetSourceVertex(ei))
             << ":"   << helper->GetVertexIndex(G->GetSourceVertex(ei))
             << ")->(" << helper->GetVertexOwner(G->GetTargetVertex(ei))
             << ":"   << helper->GetVertexIndex(G->GetTargetVertex(ei))
             << ")";
        cout << endl;
        fflush(stdout);
        }
    }
  world.barrier();
#endif
}


//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  TestPBGLGraphSQLReaderFile(argc, argv);
  cerr << "finalizing." << endl;
  MPI_Finalize();
  cerr << "done." << endl;
  return 0;
}
// EOF
