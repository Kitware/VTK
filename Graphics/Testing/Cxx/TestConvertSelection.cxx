/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConvertSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtksys/stl/map>

template <typename T>
int CompareArrays(T* a, T* b, vtkIdType n)
{
  int errors = 0;
  for (vtkIdType i = 0; i < n; i++)
    {
    if (a[i] != b[i])
      {
      cerr << "ERROR: Arrays do not match at index " << i << " (" << a[i] << "!=" << b[i] << ")" << endl;
      errors++;
      }
    }
  return errors;
}

const char* SelectionTypeToString(int type)
{
  switch (type)
  {
  case vtkSelectionNode::SELECTIONS:
  return "Selections";
  case vtkSelectionNode::GLOBALIDS:
  return "Global IDs";
  case vtkSelectionNode::PEDIGREEIDS:
  return "Pedigree IDs";
  case vtkSelectionNode::VALUES:
  return "Values";
  case vtkSelectionNode::INDICES:
  return "Indices";
  case vtkSelectionNode::FRUSTUM:
  return "Frustum";
  case vtkSelectionNode::THRESHOLDS:
  return "Thresholds";
  case vtkSelectionNode::LOCATIONS:
  return "Locations";
  default:
  return "Unknown";
  }
}

int CompareSelections(vtkSelectionNode* a, vtkSelectionNode* b)
{
  int errors = 0;
  if (a->GetContentType() != b->GetContentType())
    {
    cerr << "ERROR: Content type " << SelectionTypeToString(a->GetContentType()) << " does not match " << SelectionTypeToString(b->GetContentType()) << endl;
    errors++;
    }
  if (a->GetFieldType() != b->GetFieldType())
    {
    cerr << "ERROR: Field type " << a->GetFieldType() << " does not match " << b->GetFieldType() << endl;
    errors++;
    }
  vtkAbstractArray* arra = a->GetSelectionList();
  vtkAbstractArray* arrb = b->GetSelectionList();
  if (arra->GetName() && !arrb->GetName())
    {
    cerr << "ERROR: Array name a is not null but b is" << endl;
    errors++;
    }
  else if (!arra->GetName() && arrb->GetName())
    {
    cerr << "ERROR: Array name a is null but b is not" << endl;
    errors++;
    }
  else if (arra->GetName() && strcmp(arra->GetName(), arrb->GetName()))
    {
    cerr << "ERROR: Array name " << arra->GetName() << " does not match " << arrb->GetName() << endl;
    errors++;
    }
  if (arra->GetDataType() != arrb->GetDataType())
    {
    cerr << "ERROR: Array type " << arra->GetDataType() << " does not match " << arrb->GetDataType() << endl;
    errors++;
    }
  else if (arra->GetNumberOfTuples() != arrb->GetNumberOfTuples())
    {
    cerr << "ERROR: Array tuples " << arra->GetNumberOfTuples() << " does not match " << arrb->GetNumberOfTuples() << endl;
    errors++;
    }
  else
    {
    vtkSortDataArray::Sort(arra);
    vtkSortDataArray::Sort(arrb);
    switch (arra->GetDataType())
      {
      vtkExtendedTemplateMacro(errors += CompareArrays((VTK_TT*)arra->GetVoidPointer(0), (VTK_TT*)arrb->GetVoidPointer(0), arra->GetNumberOfTuples()));
      }
    }
  return errors;
}

int TestConvertSelectionType(
  vtksys_stl::map<int, vtkSmartPointer<vtkSelection> >& selMap,
  vtkDataObject* data,
  int inputType,
  int outputType,
  vtkStringArray* arr = 0)
{
  cerr << "Testing conversion from type " << SelectionTypeToString(inputType) << " to " << SelectionTypeToString(outputType) << "..." << endl;
  vtkSelection* s = vtkConvertSelection::ToSelectionType(selMap[inputType], data, outputType, arr);
  int errors = CompareSelections(selMap[outputType]->GetNode(0), s->GetNode(0));
  s->Delete();
  cerr << "...done." << endl;
  return errors;
}

