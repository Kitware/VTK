// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArrayRange.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPixel.h"
#include "vtkPoints.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkVoxel.h"

int TestCellInflation(int, char*[])
{
  {
    vtkNew<vtkTriangle> triangle;
    auto pointRange = vtk::DataArrayTupleRange<3>(triangle->Points->GetData());
    using TupleRef = decltype(pointRange)::TupleReferenceType;
    TupleRef p0 = pointRange[0], p1 = pointRange[1], p2 = pointRange[2];
    p0[0] = 0.0;
    p0[1] = 0.0;
    p0[2] = 0.0;
    p1[0] = 0.0;
    p1[1] = 1.0;
    p1[2] = 0.0;
    p2[0] = 1.0;
    p2[1] = 0.0;
    p2[2] = 0.0;
    triangle->Inflate(0.5);
    if (!vtkMathUtilities::NearlyEqual<double>(p0[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p0[1], -0.5) || std::abs(p0[2]) > VTK_DBL_EPSILON ||
      !vtkMathUtilities::NearlyEqual<double>(p1[0], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[1], 1.5 + 1.0 / std::sqrt(2.0)) ||
      std::abs(p1[2]) > VTK_DBL_EPSILON ||
      !vtkMathUtilities::NearlyEqual<double>(p2[0], 1.5 + 1.0 / std::sqrt(2.0)) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[1], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      std::abs(p2[2]) > VTK_DBL_EPSILON)
    {
      std::cerr << "Inflating triangle failed" << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkTetra> tetra;
    auto pointRange = vtk::DataArrayTupleRange<3>(tetra->Points->GetData());
    using TupleRef = decltype(pointRange)::TupleReferenceType;
    TupleRef p0 = pointRange[0], p1 = pointRange[1], p2 = pointRange[2], p3 = pointRange[3];
    p0[0] = 0.0;
    p0[1] = 0.0;
    p0[2] = 0.0;
    p1[0] = 0.0;
    p1[1] = 1.0;
    p1[2] = 0.0;
    p2[0] = 1.0;
    p2[1] = 0.0;
    p2[2] = 0.0;
    p3[0] = 0.0;
    p3[1] = 0.0;
    p3[2] = 1.0;
    tetra->Inflate(0.5);
    if (!vtkMathUtilities::NearlyEqual<double>(p0[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p0[1], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p0[2], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[0], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[1], 2.0 + 0.5 * std::sqrt(3.0)) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[2], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[0], 2.0 + 0.5 * std::sqrt(3.0)) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[1], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[2], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      !vtkMathUtilities::NearlyEqual<double>(p3[0], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      !vtkMathUtilities::NearlyEqual<double>(p3[1], -0.5, 2.0 * VTK_DBL_EPSILON) ||
      !vtkMathUtilities::NearlyEqual<double>(p3[2], 2.0 + 0.5 * std::sqrt(3.0)))
    {
      std::cerr << "Inflating tetra failed" << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkPixel> pixel;
    auto pointRange = vtk::DataArrayTupleRange<3>(pixel->Points->GetData());
    using TupleRef = decltype(pointRange)::TupleReferenceType;
    TupleRef p0 = pointRange[0], p1 = pointRange[1], p2 = pointRange[2], p3 = pointRange[3];
    p0[0] = 0.0;
    p0[1] = 0.0;
    p0[2] = 0.0;
    p1[0] = 1.0;
    p1[1] = 0.0;
    p1[2] = 0.0;
    p2[0] = 0.0;
    p2[1] = 1.0;
    p2[2] = 0.0;
    p3[0] = 1.0;
    p3[1] = 1.0;
    p3[2] = 0.0;
    pixel->Inflate(0.5);
    if (!vtkMathUtilities::NearlyEqual<double>(p0[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p0[1], -0.5) || std::abs(p0[2]) > VTK_DBL_EPSILON ||
      !vtkMathUtilities::NearlyEqual<double>(p1[0], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[1], -0.5) || std::abs(p1[2]) > VTK_DBL_EPSILON ||
      !vtkMathUtilities::NearlyEqual<double>(p2[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[1], 1.5) || std::abs(p2[2]) > VTK_DBL_EPSILON ||
      !vtkMathUtilities::NearlyEqual<double>(p3[0], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p3[1], 1.5) || std::abs(p3[2]) > VTK_DBL_EPSILON)
    {
      std::cerr << "Inflating pixel failed" << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkVoxel> voxel;
    auto pointRange = vtk::DataArrayTupleRange<3>(voxel->Points->GetData());
    using TupleRef = decltype(pointRange)::TupleReferenceType;
    TupleRef p0 = pointRange[0], p1 = pointRange[1], p2 = pointRange[2], p3 = pointRange[3];
    TupleRef p4 = pointRange[4], p5 = pointRange[5], p6 = pointRange[6], p7 = pointRange[7];
    p0[0] = 0.0;
    p0[1] = 0.0;
    p0[2] = 0.0;
    p1[0] = 1.0;
    p1[1] = 0.0;
    p1[2] = 0.0;
    p2[0] = 0.0;
    p2[1] = 1.0;
    p2[2] = 0.0;
    p3[0] = 1.0;
    p3[1] = 1.0;
    p3[2] = 0.0;
    p4[0] = 0.0;
    p4[1] = 0.0;
    p4[2] = 1.0;
    p5[0] = 1.0;
    p5[1] = 0.0;
    p5[2] = 1.0;
    p6[0] = 0.0;
    p6[1] = 1.0;
    p6[2] = 1.0;
    p7[0] = 1.0;
    p7[1] = 1.0;
    p7[2] = 1.0;
    voxel->Inflate(0.5);
    if (!vtkMathUtilities::NearlyEqual<double>(p0[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p0[1], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p0[2], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[0], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[1], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p1[2], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[1], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p2[2], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p3[0], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p3[1], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p3[2], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p4[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p4[1], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p4[2], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p5[0], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p5[1], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p5[2], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p6[0], -0.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p6[1], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p6[2], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p7[0], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p7[1], 1.5) ||
      !vtkMathUtilities::NearlyEqual<double>(p7[2], 1.5))
    {
      std::cerr << "Inflating voxel failed" << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkLine> line;
    auto pointRange = vtk::DataArrayTupleRange<3>(line->Points->GetData());
    using TupleRef = decltype(pointRange)::TupleReferenceType;
    TupleRef p0 = pointRange[0], p1 = pointRange[1];
    p0[0] = 0.0;
    p0[1] = 0.0;
    p0[2] = 0.0;
    p1[0] = 1.0;
    p1[1] = 0.0;
    p1[2] = 0.0;
    line->Inflate(0.5);
    if (!vtkMathUtilities::NearlyEqual<double>(p0[0], -0.5) || p0[1] != 0.0 || p0[2] != 0.0 ||
      !vtkMathUtilities::NearlyEqual<double>(p1[0], 1.5) || p1[1] != 0.0 || p1[2] != 0.0)
    {
      std::cerr << "Inflating line failed" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
