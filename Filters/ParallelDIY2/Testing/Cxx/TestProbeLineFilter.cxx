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
#include "vtkDummyController.h"
#include "vtkLineSource.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkProbeLineFilter.h"
#include "vtkRTAnalyticSource.h"

#include <algorithm>
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

  vtkNew<vtkLineSource> line;
  line->SetPoint1(-10, -10, -10);
  line->SetPoint2(10, 10, 10);
  line->SetResolution(1);
  line->Update();
  vtkNew<vtkProbeLineFilter> probeLine;
  probeLine->SetInputConnection(point2cell->GetOutputPort());
  probeLine->SetSourceConnection(line->GetOutputPort());
  probeLine->SetController(contr);
  probeLine->SetLineResolution(10);

  auto CheckForErrors = [myrank](vtkPolyData* pd, const double* expected, vtkIdType size,
                          const char* name) {
    // Ignore rank != 0 as all results are passed to rank 0
    if (myrank != 0)
    {
      return EXIT_SUCCESS;
    }

    if (pd == nullptr)
    {
      vtkLog(ERROR, << "Wrong output type");
      return EXIT_FAILURE;
    }

    vtkDataArray* result = pd->GetPointData()->GetArray("RTData");
    if (!result)
    {
      vtkLog(ERROR, << name << ": missing 'RTData' array");
      return EXIT_FAILURE;
    }

    vtkIdType minsize = std::min(size, result->GetNumberOfTuples());
    if (result->GetNumberOfValues() != size)
    {
      vtkLog(ERROR, << name << ": result and expected result does not have the same size (resp. "
                    << result->GetNumberOfValues() << " vs " << size
                    << " values). Still checking the " << minsize << " first elements ...");
    }

    int code = EXIT_SUCCESS;
    for (vtkIdType pointId = 0; pointId < minsize; ++pointId)
    {
      if (std::fabs(result->GetTuple1(pointId) - expected[pointId]) > 0.001)
      {
        vtkLog(ERROR, << name << " failed at " << pointId);
        code = EXIT_FAILURE;
      }
    }
    return code;
  };

  // Check result for polyData output
  vtkLog(INFO, << "Testing vtkProbeLineFilter with polydata output");
  probeLine->AggregateAsPolyDataOn();
  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES);
  probeLine->Update();
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingAtCellBoundaries.data(), ProbingAtCellBoundaries.size(),
    "SAMPLE_LINE_AT_CELL_BOUNDARIES");

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS);
  probeLine->Update();
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingAtSegmentCenters.data(), ProbingAtSegmentCenters.size(),
    "SAMPLE_LINE_AT_SEGMENT_CENTERS");

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY);
  probeLine->Update();
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingUniformly.data(), ProbingUniformly.size(), "SAMPLE_LINE_UNIFORMLY");

  // Check result for multiblock ouput
  vtkLog(INFO, << "Testing vtkProbeLineFilter with multiblock output");
  probeLine->AggregateAsPolyDataOff();
  vtkMultiBlockDataSet* mbds;
  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES);
  probeLine->Update();
  mbds = vtkMultiBlockDataSet::SafeDownCast(probeLine->GetOutputDataObject(0));
  if (mbds == nullptr)
  {
    vtkLog(ERROR, << "Expecting a multiblock output, found something else");
    return EXIT_FAILURE;
  }
  if (mbds->GetNumberOfBlocks() != 1)
  {
    vtkLog(ERROR, "Wrong number of blocks in the output");
    return EXIT_FAILURE;
  }
  retVal |=
    CheckForErrors(vtkPolyData::SafeDownCast(mbds->GetBlock(0)), ProbingAtCellBoundaries.data(),
      ProbingAtCellBoundaries.size(), "SAMPLE_LINE_AT_CELL_BOUNDARIES");

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS);
  probeLine->Update();
  mbds = vtkMultiBlockDataSet::SafeDownCast(probeLine->GetOutputDataObject(0));
  if (mbds == nullptr)
  {
    vtkLog(ERROR, << "Expecting a multiblock output, found something else");
    return EXIT_FAILURE;
  }
  if (mbds->GetNumberOfBlocks() != 1)
  {
    vtkLog(ERROR, "Wrong number of blocks in the output");
    return EXIT_FAILURE;
  }
  retVal |=
    CheckForErrors(vtkPolyData::SafeDownCast(mbds->GetBlock(0)), ProbingAtSegmentCenters.data(),
      ProbingAtSegmentCenters.size(), "SAMPLE_LINE_AT_SEGMENT_CENTERS");

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY);
  probeLine->Update();
  mbds = vtkMultiBlockDataSet::SafeDownCast(probeLine->GetOutputDataObject(0));
  if (mbds == nullptr)
  {
    vtkLog(ERROR, << "Expecting a multiblock output, found something else");
    return EXIT_FAILURE;
  }
  if (mbds->GetNumberOfBlocks() != 1)
  {
    vtkLog(ERROR, "Wrong number of blocks in the output");
    return EXIT_FAILURE;
  }
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(mbds->GetBlock(0)), ProbingUniformly.data(),
    ProbingUniformly.size(), "SAMPLE_LINE_UNIFORMLY");

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}
