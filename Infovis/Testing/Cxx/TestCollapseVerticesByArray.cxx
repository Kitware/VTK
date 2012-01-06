/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCollapseVerticesByArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCollapseVerticesByArray.h"
#include "vtkVertexListIterator.h"
#include "vtkEdgeListIterator.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"
#include "vtkVariantArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkDataSetAttributes.h"
#include "vtkSmartPointer.h"

int TestCollapseVerticesByArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  typedef
  vtkSmartPointer<vtkMutableDirectedGraph>    vtkMutableDirectedGraphRefPtr;
  typedef
  vtkSmartPointer<vtkCollapseVerticesByArray> vtkCollapseVerticesByArrayRefPtr;
  typedef vtkSmartPointer<vtkVertexListIterator>
                                              vtkVertexListIteratorRefPtr;
  typedef vtkSmartPointer<vtkEdgeListIterator>
                                              vtkEdgeListIteratorRefPtr;
  typedef vtkSmartPointer <vtkDoubleArray>    vtkDoubleArrayRefPtr;
  typedef vtkSmartPointer <vtkIntArray>       vtkIntArrayRefPtr;
  typedef vtkSmartPointer <vtkStringArray>    vtkStringArrayRefPtr;
  typedef vtkSmartPointer <vtkVariantArray>   vtkVariantArrayRefPtr;

  int retVal = 0;

  // Create an empty graph.
  vtkMutableDirectedGraphRefPtr inGraph (vtkMutableDirectedGraphRefPtr::New());

  const int NO_OF_VERTICES = 3;
  vtkIdType vertexIds[NO_OF_VERTICES];

  for(int i=0; i < NO_OF_VERTICES; ++i)
    {
    vertexIds[i] = inGraph->AddVertex();
    }

  inGraph->AddEdge(vertexIds[0], vertexIds[1]);
  inGraph->AddEdge(vertexIds[0], vertexIds[2]);
  inGraph->AddEdge(vertexIds[2], vertexIds[1]);

  // Populate arrays.
  vtkStringArrayRefPtr idsArray         (vtkStringArrayRefPtr::New());
  vtkStringArrayRefPtr ownerArray       (vtkStringArrayRefPtr::New());
  vtkDoubleArrayRefPtr dataTransfer     (vtkDoubleArrayRefPtr::New());
  vtkDoubleArrayRefPtr avgDataTransfer  (vtkDoubleArrayRefPtr::New());
  vtkIntArrayRefPtr    capacityArray    (vtkIntArrayRefPtr::New());

  // idsArray
  idsArray->SetName("id");
  idsArray->InsertNextValue("CELL_TOWER_A");
  idsArray->InsertNextValue("CELL_TOWER_B");
  idsArray->InsertNextValue("CELL_TOWER_C");

  // ownerArray
  ownerArray->SetName("owner_company");
  ownerArray->InsertNextValue("AT&T");
  ownerArray->InsertNextValue("VERIZON");
  ownerArray->InsertNextValue("AT&T");

  // dataTransfer
  dataTransfer->SetName("data_transfer");
  dataTransfer->InsertNextValue(500);
  dataTransfer->InsertNextValue(100);
  dataTransfer->InsertNextValue(200);

  // avgDataTransfer
  avgDataTransfer->SetName("avg_data_transfer");
  avgDataTransfer->InsertNextValue(200);
  avgDataTransfer->InsertNextValue(300);
  avgDataTransfer->InsertNextValue(50);

  // capacityArray
  capacityArray->SetName("tower_capacity");
  capacityArray->InsertNextValue(1000);
  capacityArray->InsertNextValue(300);
  capacityArray->InsertNextValue(2000);

  inGraph->GetVertexData()->SetPedigreeIds(idsArray);
  inGraph->GetVertexData()->AddArray(ownerArray);
  inGraph->GetVertexData()->AddArray(capacityArray);
  inGraph->GetEdgeData()->AddArray(dataTransfer);
  inGraph->GetEdgeData()->AddArray(avgDataTransfer);


  vtkCollapseVerticesByArrayRefPtr cvs(vtkCollapseVerticesByArrayRefPtr::New());
  cvs->SetCountEdgesCollapsed(1);
  cvs->SetEdgesCollapsedArray("weight_edges");
  cvs->SetCountVerticesCollapsed(1);
  cvs->SetVerticesCollapsedArray("weight_vertices");
  cvs->SetVertexArray("owner_company");
  cvs->AddAggregateEdgeArray("data_transfer");
  cvs->SetInputData(inGraph);
  cvs->Update();

  // Check values here.
  vtkVariantArrayRefPtr resultNoSelfLoop (vtkVariantArrayRefPtr::New());
  vtkVariantArrayRefPtr resultSelfLoop   (vtkVariantArrayRefPtr::New());
  vtkVariantArrayRefPtr validResult      (vtkVariantArrayRefPtr::New());

  validResult->InsertNextValue("CELL_TOWER_C");
  validResult->InsertNextValue("AT&T");
  validResult->InsertNextValue(2000);
  validResult->InsertNextValue(2);
  validResult->InsertNextValue("CELL_TOWER_B");
  validResult->InsertNextValue("VERIZON");
  validResult->InsertNextValue(300);
  validResult->InsertNextValue(1);
  validResult->InsertNextValue(700);
  validResult->InsertNextValue(50);
  validResult->InsertNextValue(2);
  validResult->InsertNextValue(100);
  validResult->InsertNextValue(300);
  validResult->InsertNextValue(1);

  vtkVertexListIteratorRefPtr outVtxLstItr (vtkVertexListIteratorRefPtr::New());
  vtkEdgeListIteratorRefPtr   outEgeLstItr (vtkEdgeListIteratorRefPtr::New());

  outVtxLstItr->SetGraph(cvs->GetOutput());
  outEgeLstItr->SetGraph(cvs->GetOutput());

  vtkSmartPointer<vtkGraph> outGraph (cvs->GetOutput());

  while(outVtxLstItr->HasNext())
    {
    vtkIdType vtxId = outVtxLstItr->Next();
    for(int i=0; i < outGraph->GetVertexData()->GetNumberOfArrays(); ++i)
      {
      resultNoSelfLoop->InsertNextValue(
        outGraph->GetVertexData()->GetAbstractArray(i)->
        GetVariantValue(vtxId));
      }
    }

  while(outEgeLstItr->HasNext())
    {
    vtkEdgeType edge = outEgeLstItr->Next();
    for(int i=0; i < outGraph->GetEdgeData()->GetNumberOfArrays(); ++i)
      {
      resultNoSelfLoop->InsertNextValue(outGraph->GetEdgeData()->
        GetAbstractArray(i)->GetVariantValue(edge.Id));
      }
    }


  // Checking for self loops.
  cvs->AllowSelfLoopsOn();
  cvs->Update();

  outVtxLstItr->SetGraph(cvs->GetOutput());
  outEgeLstItr->SetGraph(cvs->GetOutput());

  while(outVtxLstItr->HasNext())
    {
    vtkIdType vtxId = outVtxLstItr->Next();

    for(int i=0; i < outGraph->GetVertexData()->GetNumberOfArrays(); ++i)
      {
      resultSelfLoop->InsertNextValue(
        outGraph->GetVertexData()->GetAbstractArray(i)->
        GetVariantValue(vtxId));
      }
    }

  while(outEgeLstItr->HasNext())
    {
    vtkEdgeType edge = outEgeLstItr->Next();

    for(int i=0; i < outGraph->GetEdgeData()->GetNumberOfArrays(); ++i)
      {
      resultSelfLoop->InsertNextValue(outGraph->GetEdgeData()->
        GetAbstractArray(i)->GetVariantValue(edge.Id));
      }
    }

  // Compare with the valid dataset.
  for(int i=0; i < resultNoSelfLoop->GetDataSize(); ++i)
    {
    if(resultNoSelfLoop->GetValue(i) != validResult->GetValue(i))
      {
      retVal++;
      }
    }

  for(int i=0; i < resultSelfLoop->GetDataSize(); ++i)
    {
    if(resultSelfLoop->GetValue(i) != validResult->GetValue(i))
      {
      retVal++;
      }
    }

  if(retVal !=0)
    {
    cerr << "Data mismatch with the valid dataset." << endl;
    }

  return retVal;
}

