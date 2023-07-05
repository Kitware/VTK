// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGenerateGlobalIds.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkVector.h"

#include <cmath>

namespace
{
vtkSmartPointer<vtkPolyData> GetHemisphere(int piece, int numPieces)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->UpdatePiece(piece, numPieces, 0);
  return sphere->GetOutput();
}

vtkVector2d GetRange(vtkDataObject* dobj)
{
  vtkVector2d range{ VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  for (auto& ds : vtkCompositeDataSet::GetDataSets<vtkDataSet>(dobj))
  {
    if (auto array = ds->GetPointData()->GetArray("GlobalPointIds"))
    {
      vtkVector2d crange;
      array->GetRange(crange.GetData(), 0);
      range[0] = std::min(range[0], crange[0]);
      range[1] = std::max(range[1], crange[1]);
    }
  }
  return range;
}
}

int TestGenerateGlobalIdsSphere(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkPartitionedDataSet> pd;
  pd->SetPartition(0, GetHemisphere(0, 2));
  pd->SetPartition(1, GetHemisphere(1, 2));

  vtkNew<vtkGenerateGlobalIds> gids;
  gids->SetInputDataObject(pd);
  gids->Update();

  const vtkVector2d range0 = ::GetRange(gids->GetOutputDataObject(0));
  vtkLogF(INFO, "tolerance = 0.0, range(%lf, %lf)", range0[0], range0[1]);

  gids->SetTolerance(0.0001);
  gids->Update();

  const vtkVector2d range1 = ::GetRange(gids->GetOutputDataObject(0));
  vtkLogF(INFO, "tolerance = 0.0001, range(%lf, %lf)", range1[0], range1[1]);
  return range0[1] == 55 && range1[1] == 49 ? EXIT_SUCCESS : EXIT_FAILURE;
}
