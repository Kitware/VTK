/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTableModelAdapter.cxx

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
// Tests vtkQtTableModelAdapter.

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

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
    cerr << "ERROR: Wrong number of rows." << endl;
    ++errors;
  }
  if (adapter.columnCount(QModelIndex()) != 2)
  {
    cerr << "ERROR: Wrong number of columns." << endl;
    ++errors;
  }
  for (int i = 0; i < numRows; ++i)
  {
    QModelIndex ind = adapter.index(i, 0);
#if 0 // FIXME to work with new selection conversion routines
    QModelIndex pind = adapter.PedigreeToQModelIndex(i);
    if (ind != pind)
    {
      cerr << "ERROR: Pedigree lookup failed." << endl;
      ++errors;
    }
#endif
    if (adapter.rowCount(ind) != 0)
    {
      cerr << "ERROR: Row should have zero sub-rows." << endl;
      ++errors;
    }
    if (adapter.parent(ind) != QModelIndex())
    {
      cerr << "ERROR: Wrong parent." << endl;
      ++errors;
    }
  }
  return errors;
}


