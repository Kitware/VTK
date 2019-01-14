/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherStringArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDebugLeaks.h"
#include "vtkCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkStringArray.h"
#include "vtkIdList.h"

#include <sstream>

#define SIZE 1000

int doStringArrayTest(ostream& strm, int size)
{
  int errors = 0;

  vtkStringArray *ptr = vtkStringArray::New();
  vtkStdString *strings = new vtkStdString[SIZE];
  for (int i = 0; i < SIZE; ++i)
  {
    char buf[1024];
    snprintf(buf, sizeof(buf), "string entry %d", i);
    strings[i] = vtkStdString(buf);
  }

  strm << "\tResize(0)...";
  ptr->Resize(0);
  strm << "OK" << endl;

  strm << "\tResize(10)...";
  ptr->Resize(10);
  strm << "OK" << endl;

  strm << "\tResize(5)...";
  ptr->Resize(5);
  strm << "OK" << endl;

  strm << "\tResize(size)...";
  ptr->Resize(size);
  strm << "OK" << endl;

  strm << "\tSetNumberOfValues...";
  ptr->SetNumberOfValues(100);
  if (ptr->GetNumberOfValues() == 100) strm << "OK" << endl;
  else
  {
    ++errors;
    strm << "FAILED" << endl;
  }

  strm << "\tSetVoidArray...";
  ptr->SetVoidArray(strings, size, 1);
  strm << "OK" << endl;

  strm << "\tGetValue...";
  vtkStdString value = ptr->GetValue(123);
  if (value == "string entry 123")
  {
    strm << "OK" << endl;
  }
  else
  {
    ++errors;
    strm << "FAILED.  Expected 'string entry 123', got '"
         << value << "'" << endl;
#ifdef DUMP_VALUES
    for (int i = 0; i < ptr->GetNumberOfValues(); ++i)
    {
      strm << "\t\tValue " << i << ": " << ptr->GetValue(i) << endl;
    }
#endif
  }

  strm << "\tSetValue...";
  ptr->SetValue(124, "jabberwocky");
  if (ptr->GetValue(124) == "jabberwocky")
  {
    strm << "OK" << endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << endl;
  }

  strm << "\tInsertValue...";
  ptr->InsertValue(500, "There and Back Again");
  if (ptr->GetValue(500) == "There and Back Again")
  {
    strm << "OK" << endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << endl;
  }

  strm << "\tInsertNextValue...";
  if (ptr->GetValue(ptr->InsertNextValue("3.141592653589")) ==
      "3.141592653589")
  {
    strm << "OK" << endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << endl;
  }

  strm << "\tvtkAbstractArray::GetTuples(vtkIdList)...";
  vtkIdList *indices = vtkIdList::New();
  indices->InsertNextId(10);
  indices->InsertNextId(20);
  indices->InsertNextId(314);

  vtkStringArray *newValues = vtkStringArray::New();
  newValues->SetNumberOfValues(3);
  ptr->GetTuples(indices, newValues);

  if (newValues->GetValue(0) == "string entry 10" &&
      newValues->GetValue(1) == "string entry 20" &&
      newValues->GetValue(2) == "string entry 314")
  {
    strm << "OK" << endl;
  }
  else
  {
    ++errors;
    strm << "FAILED.  Results:" << endl;
    strm << "\tExpected: 'string entry 10'\tActual: '"
         << newValues->GetValue(0) << "'" << endl;
    strm << "\tExpected: 'string entry 20'\tActual: '"
         << newValues->GetValue(1) << "'" << endl;
    strm << "\tExpected: 'string entry 314'\tActual: '"
         << newValues->GetValue(2) << "'" << endl;
  }

  newValues->Reset();

  strm << "\tvtkAbstractArray::GetTuples(vtkIdType, vtkIdType)...";
  newValues->SetNumberOfValues(3);
  ptr->GetTuples(30, 32, newValues);
  if (newValues->GetValue(0) == "string entry 30" &&
      newValues->GetValue(1) == "string entry 31" &&
      newValues->GetValue(2) == "string entry 32")
  {
    strm << "OK" << endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << endl;
  }

  strm << "\tvtkAbstractArray::InsertTuple...";
  ptr->InsertTuple(150, 2, newValues);
  if (ptr->GetValue(150) == "string entry 32")
  {
    strm << "OK" << endl;
  }
  else
  {
    ++errors;
    strm << "FAILED" << endl;
  }

  newValues->Delete();
  indices->Delete();

  strm << "PrintSelf..." << endl;
  strm << *ptr;

  ptr->Delete();
  delete [] strings;
  return errors;
}

int otherStringArrayTest(ostream& strm)
{
  int errors = 0;
  {
    strm << "Test StringArray" << endl;
    errors += doStringArrayTest(strm, SIZE);
  }

    return errors;
}

int otherStringArray(int, char *[])
{
  return otherStringArrayTest(cerr);
}
