/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test some generic features of vtkLookupTable

#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkCommand.h"
#include "vtkSmartPointer.h"

// simple macro for performing tests
#define TestAssert(t) \
if (!(t)) \
{ \
  cerr << "In " << __FILE__ << ":"; \
  cerr << " Test assertion failed line " << __LINE__ << ": " << #t << "\n"; \
  rval |= 1; \
}

#define TestVector4d(t1, t2)                                        \
  TestAssert(                                                       \
    (vtkMathUtilities::FuzzyCompare<double>(t1[0], t2[0])           \
     && vtkMathUtilities::FuzzyCompare<double>(t1[1], t2[1])        \
     && vtkMathUtilities::FuzzyCompare<double>(t1[2], t2[2])        \
     && vtkMathUtilities::FuzzyCompare<double>(t1[3], t2[3])))

int TestColor4uc(unsigned char* expected, unsigned char* test)
{
  int failed = expected[0] != test[0] || expected[1] != test[1] ||
    expected[2] != test[2] || expected[3] != test[3] ? 1 : 0;
  if (failed)
  {
    std::cerr << "Expected color: " <<
      static_cast<int>(expected[0]) << ", " <<
      static_cast<int>(expected[1]) << ", " <<
      static_cast<int>(expected[2]) << ", " <<
      static_cast<int>(expected[3]) << std::endl;
    std::cerr << "Test color: " <<
      static_cast<int>(test[0]) << ", " <<
      static_cast<int>(test[1]) << ", " <<
      static_cast<int>(test[2]) << ", " <<
      static_cast<int>(test[3]) << std::endl;
  }

  return !failed;
}

// a simple error observer
namespace {

class errorObserver
{
public:
  errorObserver() : EventId(0) {}

  void Reset() { this->EventId = 0; }

  unsigned long GetEvent() const { return this->EventId; }

  void Callback(vtkObject *, unsigned long e, void *) {
    this->EventId = e; }

private:
  unsigned long EventId;
};

}

