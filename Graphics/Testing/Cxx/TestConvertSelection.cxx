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

#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
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
  case vtkSelection::SELECTIONS:
  return "Selections";
  case vtkSelection::COMPOSITE_SELECTIONS:
  return "Composite Selections";
  case vtkSelection::GLOBALIDS:
  return "Global IDs";
  case vtkSelection::PEDIGREEIDS:
  return "Pedigree IDs";
  case vtkSelection::VALUES:
  return "Values";
  case vtkSelection::INDICES:
  return "Indices";
  case vtkSelection::FRUSTUM:
  return "Frustum";
  case vtkSelection::THRESHOLDS:
  return "Thresholds";
  case vtkSelection::LOCATIONS:
  return "Locations";
  default:
  return "Unknown";
  }
}

int CompareSelections(vtkSelection* a, vtkSelection* b)
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
  int errors = CompareSelections(selMap[outputType], s);
  s->Delete();
  cerr << "...done." << endl;
  return errors;
}

int TestConvertSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;
  int size = 10;
  
  // Create the test data
  VTK_CREATE(vtkGraph, g);
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
  globalIdsSelection->SetContentType(vtkSelection::GLOBALIDS);
  globalIdsSelection->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkIdTypeArray, globalIdsArr);
  globalIdsSelection->SetSelectionList(globalIdsArr);
  for (int i = 0; i < size; i += 2)
    {
    globalIdsArr->InsertNextValue(i);
    }
  selMap[vtkSelection::GLOBALIDS] = globalIdsSelection;
  
  VTK_CREATE(vtkSelection, pedigreeIdsSelection);
  pedigreeIdsSelection->SetContentType(vtkSelection::PEDIGREEIDS);
  pedigreeIdsSelection->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkIdTypeArray, pedigreeIdsArr);
  pedigreeIdsSelection->SetSelectionList(pedigreeIdsArr);
  for (int i = 0; i < size; i += 2)
    {
    pedigreeIdsArr->InsertNextValue(i);
    }
  selMap[vtkSelection::PEDIGREEIDS] = pedigreeIdsSelection;
  
  VTK_CREATE(vtkSelection, valuesSelection);
  valuesSelection->SetContentType(vtkSelection::VALUES);
  valuesSelection->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkStringArray, valuesArr);
  valuesArr->SetName("String");
  valuesSelection->SetSelectionList(valuesArr);
  for (int i = 0; i < size; i += 2)
    {
    valuesArr->InsertNextValue(vtkVariant(i).ToString());
    }
  selMap[vtkSelection::VALUES] = valuesSelection;
  
  VTK_CREATE(vtkSelection, indicesSelection);
  indicesSelection->SetContentType(vtkSelection::INDICES);
  indicesSelection->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkIdTypeArray, indicesArr);
  indicesSelection->SetSelectionList(indicesArr);
  for (int i = 0; i < size; i += 2)
    {
    indicesArr->InsertNextValue(i);
    }
  selMap[vtkSelection::INDICES] = indicesSelection;
  
  VTK_CREATE(vtkSelection, frustumSelection);
  frustumSelection->SetContentType(vtkSelection::FRUSTUM);
  frustumSelection->SetFieldType(vtkSelection::POINT);
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
  frustumSelection->SetSelectionList(frustumArr);
  selMap[vtkSelection::FRUSTUM] = frustumSelection;
  
  VTK_CREATE(vtkSelection, locationsSelection);
  locationsSelection->SetContentType(vtkSelection::INDICES);
  locationsSelection->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkFloatArray, locationsArr);
  locationsArr->SetNumberOfComponents(3);
  locationsSelection->SetSelectionList(locationsArr);
  for (int i = 0; i < size; i += 2)
    {
    locationsArr->InsertNextTuple3(i, 0, 0);
    }
  selMap[vtkSelection::LOCATIONS] = locationsSelection;
  
  VTK_CREATE(vtkSelection, thresholdsSelection);
  thresholdsSelection->SetContentType(vtkSelection::THRESHOLDS);
  thresholdsSelection->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkDoubleArray, thresholdsArr);
  thresholdsArr->SetName("Double");
  thresholdsArr->InsertNextValue(-0.5);
  thresholdsArr->InsertNextValue(0.5);
  thresholdsSelection->SetSelectionList(thresholdsArr);
  selMap[vtkSelection::THRESHOLDS] = thresholdsSelection;
  
  VTK_CREATE(vtkStringArray, arrNames);
  arrNames->InsertNextValue("String");

  errors += TestConvertSelectionType(selMap, g, vtkSelection::GLOBALIDS, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::GLOBALIDS, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::GLOBALIDS, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::PEDIGREEIDS, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::PEDIGREEIDS, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::PEDIGREEIDS, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::VALUES, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::VALUES, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::VALUES, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::INDICES, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::INDICES, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::INDICES, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::INDICES);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::GLOBALIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::PEDIGREEIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::VALUES, arrNames);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::INDICES);
  
  //
  // Test cell selections
  //
  
  selMap[vtkSelection::GLOBALIDS]->SetFieldType(vtkSelection::CELL);
  selMap[vtkSelection::PEDIGREEIDS]->SetFieldType(vtkSelection::CELL);
  selMap[vtkSelection::VALUES]->SetFieldType(vtkSelection::CELL);
  selMap[vtkSelection::INDICES]->SetFieldType(vtkSelection::CELL);
  selMap[vtkSelection::THRESHOLDS]->SetFieldType(vtkSelection::CELL);
  selMap[vtkSelection::FRUSTUM]->SetFieldType(vtkSelection::CELL);
  selMap[vtkSelection::LOCATIONS]->SetFieldType(vtkSelection::CELL);
  
  errors += TestConvertSelectionType(selMap, g, vtkSelection::GLOBALIDS, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::GLOBALIDS, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::GLOBALIDS, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::PEDIGREEIDS, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::PEDIGREEIDS, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::PEDIGREEIDS, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::VALUES, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::VALUES, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::VALUES, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::INDICES, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::INDICES, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::INDICES, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::THRESHOLDS, vtkSelection::INDICES);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::GLOBALIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::PEDIGREEIDS);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::VALUES, arrNames);
  errors += TestConvertSelectionType(selMap, g, vtkSelection::FRUSTUM, vtkSelection::INDICES);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::GLOBALIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::PEDIGREEIDS);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::VALUES, arrNames);
  //errors += TestConvertSelectionType(selMap, g, vtkSelection::LOCATIONS, vtkSelection::INDICES);
  
  return errors;
}
