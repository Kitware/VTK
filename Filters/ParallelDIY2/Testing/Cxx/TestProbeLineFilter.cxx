// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCutter.h"
#include "vtkDataArray.h"
#include "vtkDummyController.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkLineSource.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPlane.h"
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

static constexpr std::array<double, 22> ProbingAtSegmentCenters = { 10.3309, 10.3309, 1.68499,
  -8.10485, 3.09254, 26.3884, 49.8718, 77.2904, 136.737, 211.899, 255.795, 236.429, 192.787,
  150.466, 101.16, 36.8801, 3.09331, 10.6523, 18.0772, -3.63279, -15.5258, -15.5258 };

static constexpr std::array<double, 11> ProbingUniformly = { 10.3309, -8.10485, 26.3884, 77.2904,
  211.899, 236.429, 150.466, 36.8801, 10.6523, -3.63279, -15.5258 };

static constexpr std::array<double, 40> ProbingAtCellBoundaries_2D = { 0, 0.9999, 0.9999, 2, 2, 3,
  3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17,
  17, 18, 18, 19.0001, 19.0001, 19.1 };

static const std::array<double, 22> ProbingAtSegmentCenters_2D = { 0, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5,
  6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5, 16.5, 17.5, 18.5, 19.05, 19.1 };

static constexpr double eps = 1e-6;

// ----------------------------------------------------------------------------
int CheckForErrors(vtkPolyData* pd, const double* expected, vtkIdType size, const char* arrayName,
  const char* samplingName, int rank)
{
  // Ignore rank != 0 as all results are passed to rank 0
  if (rank != 0)
  {
    return EXIT_SUCCESS;
  }

  if (pd == nullptr)
  {
    vtkLog(ERROR, << "Wrong output type");
    return EXIT_FAILURE;
  }

  vtkDataArray* result = pd->GetPointData()->GetArray(arrayName);
  if (!result)
  {
    vtkLog(ERROR, << samplingName << ": missing '" << arrayName << "' data array");
    return EXIT_FAILURE;
  }

  vtkIdType minsize = std::min(size, result->GetNumberOfTuples());
  int code = EXIT_SUCCESS;
  if (result->GetNumberOfValues() != size)
  {
    vtkLog(ERROR, << samplingName
                  << ": result and expected result does not have the same size (resp. "
                  << result->GetNumberOfValues() << " vs " << size << " values).");
    code = EXIT_FAILURE;
  }

  for (vtkIdType pointId = 0; pointId < minsize; ++pointId)
  {
    if (std::fabs(result->GetTuple1(pointId) - expected[pointId]) > 0.001)
    {
      vtkLog(ERROR, << samplingName << "'s wrong starting: " << pointId);
      code = EXIT_FAILURE;
      break;
    }
  }

  if (code == EXIT_FAILURE)
  {
    std::cerr << "Expected: [ ";
    for (vtkIdType i = 0; i < size; ++i)
      std::cerr << expected[i] << " ";
    std::cerr << "]" << std::endl;
    std::cerr << "But got : [ ";
    for (vtkIdType i = 0; i < result->GetNumberOfValues(); ++i)
      std::cerr << result->GetTuple1(i) << " ";
    std::cerr << "]" << std::endl;
  }

  return code;
}

