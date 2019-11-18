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

#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkSplitColumnComponents.h"
#include "vtkTable.h"

#define GET_ARRAYS(arrays, out)                                                                    \
  for (int cc = 0; cc < 9; ++cc)                                                                   \
  {                                                                                                \
    arrays[cc] = vtkArrayDownCast<vtkIntArray>(out->GetColumn(cc));                                \
    if (arrays[cc] == nullptr)                                                                     \
    {                                                                                              \
      vtkGenericWarningMacro(<< cc << ": one of the output arrays was zero - type change?");       \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  }

int TestTableSplitColumnComponents(int, char*[])
{
  // Create a single column array, and a three component array
  vtkNew<vtkIntArray> single;
  single->SetNumberOfComponents(1);
  single->SetNumberOfTuples(5);
  single->SetName("Single");

  vtkNew<vtkIntArray> multi;
  multi->SetNumberOfComponents(3);
  multi->SetNumberOfTuples(5);
  multi->SetName("Multi");

  for (int i = 0; i < 5; ++i)
  {
    single->InsertValue(i, i);
    int ints[] = { i + 1, 2 * (i + 1), 3 * (i + 1) };
    multi->InsertTypedTuple(i, ints);
  }

  vtkNew<vtkIntArray> multinamed;
  multinamed->DeepCopy(multi);
  multinamed->SetName("Multinamed");
  multinamed->SetComponentName(0, "zero");
  multinamed->SetComponentName(1, "one");
  multinamed->SetComponentName(2, "two");

  vtkNew<vtkTable> table;
  table->AddColumn(single);
  table->AddColumn(multi);
  table->AddColumn(multinamed);

  // Merge the two tables
  vtkNew<vtkSplitColumnComponents> split;
  split->SetInputData(table);
  split->Update();

  vtkTable* out = split->GetOutput(0);
  if (out->GetNumberOfColumns() != 9) // 1 + (1+3) + (1+3)
  {
    vtkGenericWarningMacro(<< "Incorrect column count: " << out->GetNumberOfColumns());
    return EXIT_FAILURE;
  }
  vtkIntArray* arrays[9];
  GET_ARRAYS(arrays, out);

  for (int i = 0; i < 5; ++i)
  {
    if (arrays[0]->GetValue(i) != i || arrays[1]->GetValue(i) != i + 1 ||
      arrays[2]->GetValue(i) != 2 * (i + 1) || arrays[3]->GetValue(i) != 3 * (i + 1))
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
  GET_ARRAYS(arrays, out);
  if (strcmp(arrays[1]->GetName(), "Multi_0") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }
  if (strcmp(arrays[5]->GetName(), "Multinamed_0") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }

  split->SetNamingModeToNamesWithParens();
  split->Update();
  out = split->GetOutput(0);
  GET_ARRAYS(arrays, out);
  if (strcmp(arrays[1]->GetName(), "Multi (X)") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }
  if (strcmp(arrays[5]->GetName(), "Multinamed (zero)") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }

  split->SetNamingModeToNamesWithUnderscores();
  split->Update();
  out = split->GetOutput(0);
  GET_ARRAYS(arrays, out);
  if (strcmp(arrays[1]->GetName(), "Multi_X") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }
  if (strcmp(arrays[5]->GetName(), "Multinamed_zero") != 0)
  {
    vtkGenericWarningMacro("Incorrect name. NamingMode not being respected correctly.");
    return EXIT_FAILURE;
  }

  auto a1info = arrays[1]->GetInformation();
  if (!a1info->Has(vtkSplitColumnComponents::ORIGINAL_ARRAY_NAME()) ||
    strcmp(a1info->Get(vtkSplitColumnComponents::ORIGINAL_ARRAY_NAME()), "Multi") != 0 ||
    !a1info->Has(vtkSplitColumnComponents::ORIGINAL_COMPONENT_NUMBER()) ||
    a1info->Get(vtkSplitColumnComponents::ORIGINAL_COMPONENT_NUMBER()) != 0)
  {
    vtkGenericWarningMacro("Missing array information about original name and component!");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