void GraphConvertSelections(int & errors, int size)
{
  // Create the test data
  VTK_CREATE(vtkMutableUndirectedGraph, g);
  VTK_CREATE(vtkIdTypeArray, pedIdVertArr);
  pedIdVertArr->SetName("PedId");
  g->GetVertexData()->AddArray(pedIdVertArr);
  g->GetVertexData()->SetPedigreeIds(pedIdVertArr);
  VTK_CREATE(vtkIdTypeArray, globalIdVertArr);
  globalIdVertArr->SetName("GlobalId");
  g->GetVertexData()->AddArray(globalIdVertArr);
  g->GetVertexData()->SetGlobalIds(globalIdVertArr);
  VTK_CREATE(vtkDoubleArray, doubleVertArr);
  doubleVertArr->SetName("Double");
  g->GetVertexData()->AddArray(doubleVertArr);
  VTK_CREATE(vtkStringArray, stringVertArr);
  stringVertArr->SetName("String");
  g->GetVertexData()->AddArray(stringVertArr);
  VTK_CREATE(vtkPoints, pts);
  for (int i = 0; i < size; i++)
    {
    g->AddVertex();
    doubleVertArr->InsertNextValue(i%2);
    stringVertArr->InsertNextValue(vtkVariant(i).ToString());
    pedIdVertArr->InsertNextValue(i);
    globalIdVertArr->InsertNextValue(i);
    pts->InsertNextPoint(i, i%2, 0);
    }
  g->SetPoints(pts);
  
  g->GetEdgeData()->AddArray(pedIdVertArr);
  g->GetEdgeData()->SetPedigreeIds(pedIdVertArr);
  g->GetEdgeData()->AddArray(globalIdVertArr);
  g->GetEdgeData()->SetGlobalIds(globalIdVertArr);
  g->GetEdgeData()->AddArray(doubleVertArr);
  g->GetEdgeData()->AddArray(stringVertArr);
  for (int i = 0; i < size; i++)
    {
    g->AddEdge(i, i);
    }
  
  vtksys_stl::map<int, vtkSmartPointer<vtkSelection> > selMap;
  
  VTK_CREATE(vtkSelection, globalIdsSelection);
  VTK_CREATE(vtkSelectionNode, globalIdsSelectionNode);
  globalIdsSelection->AddNode(globalIdsSelectionNode);
  globalIdsSelectionNode->SetContentType(vtkSelectionNode::GLOBALIDS);
  globalIdsSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
  VTK_CREATE(vtkIdTypeArray, globalIdsArr);
  globalIdsArr->SetName("GlobalId");
  globalIdsSelectionNode->SetSelectionList(globalIdsArr);
  for (int i = 0; i < size; i += 2)
    {
    globalIdsArr->InsertNextValue(i);
    }
  selMap[vtkSelectionNode::GLOBALIDS] = globalIdsSelection;
  
  VTK_CREATE(vtkSelection, pedigreeIdsSelection);
  VTK_CREATE(vtkSelectionNode, pedigreeIdsSelectionNode);
  pedigreeIdsSelection->AddNode(pedigreeIdsSelectionNode);
  pedigreeIdsSelectionNode->SetContentType(vtkSelectionNode::PEDIGREEIDS);
  pedigreeIdsSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
  VTK_CREATE(vtkIdTypeArray, pedigreeIdsArr);
  pedigreeIdsArr->SetName("PedId");
  pedigreeIdsSelectionNode->SetSelectionList(pedigreeIdsArr);
  for (int i = 0; i < size; i += 2)
    {
    pedigreeIdsArr->InsertNextValue(i);
    }
  selMap[vtkSelectionNode::PEDIGREEIDS] = pedigreeIdsSelection;
  
  VTK_CREATE(vtkSelection, valuesSelection);
  VTK_CREATE(vtkSelectionNode, valuesSelectionNode);
  valuesSelection->AddNode(valuesSelectionNode);
  valuesSelectionNode->SetContentType(vtkSelectionNode::VALUES);
  valuesSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
  VTK_CREATE(vtkStringArray, valuesArr);
  valuesArr->SetName("String");
  valuesSelectionNode->SetSelectionList(valuesArr);
  for (int i = 0; i < size; i += 2)
    {
    valuesArr->InsertNextValue(vtkVariant(i).ToString());
    }
  selMap[vtkSelectionNode::VALUES] = valuesSelection;
  
  VTK_CREATE(vtkSelection, indicesSelection);
  VTK_CREATE(vtkSelectionNode, indicesSelectionNode);
  indicesSelection->AddNode(indicesSelectionNode);
  indicesSelectionNode->SetContentType(vtkSelectionNode::INDICES);
  indicesSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
  VTK_CREATE(vtkIdTypeArray, indicesArr);
  indicesSelectionNode->SetSelectionList(indicesArr);
  for (int i = 0; i < size; i += 2)
    {
    indicesArr->InsertNextValue(i);
    }
  selMap[vtkSelectionNode::INDICES] = indicesSelection;
  
  VTK_CREATE(vtkSelection, frustumSelection);
  VTK_CREATE(vtkSelectionNode, frustumSelectionNode);
  frustumSelection->AddNode(frustumSelectionNode);
  frustumSelectionNode->SetContentType(vtkSelectionNode::FRUSTUM);
  frustumSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
  // near lower left, 
  // far lower left,
  // near upper left, 
  // far upper left,
  // near lower right, 
  // far lower right,
  // near upper right, 
  // far upper right,
  double corners[] = {
    -1.0, -0.5,  1.0, 1.0,
    -1.0, -0.5, -1.0, 1.0,
    -1.0,  0.5,  1.0, 1.0,
    -1.0,  0.5, -1.0, 1.0,
    size, -0.5,  1.0, 1.0,
    size, -0.5, -1.0, 1.0,
    size,  0.5,  1.0, 1.0,
    size,  0.5, -1.0, 1.0
  };
  VTK_CREATE(vtkDoubleArray, frustumArr);
  for (vtkIdType i = 0; i < 32; i++)
    {
    frustumArr->InsertNextValue(corners[i]);
    }
  frustumSelectionNode->SetSelectionList(frustumArr);
  selMap[vtkSelectionNode::FRUSTUM] = frustumSelection;
  
  VTK_CREATE(vtkSelection, locationsSelection);
  VTK_CREATE(vtkSelectionNode, locationsSelectionNode);
  locationsSelection->AddNode(locationsSelectionNode);
  locationsSelectionNode->SetContentType(vtkSelectionNode::INDICES);
  locationsSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
  VTK_CREATE(vtkFloatArray, locationsArr);
  locationsArr->SetNumberOfComponents(3);
  locationsSelectionNode->SetSelectionList(locationsArr);
  for (int i = 0; i < size; i += 2)
    {
    locationsArr->InsertNextTuple3(i, 0, 0);
    }
  selMap[vtkSelectionNode::LOCATIONS] = locationsSelection;
  
  VTK_CREATE(vtkSelection, thresholdsSelection);
  VTK_CREATE(vtkSelectionNode, thresholdsSelectionNode);
  thresholdsSelection->AddNode(thresholdsSelectionNode);
  thresholdsSelectionNode->SetContentType(vtkSelectionNode::THRESHOLDS);
  thresholdsSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
  VTK_CREATE(vtkDoubleArray, thresholdsArr);
  thresholdsArr->SetName("Double");
  thresholdsArr->InsertNextValue(-0.5);
  thresholdsArr->InsertNextValue(0.5);
  thresholdsSelectionNode->SetSelectionList(thresholdsArr);
  selMap[vtkSelectionNode::THRESHOLDS] = thresholdsSelection;
  
  VTK_CREATE(vtkStringArray, arrNames);
  arrNames->InsertNextValue("String");

  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);
  
  //
  // Test cell selections
  //
  
  selMap[vtkSelectionNode::GLOBALIDS]->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
  selMap[vtkSelectionNode::PEDIGREEIDS]->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
  selMap[vtkSelectionNode::VALUES]->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
  selMap[vtkSelectionNode::INDICES]->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
  selMap[vtkSelectionNode::THRESHOLDS]->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
  selMap[vtkSelectionNode::FRUSTUM]->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
  selMap[vtkSelectionNode::LOCATIONS]->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
  
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);
}  

