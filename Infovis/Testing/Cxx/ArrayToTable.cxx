/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayToTable.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkAbstractArray.h>
#include <vtkArrayData.h>
#include <vtkArrayToTable.h>
#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>
#include <vtkTable.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw vtkstd::runtime_error("Expression failed: " #expression); \
}

int ArrayToTable(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDenseArray<vtkStdString> > a = vtkSmartPointer<vtkDenseArray<vtkStdString> >::New();
    a->Resize(2);
    a->SetValue(0, "Howdy");
    a->SetValue(1, "World!");

    vtkSmartPointer<vtkArrayData> b = vtkSmartPointer<vtkArrayData>::New();
    b->AddArray(a);

    vtkSmartPointer<vtkArrayToTable> c = vtkSmartPointer<vtkArrayToTable>::New();
    c->SetInputConnection(0, b->GetProducerPort());
    c->Update();

    test_expression(c->GetOutput()->GetNumberOfColumns() == 1);
    test_expression(c->GetOutput()->GetNumberOfRows() == 2);
    test_expression(vtkStdString(c->GetOutput()->GetColumn(0)->GetName()) == "");
    test_expression(c->GetOutput()->GetValue(0, 0).ToString() == "Howdy");
    test_expression(c->GetOutput()->GetValue(1, 0).ToString() == "World!");

    vtkSmartPointer<vtkSparseArray<double> > d = vtkSmartPointer<vtkSparseArray<double> >::New();
    d->Resize(2, 2);
    d->SetValue(0, 0, 1.0);
    d->SetValue(1, 1, 2.0);

    vtkSmartPointer<vtkArrayData> e = vtkSmartPointer<vtkArrayData>::New();
    e->AddArray(d);

    vtkSmartPointer<vtkArrayToTable> f = vtkSmartPointer<vtkArrayToTable>::New();
    f->SetInputConnection(0, e->GetProducerPort());
    f->Update();

    test_expression(f->GetOutput()->GetNumberOfColumns() == 2);
    test_expression(f->GetOutput()->GetNumberOfRows() == 2);
    test_expression(vtkStdString(f->GetOutput()->GetColumn(0)->GetName()) == "0");
    test_expression(vtkStdString(f->GetOutput()->GetColumn(1)->GetName()) == "1");
    test_expression(f->GetOutput()->GetValue(0, 0).ToDouble() == 1.0);
    test_expression(f->GetOutput()->GetValue(0, 1).ToDouble() == 0.0);
    test_expression(f->GetOutput()->GetValue(1, 0).ToDouble() == 0.0);
    test_expression(f->GetOutput()->GetValue(1, 1).ToDouble() == 2.0);

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