// ----------------------------------------------------------------------------
int Check2DHTG(vtkMultiProcessController* contr, vtkDataSet* outDataSet)
{
  int numPoints = outDataSet->GetNumberOfPoints();
  vtkDataArray* da = outDataSet->GetPointData()->GetArray("Depth");
  int retVal = EXIT_SUCCESS;
  if (contr->GetLocalProcessId() == 0)
  {
    for (int i = 0; i < numPoints; i++)
    {
      std::array<double, 3> pt{ 0.0, 0.0, 0.0 };
      outDataSet->GetPoint(i, pt.data());
      int foundDepth = -1;
      bool isDemarcation = false;
      if (std::abs(pt[0]) < 1.0 && std::abs(pt[1]) < 1.0)
      {
        foundDepth = 0.0;
      }
      else if ((std::abs(pt[0]) - 1.0) < eps && (std::abs(pt[1]) - 1.0) < eps)
      {
        foundDepth = 0.0;
        if (vtkMath::IsNan(da->GetComponent(i, 0)))
        {
          continue;
        }
      }
      for (int d = 0; d < 4; d++) // iterate over levels
      {
        double demarcation = 0.5 / std::pow(2, d);
        if (std::abs(pt[0]) < demarcation && std::abs(pt[1]) < demarcation)
        {
          foundDepth = d;
          continue;
        }
        else if ((std::abs(pt[0]) - demarcation) < eps && (std::abs(pt[1]) - demarcation) < eps)
        {
          isDemarcation = true;
        }
        else
        {
          break;
        }
      }
      /*
       * Check here if the depth array has the correct value computed above specifically for this
       * HTG Slight indetermination due to the INTERSECT_WITH_CELLS mode for points which are
       * exactly in between two depth zones. The second part of the check takes care of that.
       */
      if (!((foundDepth - da->GetComponent(i, 0) < 1) ||
            (isDemarcation && ((foundDepth - da->GetComponent(i, 0) + 1) < 1))))
      {
        vtkLog(ERROR, << "Probe Line on HTG 2D failed for point " << pt[0] << ", " << pt[1]
                      << " with depth " << da->GetComponent(i, 0) << " when it should be "
                      << foundDepth);
        retVal = EXIT_FAILURE;
        break;
      }
    }
  }
  return retVal;
}

// ----------------------------------------------------------------------------
int Check3DHTG(vtkMultiProcessController* contr, vtkDataSet* outDataSet)
{
  int numPoints = outDataSet->GetNumberOfPoints();
  vtkDataArray* da = outDataSet->GetPointData()->GetArray("Depth");
  int retVal = EXIT_SUCCESS;
  if (contr->GetLocalProcessId() == 0)
  {
    for (int i = 0; i < numPoints; i++)
    {
      std::array<double, 3> pt{ 0.0, 0.0, 0.0 };
      outDataSet->GetPoint(i, pt.data());
      int foundDepth = -1;
      bool isDemarcation = false;
      if (std::abs(pt[0]) < 1.0 && std::abs(pt[1]) < 1.0 && std::abs(pt[2]))
      {
        foundDepth = 0;
      }
      else if ((std::abs(pt[0]) - 1.0) < eps && (std::abs(pt[1]) - 1.0) < eps &&
        (std::abs(pt[2]) - 1.0) < eps)
      {
        foundDepth = 0;
        if (vtkMath::IsNan(da->GetComponent(i, 0)))
        {
          continue;
        }
      }
      for (int d = 0; d < 4; d++) // iterate over levels
      {
        double demarcation = 0.5 / std::pow(2, d);
        if (std::abs(pt[0]) < demarcation && std::abs(pt[1]) < demarcation &&
          std::abs(pt[2]) < demarcation)
        {
          foundDepth = d;
          continue;
        }
        else if ((std::abs(pt[0]) - demarcation) < eps && (std::abs(pt[1]) - demarcation) < eps &&
          (std::abs(pt[2]) - demarcation) < eps)
        {
          isDemarcation = true;
          foundDepth = d;
          continue;
        }
        else
        {
          break;
        }
      }
      /*
       * Check here if the depth array has the correct value computed above specifically for this
       * HTG Slight indetermination due to the INTERSECT_WITH_CELLS mode for points which are
       * exactly in between two depth zones. The second part of the check takes care of that.
       */
      if (!((foundDepth - da->GetComponent(i, 0) < 1) ||
            (isDemarcation && ((foundDepth - da->GetComponent(i, 0) + 1) < 1))))
      {
        vtkLog(ERROR, << "Probe Line on HTG 3D failed for " << i << "th point " << pt[0] << ", "
                      << pt[1] << ", " << pt[2] << " with depth " << da->GetComponent(i, 0)
                      << " when it should be " << foundDepth);
        retVal = EXIT_FAILURE;
      }
    }
  }
  return retVal;
}

