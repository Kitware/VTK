// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"

#include <sstream>

#include <iostream>

#define SIZE 1000

namespace
{
ostream& printStrings(ostream& os, const vtkStringArray* list)
{
  const vtkIdType len = list->GetNumberOfValues();

  for (vtkIdType i = 0; i < len; ++i)
  {
    os << "\t\tValue " << i << ": " << list->GetValue(i) << std::endl;
  }

  return os;
}

} // End anonymous namespace

int doStringArrayTest(ostream& strm, int size)
{
  int errors = 0;

  vtkNew<vtkStringArray> ptr;
  vtkStdString* strings = new vtkStdString[SIZE];
  for (int i = 0; i < SIZE; ++i)
  {
    strings[i] = "string entry " + vtk::to_string(i);
  }

  strm << "\tResize(0)...";
  ptr->Resize(0);
  strm << "OK" << std::endl;

  strm << "\tResize(10)...";
  ptr->Resize(10);
  strm << "OK" << std::endl;

  strm << "\tResize(5)...";
  ptr->Resize(5);
  strm << "OK" << std::endl;

  strm << "\tResize(size)...";
  ptr->Resize(size);
  strm << "OK" << std::endl;

  strm << "\tSetNumberOfValues...";
  ptr->SetNumberOfValues(100);
  if (ptr->GetNumberOfValues() == 100)
    strm << "OK" << std::endl;
  else
  {
    ++errors;
    strm << "FAILED" << std::endl;
  }

  strm << "\tSetVoidArray...";
  ptr->SetVoidArray(strings, size, 1);
  strm << "OK" << std::endl;

  strm << "\tGetValue...";
  vtkStdString value = ptr->GetValue(123);
  if (value == "string entry 123")
  {
    strm << "OK" << std::endl;
  }
  else
  {
    ++errors;
    strm << "FAILED.  Expected 'string entry 123', got '" << value << "'" << std::endl;
    bool dump = false;
#ifdef DUMP_VALUES
    dump = true;
#endif
    if (dump)
    {
      ::printStrings(strm, ptr);
    }
  }

  strm << "\tSetValue...";
  ptr->SetValue(124, "jabberwocky");
  if (ptr->GetValue(124) == "jabberwocky")
  {
    strm << "OK" << std::endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << std::endl;
  }

  strm << "\tInsertValue...";
  ptr->InsertValue(500, "There and Back Again");
  if (ptr->GetValue(500) == "There and Back Again")
  {
    strm << "OK" << std::endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << std::endl;
  }

  strm << "\tInsertNextValue...";
  if (ptr->GetValue(ptr->InsertNextValue("3.141592653589")) == "3.141592653589")
  {
    strm << "OK" << std::endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << std::endl;
  }

  strm << "\tvtkAbstractArray::GetTuples(vtkIdList)...";
  vtkNew<vtkIdList> indices;
  indices->InsertNextId(10);
  indices->InsertNextId(20);
  indices->InsertNextId(314);

  vtkNew<vtkStringArray> newValues;
  newValues->SetNumberOfValues(3);
  ptr->GetTuples(indices, newValues);

  if (newValues->GetValue(0) == "string entry 10" && newValues->GetValue(1) == "string entry 20" &&
    newValues->GetValue(2) == "string entry 314")
  {
    strm << "OK" << std::endl;
  }
  else
  {
    ++errors;
    strm << "FAILED.  Results:" << std::endl;
    strm << "\tExpected: 'string entry 10'\tActual: '" << newValues->GetValue(0) << "'"
         << std::endl;
    strm << "\tExpected: 'string entry 20'\tActual: '" << newValues->GetValue(1) << "'"
         << std::endl;
    strm << "\tExpected: 'string entry 314'\tActual: '" << newValues->GetValue(2) << "'"
         << std::endl;
  }

  newValues->Reset();

  strm << "\tvtkAbstractArray::GetTuples(vtkIdType, vtkIdType)...";
  newValues->SetNumberOfValues(3);
  ptr->GetTuples(30, 32, newValues);
  if (newValues->GetValue(0) == "string entry 30" && newValues->GetValue(1) == "string entry 31" &&
    newValues->GetValue(2) == "string entry 32")
  {
    strm << "OK" << std::endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << std::endl;
  }

  strm << "\tvtkAbstractArray::InsertTuple...";
  ptr->InsertTuple(150, 2, newValues);
  if (ptr->GetValue(150) == "string entry 32")
  {
    strm << "OK" << std::endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << std::endl;
  }

  strm << "PrintSelf..." << std::endl;
  strm << *ptr;

  delete[] strings;
  return errors;
}

int otherStringArrayTest(ostream& strm)
{
  int errors = 0;
  {
    strm << "Test StringArray" << std::endl;
    errors += doStringArrayTest(strm, SIZE);
  }

  return errors;
}

int otherStringArray(int, char*[])
{
  return otherStringArrayTest(std::cerr);
}
