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
  TestAssert(table->GetIndex(lo) == 0);
  TestAssert(table->GetIndex(hi) == 255);
  TestAssert(table->GetIndex(lo+tol) == 0);
  TestAssert(table->GetIndex(hi-tol) == 255);
  TestAssert(table->GetIndex(lo-step) == 0);
  TestAssert(table->GetIndex(hi+step) == 255);
  TestAssert(table->GetIndex(lo+step) == 1);
  TestAssert(table->GetIndex(hi-step) == 254);

  // log range test
  lo = pow(10.0,lo);
  hi = pow(10.0,hi);
  step = pow(10.0,step);
  table->SetScaleToLog10();
  table->SetTableRange(lo,hi);
  TestAssert(table->GetIndex(lo) == 0);
  TestAssert(table->GetIndex(hi) == 255);
  TestAssert(table->GetIndex(lo+tol) == 0);
  TestAssert(table->GetIndex(hi-tol) == 255);
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