// ----------------------------------------------------------------------------
int Test2DProbing(vtkMultiProcessController* controller)
{
  int myrank = controller->GetLocalProcessId();
  int retVal = EXIT_SUCCESS;

  // ---------------
  // Initialize data
  vtkNew<vtkRTAnalyticSource> wavelet1;
  vtkNew<vtkRTAnalyticSource> wavelet2;
  if (myrank == 0)
  {
    wavelet1->SetWholeExtent(0, 0, -10, 10, -10, -5);
    wavelet2->SetWholeExtent(0, 0, -10, 10, -5, 0);
  }
  else if (myrank == 1)
  {
    wavelet1->SetWholeExtent(0, 0, -10, 10, 0, 5);
    wavelet2->SetWholeExtent(0, 0, -10, 10, 5, 10);
  }

  wavelet1->Update();
  wavelet2->Update();

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(2);
  pds->SetPartition(0, wavelet1->GetOutputDataObject(0));
  pds->SetPartition(1, wavelet2->GetOutputDataObject(0));

  vtkNew<vtkLineSource> line;
  line->SetResolution(1);
  line->SetPoint1(0.0, 0.4, -10.0);
  line->SetPoint2(0.0, 0.4, 9.1);
  line->Update();

  vtkNew<vtkProbeLineFilter> probeLine;
  probeLine->SetInputData(pds);
  probeLine->SetSourceConnection(line->GetOutputPort());
  probeLine->SetController(controller);

  // ------------------------------------------------------------------
  // Make the actual testing. Here we mainly test the probing locations
  vtkLog(INFO, << "Testing vtkProbeLineFilter with 2D data input (cut wavelet)");
  probeLine->AggregateAsPolyDataOn();
  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES);
  probeLine->Update();

  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingAtCellBoundaries_2D.data(), ProbingAtCellBoundaries_2D.size(), "arc_length",
    "SAMPLE_LINE_AT_CELL_BOUNDARIES", myrank);

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS);
  probeLine->Update();
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingAtSegmentCenters_2D.data(), ProbingAtSegmentCenters_2D.size(), "arc_length",
    "SAMPLE_LINE_AT_SEGMENT_CENTERS", myrank);

  return retVal;
}

// ----------------------------------------------------------------------------
int Test3DProbing(vtkMultiProcessController* controller)
{
  int myrank = controller->GetLocalProcessId();
  int retVal = EXIT_SUCCESS;

  // ---------------
  // Initialize data
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
  probeLine->SetController(controller);
  probeLine->SetLineResolution(10);

  // ---------------------------------
  // Check result for polydata output
  vtkLog(INFO, << "Testing vtkProbeLineFilter with polydata output");
  probeLine->AggregateAsPolyDataOn();

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES);
  probeLine->Update();
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingAtCellBoundaries.data(), ProbingAtCellBoundaries.size(), "RTData",
    "SAMPLE_LINE_AT_CELL_BOUNDARIES", myrank);

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS);
  probeLine->Update();
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingAtSegmentCenters.data(), ProbingAtSegmentCenters.size(), "RTData",
    "SAMPLE_LINE_AT_SEGMENT_CENTERS", myrank);

  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY);
  probeLine->Update();
  retVal |= CheckForErrors(vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0)),
    ProbingUniformly.data(), ProbingUniformly.size(), "RTData", "SAMPLE_LINE_UNIFORMLY", myrank);

  // ---------------------------------
  // Check result for multiblock output
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
      ProbingAtCellBoundaries.size(), "RTData", "SAMPLE_LINE_AT_CELL_BOUNDARIES", myrank);

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
      ProbingAtSegmentCenters.size(), "RTData", "SAMPLE_LINE_AT_SEGMENT_CENTERS", myrank);

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
    ProbingUniformly.size(), "RTData", "SAMPLE_LINE_UNIFORMLY", myrank);

  return retVal;
}
// ----------------------------------------------------------------------------
int Test3DProbing2(vtkMultiProcessController* controller)
{
  int myrank = controller->GetLocalProcessId();
  int retVal = EXIT_SUCCESS;

  // ---------------
  // Initialize data
  vtkNew<vtkPartitionedDataSet> pds;

  vtkNew<vtkRTAnalyticSource> wavelet1;
  if (myrank == 1)
  {
    wavelet1->SetWholeExtent(-10, 10, -10, 10, 0, 10);
    wavelet1->Update();
    pds->SetNumberOfPartitions(1);
    pds->SetPartition(0, wavelet1->GetOutputDataObject(0));
  }

  vtkNew<vtkLineSource> line;
  line->SetPoint1(0, 0, -10);
  line->SetPoint2(0, 0, 10);
  line->SetResolution(2);

  vtkNew<vtkProbeLineFilter> probeLine;
  probeLine->SetInputData(pds);
  probeLine->SetSourceConnection(line->GetOutputPort());
  probeLine->SetController(controller);
  probeLine->SetLineResolution(50);
  probeLine->SetSamplingPattern(vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY);
  probeLine->Update();

  vtkPolyData* pd = vtkPolyData::SafeDownCast(probeLine->GetOutputDataObject(0));
  if (myrank == 0 && !pd->GetPointData()->GetArray("RTData"))
  {
    vtkLog(ERROR, "RTData array not found");
    return EXIT_FAILURE;
  }
  return retVal;
}

