/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDispatchers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test Dispatchers
// .SECTION Description
// Tests vtkDispatcher and vtkDoubleDispatcher

#include "vtkObjectFactory.h"
#include "vtkDispatcher.h"
#include "vtkDoubleDispatcher.h"
#include "vtkDataArrayDispatcher.h"
#include "vtkNew.h"

//classes we will be using in the test
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"
#include "vtkPoints.h"

#include <stdexcept>
#include <algorithm>
namespace
{

void test_expression(bool valid, std::string msg)
{
  if(!valid)
  {
    throw std::runtime_error(msg);
  }
}

template<typename T, typename U>
inline T* as(U* u)
{
  return dynamic_cast<T*>(u);
}

struct singleFunctor
{
  int timesCalled;
  singleFunctor():timesCalled(0){}
  template<typename T>
  int operator()(T&)
  {
    return ++timesCalled;
  }
};

struct doubleFunctor
{
  int timesCalled;
  doubleFunctor():timesCalled(0){}
  template<typename T, typename U>
  int operator()(T&, U&)
  {
    return ++timesCalled;
  }
};

//type traits for vtkTTFunctor and pointsWrapper
template<typename T> struct FieldType;
template<> struct FieldType<vtkIntArray>
{
  enum {VTK_DATA_TYPE=VTK_INT};
  typedef int ValueType;
};

template<> struct FieldType<vtkDoubleArray>
{
  enum {VTK_DATA_TYPE=VTK_DOUBLE};
  typedef double ValueType;
};

template<> struct FieldType<vtkCharArray>
{
  enum {VTK_DATA_TYPE=VTK_CHAR};
  typedef char ValueType;
};

//this functor replaces the usage of VTK_TT macro, by showing
//how to use template traits
struct vtkTTFunctor
{
  template<typename T>
  void operator()(T& t) const
  {
    //example that sorts, only works on single component
    typedef typename FieldType<T>::ValueType ValueType;
    if(t.GetNumberOfComponents() == 1)
    {
      ValueType* start = static_cast<ValueType*>(t.GetVoidPointer(0));
      ValueType* end = static_cast<ValueType*>(t.GetVoidPointer(
                                                 t.GetNumberOfTuples()));
      std::sort(start,end);
    }
  }
};

struct pointsFunctor
{
  vtkPoints* operator()(vtkDoubleArray& dataArray) const
  {
    vtkPoints* points = vtkPoints::New();
    points->SetData(&dataArray);
    return points;
  }

  vtkPoints* operator()(vtkIntArray& dataArray) const
  {
    vtkPoints* points = vtkPoints::New();
    points->SetNumberOfPoints(dataArray.GetNumberOfTuples());
    return points;
  }
};

}

bool TestSingleDispatch()
{
  //statefull dispatching
  singleFunctor functor;
  vtkDispatcher<vtkObject,int> dispatcher;
  dispatcher.Add<vtkDoubleArray>(&functor);
  dispatcher.Add<vtkStringArray>(&functor);
  dispatcher.Add<vtkIntArray>(&functor);

  //verify the dispatching
  vtkNew<vtkDoubleArray> doubleArray;
  vtkNew<vtkStringArray> stringArray;
  vtkNew<vtkIntArray> intArray;
  vtkNew<vtkPoints> pointsArray;

  int result = dispatcher.Go(as<vtkObject>(doubleArray.GetPointer()));
  test_expression(result==1,"double array dispatch failed with statefull functor");

  result = dispatcher.Go(stringArray.GetPointer());
  test_expression(result==2,"string array dispatch failed with statefull functor");

  result = dispatcher.Go(intArray.GetPointer());
  test_expression(result==3,"int array dispatch failed with statefull functor");

  result = dispatcher.Go(pointsArray.GetPointer());
  test_expression(result==0,"points array didn't fail");

  return true;
}

bool TestStatelessSingleDispatch()
{
  //stateless dispatching
  vtkDispatcher<vtkObject,int> dispatcher;
  dispatcher.Add<vtkDoubleArray>(singleFunctor());
  dispatcher.Add<vtkStringArray>(singleFunctor());

  //verify the dispatching
  vtkNew<vtkDoubleArray> doubleArray;
  vtkNew<vtkStringArray> stringArray;

  int result = dispatcher.Go(doubleArray.GetPointer());
  test_expression(result==1,"double array dispatch failed with stateless functor");

  result = dispatcher.Go(as<vtkObject>(stringArray.GetPointer()));
  test_expression(result==1,"string array dispatch failed with stateless functor");

  return true;
}

bool TestDoubleDispatch()
{
  //statefull dispatching
  doubleFunctor functor;
  vtkDoubleDispatcher<vtkObject,vtkObject,int> dispatcher;
  dispatcher.Add<vtkDoubleArray,vtkStringArray>(&functor);
  dispatcher.Add<vtkStringArray,vtkStringArray>(&functor);
  dispatcher.Add<vtkIntArray,vtkDoubleArray>(&functor);

  //verify the dispatching
  vtkNew<vtkDoubleArray> doubleArray;
  vtkNew<vtkStringArray> stringArray;
  vtkNew<vtkIntArray> intArray;
  vtkNew<vtkPoints> pointsArray;

  int result = dispatcher.Go(as<vtkObject>(doubleArray.GetPointer()),
                             as<vtkObject>(stringArray.GetPointer()));
  test_expression(result==1,"double array dispatch failed with statefull functor");

  result = dispatcher.Go(stringArray.GetPointer(),stringArray.GetPointer());
  test_expression(result==2,"string array dispatch failed with statefull functor");

  result = dispatcher.Go(as<vtkObject>(intArray.GetPointer()),doubleArray.GetPointer());
  test_expression(result==3,"int array dispatch failed with statefull functor");

  result = dispatcher.Go(intArray.GetPointer(),pointsArray.GetPointer());
  test_expression(result==0,"points array didn't fail");

  return true;
}

