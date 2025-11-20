// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataArraySelection.h"
#include "vtkNew.h"

#include <iostream>

#define TASSERT(x)                                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      std::cerr << "ERROR: failed at " << __LINE__ << "!" << std::endl; /*return EXIT_FAILURE;*/   \
    }                                                                                              \
  } while (false)

int TestDataArraySelection(int, char*[])
{
  vtkNew<vtkDataArraySelection> sel;
  sel->EnableArray("Temperature");
  sel->EnableArray("Pressure");
  sel->DisableArray("Pressure");
  sel->Print(std::cout);

  TASSERT(sel->ArrayExists("Temperature") && sel->ArrayIsEnabled("Temperature"));
  TASSERT(!sel->ArrayExists("Temperature2") && !sel->ArrayIsEnabled("Temperature2"));
  TASSERT(sel->ArrayExists("Pressure") && !sel->ArrayIsEnabled("Pressure"));

  vtkNew<vtkDataArraySelection> sel2;
  sel2->EnableArray("Pressure");
  sel2->EnableArray("Voltage");
  sel2->Union(sel);
  sel2->Print(std::cout);

  TASSERT(sel2->ArrayExists("Temperature") && sel2->ArrayIsEnabled("Temperature"));
  TASSERT(!sel2->ArrayExists("Temperature2") && !sel2->ArrayIsEnabled("Temperature2"));
  TASSERT(sel2->ArrayExists("Pressure") && sel2->ArrayIsEnabled("Pressure"));
  TASSERT(sel2->ArrayExists("Voltage") && sel2->ArrayIsEnabled("Voltage"));
  return EXIT_SUCCESS;
}
