/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBGLPedigrees.cxx

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
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkVertexListIterator.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>

namespace
{

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=================================================================================
//  Sample output with 2,3,4 procs:
//$ mpirun -np 2 TestPBGLPedigrees
//owner of A should be proc  0
//owner of B should be proc  1
//owner of C should be proc  0
//owner of D should be proc  1
//owner of E should be proc  0
// >>Rank 0 has 3 verts
// >>Rank 1 has 2 verts
// ======Rank 0: vertex A (0) owner=0,  index=0
// ======Rank 0: vertex C (1) owner=0,  index=1
// ======Rank 0: vertex E (2) owner=0,  index=2
// ======Rank 1: vertex B (4000000000000000) owner=1,  index=0
// ======Rank 1: vertex D (4000000000000001) owner=1,  index=1

//$ mpirun -np 3 TestPBGLPedigrees
//owner of A should be proc  0
//owner of B should be proc  0
//owner of C should be proc  2
//owner of D should be proc  0
//owner of E should be proc  2
// >>Rank 0 has 3 verts
// >>Rank 1 has 0 verts
// >>Rank 2 has 2 verts
// ======Rank 0: vertex A (0) owner=0,  index=0
// ======Rank 0: vertex B (1) owner=0,  index=1
// ======Rank 0: vertex D (2) owner=0,  index=2
// ======Rank 2: vertex C (4000000000000000) owner=2,  index=0
// ======Rank 2: vertex E (4000000000000001) owner=2,  index=1
//
//
// $ mpirun -np 4 TestPBGLPedigrees
//  >>Rank 2 has 1 verts
//  >>Rank 3 has 1 verts
// owner of A should be proc  0
// owner of B should be proc  3
// owner of C should be proc  2
// owner of D should be proc  1
// owner of E should be proc  0
//  >>Rank 0 has 2 verts
//  ======Rank 0: vertex A (0) owner=0,  index=0
//  ======Rank 0: vertex E (1) owner=0,  index=1
//  >>Rank 1 has 1 verts
//  ======Rank 1: vertex D (2000000000000000) owner=1,  index=0
//  ======Rank 2: vertex C (4000000000000000) owner=2,  index=0
//  ======Rank 3: vertex B (6000000000000000) owner=3,  index=0

void UseCase0()
{
  VTK_CREATE(vtkMutableDirectedGraph, mdg);
  VTK_CREATE(vtkPBGLDistributedGraphHelper, helper);
  mdg->SetDistributedGraphHelper(helper);

  int myRank = mdg->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs = mdg->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (myRank == 0)
    cout << "-----------------   UseCase0  ----------------------------\n";

  // Not doing this will create a run-time error when we try to do the AddVertex(pedigreeId)
  VTK_CREATE(vtkVariantArray, pedIds);
  mdg->GetVertexData()->SetPedigreeIds(pedIds);
  if (mdg->GetVertexData()->GetPedigreeIds() )
  {
    if (mdg->GetVertexData()->GetPedigreeIds()->GetName() )
      cout << " after SetPedIds(), pedId Name= "<<mdg->GetVertexData()->GetPedigreeIds()->GetName()<<endl;
    else
      cout << " after SetPedIds(), no Name set yet."<<endl;
  }
  else
  {
    cout << "  after SetPedIds, GetPedigreeIds == NULL\n";
  }


  // Have every proc (try to) add these vertices.  However, since they are uniquely defined
  // by pedigreeIds, only one vertex per pedId will actually be added to the graph.
  mdg->AddVertex("A");
  mdg->AddVertex("B");
  mdg->AddVertex("C");
  mdg->AddVertex("D");
  mdg->AddVertex("E");

  helper->Synchronize();  // don't forget to sync!

  // This method will tell us where (on which proc) a pedId *should* be stored
  if (myRank == 0)
  {
    cout << "owner of A should be proc  "<< helper->GetVertexOwnerByPedigreeId("A") << endl;
    cout << "owner of B should be proc  "<< helper->GetVertexOwnerByPedigreeId("B") << endl;
    cout << "owner of C should be proc  "<< helper->GetVertexOwnerByPedigreeId("C") << endl;
    cout << "owner of D should be proc  "<< helper->GetVertexOwnerByPedigreeId("D") << endl;
    cout << "owner of E should be proc  "<< helper->GetVertexOwnerByPedigreeId("E") << endl;
  }

  // Returns # of verts stored locally
  cout << " >>Rank "<< myRank << " has "<<mdg->GetNumberOfVertices() << " verts\n";

  VTK_CREATE(vtkVertexListIterator, vit);
  mdg->GetVertices(vit);
  vtkAbstractArray *peds = mdg->GetVertexData()->GetPedigreeIds();
  while (vit->HasNext())
  {
    vtkIdType vtx = vit->Next();
    vtkVariant ped = vtkArrayDownCast<vtkVariantArray>(peds)->GetValue(helper->GetVertexIndex(vtx));
    cout << " ======Rank " << myRank << ": vertex " << ped.ToString() << " ("
         << hex << vtx << ")" << " owner="<<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
         << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;
  }

  helper->Synchronize();
  if (myRank == 0)
  {
    (cout << " -------------done.\n").flush();
  }
}
//=================================================================================
// Same functionality as above, but add the pedigreeIds via single-element vtkVariantArray.
// Also, after SetPedId(), pedId Name= myPeds
void UseCase1()
{
  VTK_CREATE(vtkMutableDirectedGraph, mdg);
  VTK_CREATE(vtkPBGLDistributedGraphHelper, helper);
  mdg->SetDistributedGraphHelper(helper);

  int myRank = mdg->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = mdg->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (myRank == 0)
    cout << "-----------------   UseCase1  ----------------------------\n";

  // Make it a graph with vertex properties.  In this case, we'll only have 1 prop (the pedId).
  VTK_CREATE(vtkVariantArray, vertexPropArr);
//  vtkSmartPointer<vtkVariantArray> vertexPropArr
//    = vtkSmartPointer<vtkVariantArray>::New();
  vertexPropArr->SetNumberOfValues(1);  // Required.  Doing 'SetPed--' below doesn't incr # arrays.

  // Create the pedIds array
  VTK_CREATE(vtkVariantArray, pedIds);
//  vtkSmartPointer<vtkVariantArray> pedIds
//    = vtkSmartPointer<vtkVariantArray>::New();
//  vtkSmartPointer<vtkIntArray> pedIds
//      = vtkSmartPointer<vtkIntArray>::New();     // --> runtime error

  pedIds->SetName("myPeds");
  mdg->GetVertexData()->SetPedigreeIds(pedIds);

  if (mdg->GetVertexData()->GetPedigreeIds() )
  {
    if (mdg->GetVertexData()->GetPedigreeIds()->GetName() )
      cout << " after SetPedIds(), pedId Name= "<<mdg->GetVertexData()->GetPedigreeIds()->GetName()<<endl;
    else
      cout << " after SetPedIds(), no Name set yet."<<endl;
  }
  else
  {
    cout << "  after SetPedIds, GetPedigreeIds == NULL\n";
  }

  helper->Synchronize();
  cout << "num property arrays =" << vertexPropArr->GetNumberOfValues() <<endl;

  // Build the graph.
  vtkVariant ped;
  ped = vtkVariant("A");
  vertexPropArr->SetValue(0,ped);
  mdg->AddVertex(vertexPropArr);

  ped = vtkVariant("B");
  vertexPropArr->SetValue(0,ped);
  mdg->AddVertex(vertexPropArr);

  ped = vtkVariant("C");
  vertexPropArr->SetValue(0,ped);
  mdg->AddVertex(vertexPropArr);

  ped = vtkVariant("D");
  vertexPropArr->SetValue(0,ped);
  mdg->AddVertex(vertexPropArr);

  ped = vtkVariant("E");
  vertexPropArr->SetValue(0,ped);
  mdg->AddVertex(vertexPropArr);

  helper->Synchronize();

  helper->Synchronize();
  if (myRank == 0)
  {
    cout << "owner of A should be proc  "<< helper->GetVertexOwnerByPedigreeId("A") << endl;
    cout << "owner of B should be proc  "<< helper->GetVertexOwnerByPedigreeId("B") << endl;
    cout << "owner of C should be proc  "<< helper->GetVertexOwnerByPedigreeId("C") << endl;
    cout << "owner of D should be proc  "<< helper->GetVertexOwnerByPedigreeId("D") << endl;
    cout << "owner of E should be proc  "<< helper->GetVertexOwnerByPedigreeId("E") << endl;
  }

  // Returns # of verts stored locally
  cout << " >>Rank "<< myRank << " has "<<mdg->GetNumberOfVertices() << " verts\n";

  // ----------- Dump the vertices
  VTK_CREATE(vtkVertexListIterator, vit);
  mdg->GetVertices(vit);
  vtkAbstractArray *peds = mdg->GetVertexData()->GetPedigreeIds();
  while (vit->HasNext())
  {
    vtkIdType vtx = vit->Next();
    vtkVariant ped = vtkArrayDownCast<vtkVariantArray>(peds)->GetValue(helper->GetVertexIndex(vtx));
    cout << " ======Rank " << myRank << ": vertex " << ped.ToString() << " ("
         << hex << vtx << ")" << " owner="<<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
         << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;
  }

  helper->Synchronize();
  if (myRank == 0)
  {
    (cout << " -------------done.\n").flush();
  }
}

//=================================================================================
// Create a non-trivial vertex property array (>1) with the pedId as one element.
void UseCase2()
{
  VTK_CREATE(vtkMutableDirectedGraph, mdg);
  VTK_CREATE(vtkPBGLDistributedGraphHelper, helper);
  mdg->SetDistributedGraphHelper(helper);

  int myRank = mdg->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = mdg->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (myRank == 0)
    cout << "-----------------   UseCase2  ----------------------------\n";

  if (myRank == 0)
  {
    cout << "owner of pedA should be = "<< helper->GetVertexOwnerByPedigreeId("pedA") << endl;
    cout << "owner of pedB should be = "<< helper->GetVertexOwnerByPedigreeId("pedB") << endl;
    cout << "owner of pedC should be = "<< helper->GetVertexOwnerByPedigreeId("pedC") << endl;
    (cout << " done.\n").flush();
  }


  //  Create some vertex property arrays -  this includes a pedigreeID array too.
  VTK_CREATE(vtkVariantArray, vertexPropertyArr);
  int numVertexProperties = 4;
  vertexPropertyArr->SetNumberOfValues(numVertexProperties);
  vertexPropertyArr->SetName("MyBigFatProperties");

  // Make it a mdg with the pedigree IDs vertices
  VTK_CREATE(vtkVariantArray, pedigreeIds);
//  pedigreeIds->SetName("myPeds");  // Optional

  VTK_CREATE(vtkStringArray, vertexProp0Array);
  vertexProp0Array->SetName("labels");
  mdg->GetVertexData()->AddArray(vertexProp0Array);

  VTK_CREATE(vtkFloatArray, vertexProp1Array);
  vertexProp1Array->SetName("weight");
  mdg->GetVertexData()->AddArray(vertexProp1Array);

  VTK_CREATE(vtkIntArray, vertexProp2Array);
  vertexProp2Array->SetName("age");
  mdg->GetVertexData()->AddArray(vertexProp2Array);

  mdg->GetVertexData()->SetPedigreeIds(pedigreeIds);

  const char *stringProp;
  float weight;
  int age;
  vtkVariant ped;

  for (vtkIdType i = 0; i < 3; ++i)
  {
    if (i==0)
    {
      stringProp = "labelA";
      weight = 40.0;
      age = 10;
      ped = vtkVariant("pedA");
    }
    else if (i==1)
    {
      stringProp = "labelB";
      weight = 41.0;
      age = 11;
      ped = vtkVariant("pedB");
    }
    else if (i==2)
    {
      stringProp = "labelC";
      weight = 42.0;
      age = 12;
      ped = vtkVariant("pedC");
    }
//    cout << myRank <<" vertex "<< v <<","<< stringProp <<","<<weight<< endl;
    vertexPropertyArr->SetValue(0,stringProp);
    vertexPropertyArr->SetValue(1,weight);
    vertexPropertyArr->SetValue(2,age);
    vertexPropertyArr->SetValue(3,ped);  // add pedId here

    mdg->AddVertex(vertexPropertyArr);
//    if (i == 0) mdg->AddVertex(vertexPropertyArr);  // what should this do? (adding a vert w/ an existing pedId)
  }

  // Create some edges
//  mdgTree->AddEdge(0, 1);

  cout << myRank<<") num vertexdata arrays = " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
  if (mdg->GetVertexData()->HasArray("weight") ) cout << myRank <<")    got weight...\n";

  cout << myRank<<") num verts= " << mdg->GetNumberOfVertices() << endl;


  helper->Synchronize();

  int numProps = mdg->GetVertexData()->GetNumberOfArrays();   // # of properties = # of arrays
  cout << "numProps = "<<numProps<<endl;
  vtkAbstractArray *peds = mdg->GetVertexData()->GetPedigreeIds();
  if (peds == NULL)
  {
    cout << "  No peds here!!\n";
  }
  else
  {
    cout << "  We have peds!\n";
  }

  VTK_CREATE(vtkVertexListIterator, vit);
  mdg->GetVertices(vit);
  while (vit->HasNext())
  {
    vtkIdType vtx = vit->Next();
    int idx = mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx);

    vtkVariant ped = vtkArrayDownCast<vtkVariantArray>(peds)->GetValue(helper->GetVertexIndex(vtx));

    cout << "  Rank #" << myRank << ": vertex " << ped.ToString() << " ("
         << hex << vtx << ")" << " owner="<<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
         << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;

    cout << myRank<<")   GetNumberOfArrays= " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
    for (int iprop=0; iprop<numProps; iprop++)
    {
      vtkAbstractArray* aa = vtkArrayDownCast<vtkAbstractArray>(mdg->GetVertexData()->GetAbstractArray(iprop));
//      int idx = helper->GetVertexIndex(vtx);
      cout << "     idx="<<idx<<") = "<< aa->GetVariantValue(idx).ToString() <<endl;
    }
    cout.flush();
  }


