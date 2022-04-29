/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAlignImageDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
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

bool Validate(vtkDataObject* dobj, vtkVector3d origin)
{
  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  auto images = vtkCompositeDataSet::GetDataSets<vtkImageData>(dobj);
  int piece = 0;
  for (auto& image : images)
  {
    if (vtkVector3d(image->GetOrigin()) != origin)
    {
      vtkLogF(ERROR, "Incorrect origin!");
      return false;
    }

    source->UpdatePiece(piece, NUM_PIECES, NUM_GHOSTS);
    auto* input = vtkImageData::SafeDownCast(source->GetOutputDataObject(0));
    const vtkVector3d p0(input->GetPoint(0));
    const vtkVector3d p1(image->GetPoint(0));
    if (p0 != p1)
    {
      vtkLogF(ERROR, "Incorrect point 0 (%f, %f, %f) != (%f, %f, %f)", p0[0], p0[1], p0[2], p1[0],
        p1[1], p1[2]);
      return false;
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

  return EXIT_SUCCESS;
}
