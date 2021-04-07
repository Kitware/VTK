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
#include "vtkDataArrayRange.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include <functional>
#include <set>
#include <vector>

static const int Target = 10000;

class ARangeFunctor
{
public:
  vtkSMPThreadLocal<int> Counter;

  ARangeFunctor()
    : Counter(0)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    for (int i = begin; i < end; i++)
      this->Counter.Local()++;
  }
};

class MyVTKClass : public vtkObject
{
  int Value;

  MyVTKClass()
    : Value(0)
  {
  }

public:
  vtkTypeMacro(MyVTKClass, vtkObject);
  static MyVTKClass* New();

  void SetInitialValue(int value) { this->Value = value; }

  int GetValue() { return this->Value; }

  void Increment() { this->Value++; }
};

vtkStandardNewMacro(MyVTKClass);

class InitializableFunctor
{
public:
  vtkSMPThreadLocalObject<MyVTKClass> CounterObject;

  void Initialize() { CounterObject.Local()->SetInitialValue(5); }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    for (int i = begin; i < end; i++)
      this->CounterObject.Local()->Increment();
  }

  void Reduce() {}
};

// For sorting comparison
bool myComp(double a, double b)
{
  return (a < b);
}

int TestSMP(int, char*[])
{
  // vtkSMPTools::Initialize(8);

  ARangeFunctor functor1;

  vtkSMPTools::For(0, Target, functor1);

  vtkSMPThreadLocal<int>::iterator itr1 = functor1.Counter.begin();
  vtkSMPThreadLocal<int>::iterator end1 = functor1.Counter.end();

  int total = 0;
  while (itr1 != end1)
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
  while (itr2 != end2)
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
  double data0[] = { 2, 1, 0, 3, 9, 6, 7, 3, 8, 4, 5 };
  std::vector<double> myvector(data0, data0 + 11);
  double data1[] = { 2, 1, 0, 3, 9, 6, 7, 3, 8, 4, 5 };
  double sdata[] = { 0, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9 };

  // using default comparison (operator <):
  vtkSMPTools::Sort(myvector.begin(), myvector.begin() + 11);
  for (int i = 0; i < 11; ++i)
  {
    if (myvector[i] != sdata[i])
    {
      cerr << "Error: Bad vector sort!" << endl;
      return 1;
    }
  }

  vtkSMPTools::Sort(data1, data1 + 11, myComp);
  for (int i = 0; i < 11; ++i)
  {
    if (data1[i] != sdata[i])
    {
      cerr << "Error: Bad comparison sort!" << endl;
      return 1;
    }
  }

  // Test transform
  std::vector<double> transformData0 = { 51, 9, 3, -10, 27, 1, -5, 82, 31, 9, 21 };
  std::vector<double> transformData1 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  std::set<double> transformData2 = { 7, 24, 98, 256, 72, 19, 3, 21, 2, 12 };
  std::vector<double> transformData3 = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  vtkNew<vtkAOSDataArrayTemplate<double>> transformArray0;
  vtkNew<vtkAOSDataArrayTemplate<double>> transformArray1;

  transformArray0->SetNumberOfComponents(1);
  transformArray0->SetArray(transformData0.data(), transformData0.size(), 1);
  transformArray1->SetNumberOfComponents(1);
  transformArray1->SetArray(transformData1.data(), transformData1.size(), 1);

  const auto transformRange0 = vtk::DataArrayValueRange<1>(transformArray0);
  auto transformRange1 = vtk::DataArrayValueRange<1>(transformArray1);
  vtkSMPTools::Transform(transformRange0.cbegin(), transformRange0.cend(), transformRange1.cbegin(),
    transformRange1.begin(), [](double x, double y) { return x * y; });
  auto it0 = transformRange0.begin();
  auto it1 = transformRange1.begin();
  for (vtkIdType i = 0; i < 11; ++i, it0++, it1++)
  {
    if (*it1 != *it0 * i)
    {
      cerr << "Error: Bad comparison transform!" << endl;
      return 1;
    }
  }

  vtkSMPTools::Transform(transformData2.cbegin(), transformData2.cend(), transformData3.begin(),
    [](double x) { return x - 1; });
  auto it2 = transformData2.begin();
  for (const auto& it3 : transformData3)
  {
    if (it3 != *it2 - 1)
    {
      cerr << "Error: Bad comparison transform!" << endl;
      return 1;
    }
    it2++;
  }

  // Test fill
  std::vector<double> fillData0 = { 51, 9, 3, -10, 27, 1, -5, 82, 31, 9, 21 };
  vtkNew<vtkAOSDataArrayTemplate<double>> fillArray0;

  fillArray0->SetNumberOfComponents(1);
  fillArray0->SetArray(fillData0.data(), fillData0.size(), 1);

  // Fill range2 with its first value
  auto fillRange0 = vtk::DataArrayTupleRange<1>(fillArray0);
  const auto value = *fillRange0.begin();
  vtkSMPTools::Fill(fillRange0.begin(), fillRange0.end(), value);
  for (auto it : fillRange0)
  {
    if (it != value)
    {
      cerr << "Error: Bad comparison transform!" << endl;
      return 1;
    }
  }
  return 0;
}