  helper->Synchronize();
//  if (myRank == 0)
//    cout << "===================================\n"; cout.flush();


  if (myRank == 0)
  {
    cout << "  ------------------- add pedA again, but different props ---------------\n";
    // Add an existing vertex (existing pedId), but with different properties
    stringProp = "labelA-new";
    weight = 50.0;
    age = 20;
    ped = vtkVariant("pedA");
    vertexPropertyArr->SetValue(0,stringProp);
    vertexPropertyArr->SetValue(1,weight);
    vertexPropertyArr->SetValue(2,age);
    vertexPropertyArr->SetValue(3,ped);
    mdg->AddVertex(vertexPropertyArr);
  }

  helper->Synchronize();

  if (myRank == 0)
    cout << "===============  dump verts again after changing weight of pedA\n"; cout.flush();

  mdg->GetVertices(vit);
  peds = mdg->GetVertexData()->GetPedigreeIds();
  while (vit->HasNext())
  {
    vtkIdType vtx = vit->Next();
    int idx = mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx);

    vtkVariant ped = vtkArrayDownCast<vtkVariantArray>(peds)->GetValue(helper->GetVertexIndex(vtx));

    cout << "  Rank #" << myRank << ": vertex " << ped.ToString() << " ("
         << hex << vtx << ")" << " owner="<<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
         << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;