bool TestStatelessDoubleDispatch()
{
  //stateless dispatching
  vtkDoubleDispatcher<vtkObject,vtkObject,int> dispatcher;
  dispatcher.Add<vtkDoubleArray,vtkStringArray>(doubleFunctor());
  dispatcher.Add<vtkStringArray,vtkStringArray>(doubleFunctor());
  dispatcher.Add<vtkIntArray,vtkDoubleArray>(doubleFunctor());

  //verify the dispatching
  vtkNew<vtkDoubleArray> doubleArray;
  vtkNew<vtkStringArray> stringArray;
  vtkNew<vtkIntArray> intArray;
  vtkNew<vtkPoints> pointsArray;

  int result = dispatcher.Go(doubleArray.GetPointer(),stringArray.GetPointer());
  test_expression(result==1,"double array dispatch failed with statefull functor");

  result = dispatcher.Go(stringArray.GetPointer(),stringArray.GetPointer());
  test_expression(result==1,"string array dispatch failed with statefull functor");

  result = dispatcher.Go(intArray.GetPointer(),doubleArray.GetPointer());
  test_expression(result==1,"int array dispatch failed with statefull functor");

  result = dispatcher.Go(intArray.GetPointer(),pointsArray.GetPointer());
  test_expression(result==0,"points array didn't fail");

  return true;
}


bool TestMixedDispatch()
{
  //stateless dispatching
  singleFunctor functor;
  vtkDispatcher<vtkDataArray,int> dispatcher;
  dispatcher.Add<vtkDoubleArray>(&functor);
  dispatcher.Add<vtkIntArray>(&functor);
  dispatcher.Add<vtkCharArray>(singleFunctor());

  //verify the dispatching
  vtkNew<vtkDoubleArray> doubleArray;
  vtkNew<vtkIntArray> intArray;
  vtkNew<vtkCharArray> charArray;

  int result = dispatcher.Go(as<vtkDataArray>(doubleArray.GetPointer()));
  result = dispatcher.Go(intArray.GetPointer());
  test_expression(result==2,"unexpected");
  result = dispatcher.Go(doubleArray.GetPointer());
  test_expression(result==3,"statefull functor failed with int and double");

  result = dispatcher.Go(charArray.GetPointer());
  test_expression(result==1,"");


  return true;
}

bool TestVTKTTReplacement()
{
  //stateless dispatching
  vtkDispatcher<vtkDataArray> dispatcher; //default return type is void
  dispatcher.Add<vtkDoubleArray>(vtkTTFunctor());
  dispatcher.Add<vtkIntArray>(vtkTTFunctor());

  //verify the dispatching
  vtkNew<vtkDoubleArray> doubleArray;
  vtkNew<vtkIntArray> intArray;

  doubleArray->SetNumberOfValues(10);
  intArray->SetNumberOfValues(10);

  for(int i=0; i < 10; ++i)
  {
    doubleArray->SetValue(i,10-i);
    intArray->SetValue(i,-10*i);
  }

  //sort the array, passing in as vtkObject to show we use RTTI
  //to get out the derived class info
  dispatcher.Go(as<vtkDataArray>(doubleArray.GetPointer()));
  dispatcher.Go(as<vtkDataArray>(intArray.GetPointer()));

  //verify the array is sorted, by checking min & max
  test_expression(doubleArray->GetValue(0)==1,"double array not sorted");
  test_expression(doubleArray->GetValue(9)==10,"double array not sorted");

  test_expression(intArray->GetValue(0)==-90,"int array not sorted");
  test_expression(intArray->GetValue(9)==0,"int array not sorted");

  return true;
}

bool TestReturnVtkObject()
{
  //This example shows how to return a vtkObject that is filled by the algorithm
  //that you passed in.
  vtkDispatcher<vtkDataArray,vtkPoints*> dispatcher; //default return type is void
  dispatcher.Add<vtkDoubleArray>(pointsFunctor());
  dispatcher.Add<vtkIntArray>(pointsFunctor());

  //verify the dispatching
  vtkNew<vtkDoubleArray> doubleArray;
  doubleArray->SetNumberOfComponents(3);
  doubleArray->SetNumberOfTuples(1);

  //make sure the result isn't copied anywhere
  vtkPoints* result = dispatcher.Go(as<vtkDataArray>(doubleArray.GetPointer()));

  test_expression(result != NULL, "Returned points not valid");
  test_expression(result->GetData() == doubleArray.GetPointer(),
                  "Returned points not equal to the passed in double array");
  result->Delete();

  //on an integer function we should get a whole new points array
  vtkNew<vtkIntArray> intArray;
  result = dispatcher.Go(as<vtkDataArray>( intArray.GetPointer() ));

  test_expression(result != NULL, "Returned points not valid");
  result->Delete();

  return true;
}


int TestDispatchers(int /*argc*/, char* /*argv*/[])
{

  bool passed = TestSingleDispatch();
  passed &= TestStatelessSingleDispatch();
  passed &= TestDoubleDispatch();
  passed &= TestStatelessDoubleDispatch();
  passed &= TestMixedDispatch();
  passed &= TestVTKTTReplacement();
  passed &= TestReturnVtkObject();

  return passed ? 0 : 1;
}
