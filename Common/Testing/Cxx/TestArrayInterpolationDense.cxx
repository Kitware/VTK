/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayInterpolationDense.cxx

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

#include <vtkArrayInterpolate.h>
#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

void test_expression(const bool expression, const vtkstd::string& message)
{
  if(!expression)
    throw vtkstd::runtime_error(message);
}

int TestArrayInterpolationDense(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDenseArray<double> > a = vtkSmartPointer<vtkDenseArray<double> >::New();
    a->Resize(4);
    a->SetValue(0, 0);
    a->SetValue(1, 1);
    a->SetValue(2, 2);
    a->SetValue(3, 3);

    vtkSmartPointer<vtkDenseArray<double> > b = vtkSmartPointer<vtkDenseArray<double> >::New();
    b->Resize(vtkArrayExtents(2));

    vtkInterpolate(a.GetPointer(), vtkArrayExtentsList(vtkArrayExtents(vtkArrayRange(0, 1)), vtkArrayExtents(vtkArrayRange(1, 2))), vtkArrayWeights(0.5, 0.5), vtkArrayExtents(vtkArrayRange(0, 1)), b.GetPointer());
    vtkInterpolate(a.GetPointer(), vtkArrayExtentsList(vtkArrayExtents(vtkArrayRange(2, 3)), vtkArrayExtents(vtkArrayRange(3, 4))), vtkArrayWeights(0.5, 0.5), vtkArrayExtents(vtkArrayRange(1, 2)), b.GetPointer());

    test_expression(b->GetValue(0) == 0.5, "expected 0.5");
    test_expression(b->GetValue(1) == 2.5, "expected 2.5");

    vtkSmartPointer<vtkDenseArray<double> > c = vtkSmartPointer<vtkDenseArray<double> >::New();
    c->Resize(4, 2);
    c->SetValue(0, 0, 0);
    c->SetValue(0, 1, 1);
    c->SetValue(1, 0, 2);
    c->SetValue(1, 1, 3);
    c->SetValue(2, 0, 4);
    c->SetValue(2, 1, 5);
    c->SetValue(3, 0, 6);
    c->SetValue(3, 1, 7);

    vtkSmartPointer<vtkDenseArray<double> > d = vtkSmartPointer<vtkDenseArray<double> >::New();
    d->Resize(vtkArrayExtents(2, 2));

    vtkInterpolate(c.GetPointer(), vtkArrayExtentsList(vtkArrayExtents(vtkArrayRange(0, 1), vtkArrayRange(0, 2)), vtkArrayExtents(vtkArrayRange(1, 2), vtkArrayRange(0, 2))), vtkArrayWeights(0.5, 0.5), vtkArrayExtents(vtkArrayRange(0, 1), vtkArrayRange(0, 2)), d.GetPointer());
    vtkInterpolate(c.GetPointer(), vtkArrayExtentsList(vtkArrayExtents(vtkArrayRange(2, 3), vtkArrayRange(0, 2)), vtkArrayExtents(vtkArrayRange(3, 4), vtkArrayRange(0, 2))), vtkArrayWeights(0.5, 0.5), vtkArrayExtents(vtkArrayRange(1, 2), vtkArrayRange(0, 2)), d.GetPointer());

    test_expression(d->GetValue(0, 0) == 1, "expected 1");
    test_expression(d->GetValue(0, 1) == 2, "expected 2");
    test_expression(d->GetValue(1, 0) == 5, "expected 5");
    test_expression(d->GetValue(1, 1) == 6, "expected 6");

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}
