// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Regression test for https://gitlab.kitware.com/vtk/vtk/-/work_items/17013:
// vtkBandedPolyDataContourFilter used to hang (or trip an assertion in debug
// builds) when point scalars contained NaN values. The filter should now
// silently skip any cell that touches a non-finite scalar and produce valid
// output for the remaining cells.

#include "vtkBandedPolyDataContourFilter.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{

// Build a unit square in z=0 split into two triangles, with point scalars
// [s0, s1, s2, s3]. Point ordering: (0,0), (1,0), (1,1), (0,1). The first
// triangle is (0,1,2); the second is (0,2,3).
vtkSmartPointer<vtkPolyData> MakeTwoTrianglePolyData(double s0, double s1, double s2, double s3)
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);

  vtkNew<vtkCellArray> tris;
  vtkIdType t0[3] = { 0, 1, 2 };
  vtkIdType t1[3] = { 0, 2, 3 };
  tris->InsertNextCell(3, t0);
  tris->InsertNextCell(3, t1);

  vtkNew<vtkFloatArray> scalars;
  scalars->SetName("s");
  scalars->SetNumberOfTuples(4);
  scalars->SetValue(0, static_cast<float>(s0));
  scalars->SetValue(1, static_cast<float>(s1));
  scalars->SetValue(2, static_cast<float>(s2));
  scalars->SetValue(3, static_cast<float>(s3));

  vtkNew<vtkPolyData> pd;
  pd->SetPoints(points);
  pd->SetPolys(tris);
  pd->GetPointData()->SetScalars(scalars);
  return pd;
}

int RunFilter(vtkPolyData* input, vtkPolyData* output)
{
  vtkNew<vtkBandedPolyDataContourFilter> bpc;
  bpc->SetInputData(input);
  bpc->GenerateValues(5, 0.0, 2.0);
  bpc->SetScalarModeToValue();
  bpc->SetClipping(0);
  bpc->Update();
  output->ShallowCopy(bpc->GetOutput());
  return 0;
}

} // namespace

int UnitTestBandedPolyDataContourFilterNaN(int, char*[])
{
  int status = 0;
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();

  // 1. Baseline: no NaN. Must produce non-empty output.
  {
    vtkSmartPointer<vtkPolyData> pd = ::MakeTwoTrianglePolyData(0.0, 1.0, 2.0, 0.5);
    vtkNew<vtkPolyData> out;
    ::RunFilter(pd, out);
    if (out->GetNumberOfPolys() == 0)
    {
      std::cerr << "FAIL: baseline (no NaN) produced empty output.\n";
      ++status;
    }
  }

  // 2. One NaN scalar at point 3. The triangle (0,2,3) touches it and must be
  // skipped; the triangle (0,1,2) must still be contoured into >= 1 band.
  {
    vtkSmartPointer<vtkPolyData> pd = ::MakeTwoTrianglePolyData(0.0, 1.0, 2.0, nan);
    vtkNew<vtkPolyData> out;
    ::RunFilter(pd, out);
    if (out->GetNumberOfPolys() < 1)
    {
      std::cerr << "FAIL: NaN at one point produced no output polys.\n";
      ++status;
    }
    // Output scalars must all be finite.
    auto* outScalars = vtkFloatArray::SafeDownCast(out->GetPointData()->GetScalars());
    if (outScalars)
    {
      for (vtkIdType i = 0; i < outScalars->GetNumberOfTuples(); ++i)
      {
        if (!std::isfinite(outScalars->GetValue(i)))
        {
          std::cerr << "FAIL: output point scalar " << i << " is non-finite.\n";
          ++status;
          break;
        }
      }
    }
  }

  // 3. +Inf scalar: same expectation as NaN — touched cell skipped.
  {
    vtkSmartPointer<vtkPolyData> pd = ::MakeTwoTrianglePolyData(0.0, 1.0, 2.0, inf);
    vtkNew<vtkPolyData> out;
    ::RunFilter(pd, out);
    if (out->GetNumberOfPolys() < 1)
    {
      std::cerr << "FAIL: +Inf at one point produced no output polys.\n";
      ++status;
    }
  }

  // 4. All scalars NaN: filter must return cleanly with no output polys, not
  // hang and not crash. The filter intentionally emits an error in this case,
  // so silence it to keep the test harness from flagging the expected output.
  {
    vtkSmartPointer<vtkPolyData> pd = ::MakeTwoTrianglePolyData(nan, nan, nan, nan);
    vtkNew<vtkPolyData> out;
    vtkObject::GlobalWarningDisplayOff();
    ::RunFilter(pd, out);
    vtkObject::GlobalWarningDisplayOn();
    if (out->GetNumberOfPolys() != 0)
    {
      std::cerr << "FAIL: all-NaN input produced " << out->GetNumberOfPolys()
                << " polys, expected 0.\n";
      ++status;
    }
  }

  return status;
}