void PolyDataConvertSelections(int & errors, int size)
{
  // Create the test data
  VTK_CREATE(vtkPolyData, g);
  VTK_CREATE(vtkIdTypeArray, pedIdVertArr);
  pedIdVertArr->SetName("PedId");
  g->GetPointData()->AddArray(pedIdVertArr);
  g->GetPointData()->SetPedigreeIds(pedIdVertArr);
  VTK_CREATE(vtkIdTypeArray, globalIdVertArr);
  globalIdVertArr->SetName("GlobalId");
  g->GetPointData()->AddArray(globalIdVertArr);
  g->GetPointData()->SetGlobalIds(globalIdVertArr);
  VTK_CREATE(vtkDoubleArray, doubleVertArr);
  doubleVertArr->SetName("Double");
  g->GetPointData()->AddArray(doubleVertArr);
  VTK_CREATE(vtkStringArray, stringVertArr);
  stringVertArr->SetName("String");
  g->GetPointData()->AddArray(stringVertArr);
  VTK_CREATE(vtkPoints, pts);
  for (int i = 0; i < size; i++)
    {
    doubleVertArr->InsertNextValue(i%2);
    stringVertArr->InsertNextValue(vtkVariant(i).ToString());
    pedIdVertArr->InsertNextValue(i);
    globalIdVertArr->InsertNextValue(i);
    pts->InsertNextPoint(i, i%2, 0);
    }
  g->SetPoints(pts);
  
  g->GetCellData()->AddArray(pedIdVertArr);
  g->GetCellData()->SetPedigreeIds(pedIdVertArr);
  g->GetCellData()->AddArray(globalIdVertArr);
  g->GetCellData()->SetGlobalIds(globalIdVertArr);
  g->GetCellData()->AddArray(doubleVertArr);
  g->GetCellData()->AddArray(stringVertArr);

  VTK_CREATE(vtkCellArray, newLines);
  newLines->Allocate(newLines->EstimateSize(size, 2));
  vtkIdType cellPts[2];
  for (int i = 0; i < size; i++)
    {
    cellPts[0] = i;
    cellPts[1] = i;
    newLines->InsertNextCell(2, cellPts);
    }
  g->SetLines(newLines);
  
  vtksys_stl::map<int, vtkSmartPointer<vtkSelection> > selMap;
  
  VTK_CREATE(vtkSelection, globalIdsSelection);
  VTK_CREATE(vtkSelectionNode, globalIdsSelectionNode);
  globalIdsSelection->AddNode(globalIdsSelectionNode);
  globalIdsSelectionNode->SetContentType(vtkSelectionNode::GLOBALIDS);
  globalIdsSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  VTK_CREATE(vtkIdTypeArray, globalIdsArr);
  globalIdsArr->SetName("GlobalId");
  globalIdsSelectionNode->SetSelectionList(globalIdsArr);
  for (int i = 0; i < size; i += 2)
    {
    globalIdsArr->InsertNextValue(i);
    }
  selMap[vtkSelectionNode::GLOBALIDS] = globalIdsSelection;
  
  VTK_CREATE(vtkSelection, pedigreeIdsSelection);
  VTK_CREATE(vtkSelectionNode, pedigreeIdsSelectionNode);
  pedigreeIdsSelection->AddNode(pedigreeIdsSelectionNode);
  pedigreeIdsSelectionNode->SetContentType(vtkSelectionNode::PEDIGREEIDS);
  pedigreeIdsSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  VTK_CREATE(vtkIdTypeArray, pedigreeIdsArr);
  pedigreeIdsArr->SetName("PedId");
  pedigreeIdsSelectionNode->SetSelectionList(pedigreeIdsArr);
  for (int i = 0; i < size; i += 2)
    {
    pedigreeIdsArr->InsertNextValue(i);
    }
  selMap[vtkSelectionNode::PEDIGREEIDS] = pedigreeIdsSelection;
  
  VTK_CREATE(vtkSelection, valuesSelection);
  VTK_CREATE(vtkSelectionNode, valuesSelectionNode);
  valuesSelection->AddNode(valuesSelectionNode);
  valuesSelectionNode->SetContentType(vtkSelectionNode::VALUES);
  valuesSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  VTK_CREATE(vtkStringArray, valuesArr);
  valuesArr->SetName("String");
  valuesSelectionNode->SetSelectionList(valuesArr);
  for (int i = 0; i < size; i += 2)
    {
    valuesArr->InsertNextValue(vtkVariant(i).ToString());
    }
  selMap[vtkSelectionNode::VALUES] = valuesSelection;
  
  VTK_CREATE(vtkSelection, indicesSelection);
  VTK_CREATE(vtkSelectionNode, indicesSelectionNode);
  indicesSelection->AddNode(indicesSelectionNode);
  indicesSelectionNode->SetContentType(vtkSelectionNode::INDICES);
  indicesSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  VTK_CREATE(vtkIdTypeArray, indicesArr);
  indicesSelectionNode->SetSelectionList(indicesArr);
  for (int i = 0; i < size; i += 2)
    {
    indicesArr->InsertNextValue(i);
    }
  selMap[vtkSelectionNode::INDICES] = indicesSelection;
  
  VTK_CREATE(vtkSelection, frustumSelection);
  VTK_CREATE(vtkSelectionNode, frustumSelectionNode);
  frustumSelection->AddNode(frustumSelectionNode);
  frustumSelectionNode->SetContentType(vtkSelectionNode::FRUSTUM);
  frustumSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  // near lower left, far lower left
  // near upper left, far upper left
  // near lower right, far lower right
  // near upper right, far upper right
  double corners[] = {
    -1.0, -0.5,  1.0, 1.0,
    -1.0, -0.5, -1.0, 1.0,
    -1.0,  0.5,  1.0, 1.0,
    -1.0,  0.5, -1.0, 1.0,
    size, -0.5,  1.0, 1.0,
    size, -0.5, -1.0, 1.0,
    size,  0.5,  1.0, 1.0,
    size,  0.5, -1.0, 1.0
  };
  VTK_CREATE(vtkDoubleArray, frustumArr);
  for (vtkIdType i = 0; i < 32; i++)
    {
    frustumArr->InsertNextValue(corners[i]);
    }
  frustumSelectionNode->SetSelectionList(frustumArr);
  selMap[vtkSelectionNode::FRUSTUM] = frustumSelection;
  
  VTK_CREATE(vtkSelection, locationsSelection);
  VTK_CREATE(vtkSelectionNode, locationsSelectionNode);
  locationsSelection->AddNode(locationsSelectionNode);
  locationsSelectionNode->SetContentType(vtkSelectionNode::INDICES);
  locationsSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  VTK_CREATE(vtkFloatArray, locationsArr);
  locationsArr->SetNumberOfComponents(3);
  locationsSelectionNode->SetSelectionList(locationsArr);
  for (int i = 0; i < size; i += 2)
    {
    locationsArr->InsertNextTuple3(i, 0, 0);
    }
  selMap[vtkSelectionNode::LOCATIONS] = locationsSelection;
  
  VTK_CREATE(vtkSelection, thresholdsSelection);
  VTK_CREATE(vtkSelectionNode, thresholdsSelectionNode);
  thresholdsSelection->AddNode(thresholdsSelectionNode);
  thresholdsSelectionNode->SetContentType(vtkSelectionNode::THRESHOLDS);
  thresholdsSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  VTK_CREATE(vtkDoubleArray, thresholdsArr);
  thresholdsArr->SetName("Double");
  thresholdsArr->InsertNextValue(-0.5);
  thresholdsArr->InsertNextValue(0.5);
  thresholdsSelectionNode->SetSelectionList(thresholdsArr);
  selMap[vtkSelectionNode::THRESHOLDS] = thresholdsSelection;
  
  VTK_CREATE(vtkStringArray, arrNames);
  arrNames->InsertNextValue("String");

  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::INDICES);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::GLOBALIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::PEDIGREEIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::VALUES, arrNames);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::INDICES);
  
  //
  // Test cell selections
  //
  
  selMap[vtkSelectionNode::GLOBALIDS]->GetNode(0)->SetFieldType(vtkSelectionNode::CELL);
  selMap[vtkSelectionNode::PEDIGREEIDS]->GetNode(0)->SetFieldType(vtkSelectionNode::CELL);
  selMap[vtkSelectionNode::VALUES]->GetNode(0)->SetFieldType(vtkSelectionNode::CELL);
  selMap[vtkSelectionNode::INDICES]->GetNode(0)->SetFieldType(vtkSelectionNode::CELL);
  selMap[vtkSelectionNode::THRESHOLDS]->GetNode(0)->SetFieldType(vtkSelectionNode::CELL);
  selMap[vtkSelectionNode::FRUSTUM]->GetNode(0)->SetFieldType(vtkSelectionNode::CELL);
  selMap[vtkSelectionNode::LOCATIONS]->GetNode(0)->SetFieldType(vtkSelectionNode::CELL);
  
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::INDICES);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::GLOBALIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::PEDIGREEIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::VALUES, arrNames);
  //errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::INDICES);
}  

int TestConvertSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;
  int size = 10;

  GraphConvertSelections(errors, size);
  PolyDataConvertSelections(errors, size);
  
  return errors;
}
