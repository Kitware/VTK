/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPassArrays.cxx

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
#include "vtkPassArrays.h"

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

int TestPassArrays(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  VTK_CREATE(vtkPassArrays, pass);

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
  pass->SetInputData(pd);

  for (int type = 0; type < 3; type++)
    {
    for (int removeArrays = 0; removeArrays <= 1; removeArrays++)
      {
      for (int useFieldTypes = 0; useFieldTypes <= 1; useFieldTypes++)
        {
        cerr << "Passing a column from field type " << type << " ..." << endl;
        pass->ClearArrays();
        pass->AddArray(type, "column1");

        cerr << "RemoveArrays flag is " << removeArrays << endl;
        pass->SetRemoveArrays(removeArrays > 0 ? true : false);

        cerr << "UseFieldTypes flag is " << useFieldTypes << endl;
        pass->SetUseFieldTypes(useFieldTypes > 0 ? true : false);
        pass->ClearFieldTypes();
        int processType = (type+1)%3;
        cerr << "FieldType is " << processType << endl;
        pass->AddFieldType(processType);

        pass->Update();
        vtkPolyData* out = vtkPolyData::SafeDownCast(pass->GetOutput());
        cerr << "... done" << endl;

        cerr << "Checking output ..." << endl;
        vtkFieldData* outAttrib = out->GetAttributesAsFieldData(type);
        vtkIntArray* out1 = vtkIntArray::SafeDownCast(outAttrib->GetAbstractArray("column1"));
        vtkIntArray* out2 = vtkIntArray::SafeDownCast(outAttrib->GetAbstractArray("column2"));
        if (useFieldTypes)
          {
          if (!out1 || !out2)
            {
            ++errors;
            cerr << "ERROR: Output arrays should not have been touched" << endl;
            }
          if (!removeArrays && out->GetAttributesAsFieldData(processType)->GetNumberOfArrays() != 0)
            {
            ++errors;
            cerr << "ERROR: Processed field data should have been cleared" << endl;
            }
          if (removeArrays && out->GetAttributesAsFieldData(processType)->GetNumberOfArrays() != 2)
            {
            ++errors;
            cerr << "ERROR: Processed field data should remain the same" << endl;
            }
          }
        else
          {
          if (removeArrays && out1)
            {
            ++errors;
            cerr << "ERROR: Array output1 should have been removed but it wasn't" << endl;
            }
          if (!removeArrays && !out1)
            {
            ++errors;
            cerr << "ERROR: Array output1 should have been passed but it wasn't" << endl;
            }
          if (removeArrays && !out2)
            {
            ++errors;
            cerr << "ERROR: Array output2 should have been passed but it wasn't" << endl;
            }
          if (!removeArrays && out2)
            {
            ++errors;
            cerr << "ERROR: Array output2 should have been removed but it wasn't" << endl;
            }
          }
        for (vtkIdType j = 0; j < 10; j++)
          {
          if (out1 && out1->GetValue(j) != col1->GetValue(j))
            {
            errors++;
            cerr << "ERROR: column1 output does not match input "
                 << out1->GetValue(j) << "!=" << col1->GetValue(j)
                 << " for field type " << type << endl;
            break;
            }
          if (out2 && out2->GetValue(j) != col2->GetValue(j))
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
      }
    }

  cerr << errors << " errors" << endl;
  return errors;
}