    cout << myRank<<")   GetNumberOfArrays= " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
    for (int iprop=0; iprop<numProps; iprop++)
    {
      vtkAbstractArray* aa = vtkArrayDownCast<vtkAbstractArray>(mdg->GetVertexData()->GetAbstractArray(iprop));
//      int idx = helper->GetVertexIndex(vtx);
      cout << "     idx="<<idx<<") = "<< aa->GetVariantValue(idx).ToString() <<endl;
    }
    cout.flush();
  }

  helper->Synchronize();
  if (myRank == 0)
  {
    (cout << " -------------done.\n").flush();
  }
}
//=================================================================================
// Create vertices (w/ pedIds) implicitly via AddEdge(pedId,pedId)
void UseCase3()
{
  VTK_CREATE(vtkMutableDirectedGraph, mdg);
  VTK_CREATE(vtkPBGLDistributedGraphHelper, helper);
  mdg->SetDistributedGraphHelper(helper);

  int myRank = mdg->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = mdg->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (myRank == 0)
    cout << "-----------------   UseCase3  ----------------------------\n";

  // required (pedIds created implicitly via AddEdge(pedId,pedId)
  VTK_CREATE(vtkVariantArray, pedigreeIds);
  mdg->GetVertexData()->SetPedigreeIds(pedigreeIds);

  mdg->AddEdge("A","B");
  mdg->AddEdge("B","C");
  mdg->AddEdge("C","A");
  mdg->AddEdge("D","E");


  helper->Synchronize();

  if (myRank == 0)
  {
    cout << "===================================\n"; cout.flush();
    cout << "owner of A= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("A")) << endl;
    cout << "owner of B= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("B")) << endl;
    cout << "owner of C= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("C")) << endl;
    cout << "owner of D= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("D")) << endl;
    cout << "owner of E= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("E")) << endl;
    (cout << " done.\n").flush();
  }

  int numProps = mdg->GetVertexData()->GetNumberOfArrays();   // # of properties = # of arrays
  if (myRank == 0) cout << "   numProps = "<<numProps<<endl;
  vtkAbstractArray *peds = mdg->GetVertexData()->GetPedigreeIds();
  if (myRank == 0)
  {
    if (peds == NULL) cout << "  No peds here!!\n";
    else cout << "  We have peds!\n";
  }


  if (myRank == 0)
    cout << "=============== dump vertices\n"; cout.flush();
  VTK_CREATE(vtkVertexListIterator, vit);
  mdg->GetVertices(vit);
  while (vit->HasNext())
  {
    vtkIdType vtx = vit->Next();
    int idx = mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx);

    vtkVariant ped = vtkArrayDownCast<vtkVariantArray>(peds)->GetValue(helper->GetVertexIndex(vtx));

    cout <<"  Rank #" << myRank << ": vertex " << ped.ToString() << " ("
         << hex << vtx << ")" << " owner="<<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
         << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;

    cout << myRank<<") "<<"  GetNumberOfArrays= " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
    for (int iprop=0; iprop<numProps; iprop++)
    {
      vtkAbstractArray* aa = vtkArrayDownCast<vtkAbstractArray>(mdg->GetVertexData()->GetAbstractArray(iprop));
//      int idx = helper->GetVertexIndex(vtx);
      cout << "     idx="<<idx<<") = "<< aa->GetVariantValue(idx).ToString() <<endl;
    }
    cout.flush();
  }

  if (myRank == 0)
    cout << "=============== dump edges\n"; cout.flush();
  VTK_CREATE(vtkEdgeListIterator, eit);
  mdg->GetEdges(eit);
  while (eit->HasNext())
  {
    vtkEdgeType etx = eit->Next();

    cerr << "PROCESS " << myRank << " edge: " << hex << etx.Id
      << " (" << etx.Source << "," << etx.Target << ")" <<endl;
  }


  helper->Synchronize();
  if (myRank == 0)
  {
    (cout << " -------------done.\n").flush();
  }
}
//=================================================================================
// Trying to mimic TestPBGLGraphSQLReader
void UseCase4()
{
  VTK_CREATE(vtkMutableDirectedGraph, mdg);
  VTK_CREATE(vtkPBGLDistributedGraphHelper, helper);
  mdg->SetDistributedGraphHelper(helper);

  int myRank = mdg->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = mdg->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (myRank == 0)
    cout << "-----------------   UseCase4  ----------------------------\n";

  //  Create some vertex property arrays -  this includes a pedigreeID array too.
  VTK_CREATE(vtkVariantArray, vertexPropertyArr);
  int numVertexProperties = 2;
  vertexPropertyArr->SetNumberOfValues(numVertexProperties);
  vertexPropertyArr->SetName("MyIntProperties");

  // Make it a mdg with the pedigree IDs vertices
  VTK_CREATE(vtkVariantArray, pedigreeIds);
  pedigreeIds->SetName("myPeds");    // optional

    mdg->GetVertexData()->SetPedigreeIds(pedigreeIds);
    if (mdg->GetVertexData()->GetPedigreeIds())
    {
      cout << "  Yes, GetVertexData()->GetPedigreeIds()  is non-NULL\n";
      char *pedIdArrayName = mdg->GetVertexData()->GetPedigreeIds()->GetName();
      cout << "  name of pedigrees array= " << pedIdArrayName << endl;
    }

  VTK_CREATE(vtkFloatArray, vertexProp1Array);
  vertexProp1Array->SetName("weight");
  mdg->GetVertexData()->AddArray(vertexProp1Array);

  vtkVariant ped;
  float weight;

  for (vtkIdType i = 0; i < 3; ++i)
  {
    if (i==0)
    {
      weight = 40.0;
      ped = vtkVariant(0);
    }
    else if (i==1)
    {
      weight = 41.0;
      ped = vtkVariant(1);
    }
    else if (i==2)
    {
      weight = 42.0;
      ped = vtkVariant(2);
    }
//    cout << myRank <<" vertex "<< v <<","<< stringProp <<","<<weight<< endl;

    vertexPropertyArr->SetValue(0,ped);
    vertexPropertyArr->SetValue(1,weight);

    mdg->AddVertex(vertexPropertyArr);
//    if (i == 0) mdg->AddVertex(vertexPropertyArr);  // what should this do? (adding a vert w/ an existing pedId)
  }

  // Create some edges
  mdg->AddEdge(vtkVariant(0), vtkVariant(1));
//  mdg->AddEdge(vtkVariant(1), vtkVariant(0));

  cout << myRank<<")   num vertexdata arrays = " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
  if (mdg->GetVertexData()->HasArray("weight") ) cout << myRank<<")    got weight...\n";

  cout << myRank<<")   num verts= " << mdg->GetNumberOfVertices() << endl;

  vtkAbstractArray *peds = mdg->GetVertexData()->GetPedigreeIds();

  if (myRank == 0)
  {
    if (peds == NULL) cout << "  No peds here!!\n";
    else
    {
      cout << "  We have peds!\n";

      // This is only valid if we did a SetName on the pedIds above
      int pedIdx = mdg->GetVertexData()->SetActiveAttribute("myPeds", vtkDataSetAttributes::PEDIGREEIDS);
  //    vtkFieldData *da = mdg->GetVertexData()->GetArray("myPeds", &pedIdx);
      cout << "               pedIdx= "<<pedIdx<<endl;

      vtkStringArray *charArr = vtkArrayDownCast<vtkStringArray>(mdg->GetVertexData()->GetAbstractArray("labels"));
      cout <<    "  yes, we got  --labels--\n";
  //    cout << "  --labels-- array= " << *charArr <<endl;
      vtkVariantArray *pedArr = vtkArrayDownCast<vtkVariantArray>(mdg->GetVertexData()->GetAbstractArray("myPeds"));
      cout <<    "  yes, we got  --myPeds--\n";
  //    cout << "  --myPeds-- array= " << *pedArr <<endl;

  //    cout << "variant array= " << *varArr <<endl;
  //    cout << "vertexdata has peds, datatypeAsString = " << vtkArrayDownCast<vtkStringArray>(peds).GetDataTypeAsString() << endl;
  //    cout << "vertexdata has peds, name = " << peds.GetName() << endl;
  //    cout << "vertexdata has peds, size = " << peds.GetSize() << endl;
    }
  }

  helper->Synchronize();

  vtkDataSetAttributes *vertexData = mdg->GetVertexData();
  int numProps = vertexData->GetNumberOfArrays();   // # of properties = # of arrays

  cout << myRank <<") GetNumberOfVertices() = "<<mdg->GetNumberOfVertices() <<endl;
  cout << myRank <<") GetNumberOfEdges() = "<<mdg->GetNumberOfEdges() <<endl;

  cout << "   numProps = "<<numProps<<endl;

  if (myRank == 0)
    cout << "=============== dump vertices\n"; cout.flush();

  VTK_CREATE(vtkVertexListIterator, vit);
  mdg->GetVertices(vit);
  while (vit->HasNext())
  {
    vtkIdType vtx = vit->Next();

    int ind = mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx);
    vtkVariant ped = vtkArrayDownCast<vtkVariantArray>(peds)->GetValue(helper->GetVertexIndex(vtx));

    cout << "  Rank #" << myRank << ": vertex " << ped.ToString() << " ("
         << hex << vtx << "), owner="<<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
         << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;
//    int ind = output->GetDistributedGraphHelper()->GetVertexIndex(v);
//    int owner = output->GetDistributedGraphHelper()->GetVertexOwner(v);

    cout << myRank<<")   GetNumberOfArrays= " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
    for (int iprop=0; iprop<numProps; iprop++)
    {
      vtkAbstractArray* aa = vtkArrayDownCast<vtkAbstractArray>(mdg->GetVertexData()->GetAbstractArray(iprop));
//      int ind = helper->GetVertexIndex(vtx);
      cout << "     ind="<<ind<<") = "<< aa->GetVariantValue(ind).ToString() <<endl;
    }
    cout.flush();
  }

  if (myRank == 0)
    cout << "=============== dump edges\n"; cout.flush();

