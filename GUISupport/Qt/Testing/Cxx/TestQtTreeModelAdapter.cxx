/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTreeModelAdapter.cxx

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
// Tests vtkQtTreeModelAdapter.

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkQtTreeModelAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestQtTreeModelAdapter(int, char*[])
{
  int numVerts = 6;
  int errors = 0;
  VTK_CREATE(vtkMutableDirectedGraph, builder);
  builder->AddVertex(); // 0
  builder->AddChild(0); // 1
  builder->AddChild(0); // 2
  builder->AddChild(0); // 3
  builder->AddChild(1); // 4
  builder->AddChild(1); // 5
  VTK_CREATE(vtkTree, tree);
  tree->ShallowCopy(builder);
  VTK_CREATE(vtkIntArray, intArr);
  intArr->SetName("int");
  VTK_CREATE(vtkDoubleArray, doubleArr);
  doubleArr->SetName("double");
  for (int i = 0; i < numVerts; ++i)
    {
    intArr->InsertNextValue(i);
    doubleArr->InsertNextValue(-i);
    }
  tree->GetVertexData()->AddArray(intArr);
  tree->GetVertexData()->AddArray(doubleArr);
  vtkQtTreeModelAdapter adapter(0, tree);
  if (adapter.rowCount(QModelIndex()) != 1)
    {
    cerr << "ERROR: Wrong number of rows." << endl;
    ++errors;
    }
  if (adapter.columnCount(QModelIndex()) != 2)
    {
    cerr << "ERROR: Wrong number of columns." << endl;
    ++errors;
    }

  QModelIndex ind0 = adapter.index(0, 0);
  QModelIndex ind1 = adapter.index(0, 0, ind0);
  for (int i = 0; i < numVerts; ++i)
    {
    QModelIndex ind;
    QModelIndex parent;
    int rows;
    if (i == 0)
      {
      ind = ind0;
      parent = QModelIndex();
      rows = 3;
      }
    else if (i == 1)
      {
      ind = ind1;
      parent = ind0;
      rows = 2;
      }
    else if (i < 4)
      {
      ind = adapter.index(i-1, 0, ind0);
      parent = ind0;
      rows = 0;
      }
    else
      {
      ind = adapter.index(i-4, 0, ind1);
      parent = ind1;
      rows = 0;
      }
#if 0 // FIXME to work with new selection conversion routines
    QModelIndex pind = adapter.PedigreeToQModelIndex(i);
    if (ind != pind)
      {
      cerr << "ERROR: Pedigree lookup failed." << endl;
      ++errors;
      }
#endif
    if (adapter.rowCount(ind) != rows)
      {
      cerr << "ERROR: Row should have zero sub-rows." << endl;
      ++errors;
      }
    if (adapter.parent(ind) != parent)
      {
      cerr << "ERROR: Wrong parent." << endl;
      ++errors;
      }
    }
  return errors;
}


