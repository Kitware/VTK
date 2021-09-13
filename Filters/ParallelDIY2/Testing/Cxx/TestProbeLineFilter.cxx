/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProbeLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkDataArray.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkProbeLineFilter.h"
#include "vtkRTAnalyticSource.h"

#include <array>

static constexpr std::array<double, 40> ProbingAtCellBoundaries = { 10.3309, 10.3309, 1.68499,
  1.68499, -8.10485, -8.10485, 3.09254, 3.09254, 26.3884, 26.3884, 49.8718, 49.8718, 77.2904,
  77.2904, 136.737, 136.737, 211.899, 211.899, 255.795, 255.795, 236.429, 236.429, 192.787, 192.787,
  150.466, 150.466, 101.16, 101.16, 36.8801, 36.8801, 3.09331, 3.09331, 10.6523, 10.6523, 18.0772,
  18.0772, -3.63279, -3.63279, -15.5258, -15.5258 };

static constexpr std::array<double, 20> ProbingAtSegmentCenters = { 10.3309, 1.68499, -8.10485,
  3.09254, 26.3884, 49.8718, 77.2904, 136.737, 211.899, 255.795, 236.429, 192.787, 150.466, 101.16,
  36.8801, 3.09331, 10.6523, 18.0772, -3.63279, -15.5258 };

static constexpr std::array<double, 11> ProbingUniformly = { 10.3309, -8.10485, 26.3884, 77.2904,
  211.899, 236.429, 150.466, 36.8801, 10.6523, -3.63279, -15.5258 };

int TestProbeLineFilter(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif

  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int myrank = contr->GetLocalProcessId();

  int retVal = EXIT_SUCCESS;

  vtkNew<vtkRTAnalyticSource> wavelet1;
  vtkNew<vtkRTAnalyticSource> wavelet2;
  if (myrank == 0)
  {
    wavelet1->SetWholeExtent(-10, 10, -10, 10, -10, -5);
    wavelet2->SetWholeExtent(-10, 10, -10, 10, -5, 0);
  }
  else if (myrank == 1)
  {
    wavelet1->SetWholeExtent(-10, 10, -10, 10, 0, 5);
    wavelet2->SetWholeExtent(-10, 10, -10, 10, 5, 10);
  }

  wavelet1->Update();
  wavelet2->Update();

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(2);
  pds->SetPartition(0, wavelet1->GetOutputDataObject(0));
  pds->SetPartition(1, wavelet2->GetOutputDataObject(0));

  vtkNew<vtkPointDataToCellData> point2cell;
  point2cell->SetInputData(pds);

  vtkNew<vtkProbeLineFilter> probeLine;
  probeLine->SetInputConnection(point2cell->GetOutputPort());
  probeLine->SetController(contr);
  probeLine->SetPoint1(-10, -10, -10);
  probeLine->SetPoint2(10, 10, 10);
  probeLine->SetLineResolution(10);

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES);
  probeLine->Update();

  vtkPolyData* pd = vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0));
  vtkDataArray* array = pd->GetPointData()->GetArray("RTData");

  for (vtkIdType pointId = 0; pointId < pd->GetNumberOfPoints(); ++pointId)
  {
    if (std::fabs(array->GetTuple1(pointId) - ProbingAtCellBoundaries[pointId]) > 0.001)
    {
      vtkLog(ERROR, "Failed to probe line with SAMPLE_LINE_AT_CELL_BOUNDARIES at " << pointId);
      retVal = EXIT_FAILURE;
    }
  }

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS);
  probeLine->Update();

  pd = vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0));
  array = pd->GetPointData()->GetArray("RTData");
  for (vtkIdType pointId = 0; pointId < pd->GetNumberOfPoints(); ++pointId)
  {
    if (std::fabs(array->GetTuple1(pointId) - ProbingAtSegmentCenters[pointId]) > 0.001)
    {
      vtkLog(ERROR, "Failed to probe line with SAMPLE_LINE_AT_SEGMENT_CENTERS at " << pointId);
      retVal = EXIT_FAILURE;
    }
  }

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY);
  probeLine->Update();

  pd = vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0));
  array = pd->GetPointData()->GetArray("RTData");
  for (vtkIdType pointId = 0; pointId < pd->GetNumberOfPoints(); ++pointId)
  {
    if (std::fabs(array->GetTuple1(pointId) - ProbingUniformly[pointId]) > 0.001)
    {
      vtkLog(ERROR, "Failed to probe line with SAMPLE_LINE_UNIFORMLY at " << pointId);
      retVal = EXIT_FAILURE;
    }
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}
