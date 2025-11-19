// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
// Tests vtkQtTableModelAdapter.

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <iostream>

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestQtTableModelAdapter(int, char*[])
{
  int numRows = 10;
  int errors = 0;
  VTK_CREATE(vtkTable, table);
  VTK_CREATE(vtkIntArray, intArr);
  intArr->SetName("int");
  VTK_CREATE(vtkDoubleArray, doubleArr);
  doubleArr->SetName("double");
  for (int i = 0; i < numRows; ++i)
  {
    intArr->InsertNextValue(i);
    doubleArr->InsertNextValue(-i);
  }
  table->AddColumn(intArr);
  table->AddColumn(doubleArr);
  vtkQtTableModelAdapter adapter(table);
  if (adapter.rowCount(QModelIndex()) != numRows)
  {
    std::cerr << "ERROR: Wrong number of rows." << std::endl;
    ++errors;
  }
  if (adapter.columnCount(QModelIndex()) != 2)
  {
    std::cerr << "ERROR: Wrong number of columns." << std::endl;
    ++errors;
  }
  for (int i = 0; i < numRows; ++i)
  {
    QModelIndex ind = adapter.index(i, 0);
#if 0 // FIXME to work with new selection conversion routines
    QModelIndex pind = adapter.PedigreeToQModelIndex(i);
    if (ind != pind)
    {
      std::cerr << "ERROR: Pedigree lookup failed." << std::endl;
      ++errors;
    }
#endif
    if (adapter.rowCount(ind) != 0)
    {
      std::cerr << "ERROR: Row should have zero sub-rows." << std::endl;
      ++errors;
    }
    if (adapter.parent(ind) != QModelIndex())
    {
      std::cerr << "ERROR: Wrong parent." << std::endl;
      ++errors;
    }
  }
  return errors;
}
