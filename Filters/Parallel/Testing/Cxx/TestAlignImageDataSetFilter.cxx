// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAlignImageDataSetFilter.h"
#include "vtkBoundingBox.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkRTAnalyticSource.h"
#include "vtkVector.h"

namespace
{
constexpr int NUM_PIECES = 6;
constexpr int NUM_GHOSTS = 1;

bool Validate(vtkDataObject* dobj, vtkVector3d origin, bool checkCoord = true)
{
  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  auto images = vtkCompositeDataSet::GetDataSets<vtkImageData>(dobj);
  int piece = 0;
  for (auto& image : images)
  {
    if (vtkVector3d(image->GetOrigin()) != origin)
    {
      double outputOrigin[3];
      image->GetOrigin(outputOrigin);
      vtkLogF(ERROR, "Incorrect origin (%f, %f, %f) != (%f, %f, %f) for piece %d!", outputOrigin[0],
        outputOrigin[1], outputOrigin[2], origin[0], origin[1], origin[2], piece);
      return false;
    }

    if (checkCoord)
    {
      source->UpdatePiece(piece, NUM_PIECES, NUM_GHOSTS);
      auto* input = vtkImageData::SafeDownCast(source->GetOutputDataObject(0));
      const vtkVector3d p0(input->GetPoint(0));
      const vtkVector3d p1(image->GetPoint(0));
      if (p0 != p1)
      {
        vtkLogF(ERROR, "Incorrect point 0 (%f, %f, %f) != (%f, %f, %f) for piece %d", p0[0], p0[1],
          p0[2], p1[0], p1[1], p1[2], piece);
        return false;
      }
    }

    ++piece;
  }

  return true;
}

}

int TestAlignImageDataSetFilter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkPartitionedDataSet> pd;

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  for (int cc = 0; cc < NUM_PIECES; ++cc)
  {
    source->UpdatePiece(cc, NUM_PIECES, NUM_GHOSTS);
    auto* img = vtkImageData::SafeDownCast(source->GetOutputDataObject(0));

    // Create a clone with extent (0, x, 0, y, 0, z). Thus each image has
    // a different origin, but all images still form a single dataset in 3D
    // space.
    vtkNew<vtkImageData> clone;
    clone->ShallowCopy(img);

    vtkBoundingBox bbox(clone->GetBounds());
    vtkVector3d origin(bbox.GetMinPoint());
    vtkVector3i dims(clone->GetDimensions());

    clone->SetOrigin(origin.GetData());
    clone->SetExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
    pd->SetPartition(cc, clone);
  }

  vtkNew<vtkAlignImageDataSetFilter> aligner;
  aligner->SetInputDataObject(pd);
  aligner->Update();
  vtkLogIf(ERROR, !Validate(aligner->GetOutputDataObject(0), vtkVector3d(-10, -10, -10)),
    "Failed case #0 (MinimumExtent=default)");

  aligner->SetMinimumExtent(10, 10, 10);
  aligner->Update();
  vtkLogIf(ERROR, !Validate(aligner->GetOutputDataObject(0), vtkVector3d(-20, -20, -20)),
    "Failed case #2 (MinimumExtent=[10, 10, 10])");

  aligner->SetMinimumExtent(-10, -10, 10);
  aligner->Update();
  vtkLogIf(ERROR, !Validate(aligner->GetOutputDataObject(0), vtkVector3d(0, 0, -20)),
    "Failed case #3 (MinimumExtent=[-10, -10, 10])");

  // case set up that ParaView issue #21285 fails on:
  // https://gitlab.kitware.com/paraview/paraview/-/issues/21285
  pd->Initialize();
  source->UpdatePiece(0, 1, 0);
  auto* img = vtkImageData::SafeDownCast(source->GetOutputDataObject(0));
  vtkNew<vtkImageData> clone0;
  clone0->ShallowCopy(img);
  pd->SetPartition(0, clone0);

  vtkNew<vtkImageData> clone1;
  clone1->ShallowCopy(img);
  clone1->SetOrigin(20, 0, 0);
  pd->SetPartition(1, clone1);
  aligner->SetInputDataObject(pd);
  aligner->SetMinimumExtent(0, 0, 0);
  aligner->Update();
  // we don't check the location of the points since we didn't use pieces from the source to set
  // up the input to the filter
  vtkLogIf(ERROR, !Validate(aligner->GetOutputDataObject(0), vtkVector3d(-10, -10, -10), false),
    "Failed case #4 (MinimumExtent=default)");

  return EXIT_SUCCESS;
}
