
/************************************************************************
 *
 * File: otherArrays.cxx
 *
 * Created:       Mon Feb  1 17:10:45 1999 by Tony Chi-shao Pan
 * Last Modified: Wed Dec 22 14:15:49 1999 by Tony Chi-shao Pan
 *
 * < General description goes here > 
 *
 ************************************************************************/


// All tests need:
//   the following include
//   a Selector proc
//   a Comparator proc
//   a Test proc
//   and a main
#include "rtOtherTestBase.h"

void SelectorCommand(ostream& strm) {
  strm << "grep -v 0x | grep -v Modified ";
}

void ComparatorCommand(ostream& strm) {
  strm << "diff -b";
}
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

template <class T, class A>
static int doArrayTest (ostream& strm, T *ptr, A *array, int size)
{
  T *ptr2;
  float tuple1[size/100];
  double tuple3[size/100];
  float *tuple2;
  
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
  for (int i = 0; i < 10; i++)
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
  for (int i = 0; i < 10; i++)
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
  for (int i = 0; i < 10; i++)
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
  for (int i=0; i < 10; i++) tuple1[i] = 0;
  ptr->GetTuple (99, tuple1);
  passed = 1;
  for (int i = 0; i < 10; i++)
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
  for (int i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (99, tuple3);
  passed = 1;
  for (int i = 0; i < 10; i++)
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
  for (int i=0; i < 10; i++) tuple1[i] = 0;
  ptr->GetTuple (100, tuple1);
  passed = 1;
  for (int i = 0; i < 10; i++)
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
  for (int i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (100, tuple3);
  passed = 1;
  for (int i = 0; i < 10; i++)
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
  for (int i=0; i < 10; i++) tuple1[i] = 0;
  ptr->GetTuple (101, tuple1);
  passed = 1;
  for (int i = 0; i < 10; i++)
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
  for (int i=0; i < 10; i++) tuple3[i] = 0;
  ptr->GetTuple (102, tuple3);
  passed = 1;
  for (int i = 0; i < 10; i++)
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

  strm << "\tPrintSelf...";
  strm << *ptr;
}


void Test(ostream& strm)
{
  {
  strm << "Test CharArray" << endl;
  vtkCharArray *ptr = vtkCharArray::New();
  char *array = new char[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedCharArray" << endl;
  vtkUnsignedCharArray *ptr = vtkUnsignedCharArray::New();
  unsigned char *array = new unsigned char[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test IntArray" << endl;
  vtkIntArray *ptr = vtkIntArray::New();
  int *array = new int[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedIntArray" << endl;
  vtkUnsignedIntArray *ptr = vtkUnsignedIntArray::New();
  unsigned int *array = new unsigned int[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test LongArray" << endl;
  vtkLongArray *ptr = vtkLongArray::New();
  long *array = new long[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedLongArray" << endl;
  vtkUnsignedLongArray *ptr = vtkUnsignedLongArray::New();
  unsigned long *array = new unsigned long[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test ShortArray" << endl;
  vtkShortArray *ptr = vtkShortArray::New();
  short *array = new short[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test UnsignedShortArray" << endl;
  vtkUnsignedShortArray *ptr = vtkUnsignedShortArray::New();
  unsigned short *array = new unsigned short[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test FloatArray" << endl;
  vtkFloatArray *ptr = vtkFloatArray::New();
  float *array = new float[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

  {
  strm << "Test DoubleArray" << endl;
  vtkDoubleArray *ptr = vtkDoubleArray::New();
  double *array = new double[1000];
  for (int i = 0; i < 1000; i++) *(array + i ) = i;
  doArrayTest (strm, ptr, array, 1000);
  ptr->Delete();
  delete []array;
  }

}

int main(int argc, char* argv[])
{
  rtOtherTestBase::RunTest(argc, argv, SelectorCommand, ComparatorCommand, Test);

  return 0;
} 
