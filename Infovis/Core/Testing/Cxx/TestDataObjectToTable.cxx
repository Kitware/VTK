/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataObjectToTable.cxx

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
#include "vtkDataObjectToTable.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTable.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestDataObjectToTable(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  VTK_CREATE(vtkDataObjectToTable, toTable);

  cerr << "Creating a simple polydata ..." << endl;
  VTK_CREATE(vtkPolyData, pd);
  VTK_CREATE(vtkIntArray, col1);
  col1->SetName("column1");
  VTK_CREATE(vtkIntArray, col2);
  col2->SetName("column2");
  VTK_CREATE(vtkCellArray, cells);
  VTK_CREATE(vtkPoints, pts);
  for (vtkIdType i = 0; i < 10; i++)
    {
    col1->InsertNextValue(i);
    col2->InsertNextValue(-i);
    pts->InsertNextPoint(0, 0, 0);
    cells->InsertNextCell(1, &i);
    }
  pd->SetPoints(pts);
  pd->SetVerts(cells);
  vtkCellData* cellData = pd->GetCellData();
  cellData->AddArray(col1);
  cellData->AddArray(col2);
  vtkPointData* pointData = pd->GetPointData();
  pointData->AddArray(col1);
  pointData->AddArray(col2);
  vtkFieldData* fieldData = pd->GetFieldData();
  fieldData->AddArray(col1);
  fieldData->AddArray(col2);
  cerr << "... done" << endl;

  int errors = 0;
  toTable->SetInputData(pd);
  for (int type = 0; type < 3; type++)
    {
    cerr << "Converting ";
    switch (type)
    {
    case 0:
      cerr << "field data";
      break;
    case 1:
      cerr << "point data";
      break;
    case 2:
      cerr << "cell data";
      break;
    };
    cerr << " to a table ..." << endl;
    toTable->SetFieldType(type);
    toTable->Update();
    vtkTable* table = toTable->GetOutput();
    cerr << "... done" << endl;

    cerr << "Checking table ..." << endl;
    // Check the table
    vtkIntArray* out1 = vtkIntArray::SafeDownCast(table->GetColumnByName("column1"));
    if (!out1)
      {
      errors++;
      cerr << "ERROR: column1 not found when extracting field type " << type << endl;
      }
    vtkIntArray* out2 = vtkIntArray::SafeDownCast(table->GetColumnByName("column2"));
    if (!out2)
      {
      errors++;
      cerr << "ERROR: column1 not found when extracting field type " << type << endl;
      }
    for (vtkIdType j = 0; j < 10; j++)
      {
      if (out1->GetValue(j) != col1->GetValue(j))
        {
        errors++;
        cerr << "ERROR: column1 output does not match input "
             << out1->GetValue(j) << "!=" << col1->GetValue(j)
             << " for field type " << type << endl;
        break;
        }
      if (out2->GetValue(j) != col2->GetValue(j))
        {
        errors++;
        cerr << "ERROR: column2 output does not match input "
             << out2->GetValue(j) << "!=" << col2->GetValue(j)
             << " for field type " << type << endl;
        break;
        }
      }
    cerr << "... done" << endl;
    }

  return errors;
}

