/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTableSplitColumnComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkIntArray.h"
#include "vtkSplitColumnComponents.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestTableSplitColumnComponents(int, char*[])
{
  // Create a single column array, and a three component array
  VTK_CREATE(vtkIntArray, single);
  single->SetNumberOfComponents(1);
  single->SetNumberOfTuples(5);
  single->SetName("Single");

  VTK_CREATE(vtkIntArray, multi);
  multi->SetNumberOfComponents(3);
  multi->SetNumberOfTuples(5);
  multi->SetName("Multi");

  for (int i = 0; i < 5; ++i)
    {
    single->InsertValue(i, i);
    int ints[] = { i+1, 2 * (i+1), 3 * (i+1) };
    multi->InsertTupleValue(i, ints);
    }

  VTK_CREATE(vtkTable, table);
  table->AddColumn(single);
  table->AddColumn(multi);

  // Merge the two tables
  VTK_CREATE(vtkSplitColumnComponents, split);
  split->SetInputData(table);
  split->Update();

  vtkTable* out = split->GetOutput(0);
  if (out->GetNumberOfColumns() != 5)
    {
    vtkGenericWarningMacro(<< "Incorrect column count: "
                           << out->GetNumberOfColumns());
    return 1;
    }
  vtkIntArray* arrays[4];
  arrays[0] = vtkIntArray::SafeDownCast(out->GetColumn(0));
  arrays[1] = vtkIntArray::SafeDownCast(out->GetColumn(1));
  arrays[2] = vtkIntArray::SafeDownCast(out->GetColumn(2));
  arrays[3] = vtkIntArray::SafeDownCast(out->GetColumn(3));
  if (arrays[0] == 0 || arrays[1] == 0 || arrays[2] == 0 || arrays[3] == 0)
    {
    vtkGenericWarningMacro(<< "One of the output arrays was zero - type change?");
    return 1;
    }

  for (int i = 0; i < 5; ++i)
    {
    if (arrays[0]->GetValue(i) != i || arrays[1]->GetValue(i) != i+1 ||
        arrays[2]->GetValue(i) != 2*(i+1) || arrays[3]->GetValue(i) != 3*(i+1))
      {
      vtkGenericWarningMacro(<< "One of the output arrays values did not match.");
      table->Dump();
      out->Dump();
      return 1;
      }
    }

  return 0;
}
