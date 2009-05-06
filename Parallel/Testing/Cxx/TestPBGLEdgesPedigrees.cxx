/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBGLEdgesPedigrees.cxx

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

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


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
//  mdg->AddEdge("B","C");
//  mdg->AddEdge("C","A");
  mdg->AddEdge("D","E");


  helper->Synchronize();

  if (myRank == 0)
    {
    cout << "===================================\n"; cout.flush();
    cout << "owner of A= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("A")) << endl;
    cout << "owner of B= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("B")) << endl;
//    cout << "owner of C= "<< helper->GetVertexOwnerByPedigreeId(vtkVariant("C")) << endl;
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

    vtkVariant ped = vtkVariantArray::SafeDownCast(peds)->GetValue(helper->GetVertexIndex(vtx));

    cout <<"  Rank #" << myRank << ": vertex " << ped.ToString() << " ("
         << hex << vtx << ")" << " owner="<<mdg->GetDistributedGraphHelper()->GetVertexOwner(vtx)<< ", "
         << " index="<<mdg->GetDistributedGraphHelper()->GetVertexIndex(vtx) << endl;

    cout << myRank<<") "<<"  GetNumberOfArrays= " << mdg->GetVertexData()->GetNumberOfArrays() << endl;
    for (int iprop=0; iprop<numProps; iprop++)
      {
      vtkAbstractArray* aa = vtkAbstractArray::SafeDownCast(mdg->GetVertexData()->GetAbstractArray(iprop));
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
int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  UseCase3();
  MPI_Finalize();
  return 0;
}
