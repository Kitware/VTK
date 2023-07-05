// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataArrayRange.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include <cstdlib>
#include <deque>
#include <functional>
#include <numeric>
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

template <typename Iterator>
class ForRangeFunctor
{
public:
  vtkSMPThreadLocal<double> Counter;

  ForRangeFunctor()
    : Counter(0)
  {
  }

  void operator()(Iterator begin, Iterator end)
  {
    for (auto it = begin; it != end; ++it)
    {
      this->Counter.Local() += *it;
    }
  }
};

template <typename Iterator>
class NestedFunctor
{
public:
  vtkSMPThreadLocal<int> Counter;
  const int Factor;

  NestedFunctor()
    : Counter(0)
    , Factor(100)
  {
  }

  void operator()(Iterator begin, Iterator end)
  {
    for (auto it = begin; it != end; ++it)
    {
      for (int i = 0; i < *it; ++i)
      {
        vtkSMPThreadLocal<int> nestedCounter(0);
        vtkSMPTools::For(0, this->Factor, [&](vtkIdType start, vtkIdType stop) {
          for (vtkIdType j = start; j < stop; ++j)
          {
            nestedCounter.Local()++;
          }
        });
        for (const auto& el : nestedCounter)
        {
          this->Counter.Local() += el;
        }
      }
    }
  }
};

class NestedSingleFunctor
{
public:
  vtkSMPThreadLocal<int> Counter;

  NestedSingleFunctor()
    : Counter(0)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    bool isSingleOuter = vtkSMPTools::GetSingleThread();
    if (!isSingleOuter)
    {
      return;
    }
    for (int i = begin; i < end; i++)
    {
      vtkSMPThreadLocal<int> nestedCounter(0);
      vtkSMPTools::For(0, 100, [&](vtkIdType start, vtkIdType stop) {
        bool isSingleInner = vtkSMPTools::GetSingleThread();
        if (!isSingleInner)
        {
          return;
        }
        for (vtkIdType j = start; j < stop; ++j)
        {
          nestedCounter.Local()++;
        }
      });

      for (const auto& el : nestedCounter)
      {
        this->Counter.Local() += el;
      }
    }
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

int doTestSMP()
{
  std::cout << "Testing SMP Tools with " << vtkSMPTools::GetBackend() << " backend." << std::endl;

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
    return EXIT_FAILURE;
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
    return EXIT_FAILURE;
  }

  // Test For with range
  std::set<double> forData0;
  for (int i = 0; i < 1000; ++i)
  {
    forData0.emplace(i * 2);
  }
  ForRangeFunctor<std::set<double>::const_reverse_iterator> functor3;
  vtkSMPTools::For(forData0.crbegin(), forData0.crend(), functor3);
  total = 0;
  int sumTarget = std::accumulate(forData0.begin(), forData0.end(), 0);
  for (const auto& el : functor3.Counter)
  {
    total += el;
  }

  if (total != sumTarget)
  {
    cerr << "Error: Invalid output for vtkSMPTools::For with iterators applied "
            "on std::set!"
         << endl;
    return EXIT_FAILURE;
  }

