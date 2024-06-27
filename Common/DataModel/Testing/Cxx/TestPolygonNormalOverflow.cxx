// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"

#include <type_traits>

// Test ComputeNormal for very large and very small polygons.
int TestPolygonNormalOverflow(int, char*[])
{
  int returnValue = EXIT_SUCCESS;

  const float scales[] = {
    1.0,   // for baseline
    1e+30, // for overflow test
    1e-30, // for underflow test
  };
  const int nscales = sizeof(scales) / sizeof(*scales);

  const float coords[] = {
    0.1, 0.1, 0.0, // 0
    0.9, 0.0, 0.0, // 1
    1.1, 1.1, 0.0, // 2
    0.0, 1.2, 0.0, // 3
  };
  const vtkIdType npoints = sizeof(coords) / (3 * sizeof(*coords));

  double baseline[3]{};
  vtkNew<vtkIdTypeArray> ids;
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_FLOAT);

  for (int j = 0; j < nscales; j++)
  {
    points->SetNumberOfPoints(npoints);
    ids->SetNumberOfValues(npoints);
    for (vtkIdType i = 0; i < npoints; i++)
    {
      float point[3];
      vtkMath::Assign(&coords[3 * i], point);
      vtkMath::MultiplyScalar(point, scales[j]);
      points->SetPoint(i, point);
      ids->SetValue(i, i);
    }

    double normal[3];
    vtkPolygon::ComputeNormal(ids, points, normal);
    if (j == 0)
    {
      // save the baseline normal
      vtkMath::Assign(normal, baseline);
    }
    else
    {
      // compare large, small polygon normal against baseline
      bool good = true;
      for (int k = 0; k < 3; k++)
      {
        // use float epsilon to check, since polygon point type is float
        double rtol = std::numeric_limits<float>::epsilon();
        good &= vtkMathUtilities::NearlyEqual(normal[k], baseline[k], rtol);
      }
      if (!good)
      {
        std::cerr << "ERROR: Bad normal for polygon scale " << scales[j] << ", expected ";
        std::cerr << "(" << baseline[0] << "," << baseline[1] << "," << baseline[2] << "), got ";
        std::cerr << "(" << normal[0] << "," << normal[1] << "," << normal[2] << ")." << std::endl;
        returnValue = EXIT_FAILURE;
      }
    }
  }

  return returnValue;
}
