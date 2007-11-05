/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDebugLeaks.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"

#include <vtksys/ios/sstream>

#define SIZE 1000

template <class T, class A, class V>
int doArrayTest (ostream& strm, T *ptr, A *array, V value, int size)
{
  float tuple1[SIZE/100];
  double tuple3[SIZE/100];
  double *tuple2;
  int i;
  int errors = 0;
  
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

  strm << "\tSetNumberOfTuples...";
  ptr->SetNumberOfTuples (100);
  if (ptr->GetNumberOfTuples() == 100) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }
  strm << "\tSetNumberOfComponents...";
  ptr->SetNumberOfComponents (10);
  if (ptr->GetNumberOfComponents() == 10) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetVoidArray...";
  ptr->SetVoidArray(array, size, 1); 
  strm << "OK" << endl;

  strm << "CreateDefaultLookupTable" << endl;
  ptr->CreateDefaultLookupTable();
  strm << *ptr;

  strm << "\tGetTuple(i)...";
  tuple2 = ptr->GetTuple (2);
  int passed = 1;
  if (tuple2)
    {
    for (i = 0; i < 10; i++)
      {
      strm << *(tuple2 + i) << " ";
      if (*(tuple2 + i) != (20 + i))
        {
        passed = 0;
        break;
        }
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tGetTuple(i, double *tuple)...";
  ptr->GetTuple (4, tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    tuple1[i] = static_cast<float>(tuple3[i]);
    strm << tuple3[i] << " ";
    if (tuple3[i] != (40 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tvtkDataArray::GetTuple(i, double *tuple)...";
  static_cast<vtkDataArray*>(ptr)->GetTuple (4, tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple3[i] << " ";
    if (tuple3[i] != (40 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetValue(i, value)...";
  ptr->SetValue (99, value);
  if (ptr->GetValue (99) == value) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertValue(i, value)...";
  ptr->InsertValue (500, value);
  if (ptr->GetValue (500) == value) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertNextValue(value)...";
  if (ptr->GetValue (ptr->InsertNextValue (static_cast<char>(22.0))) == 22.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertComponent(i, j, 5.0)...";
  ptr->InsertComponent (500, 9, 5.0);
  if (ptr->GetComponent (500, 9) == 5.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetTuple(i, float *tuple)...";
  ptr->SetTuple (99, tuple1);
  for (i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (99, tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple3[i] << " ";
    if (tuple3[i] != (40 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetTuple(i, double *tuple)...";
  ptr->SetTuple (99, tuple3);
  for (i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (99, tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple3[i] << " ";
    if (tuple3[i] != (40 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertTuple(i, float *tuple)...";
  ptr->InsertTuple (100, tuple1);
  for (i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (100, tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple3[i] << " ";
    if (tuple3[i] != (40 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertTuple(i, double *tuple)...";
  ptr->InsertTuple (100, tuple3);
  for (i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (100, tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple3[i] << " ";
    if (tuple3[i] != (40 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertNextTuple(float *tuple)...";
  for (i=0; i < 10; i++) tuple1[i] = 30 + i;
  ptr->GetTuple (ptr->InsertNextTuple (tuple1), tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple3[i] << " ";
    if (tuple3[i] != (30 + i))
      {
      strm << "Expected " << 30 + 1 << " ";
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertNextTuple(double *tuple)...";
  for (i=0; i < 10; i++) tuple3[i] = 40 + i;
  ptr->GetTuple (ptr->InsertNextTuple (tuple3), tuple3);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple3[i] << " ";
    if (tuple3[i] != (40 + i))
      {
      strm << "Expected " << 40 + 1;
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tvtkDataArray::GetData...";
  vtkDoubleArray *farray = vtkDoubleArray::New();
  farray->SetNumberOfComponents(1);
  ptr->vtkDataArray::GetData (0, 59, 1, 1,  farray);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << farray->GetTuple(i)[0] << " ";
    if (farray->GetTuple(i)[0] != (1 + i*10))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }


  strm << "\tSetTuple1...";
  ptr->SetNumberOfComponents(1);
  ptr->SetNumberOfTuples(100);
  ptr->SetTuple1(50,10.0);
  if (ptr->GetTuple1(50) == 10.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetTuple2...";
  ptr->SetNumberOfComponents(2);
  ptr->SetNumberOfTuples(100);
  ptr->SetTuple2(50,10.0,20.0);
  if (ptr->GetTuple2(50)[0] == 10.0 &&
      ptr->GetTuple2(50)[1] == 20.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetTuple3...";
  ptr->SetNumberOfComponents(3);
  ptr->SetNumberOfTuples(100);
  ptr->SetTuple3(50,10.0,20.0,30.0);
  if (ptr->GetTuple3(50)[0] == 10.0 &&
      ptr->GetTuple3(50)[1] == 20.0 &&
      ptr->GetTuple3(50)[2] == 30.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetTuple4...";
  ptr->SetNumberOfComponents(4);
  ptr->SetNumberOfTuples(100);
  ptr->SetTuple4(50,10.0,20.0,30.0,40.0);
  if (ptr->GetTuple4(50)[0] == 10.0 &&
      ptr->GetTuple4(50)[1] == 20.0 &&
      ptr->GetTuple4(50)[2] == 30.0 &&
      ptr->GetTuple4(50)[3] == 40.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tSetTuple9...";
  ptr->SetNumberOfComponents(9);
  ptr->SetNumberOfTuples(100);
  ptr->SetTuple9(50,10.0,20.0,30.0,40.0,50.0,60.0,70.0,80.0,90.0);
  if (ptr->GetTuple9(50)[0] == 10.0 &&
      ptr->GetTuple9(50)[1] == 20.0 &&
      ptr->GetTuple9(50)[2] == 30.0 &&
      ptr->GetTuple9(50)[3] == 40.0 &&
      ptr->GetTuple9(50)[4] == 50.0 &&
      ptr->GetTuple9(50)[5] == 60.0 &&
      ptr->GetTuple9(50)[6] == 70.0 &&
      ptr->GetTuple9(50)[7] == 80.0 &&
      ptr->GetTuple9(50)[8] == 90.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertTuple1...";
  ptr->SetNumberOfComponents(1);
  ptr->SetNumberOfTuples(100);
  ptr->InsertTuple1(502,10.0);
  if (ptr->GetTuple1(502) == 10.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertTuple2...";
  ptr->SetNumberOfComponents(2);
  ptr->SetNumberOfTuples(100);
  ptr->InsertTuple2(502,10.0,20.0);
  if (ptr->GetTuple2(502)[0] == 10.0 &&
      ptr->GetTuple2(502)[1] == 20.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertTuple3...";
  ptr->SetNumberOfComponents(3);
  ptr->SetNumberOfTuples(100);
  ptr->InsertTuple3(502,10.0,20.0,30.0);
  if (ptr->GetTuple3(502)[0] == 10.0 &&
      ptr->GetTuple3(502)[1] == 20.0 &&
      ptr->GetTuple3(502)[2] == 30.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertTuple4...";
  ptr->SetNumberOfComponents(4);
  ptr->SetNumberOfTuples(100);
  ptr->InsertTuple4(502,10.0,20.0,30.0,40.0);
  if (ptr->GetTuple4(502)[0] == 10.0 &&
      ptr->GetTuple4(502)[1] == 20.0 &&
      ptr->GetTuple4(502)[2] == 30.0 &&
      ptr->GetTuple4(502)[3] == 40.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertTuple9...";
  ptr->SetNumberOfComponents(9);
  ptr->SetNumberOfTuples(100);
  ptr->InsertTuple9(502,10.0,20.0,30.0,40.0,50.0,60.0,70.0,80.0,90.0);
  if (ptr->GetTuple9(502)[0] == 10.0 &&
      ptr->GetTuple9(502)[1] == 20.0 &&
      ptr->GetTuple9(502)[2] == 30.0 &&
      ptr->GetTuple9(502)[3] == 40.0 &&
      ptr->GetTuple9(502)[4] == 50.0 &&
      ptr->GetTuple9(502)[5] == 60.0 &&
      ptr->GetTuple9(502)[6] == 70.0 &&
      ptr->GetTuple9(502)[7] == 80.0 &&
      ptr->GetTuple9(502)[8] == 90.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertNextTuple1...";
  ptr->SetNumberOfComponents(1);
  ptr->SetNumberOfTuples(100);
  ptr->InsertNextTuple1(10.0);
  if (ptr->GetTuple1(100) == 10.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << " FAILED" << endl;
    }
  strm << "\tInsertNextTuple2...";
  ptr->SetNumberOfComponents(2);
  ptr->SetNumberOfTuples(100);
  ptr->InsertNextTuple2(10.0,20.0);
  if (ptr->GetTuple2(100)[0] == 10.0 &&
      ptr->GetTuple2(100)[1] == 20.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertNextTuple3...";
  ptr->SetNumberOfComponents(3);
  ptr->SetNumberOfTuples(100);
  ptr->InsertNextTuple3(10.0,20.0,30.0);
  if (ptr->GetTuple3(100)[0] == 10.0 &&
      ptr->GetTuple3(100)[1] == 20.0 &&
      ptr->GetTuple3(100)[2] == 30.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertNextTuple4...";
  ptr->SetNumberOfComponents(4);
  ptr->SetNumberOfTuples(100);
  ptr->InsertNextTuple4(10.0,20.0,30.0,40.0);
  if (ptr->GetTuple4(100)[0] == 10.0 &&
      ptr->GetTuple4(100)[1] == 20.0 &&
      ptr->GetTuple4(100)[2] == 30.0 &&
      ptr->GetTuple4(100)[3] == 40.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }

  strm << "\tInsertNextTuple9...";
  ptr->SetNumberOfComponents(9);
  ptr->SetNumberOfTuples(100);
  ptr->InsertNextTuple9(10.0,20.0,30.0,40.0,50.0,60.0,70.0,80.0,90.0);
  if (ptr->GetTuple9(100)[0] == 10.0 &&
      ptr->GetTuple9(100)[1] == 20.0 &&
      ptr->GetTuple9(100)[2] == 30.0 &&
      ptr->GetTuple9(100)[3] == 40.0 &&
      ptr->GetTuple9(100)[4] == 50.0 &&
      ptr->GetTuple9(100)[5] == 60.0 &&
      ptr->GetTuple9(100)[6] == 70.0 &&
      ptr->GetTuple9(100)[7] == 80.0 &&
      ptr->GetTuple9(100)[8] == 90.0) strm << "OK" << endl;
  else
    {
    errors++;
    strm << "FAILED" << endl;
    }


  farray->Delete();
  
  
  strm << "PrintSelf..." << endl;
  strm << *ptr;

  return errors;
}

int otherArraysTest(ostream& strm)
{
  int errors = 0;
  {
  strm << "Test CharArray" << endl;
  vtkCharArray *ptr = vtkCharArray::New();
  char *array = new char[SIZE];
  char value = static_cast<char>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedCharArray" << endl;
  vtkUnsignedCharArray *ptr = vtkUnsignedCharArray::New();
  unsigned char *array = new unsigned char[SIZE];
  unsigned char value = static_cast<unsigned char>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test IntArray" << endl;
  vtkIntArray *ptr = vtkIntArray::New();
  int *array = new int[SIZE];
  int value = static_cast<int>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedIntArray" << endl;
  vtkUnsignedIntArray *ptr = vtkUnsignedIntArray::New();
  unsigned int *array = new unsigned int[SIZE];
  unsigned int value = static_cast<unsigned int>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test LongArray" << endl;
  vtkLongArray *ptr = vtkLongArray::New();
  long *array = new long[SIZE];
  long value = static_cast<long>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedLongArray" << endl;
  vtkUnsignedLongArray *ptr = vtkUnsignedLongArray::New();
  unsigned long *array = new unsigned long[SIZE];
  unsigned long value = static_cast<unsigned long>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test ShortArray" << endl;
  vtkShortArray *ptr = vtkShortArray::New();
  short *array = new short[SIZE];
  short value = static_cast<short>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedShortArray" << endl;
  vtkUnsignedShortArray *ptr = vtkUnsignedShortArray::New();
  unsigned short *array = new unsigned short[SIZE];
  unsigned short value = static_cast<unsigned short>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test FloatArray" << endl;
  vtkFloatArray *ptr = vtkFloatArray::New();
  float *array = new float[SIZE];
  float value = static_cast<float>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test DoubleArray" << endl;
  vtkDoubleArray *ptr = vtkDoubleArray::New();
  double *array = new double[SIZE];
  double value = static_cast<double>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test IdTypeArray" << endl;
  vtkIdTypeArray *ptr = vtkIdTypeArray::New();
  vtkIdType *array = new vtkIdType[SIZE];
  vtkIdType value = static_cast<vtkIdType>(1);
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  errors += doArrayTest (strm, ptr, array, value, SIZE);
  ptr->Delete();
  delete []array;
  }
  return errors;
}

int otherArrays(int, char *[])
{
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701; 
//  return otherArraysTest(vtkmsg_with_warning_C4701);
  return otherArraysTest(cerr);

} 