  // Test IsParallelScope
  if (std::string(vtkSMPTools::GetBackend()) != "Sequential")
  {
    vtkSMPThreadLocal<int> isParallel(0);
    int target = 20;
    vtkSMPTools::For(0, target, 1, [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType i = start; i < end; ++i)
      {
        isParallel.Local() += static_cast<int>(vtkSMPTools::IsParallelScope());
      }
    });
    total = 0;
    for (const auto& it : isParallel)
    {
      total += it;
    }
    if (target != total)
    {
      cerr << "Error: on vtkSMPTools::IsParallelScope got " << total << " instead of " << target
           << endl;
      return EXIT_FAILURE;
    }
  }

  // Test nested parallelism
  for (const bool enabled : { true, false })
  {
    std::vector<int> nestedData0 = { 5, 3, 8, 1, 10 };
    NestedFunctor<std::vector<int>::const_iterator> functor4;
    vtkSMPTools::LocalScope(vtkSMPTools::Config{ enabled },
      [&]() { vtkSMPTools::For(nestedData0.cbegin(), nestedData0.cend(), functor4); });

    sumTarget = functor4.Factor * std::accumulate(nestedData0.begin(), nestedData0.end(), 0);
    total = 0;
    for (const auto& el : functor4.Counter)
    {
      total += el;
    }
    if (sumTarget != total)
    {
      cerr << "Error: on nested parallelism got " << total << " instead of " << sumTarget << endl;
      return EXIT_FAILURE;
    }
  }

  // Test GetSingleThread
  if (std::string(vtkSMPTools::GetBackend()) != "Sequential")
  {
    NestedSingleFunctor functor5;
    vtkSMPTools::LocalScope(
      vtkSMPTools::Config{ true }, [&]() { vtkSMPTools::For(0, 100, functor5); });

    vtkSMPThreadLocal<int>::iterator itr5 = functor5.Counter.begin();
    vtkSMPThreadLocal<int>::iterator end5 = functor5.Counter.end();

    total = 0;
    while (itr5 != end5)
    {
      total += *itr5;
      ++itr5;
    }

    if (total >= Target)
    {
      cerr << "Error: on GetSingleThread. " << total << " is greater than or equal to " << Target
           << endl;
      return EXIT_FAILURE;
    }
  }

  // Test LocalScope
  const int targetThreadNb = 2;
  int scopeThreadNb = 0;

  auto lambdaScope0 = [&]() { scopeThreadNb = vtkSMPTools::GetEstimatedNumberOfThreads(); };
  vtkSMPTools::LocalScope(vtkSMPTools::Config{ targetThreadNb }, lambdaScope0);
  if (scopeThreadNb <= 0 || scopeThreadNb > targetThreadNb)
  {
    cerr << "Error: on vtkSMPTools::LocalScope bad number of threads!" << endl;
    return EXIT_FAILURE;
  }

  const bool isNestedTarget = true;
  bool isNested = false;

  auto lambdaScope1 = [&]() { isNested = vtkSMPTools::GetNestedParallelism(); };
  vtkSMPTools::LocalScope(vtkSMPTools::Config{ isNestedTarget }, lambdaScope1);
  if (isNested != isNestedTarget)
  {
    cerr << "Error: on vtkSMPTools::LocalScope bad nested initialisation!" << endl;
    return EXIT_FAILURE;
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
      return EXIT_FAILURE;
    }
  }

  vtkSMPTools::Sort(data1, data1 + 11, myComp);
  for (int i = 0; i < 11; ++i)
  {
    if (data1[i] != sdata[i])
    {
      cerr << "Error: Bad comparison sort!" << endl;
      return EXIT_FAILURE;
    }
  }

  // Test transform
  std::vector<double> transformData0 = { 51, 9, 3, -10, 27, 1, -5, 82, 31, 9, 21 };
  std::vector<double> transformData1 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  std::set<double> transformData2 = { 7, 24, 98, 256, 72, 19, 3, 21, 2, 12 };
  std::vector<double> transformData3 = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  std::vector<double> transformData4 = { 8, 23, 123, 9, 23, 1, 4, 20, 1, 7, 38, 21 };
  std::vector<double> transformData5 = { 0, 0, 0, 0 };
  vtkNew<vtkAOSDataArrayTemplate<double>> transformArray0;
  vtkNew<vtkAOSDataArrayTemplate<double>> transformArray1;
  vtkNew<vtkAOSDataArrayTemplate<double>> transformArray4;
  vtkNew<vtkAOSDataArrayTemplate<double>> transformArray5;

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
      cerr << "Error: Invalid output for vtkSMPTools::Transform (binary op) applied on "
              "vtk::DataArrayValueRange!"
           << endl;
      return EXIT_FAILURE;
    }
  }

  vtkSMPTools::Transform(transformData2.cbegin(), transformData2.cend(), transformData3.begin(),
    [](double x) { return x - 1; });
  auto it2 = transformData2.begin();
  for (const auto& it3 : transformData3)
  {
    if (it3 != *it2 - 1)
    {
      cerr << "Error: Invalid output for vtkSMPTools::Transform (unary op) applied on std::set!"
           << endl;
      return EXIT_FAILURE;
    }
    it2++;
  }

  transformArray4->SetNumberOfComponents(3);
  transformArray4->SetArray(transformData4.data(), transformData4.size(), 1);
  transformArray5->SetNumberOfComponents(1);
  transformArray5->SetArray(transformData5.data(), transformData5.size(), 1);

  const auto transformRange4 = vtk::DataArrayTupleRange<3>(transformArray4);
  auto transformRange5 = vtk::DataArrayValueRange<1>(transformArray5);

  using TupleRef = typename decltype(transformRange4)::const_reference;
  using ValueType = typename decltype(transformRange5)::ValueType;
  auto computeMag = [](const TupleRef& tuple) -> ValueType {
    ValueType mag = 0;
    for (const auto& comp : tuple)
    {
      mag += static_cast<ValueType>(comp);
    }
    return mag;
  };

  vtkSMPTools::Transform(
    transformRange4.cbegin(), transformRange4.cend(), transformRange5.begin(), computeMag);
  auto it5 = transformRange5.begin();
  for (const auto it4 : transformRange4)
  {
    ValueType result = 0;
    for (const auto& comp : it4)
    {
      result += static_cast<ValueType>(comp);
    }
    if (result != *it5)
    {
      cerr << "Error: Invalid output for vtkSMPTools::Transform (unary op) applied on "
              "vtk::DataArrayTupleRange!"
           << endl;
      return EXIT_FAILURE;
    }
    it5++;
  }

  // Test fill
  std::vector<double> fillData0 = { 51, 9, 3, -10, 27, 1, -5, 82, 31, 9 };
  std::deque<double> fillData1 = { 0, 0, 0, 0, 0 };
  vtkNew<vtkAOSDataArrayTemplate<double>> fillArray0;

  fillArray0->SetNumberOfComponents(2);
  fillArray0->SetArray(fillData0.data(), fillData0.size(), 1);

  auto fillRange0 = vtk::DataArrayTupleRange<2>(fillArray0);
  const auto fillValue0 = *fillRange0.begin();
  vtkSMPTools::Fill(fillRange0.begin(), fillRange0.end(), fillValue0);
  for (auto it : fillRange0)
  {
    if (it != fillValue0)
    {
      cerr << "Error: Invalid output for vtkSMPTools::Fill applied on vtk::DataArrayTupleRange!"
           << endl;
      return EXIT_FAILURE;
    }
  }

  const double fillValue1 = 42;
  vtkSMPTools::Fill(fillData1.begin(), fillData1.end(), fillValue1);
  for (auto& it : fillData1)
  {
    if (it != fillValue1)
    {
      cerr << "Error: Invalid output for vtkSMPTools::Fill applied on std::deque!" << endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestSMP(int argc, char* argv[])
{
  int returnValue = EXIT_SUCCESS;
  for (int i = 1; i < argc; i++)
  {
    std::string argument(argv[i] + 2);
    std::size_t separator = argument.find('=');
    std::string backend = argument.substr(0, separator);
    int value = std::atoi(argument.substr(separator + 1, argument.size()).c_str());
    if (value)
    {
      vtkSMPTools::SetBackend(backend.c_str());
      if (doTestSMP() != EXIT_SUCCESS)
        returnValue = EXIT_FAILURE;
    }
  }
  return returnValue;
}
