/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAssignAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkAssignAttribute.

#include "vtkAssignAttribute.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#include <cstring>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestAssignAttribute(int, char *[])
{
  int errors = 0;

  VTK_CREATE(vtkMutableUndirectedGraph, graph);
  VTK_CREATE(vtkPolyData, poly);
  VTK_CREATE(vtkPoints, pts);
  VTK_CREATE(vtkCellArray, verts);


  VTK_CREATE(vtkDoubleArray, scalars);
  scalars->SetName("scalars");
  scalars->SetNumberOfComponents(3);

  VTK_CREATE(vtkDoubleArray, tensors);
  tensors->SetName(NULL); // no name.
  tensors->SetNumberOfComponents(9);
  for (vtkIdType i = 0; i < 10; ++i)
  {
    pts->InsertNextPoint(i, 0, 0);
    verts->InsertNextCell(1, &i);
    graph->AddVertex();
    scalars->InsertNextTuple3(i, 0.5 * i, 0.1 * i);
    tensors->InsertNextTuple9(1.,0.,0.,0.,1.,0.,0.,0.,1.);
  }
  for (vtkIdType i = 0; i < 10; ++i)
  {
    graph->AddEdge(i, (i+1)%10);
  }
  graph->GetVertexData()->AddArray(scalars);
  graph->GetEdgeData()->AddArray(scalars);
  graph->GetVertexData()->SetTensors(tensors);
  graph->GetEdgeData()->SetTensors(tensors);

  poly->SetPoints(pts);
  poly->SetVerts(verts);
  poly->GetPointData()->AddArray(scalars);
  poly->GetCellData()->AddArray(scalars);
  poly->GetPointData()->SetTensors(tensors);
  poly->GetCellData()->SetTensors(tensors);

  VTK_CREATE(vtkAssignAttribute, assign);

  assign->SetInputData(graph);
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::VERTEX_DATA);
  assign->Update();
  vtkGraph *output = vtkGraph::SafeDownCast(assign->GetOutput());
  if (output->GetVertexData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Vertex scalars not set properly" << endl;
    ++errors;
  }
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::EDGE_DATA);
  assign->Update();
  output = vtkGraph::SafeDownCast(assign->GetOutput());
  if (output->GetEdgeData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Edge scalars not set properly" << endl;
    ++errors;
  }

  assign->SetInputData(poly);
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
  assign->Update();
  vtkPolyData *outputPoly = vtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetPointData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Point scalars not set properly" << endl;
    ++errors;
  }
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
  assign->Update();
  outputPoly = vtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetCellData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Cell scalars not set properly" << endl;
    ++errors;
  }

  assign->Assign(vtkDataSetAttributes::TENSORS,
    vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
  assign->Update();
  outputPoly = vtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetPointData()->GetTensors() != tensors.GetPointer())
  {
    cerr << "Point scalar not set when name is empty" << endl;
    ++errors;
  }
  assign->Assign(vtkDataSetAttributes::TENSORS,
    vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
  assign->Update();
  outputPoly = vtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetCellData()->GetTensors() != tensors.GetPointer())
  {
    cerr << "Cell scalar not set when name is empty" << endl;
    ++errors;
  }
  vtkInformation *inInfo = assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  vtkInformation *outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outInfo->Clear();
  vtkDataObject::SetActiveAttribute(inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    scalars->GetName(), vtkDataSetAttributes::SCALARS);
  vtkDataObject::SetActiveAttributeInfo(inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS, scalars->GetName(), scalars->GetDataType(),
    scalars->GetNumberOfComponents(), scalars->GetNumberOfTuples());
  assign->Assign(scalars->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
  assign->UpdateInformation();
  vtkInformation *outFieldInfo = vtkDataObject::GetActiveFieldInformation(outInfo,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
  if (!outFieldInfo
    || !outFieldInfo->Has(vtkDataObject::FIELD_NAME())
    || std::strcmp(outFieldInfo->Get(vtkDataObject::FIELD_NAME()), scalars->GetName())
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) != scalars->GetNumberOfComponents()
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()) != scalars->GetNumberOfTuples()
    || outFieldInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()) != scalars->GetDataType())
  {
    cerr << "Scalar information not passed when attribute is assigned by name." << endl;
    ++errors;
  }
  outInfo->Clear();
  inInfo = assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  vtkDataObject::SetActiveAttribute(inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    scalars->GetName(), vtkDataSetAttributes::SCALARS);
  vtkDataObject::SetActiveAttributeInfo(inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS, scalars->GetName(), scalars->GetDataType(),
    scalars->GetNumberOfComponents(), scalars->GetNumberOfTuples());
  assign->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
  assign->UpdateInformation();
  outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outFieldInfo = vtkDataObject::GetActiveFieldInformation(outInfo,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
  if (!outFieldInfo
    || !outFieldInfo->Has(vtkDataObject::FIELD_NAME())
    || std::strcmp(outFieldInfo->Get(vtkDataObject::FIELD_NAME()), scalars->GetName())
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) != scalars->GetNumberOfComponents()
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()) != scalars->GetNumberOfTuples()
    || outFieldInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()) != scalars->GetDataType())
  {
    cerr << "Scalar information not passed when attribute is assigned by type." << endl;
    ++errors;
  }
  outInfo->Clear();
  assign->SetInputData(graph);
  tensors->SetName("tensors");
  inInfo = assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  vtkDataObject::SetActiveAttribute(inInfo, vtkDataObject::FIELD_ASSOCIATION_EDGES,
    tensors->GetName(), vtkDataSetAttributes::TENSORS);
  vtkDataObject::SetActiveAttributeInfo(inInfo, vtkDataObject::FIELD_ASSOCIATION_EDGES,
    vtkDataSetAttributes::TENSORS, tensors->GetName(), tensors->GetDataType(),
    tensors->GetNumberOfComponents(), tensors->GetNumberOfTuples());
  assign->Assign(tensors->GetName(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::EDGE_DATA);
  assign->UpdateInformation();
  outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outFieldInfo = vtkDataObject::GetActiveFieldInformation(outInfo,
    vtkDataObject::FIELD_ASSOCIATION_EDGES, vtkDataSetAttributes::SCALARS);
  if (!outFieldInfo
    || !outFieldInfo->Has(vtkDataObject::FIELD_NAME())
    || std::strcmp(outFieldInfo->Get(vtkDataObject::FIELD_NAME()), tensors->GetName())
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) != tensors->GetNumberOfComponents()
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()) != tensors->GetNumberOfTuples()
    || outFieldInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()) != tensors->GetDataType())
  {
    cerr << "Tensor information not passed when attribute is assigned by name." << endl;
    ++errors;
  }
  outInfo->Clear();
  inInfo = assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  vtkDataObject::SetActiveAttribute(inInfo, vtkDataObject::FIELD_ASSOCIATION_EDGES,
    tensors->GetName(), vtkDataSetAttributes::TENSORS);
  vtkDataObject::SetActiveAttributeInfo(inInfo, vtkDataObject::FIELD_ASSOCIATION_EDGES,
    vtkDataSetAttributes::TENSORS, tensors->GetName(), tensors->GetDataType(),
    tensors->GetNumberOfComponents(), tensors->GetNumberOfTuples());
  assign->Assign(vtkDataSetAttributes::TENSORS, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::EDGE_DATA);
  assign->UpdateInformation();
  outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outFieldInfo = vtkDataObject::GetActiveFieldInformation(outInfo,
    vtkDataObject::FIELD_ASSOCIATION_EDGES, vtkDataSetAttributes::SCALARS);
  if (!outFieldInfo
    || !outFieldInfo->Has(vtkDataObject::FIELD_NAME())
    || std::strcmp(outFieldInfo->Get(vtkDataObject::FIELD_NAME()), tensors->GetName())
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) != tensors->GetNumberOfComponents()
    || outFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()) != tensors->GetNumberOfTuples()
    || outFieldInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()) != tensors->GetDataType())
  {
    cerr << "Tensor information not passed when attribute is assigned by type." << endl;
    ++errors;
  }
  return 0;
}
