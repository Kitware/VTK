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
#include "vtkSMPThreadLocal.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"
#include <functional>
#include <vector>

static const int Target = 10000;

class ARangeFunctor
{
public:
  vtkSMPThreadLocal<int> Counter;

  ARangeFunctor(): Counter(0)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    for (int i=begin; i<end; i++)
      this->Counter.Local()++;
  }
};

class MyVTKClass : public vtkObject
{
  int Value;

  MyVTKClass() : Value(0)
  {
  }

public:
  vtkTypeMacro(MyVTKClass, vtkObject);
  static MyVTKClass* New();

  void SetInitialValue(int value)
  {
    this->Value = value;
  }

  int GetValue()
  {
    return this->Value;
  }

  void Increment()
  {
    this->Value++;
  }
};

vtkStandardNewMacro(MyVTKClass);

class InitializableFunctor
{
public:
  vtkSMPThreadLocalObject<MyVTKClass> CounterObject;

  void Initialize()
  {
    CounterObject.Local()->SetInitialValue(5);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    for (int i=begin; i<end; i++)
      this->CounterObject.Local()->Increment();
  }

  void Reduce()
  {
  }

};

// For sorting comparison
bool myComp (double a, double b) { return (a<b); }

int TestSMP(int, char*[])
{
  //vtkSMPTools::Initialize(8);

  ARangeFunctor functor1;

  vtkSMPTools::For(0, Target, functor1);

  vtkSMPThreadLocal<int>::iterator itr1 = functor1.Counter.begin();
  vtkSMPThreadLocal<int>::iterator end1 = functor1.Counter.end();

  int total = 0;
  while(itr1 != end1)
  {
    total += *itr1;
    ++itr1;
  }

  if (total != Target)
  {
    cerr << "Error: ARangeFunctor did not generate " << Target << endl;
    return 1;
  }

  InitializableFunctor functor2;

  vtkSMPTools::For(0, Target, functor2);

  vtkSMPThreadLocalObject<MyVTKClass>::iterator itr2 = functor2.CounterObject.begin();
  vtkSMPThreadLocalObject<MyVTKClass>::iterator end2 = functor2.CounterObject.end();

  int newTarget = Target;
  total = 0;
  while(itr2 != end2)
  {
    newTarget += 5; // This is the initial value of each object
    total += (*itr2)->GetValue();
    ++itr2;
  }

  if (total != newTarget)
  {
    cerr << "Error: InitializableRangeFunctor did not generate " << newTarget << endl;
    return 1;
  }

  // Test sorting
  double data0[] = {2,1,0,3,9,6,7,3,8,4,5};
  std::vector<double> myvector (data0, data0+11);
  double data1[] = {2,1,0,3,9,6,7,3,8,4,5};
  double sdata[] = {0,1,2,3,3,4,5,6,7,8,9};

  // using default comparison (operator <):
  vtkSMPTools::Sort(myvector.begin(), myvector.begin()+11);
  for (int i=0; i<11; ++i)
  {
    if ( myvector[i] != sdata[i] )
    {
      cerr << "Error: Bad vector sort!" << endl;
      return 1;
    }
  }

  vtkSMPTools::Sort(data1,data1+11,myComp);
  for (int i=0; i<11; ++i)
  {
    if ( data1[i] != sdata[i] )
    {
      cerr << "Error: Bad comparison sort!" << endl;
      return 1;
    }
  }

  return 0;
}