// ----------------------------------------------------------------------------
int Test2DProbingHTG(vtkMultiProcessController* contr)
{
  vtkLog(INFO, << "Testing vtkProbeLineFilter with 2D HyperTreeGrid input");
  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSource;
  htgSource->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);
  htgSource->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  htgSource->SetCustomDim(2);
  htgSource->SetCustomFactor(2);
  htgSource->SetCustomDepth(3);
  htgSource->SetCustomExtent(0.0, 1.0, 0.0, 1.0, 0.0, 0.0);
  htgSource->SetCustomSubdivisions(3, 3, 0);
  htgSource->Update();

  vtkNew<vtkLineSource> line;
  line->SetResolution(1);
  line->SetPoint1(0.01, 0.01, 0.00);
  line->SetPoint2(0.99, 0.99, 0.00);
  line->Update();

  vtkNew<vtkProbeLineFilter> probeLine;
  probeLine->SetInputConnection(htgSource->GetOutputPort());
  probeLine->SetSourceConnection(line->GetOutputPort());
  probeLine->SetController(contr);
  probeLine->SetLineResolution(10);
  probeLine->SetTolerance(eps);

  int retVal = EXIT_SUCCESS;

  auto Check = [&probeLine, &contr, &retVal](int pattern)
  {
    probeLine->SetSamplingPattern(pattern);
    probeLine->Update();
    vtkDataSet* outDataSet = vtkDataSet::SafeDownCast(probeLine->GetOutput());
    retVal |= Check2DHTG(contr, outDataSet);
  };
  Check(vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES);
  Check(vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS);
  Check(vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY);

  return retVal;
}

// ----------------------------------------------------------------------------
int Test3DProbingHTG(vtkMultiProcessController* contr)
{
  vtkLog(INFO, << "Testing vtkProbeLineFilter with 3D HyperTreeGrid input");
  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSource;
  htgSource->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);
  htgSource->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  htgSource->SetCustomDim(3);
  htgSource->SetCustomFactor(2);
  htgSource->SetCustomDepth(3);
  htgSource->SetCustomExtent(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
  htgSource->SetCustomSubdivisions(3, 3, 3);
  htgSource->Update();

  vtkNew<vtkLineSource> line;
  line->SetResolution(1);
  line->SetPoint1(0.02, 0.01, 0.03);
  line->SetPoint2(0.99, 0.98, 0.99);
  line->Update();

  vtkNew<vtkProbeLineFilter> probeLine;
  probeLine->SetInputConnection(htgSource->GetOutputPort());
  probeLine->SetSourceConnection(line->GetOutputPort());
  probeLine->SetController(contr);
  probeLine->SetLineResolution(10);
  probeLine->SetTolerance(eps);

  int retVal = EXIT_SUCCESS;

  auto Check = [&probeLine, &contr, &retVal](int pattern)
  {
    probeLine->SetSamplingPattern(pattern);
    probeLine->Update();
    vtkDataSet* outDataSet = vtkDataSet::SafeDownCast(probeLine->GetOutput());
    retVal |= Check3DHTG(contr, outDataSet);
  };
  Check(vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES);
  Check(vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS);
  Check(vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY);

  return retVal;
}

// ----------------------------------------------------------------------------
int TestProbeLineFilter(int argc, char* argv[])
{
  vtkNew<vtkMPIController> contr;
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int retVal = EXIT_SUCCESS;

  retVal |= Test2DProbing(contr);
  retVal |= Test3DProbing(contr);
  retVal |= Test3DProbing2(contr);
  retVal |= Test2DProbingHTG(contr);
  retVal |= Test3DProbingHTG(contr);

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}
