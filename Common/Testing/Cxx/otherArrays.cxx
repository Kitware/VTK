/************************************************************************
  Module:    otherArrays.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
 ************************************************************************/

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

#define SIZE 1000

template <class T, class A>
static int doArrayTest (ostream& strm, T *ptr, A *array, int size)
{
  T *ptr2;
  float tuple1[SIZE/100];
  double tuple3[SIZE/100];
  float *tuple2;
  int i;
  
  strm << "\tSetArray...";
  ptr->SetArray(array, size, 1); 
  strm << "OK" << endl;

  strm << "\tSetNumberOfTuples...";
  ptr->SetNumberOfTuples (100);
  if (ptr->GetNumberOfTuples() == 100) strm << "OK" << endl;
  else strm << "FAILED" << endl;

  strm << "\tSetNumberOfComponents...";
  ptr->SetNumberOfComponents (10);
  if (ptr->GetNumberOfComponents() == 10) strm << "OK" << endl;
  else strm << "FAILED" << endl;

  strm << "\tMakeObject...";
  if (ptr2 = ptr->SafeDownCast(ptr->MakeObject()))
    {
    if (ptr2->GetNumberOfComponents() == 10) strm << "OK" << endl;
    else strm << "FAILED" << endl;
    }
  else
    {
    strm << "FAILED" << endl;
    }

  strm << "\tGetTuple(i)...";
  tuple2 = ptr->GetTuple (2);
  int passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << *(tuple2 + i) << " ";
    if (*(tuple2 + i) != (20 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else strm << "FAILED" << endl;

  strm << "\tGetTuple(i, float *tuple)...";
  ptr->GetTuple (3, tuple1);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple1[i] << " ";
    if (tuple1[i] != (30 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else strm << "FAILED" << endl;

  strm << "\tGetTuple(i, double *tuple)...";
  ptr->GetTuple (4, tuple3);
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
  else strm << "FAILED" << endl;

  strm << "\tvtkDataArray::GetTuple(i, double *tuple)...";
  ptr->vtkDataArray::GetTuple (4, tuple3);
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
  else strm << "FAILED" << endl;

  strm << "\tSetTuple(i, float *tuple)...";
  ptr->SetTuple (99, tuple1);
  for (i=0; i < 10; i++) tuple1[i] = 0;
  ptr->GetTuple (99, tuple1);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple1[i] << " ";
    if (tuple1[i] != (30 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else strm << "FAILED" << endl;

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
  else strm << "FAILED" << endl;

  strm << "\tvtkDataArray::SetTuple(i, double *tuple)...";
  ptr->vtkDataArray::SetTuple (99, tuple3);
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
  else strm << "FAILED" << endl;

  strm << "\tInsertTuple(i, float *tuple)...";
  ptr->InsertTuple (100, tuple1);
  for (i=0; i < 10; i++) tuple1[i] = 0;
  ptr->GetTuple (100, tuple1);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple1[i] << " ";
    if (tuple1[i] != (30 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else strm << "FAILED" << endl;

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
  else strm << "FAILED" << endl;

  strm << "\tvtkDataArray::InsertTuple(i, double *tuple)...";
  ptr->vtkDataArray::InsertTuple (100, tuple3);
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
  else strm << "FAILED" << endl;

  strm << "\tInsertNextTuple(float *tuple)...";
  ptr->InsertNextTuple (tuple1);
  for (i=0; i < 10; i++) tuple1[i] = 0;
  ptr->GetTuple (101, tuple1);
  passed = 1;
  for (i = 0; i < 10; i++)
    {
    strm << tuple1[i] << " ";
    if (tuple1[i] != (30 + i))
      {
      passed = 0;
      break;
      }
    }
  if (passed) strm << "OK" << endl;
  else strm << "FAILED" << endl;

  strm << "\tInsertNextTuple(double *tuple)...";
  ptr->InsertNextTuple (tuple3);
  for (i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (102, tuple3);
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
  else strm << "FAILED" << endl;

  strm << "\tvtkDataArray::InsertNextTuple(double *tuple)...";
  ptr->vtkDataArray::InsertNextTuple (tuple3);
  for (i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (102, tuple3);
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
  else strm << "FAILED" << endl;

  strm << "\tvtkDataArray::GetData...";
  vtkFloatArray *farray = vtkFloatArray::New();
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
  else strm << "FAILED" << endl;
  farray->Delete();
  
  strm << "PrintSelf..." << endl;
  strm << *ptr;

  return 0;
}

void Test(ostream& strm)
{
  {
  strm << "Test CharArray" << endl;
  vtkCharArray *ptr = vtkCharArray::New();
  char *array = new char[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedCharArray" << endl;
  vtkUnsignedCharArray *ptr = vtkUnsignedCharArray::New();
  unsigned char *array = new unsigned char[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test IntArray" << endl;
  vtkIntArray *ptr = vtkIntArray::New();
  int *array = new int[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedIntArray" << endl;
  vtkUnsignedIntArray *ptr = vtkUnsignedIntArray::New();
  unsigned int *array = new unsigned int[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test LongArray" << endl;
  vtkLongArray *ptr = vtkLongArray::New();
  long *array = new long[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedLongArray" << endl;
  vtkUnsignedLongArray *ptr = vtkUnsignedLongArray::New();
  unsigned long *array = new unsigned long[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test ShortArray" << endl;
  vtkShortArray *ptr = vtkShortArray::New();
  short *array = new short[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedShortArray" << endl;
  vtkUnsignedShortArray *ptr = vtkUnsignedShortArray::New();
  unsigned short *array = new unsigned short[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test FloatArray" << endl;
  vtkFloatArray *ptr = vtkFloatArray::New();
  float *array = new float[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test DoubleArray" << endl;
  vtkDoubleArray *ptr = vtkDoubleArray::New();
  double *array = new double[SIZE];
  for (int i = 0; i < SIZE; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, SIZE);
  ptr->Delete();
  delete []array;
  }

}

int main(int argc, char* argv[])
{
  vtkDebugLeaks::PromptUserOff();

  Test(cout);

  return 0;
} 