int TestLookupTable(int,char *[])
{
  int rval = 0;

  vtkSmartPointer<vtkLookupTable> table =
    vtkSmartPointer<vtkLookupTable>::New();

  // == check computation of table index ==

  // basic mapping test
  double lo = 3.234;
  double hi = 6.123;
  double tol = 1e-6;
  double step = (hi - lo)/255.0;
  table->SetTableRange(lo,hi);
  table->Build();
  TestAssert(table->GetIndex(lo) == 0);
  TestAssert(table->GetIndex(hi) == 255);
  TestAssert(table->GetIndex(lo+tol) == 0);
  TestAssert(table->GetIndex(hi-tol) == 255);
  TestAssert(table->GetIndex(lo-step) == 0);
  TestAssert(table->GetIndex(hi+step) == 255);
  TestAssert(table->GetIndex(lo+step) == 1);
  TestAssert(table->GetIndex(hi-step) == 254);
  TestAssert(table->GetIndex(vtkMath::Nan()) == -1);

  double rgba[4];
  rgba[0] = 0.0;  rgba[1] = 0.0;  rgba[2] = 0.0;  rgba[3] = 1.0;
  TestVector4d(table->GetBelowRangeColor(), rgba);
  rgba[0] = 1.0;  rgba[1] = 1.0;  rgba[2] = 1.0;  rgba[3] = 1.0;
  TestVector4d(table->GetAboveRangeColor(), rgba);

  TestAssert(table->GetUseBelowRangeColor() == 0);
  TestAssert(table->GetUseAboveRangeColor() == 0);

  unsigned char expected[4], *result;

  // Test handling of below-range colors
  vtkMath::HSVToRGB(table->GetHueRange()[0],
                    table->GetSaturationRange()[0],
                    table->GetValueRange()[0],
                    &rgba[0], &rgba[1], &rgba[2]);
  rgba[3] = table->GetAlphaRange()[0];

  vtkLookupTable::GetColorAsUnsignedChars(rgba, expected);
  table->UseBelowRangeColorOff();
  table->Build();
  result = table->MapValue(lo);
  TestAssert(TestColor4uc(expected, result));

  table->GetBelowRangeColor(rgba);
  vtkLookupTable::GetColorAsUnsignedChars(rgba, expected);
  table->UseBelowRangeColorOn();
  table->Build();
  result = table->MapValue(lo-tol);
  TestAssert(TestColor4uc(expected, result));

  // Test handling of above-range colors
  vtkMath::HSVToRGB(table->GetHueRange()[1],
                    table->GetSaturationRange()[1],
                    table->GetValueRange()[1],
                    &rgba[0], &rgba[1], &rgba[2]);
  rgba[3] = table->GetAlphaRange()[1];

  vtkLookupTable::GetColorAsUnsignedChars(rgba, expected);
  table->UseAboveRangeColorOff();
  table->Build();
  result = table->MapValue(hi);
  TestAssert(TestColor4uc(expected, result));

  table->GetAboveRangeColor(rgba);
  vtkLookupTable::GetColorAsUnsignedChars(rgba, expected);
  table->UseAboveRangeColorOn();
  table->Build();
  result = table->MapValue(hi+tol);
  TestAssert(TestColor4uc(expected, result));

  // log range test
  lo = pow(10.0,lo);
  hi = pow(10.0,hi);
  step = pow(10.0,step);
  table->SetScaleToLog10();
  table->SetTableRange(lo,hi);
  table->Build();
  TestAssert(table->GetIndex(lo) == 0);
  TestAssert(table->GetIndex(hi) == 255);
  TestAssert(table->GetIndex(lo+tol) == 0);
  TestAssert(table->GetIndex(hi-tol) == 255);
  TestAssert(table->GetIndex(vtkMath::Nan()) == -1);

  // Note - both below- and above-range colors are enabled at this point
  TestAssert(table->GetIndex(lo/step) == 0);
  TestAssert(table->GetIndex(hi*step) == 255);
  TestAssert(table->GetIndex(lo*step) == 1);
  TestAssert(table->GetIndex(hi/step) == 254);

  // negative log range
  double tmp = hi;
  hi = -lo;
  lo = -tmp;
  step = 1.0/step;
  table->SetScaleToLog10();
  table->SetTableRange(lo,hi);
  table->Build();
  TestAssert(table->GetIndex(lo) == 0);
  TestAssert(table->GetIndex(hi) == 255);
  TestAssert(table->GetIndex(lo+tol) == 0);
  TestAssert(table->GetIndex(hi-tol) == 255);
  TestAssert(table->GetIndex(lo/step) == 0);
  TestAssert(table->GetIndex(hi*step) == 255);
  TestAssert(table->GetIndex(lo*step) == 1);
  TestAssert(table->GetIndex(hi/step) == 254);

  // == check error reporting ==

  errorObserver observer;
  unsigned long observerId =
    table->AddObserver(
      vtkCommand::ErrorEvent, &observer, &errorObserver::Callback);

  // linear table, range is null, permitted (step function)
  observer.Reset();
  table->SetScaleToLinear();
  table->SetTableRange(0,0);
  TestAssert(observer.GetEvent() == 0);

  // linear table, range is inverted, illegal
  observer.Reset();
  table->SetScaleToLinear();
  table->SetTableRange(1,-1);
  TestAssert(observer.GetEvent() != 0);

  // log table, range is null, permitted (step function)
  observer.Reset();
  table->SetScaleToLog10();
  table->SetTableRange(0,0);
  TestAssert(observer.GetEvent() == 0);

  // log table, zero on upper end of range, permitted
  observer.Reset();
  table->SetScaleToLog10();
  table->SetTableRange(-1.2,0);
  TestAssert(observer.GetEvent() == 0);

  // log table, zero on lower end of range, permitted
  observer.Reset();
  table->SetScaleToLog10();
  table->SetTableRange(0,1.3);
  TestAssert(observer.GetEvent() == 0);

  // log table, range straddles zero, illegal
  observer.Reset();
  table->SetScaleToLog10();
  table->SetTableRange(-0.5,1.1);
  TestAssert(observer.GetEvent() != 0);

  table->RemoveObserver(observerId);

  return rval;
}
