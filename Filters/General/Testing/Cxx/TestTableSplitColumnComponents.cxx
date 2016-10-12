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
    multi->InsertTypedTuple(i, ints);
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
    return EXIT_FAILURE;
  }
  vtkIntArray* arrays[4];
  arrays[0] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(0));
  arrays[1] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(1));
  arrays[2] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(2));
  arrays[3] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(3));
  if (arrays[0] == 0 || arrays[1] == 0 || arrays[2] == 0 || arrays[3] == 0)
  {
    vtkGenericWarningMacro(<< "One of the output arrays was zero - type change?");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < 5; ++i)
  {
    if (arrays[0]->GetValue(i) != i || arrays[1]->GetValue(i) != i+1 ||
        arrays[2]->GetValue(i) != 2*(i+1) || arrays[3]->GetValue(i) != 3*(i+1))
    {
      vtkGenericWarningMacro(<< "One of the output arrays values did not match.");
      table->Dump();
      out->Dump();
      return EXIT_FAILURE;
    }
  }

  // Test naming modes.
  if (strcmp(arrays[1]->GetName(), "Multi (0)") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }

  split->SetNamingModeToNumberWithUnderscores();
  split->Update();
  out = split->GetOutput(0);
  arrays[1] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(1));
  if (strcmp(arrays[1]->GetName(), "Multi_0") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }

  split->SetNamingModeToNamesWithParens();
  split->Update();
  out = split->GetOutput(0);
  arrays[1] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(1));
  if (strcmp(arrays[1]->GetName(), "Multi (X)") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }

  split->SetNamingModeToNamesWithUnderscores();
  split->Update();
  out = split->GetOutput(0);
  arrays[1] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(1));
  if (strcmp(arrays[1]->GetName(), "Multi_X") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
