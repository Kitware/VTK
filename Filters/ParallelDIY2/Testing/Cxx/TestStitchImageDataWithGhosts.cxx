// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStitchImageDataWithGhosts.h"

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStructuredData.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

int TestStitchImageDataWithGhosts(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif

  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  vtkNew<vtkRTAnalyticSource> wavelet1;
  wavelet1->SetWholeExtent(-10, 10, -10, 10, 0, 0);
  wavelet1->Update();

  vtkNew<vtkRTAnalyticSource> wavelet2;
  wavelet2->SetWholeExtent(11, 20, -10, 10, 0, 0);
  wavelet2->Update();

  vtkNew<vtkPartitionedDataSet> images;
  images->SetNumberOfPartitions(2);
  images->SetPartition(0, wavelet1->GetOutputDataObject(0));
  images->SetPartition(1, wavelet2->GetOutputDataObject(0));

  vtkNew<vtkStitchImageDataWithGhosts> stitcher;
  stitcher->SetInputData(images);
  stitcher->Update();

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();

  auto out = vtkPartitionedDataSet::SafeDownCast(stitcher->GetOutputDataObject(0));

  auto im1 = vtkImageData::SafeDownCast(out->GetPartition(0));
  auto im2 = vtkImageData::SafeDownCast(out->GetPartition(1));
  if (im1->GetNumberOfCells() != 20 * 21)
  {
    vtkLog(ERROR, "Images not stitched properly.");
    return EXIT_FAILURE;
  }

  vtkFloatArray* data1 =
    vtkArrayDownCast<vtkFloatArray>(im1->GetPointData()->GetAbstractArray("RTData"));
  vtkFloatArray* data2 =
    vtkArrayDownCast<vtkFloatArray>(im2->GetPointData()->GetAbstractArray("RTData"));

  if (!data1 || !data2)
  {
    vtkLog(ERROR, "data array absent from output.");
    return EXIT_FAILURE;
  }

  const int* e1 = im1->GetExtent();
  const int* e2 = im2->GetExtent();
  int ijk[3] = { 11, 0, 0 };
  for (ijk[1] = -10; ijk[1] <= 10; ++ijk[1])
  {
    vtkIdType pointId1 = vtkStructuredData::ComputePointIdForExtent(e1, ijk);
    vtkIdType pointId2 = vtkStructuredData::ComputePointIdForExtent(e2, ijk);
    if (data1->GetValue(pointId1) != data2->GetValue(pointId2))
    {
      vtkLog(ERROR, "Ghost data not exchanged correctly.");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
