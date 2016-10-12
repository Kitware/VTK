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
#include <vtkIdList.h>
#include <vtkDoubleArray.h>
#include <vtkVariant.h>
#include <vtkTestErrorObserver.h>

#include <iterator>
#include <iostream>
#include <sstream>
#include <stdexcept>

static int TestErrorsAndWarnings();

#define test_expression(expression) \
{ \
  if(!(expression)) \
  { \
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
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
    vtkSmartPointer<vtkUnicodeStringArray> array =
      vtkSmartPointer<vtkUnicodeStringArray>::New();
    array->ClearLookup(); // noop
    test_expression(array->GetNumberOfTuples() == 0);
    test_expression(array->GetDataType() == VTK_UNICODE_STRING);
    test_expression(array->GetDataTypeSize() == 0);
    test_expression(array->GetElementComponentSize() == 4);
    test_expression(array->IsNumeric() == 0);

    array->InsertNextValue(sample_utf8_ascii);
    test_expression(array->GetNumberOfTuples() == 1);
    test_expression((array->GetValue(0)) == sample_utf8_ascii);
    test_expression((array->GetVariantValue(0)) == sample_utf8_ascii);

    array->InsertNextValue(vtkUnicodeString::from_utf8("foo"));
    test_expression(array->GetNumberOfTuples() == 2);
    test_expression(array->LookupValue(vtkUnicodeString::from_utf8("foo")) == 1);
    test_expression(array->LookupValue(vtkUnicodeString::from_utf8("bar")) == -1);

    vtkSmartPointer<vtkUnicodeStringArray> array2 =
      vtkSmartPointer<vtkUnicodeStringArray>::New();
    array2->SetNumberOfTuples(3);
    array2->SetValue(2, sample_utf8_thai);
    array2->SetValue(1, sample_utf8_greek);
    array2->SetValue(0, sample_utf8_linear_b);
    test_expression(array2->GetNumberOfTuples() == 3);

    array2->InsertNextUTF8Value("bar");
    test_expression(array2->GetNumberOfTuples() == 4);

    array2->InsertValue(100, sample_utf8_ascii);
    test_expression(array2->GetNumberOfTuples() == 101);

    array2->SetVariantValue(100, "foo");
    test_expression(array2->GetValue(100) == vtkUnicodeString::from_utf8("foo"));

    array2->SetUTF8Value(100, "barfoo");
    test_expression(strcmp(array2->GetUTF8Value(100), "barfoo") == 0);

    array2->Initialize();
    test_expression(array2->GetNumberOfTuples() ==  0);

    vtkSmartPointer<vtkUnicodeStringArray> array3 =
      vtkSmartPointer<vtkUnicodeStringArray>::New();
    void * ptr1 = array3->GetVoidPointer(0);
    test_expression(ptr1 == NULL);

    array3->InsertTuple(0, 1, array);
    test_expression(array3->GetValue(0) == array->GetValue(1));

    array3->InsertTuple(100, 1, array);
    test_expression(array3->GetValue(100) == array->GetValue(1));

    array3->InsertNextTuple(1, array);
    test_expression(array3->GetValue(101) == array->GetValue(1));

    array3->SetTuple(0, 0, array);
    test_expression(array3->GetValue(0) == array->GetValue(0));

    vtkSmartPointer<vtkIdList> toIds =
      vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkIdList> fromIds =
      vtkSmartPointer<vtkIdList>::New();
    fromIds->InsertId(0, 1);
    fromIds->InsertId(1, 0);
    toIds->InsertId(0, array3->GetNumberOfTuples() + 1);
    toIds->InsertId(1, 1);

    array3->InsertTuples(toIds, fromIds, array);
    test_expression(array3->GetValue(array3->GetNumberOfTuples() - 1) == array->GetValue(1));
    test_expression(array3->GetValue(1) == array->GetValue(0));

    array3->InsertNextValue(vtkUnicodeString::from_utf8("foobar"));
    array3->InsertNextValue(vtkUnicodeString::from_utf8("foobar"));
    array3->InsertNextValue(vtkUnicodeString::from_utf8("foobar"));
    vtkSmartPointer<vtkIdList> lookupIds =
      vtkSmartPointer<vtkIdList>::New();
    array3->LookupValue(vtkUnicodeString::from_utf8("foobar"), lookupIds);
    test_expression(lookupIds->GetNumberOfIds() == 3);

    array3->DeepCopy(NULL); // noop
    array3->DeepCopy(array3); // noop
    array3->DeepCopy(array);
    test_expression(array3->GetActualMemorySize() == array->GetActualMemorySize());

    vtkSmartPointer<vtkUnicodeStringArray> array4 =
      vtkSmartPointer<vtkUnicodeStringArray>::New();
    array4->InsertNextValue(vtkUnicodeString::from_utf8("array4_0"));
    array4->InsertNextValue(vtkUnicodeString::from_utf8("array4_1"));
    array4->InsertNextValue(vtkUnicodeString::from_utf8("array4_2"));

    vtkSmartPointer<vtkUnicodeStringArray> array5 =
      vtkSmartPointer<vtkUnicodeStringArray>::New();
    array5->InsertNextValue(vtkUnicodeString::from_utf8("array5_0"));
    array5->InsertNextValue(vtkUnicodeString::from_utf8("array5_1"));
    array5->InsertNextValue(vtkUnicodeString::from_utf8("array5_2"));
    array5->InsertNextValue(vtkUnicodeString::from_utf8("array5_3"));

    vtkSmartPointer<vtkIdList> interpIds =
      vtkSmartPointer<vtkIdList>::New();

    array3->InterpolateTuple(5, interpIds, array4, NULL); // noop

    interpIds->InsertId(0, 0);
    interpIds->InsertId(1, 1);
    interpIds->InsertId(2, 2);
    double weights[3];
    weights[0] = .2;
    weights[1] = .8;
    weights[2] = .5;
    array3->InterpolateTuple(5, interpIds, array4, weights);
    test_expression(array3->GetValue(5) == array4->GetValue(1));

    array3->InterpolateTuple(0,
                             0, array4,
                             0, array5,
                             0.1);
    test_expression(array3->GetValue(0) == array4->GetValue(0));

    array3->InterpolateTuple(1,
                             0, array4,
                             0, array5,
                             0.6);
    test_expression(array3->GetValue(1) == array5->GetValue(0));

    array3->Squeeze();
    test_expression(array3->GetValue(5) == array4->GetValue(1));

    array3->Resize(20);
    test_expression(array3->GetValue(5) == array4->GetValue(1));

    array3->GetVoidPointer(0);

    if (TestErrorsAndWarnings() != 0)
    {
      return EXIT_FAILURE;
    }
    array3->Print(std::cout);

    return EXIT_SUCCESS;
  }
  catch(std::exception& e)
  {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
}

int TestErrorsAndWarnings()
{
  int status = 0;
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkUnicodeStringArray> array =
    vtkSmartPointer<vtkUnicodeStringArray>::New();
  array->Allocate(100, 0);
  array->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  array->AddObserver(vtkCommand::WarningEvent, errorObserver);

  // ERROR: Not implmented
  array->SetVoidArray(0, 1, 1);
  if (errorObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << errorObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Not implemented' error" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // ERROR: Not implmented
  array->NewIterator();
  if (errorObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << errorObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Not implemented' error" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // Warning: Input and output array data types do not match.
  vtkSmartPointer<vtkDoubleArray> doubleArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  array->SetTuple(0, 0, doubleArray);
  if (errorObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Input and output array data types do not match.' warning" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // Warning: Input and output array data types do not match.
  array->InsertTuple(0, 0, doubleArray);
  if (errorObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Input and output array data types do not match.' warning" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // Warning: Input and output array data types do not match.
  array->InsertNextTuple(0, doubleArray);
  if (errorObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Input and output array data types do not match.' warning" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // Warning: Input and output array data types do not match.
  array->DeepCopy(doubleArray);
  if (errorObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Input and output array data types do not match.' warning" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // Warning: Input and output array data types do not match.
  vtkSmartPointer<vtkIdList> id1 =
    vtkSmartPointer<vtkIdList>::New();
  array->InsertTuples(id1, id1, doubleArray);
  if (errorObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Input and output array data types do not match.' warning" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // Warning: Input and output id array sizes do not match.
  vtkSmartPointer<vtkIdList> id2 =
    vtkSmartPointer<vtkIdList>::New();
  id1->SetNumberOfIds(10);
  id2->SetNumberOfIds(5);
  array->InsertTuples(id1, id2, array);
  if (errorObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Input and output id array sizes do not match.' warning" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // ERROR: Cannot CopyValue from array of type
  array->InterpolateTuple(0, id1, doubleArray, NULL);
  if (errorObserver->GetError())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Cannot CopyValue from array of type' error" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  // ERROR: All arrays to InterpolateValue() must be of same type.
  array->InterpolateTuple(0,
                          0, doubleArray,
                          2, array,
                          0.0);
  if (errorObserver->GetError())
  {
    std::cout << "Caught expected warning: "
              << errorObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'All arrays to InterpolateValue() must be of same type.' error" << std::endl;
    ++status;
  }
  errorObserver->Clear();

  return status;
}
