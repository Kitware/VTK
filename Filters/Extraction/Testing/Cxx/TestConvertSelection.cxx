// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <iostream>
#include <map>

int CompareSelections(vtkSelectionNode* a, vtkSelectionNode* b)
{
  int errors = 0;
  if (!a || !b)
  {
    std::cerr << "ERROR: Empty Selection Node(s)" << std::endl;
    errors++;
    return errors;
  }
  if (a->GetContentType() != b->GetContentType())
  {
    std::cerr << "ERROR: Content type "
              << vtkSelectionNode::GetContentTypeAsString(a->GetContentType()) << " does not match "
              << vtkSelectionNode::GetContentTypeAsString(b->GetContentType()) << std::endl;
    errors++;
  }
  if (a->GetFieldType() != b->GetFieldType())
  {
    std::cerr << "ERROR: Field type " << a->GetFieldType() << " does not match "
              << b->GetFieldType() << std::endl;
    errors++;
  }
  vtkAbstractArray* arra = a->GetSelectionList();
  vtkAbstractArray* arrb = b->GetSelectionList();
  errors += !vtkTestUtilities::CompareAbstractArray(arra, arrb);

  return errors;
}

int TestConvertSelectionType(std::map<int, vtkSmartPointer<vtkSelection>>& selMap,
  vtkDataObject* data, int inputType, int outputType, vtkStringArray* arr = nullptr,
  bool allowMissingArray = false)
{
  std::cerr << "Testing conversion from type "
            << vtkSelectionNode::GetContentTypeAsString(inputType) << " to "
            << vtkSelectionNode::GetContentTypeAsString(outputType) << "..." << std::endl;
  vtkSelection* s = vtkConvertSelection::ToSelectionType(
    selMap[inputType], data, outputType, arr, -1, allowMissingArray);
  int errors = 0;
  if (!allowMissingArray)
  {
    errors = CompareSelections(selMap[outputType]->GetNode(0), s->GetNode(0));
  }
  s->Delete();
  std::cerr << "...done." << std::endl;
  return errors;
}

void GraphConvertSelections(int& errors, int size)
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
    doubleVertArr->InsertNextValue(i % 2);
    stringVertArr->InsertNextValue(vtkVariant(i).ToString());
    pedIdVertArr->InsertNextValue(i);
    globalIdVertArr->InsertNextValue(i);
    pts->InsertNextPoint(i, i % 2, 0);
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

  std::map<int, vtkSmartPointer<vtkSelection>> selMap;

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
  double corners[] = { -1.0, -0.5, 1.0, 1.0, -1.0, -0.5, -1.0, 1.0, -1.0, 0.5, 1.0, 1.0, -1.0, 0.5,
    -1.0, 1.0, static_cast<double>(size), -0.5, 1.0, 1.0, static_cast<double>(size), -0.5, -1.0,
    1.0, static_cast<double>(size), 0.5, 1.0, 1.0, static_cast<double>(size), 0.5, -1.0, 1.0 };
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

  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);

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

  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);
}

void PolyDataConvertSelections(int& errors, int size)
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
    doubleVertArr->InsertNextValue(i % 2);
    stringVertArr->InsertNextValue(vtkVariant(i).ToString());
    pedIdVertArr->InsertNextValue(i);
    globalIdVertArr->InsertNextValue(i);
    pts->InsertNextPoint(i, i % 2, 0);
  }
  g->SetPoints(pts);

  g->GetCellData()->AddArray(pedIdVertArr);
  g->GetCellData()->SetPedigreeIds(pedIdVertArr);
  g->GetCellData()->AddArray(globalIdVertArr);
  g->GetCellData()->SetGlobalIds(globalIdVertArr);
  g->GetCellData()->AddArray(doubleVertArr);
  g->GetCellData()->AddArray(stringVertArr);

  VTK_CREATE(vtkCellArray, newLines);
  newLines->AllocateEstimate(size, 2);
  vtkIdType cellPts[2];
  for (int i = 0; i < size; i++)
  {
    cellPts[0] = i;
    cellPts[1] = i;
    newLines->InsertNextCell(2, cellPts);
  }
  g->SetLines(newLines);

  std::map<int, vtkSmartPointer<vtkSelection>> selMap;

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
  double corners[] = { -1.0, -0.5, 1.0, 1.0, -1.0, -0.5, -1.0, 1.0, -1.0, 0.5, 1.0, 1.0, -1.0, 0.5,
    -1.0, 1.0, static_cast<double>(size), -0.5, 1.0, 1.0, static_cast<double>(size), -0.5, -1.0,
    1.0, static_cast<double>(size), 0.5, 1.0, 1.0, static_cast<double>(size), 0.5, -1.0, 1.0 };
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

  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::INDICES);
  // errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS,
  // vtkSelectionNode::GLOBALIDS); errors += TestConvertSelectionType(selMap, g,
  // vtkSelectionNode::LOCATIONS, vtkSelectionNode::PEDIGREEIDS); errors +=
  // TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::VALUES,
  // arrNames); errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS,
  // vtkSelectionNode::INDICES);

  // Test Quiet Error
  thresholdsArr->SetName("DoubleTmp");
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::GLOBALIDS, nullptr, true);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::PEDIGREEIDS, nullptr, true);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::VALUES, arrNames, true);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::INDICES, nullptr, true);
  thresholdsArr->SetName("Double");

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

  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::GLOBALIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::PEDIGREEIDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::VALUES, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::INDICES, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::THRESHOLDS, vtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, vtkSelectionNode::FRUSTUM, vtkSelectionNode::INDICES);
  // errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS,
  // vtkSelectionNode::GLOBALIDS); errors += TestConvertSelectionType(selMap, g,
  // vtkSelectionNode::LOCATIONS, vtkSelectionNode::PEDIGREEIDS); errors +=
  // TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS, vtkSelectionNode::VALUES,
  // arrNames); errors += TestConvertSelectionType(selMap, g, vtkSelectionNode::LOCATIONS,
  // vtkSelectionNode::INDICES);
}

int TestConvertSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;
  int size = 10;

  GraphConvertSelections(errors, size);
  PolyDataConvertSelections(errors, size);

  return errors;
}
