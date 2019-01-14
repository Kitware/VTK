/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArraySerialization.cxx

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

#include <vtkArrayReader.h>
#include <vtkArrayWriter.h>
#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
  { \
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
  } \
}

// This test ensures that we handle denormalized floating-point numbers gracefully,
// by truncating them to zero.  Otherwise, iostreams will refuse to load denormalized
// values (considering them out-of-range).

int TestArrayDenormalized(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
  {
    vtkSmartPointer<vtkDenseArray<double> > a1 = vtkSmartPointer<vtkDenseArray<double> >::New();
    a1->Resize(3);
    a1->SetValue(0, 1.0);
    a1->SetValue(1, 2.221997902944077e-314);
    a1->SetValue(2, 3.0);

    std::stringstream a_buffer;
    vtkArrayWriter::Write(a1, a_buffer);

    std::cerr << a_buffer.str() << std::endl;

    vtkSmartPointer<vtkArray> a2;
    a2.TakeReference(vtkArrayReader::Read(a_buffer));

    test_expression(a2);
    test_expression(vtkDenseArray<double>::SafeDownCast(a2));
    test_expression(a2->GetVariantValue(0).ToDouble() == 1.0);
    test_expression(a2->GetVariantValue(1).ToDouble() == 0.0);
    test_expression(a2->GetVariantValue(2).ToDouble() == 3.0);

    return 0;
  }
  catch(std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