//  mdg->AddEdge(vtkVariant(0), vtkVariant(1));
  VTK_CREATE(vtkEdgeListIterator, eit);
  mdg->GetEdges(eit);
  while (eit->HasNext())
  {
    vtkEdgeType etx = eit->Next();

    cerr << "PROCESS " << myRank << " edge: " << hex << etx.Id
      << " (" << etx.Source << "," << etx.Target << ")" <<endl;
  }


  helper->Synchronize();
  if (myRank == 0)
  {
    cout << "===================================\n"; cout.flush();
    cout << "owner of 0= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant(0)) << endl;
    cout << "owner of 1= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant(1)) << endl;
    cout << "owner of 2= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant(2)) << endl;
    (cout << " done.\n").flush();
  }
}
//=================================================================================
// No pedigreeIds, just propArr
void UseCase5()
{
  VTK_CREATE(vtkMutableDirectedGraph, mdg);
  VTK_CREATE(vtkPBGLDistributedGraphHelper, helper);
  mdg->SetDistributedGraphHelper(helper);

  int myRank = mdg->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = mdg->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (myRank == 0)
    cout << "-----------------   UseCase5  ----------------------------\n";

  //  Create some vertex property arrays -  this includes a pedigreeID array too.
  VTK_CREATE(vtkVariantArray, vertexPropertyArr);
  int numVertexProperties = 4;
  vertexPropertyArr->SetNumberOfValues(numVertexProperties);
  vertexPropertyArr->SetName("MyBigFatProperties");

  VTK_CREATE(vtkVariantArray, names);
  names->SetName("names");
  mdg->GetVertexData()->AddArray(names);

  VTK_CREATE(vtkStringArray, vertexProp0Array);
  vertexProp0Array->SetName("labels");
  mdg->GetVertexData()->AddArray(vertexProp0Array);

  VTK_CREATE(vtkFloatArray, vertexProp1Array);
  vertexProp1Array->SetName("weight");
  mdg->GetVertexData()->AddArray(vertexProp1Array);

  VTK_CREATE(vtkIntArray, vertexProp2Array);
  vertexProp2Array->SetName("age");
  mdg->GetVertexData()->AddArray(vertexProp2Array);

  vtkVariant name;
  const char *stringProp;
  float weight;
  int age;

  for (vtkIdType i = 0; i < 3; ++i)
  {
    if (i==0)
    {
      stringProp = "labelA";
      weight = 40.0;
      age = 10;
      name = vtkVariant("nameA");
    }
    else if (i==1)
    {
      stringProp = "labelB";
      weight = 41.0;
      age = 11;
      name = vtkVariant("nameB");
    }
    else if (i==2)
    {
      stringProp = "labelC";
      weight = 42.0;
      age = 12;
      name = vtkVariant("nameC");
    }

      vertexPropertyArr->SetValue(0,name);
      vertexPropertyArr->SetValue(1,stringProp);
      vertexPropertyArr->SetValue(2,weight);
      vertexPropertyArr->SetValue(3,age);

      if ((myRank % 3) == i)  // Not doing this 'if' will add all the vertices to all procs
      {
        mdg->AddVertex(vertexPropertyArr);
      }
  }


  cout << myRank<<")   num vertexdata arrays = " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
  if (mdg->GetVertexData()->HasArray("weight") ) cout << myRank<< ")   got weight...\n";
  cout << myRank<<")   num verts= " << mdg->GetNumberOfVertices() << endl;

  helper->Synchronize();

  if (myRank == 0)
    cout << "=============== dump vertices\n"; cout.flush();

  vtkDataSetAttributes *vertexData = mdg->GetVertexData();
  int numProps = vertexData->GetNumberOfArrays();   // # of properties = # of arrays
  cout << "   numProps = "<<numProps<<endl;

  VTK_CREATE(vtkVertexListIterator, vit);
  mdg->GetVertices(vit);
  while (vit->HasNext())
  {
    vtkIdType vtx = vit->Next();

    int ind = mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx);

    cout << "  Rank #" << myRank << ": vertex   ("
         << hex << vtx << "), owner="
    <<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
    << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;

    cout << myRank<<")   GetNumberOfArrays= " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
    for (int iprop=0; iprop<numProps; iprop++)
    {
      vtkAbstractArray* aa = vtkArrayDownCast<vtkAbstractArray>(mdg->GetVertexData()->GetAbstractArray(iprop));
//      int ind = helper->GetVertexIndex(vtx);
      cout << "     ind="<<ind<<") = "<< aa->GetVariantValue(ind).ToString() <<endl;
    }
    cout.flush();
  }
//  }

  helper->Synchronize();
  if (myRank == 0)
  {
    (cout << " done.\n").flush();
  }
}

}

//=================================================================================
int TestPBGLPedigrees(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  UseCase0();
  UseCase1();
  UseCase2();
  UseCase3();
  UseCase4();
  UseCase5();
  MPI_Finalize();
  return 0;
}
