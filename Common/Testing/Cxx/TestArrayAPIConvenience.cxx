/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayAPIConvenience.cxx

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

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

int TestArrayAPIConvenience(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDenseArray<double> > a = vtkSmartPointer<vtkDenseArray<double> >::New();
    vtkSmartPointer<vtkDenseArray<double> > b = vtkSmartPointer<vtkDenseArray<double> >::New();

    a->Resize(5);
    b->Resize(vtkArrayExtents(5));
    test_expression(a->GetExtents() == b->GetExtents());

    a->SetValue(2, 3);
    b->SetValue(vtkArrayCoordinates(2), 3);
    test_expression(a->GetValue(2) == b->GetValue(vtkArrayCoordinates(2)));

    a->Resize(5, 6);
    b->Resize(vtkArrayExtents(5, 6));
    test_expression(a->GetExtents() == b->GetExtents());

    a->SetValue(2, 3, 4);
    b->SetValue(vtkArrayCoordinates(2, 3), 4);
    test_expression(a->GetValue(2, 3) == b->GetValue(vtkArrayCoordinates(2, 3)));

    a->Resize(5, 6, 7);
    b->Resize(vtkArrayExtents(5, 6, 7));
    test_expression(a->GetExtents() == b->GetExtents());

    a->SetValue(2, 3, 4, 5);
    b->SetValue(vtkArrayCoordinates(2, 3, 4), 5);
    test_expression(a->GetValue(2, 3, 4) == b->GetValue(vtkArrayCoordinates(2, 3, 4)));

    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}
