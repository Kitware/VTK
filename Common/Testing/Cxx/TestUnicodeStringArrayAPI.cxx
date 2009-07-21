/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUnicodeStringArrayAPI.cxx
  
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

#include <vtkSmartPointer.h>
#include <vtkUnicodeStringArray.h>

#include <vtksys/stl/iterator>
#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

// Sample strings - nothing risque, I hope ...
static const vtkUnicodeString sample_utf8_ascii = vtkUnicodeString::from_utf8("abcde123");
static const vtkUnicodeString sample_utf8_greek = vtkUnicodeString::from_utf8("\xce\xb1\xce\xb2\xce\xb3"); // Greek lower-case alpha, beta, gamma.
static const vtkUnicodeString sample_utf8_thai = vtkUnicodeString::from_utf8("\xe0\xb8\x81\xe0\xb8\x82\xe0\xb8\x83"); // Thai ko kai, kho khai, kho khuat.
static const vtkUnicodeString sample_utf8_linear_b = vtkUnicodeString::from_utf8("\xf0\x90\x80\x80\xf0\x90\x80\x81\xf0\x90\x80\x82\xf0\x90\x80\x83\xf0\x90\x80\x84"); // Linear-B syllables a, e, i, o, u.
static const vtkUnicodeString sample_utf8_mixed = vtkUnicodeString::from_utf8("a\xce\xb1\xe0\xb8\x81\xf0\x90\x80\x80"); // a, alpha, ko kai, syllable-a.

int TestUnicodeStringArrayAPI(int, char*[])
{
  try
    {
    vtkSmartPointer<vtkUnicodeStringArray> array = vtkSmartPointer<vtkUnicodeStringArray>::New();
    test_expression(array->GetNumberOfTuples() == 0);
    test_expression(array->GetDataType() == VTK_UNICODE_STRING);
    test_expression(array->GetDataTypeSize() == 0);
    test_expression(array->GetElementComponentSize() == 4);
    test_expression(array->IsNumeric() == 0);

    array->InsertNextValue(sample_utf8_ascii);
    test_expression(array->GetNumberOfTuples() == 1);
    test_expression((array->GetValue(0)) == sample_utf8_ascii);

    array->InsertNextValue(vtkUnicodeString::from_utf8("foo"));
    test_expression(array->GetNumberOfTuples() == 2);
    test_expression(array->LookupValue(vtkUnicodeString::from_utf8("foo")) == 1);
    test_expression(array->LookupValue(vtkUnicodeString::from_utf8("bar")) == -1);
   
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

