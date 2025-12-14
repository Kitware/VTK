// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGenerateStatistics.h"

#include "vtkCompositeDataSet.h"
#include "vtkCorrelativeStatistics.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkStatisticalModel.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPartitionedDataSetCollectionReader.h"
#include "vtkXMLPartitionedDataSetCollectionWriter.h"

#include <chrono>
#include <iostream>
#include <thread>

#include <math.h>

using namespace std::chrono_literals;

namespace
{

std::string tempDir;
int rank = 0;
int numberOfRanks = 1;

std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

std::string GetTempFileName(const std::string& fnameC)
{
  std::string fname = tempDir;
  fname += "/" + fnameC;
  return fname;
}

bool RunStats(vtkMultiProcessController* controller,
  vtkXMLPartitionedDataSetCollectionReader* reader, double trainingFraction, bool singleModel,
  bool weightByMeasure, const std::string& testType,
  vtkSmartPointer<vtkPartitionedDataSetCollection> expectedModels)
{
  bool ok = false;
  static char runNumber = 'A';
  std::string runFile = "TestModelSimple_";
  std::string fname = GetTempFileName((runFile + (runNumber++)) + ".vtpc");

  if (rank == 0)
  {
    std::cout << testType << "\n";
  }

  vtkNew<vtkGenerateStatistics> stats;
  vtkNew<vtkCorrelativeStatistics> correlative;
  stats->SetInputConnection(vtkStatisticsAlgorithm::INPUT_DATA, reader->GetOutputPort());
  stats->SetStatisticsAlgorithm(correlative);
  stats->SetTrainingFraction(trainingFraction);
  stats->SetSingleModel(singleModel);
  stats->SetWeightByCellMeasure(weightByMeasure);
  // Test a component of a vector field:
  stats->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RandomPointVectors", 2);
  stats->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RandomPointScalars");

  // Some alternate correlations to test with:
  // stats->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
  // "DistanceToCenter"); stats->SetInputArrayToProcess(1, 0, 0,
  // vtkDataObject::FIELD_ASSOCIATION_POINTS, "Polynomial"); stats->SetInputArrayToProcess(1, 0, 0,
  // vtkDataObject::FIELD_ASSOCIATION_CELLS, "RandomCellScalars");
  stats->UpdatePiece(rank, numberOfRanks, /*ghostLevels*/ 0);

  auto* models = vtkPartitionedDataSetCollection::SafeDownCast(stats->GetOutputDataObject(0));
  unsigned int numModels = models ? models->GetNumberOfPartitionedDataSets() : 0;
  if (numModels != 1 && singleModel)
  {
    std::cerr << "ERROR: Set to generate a single model, but " << numModels
              << " present in output.\n";
    return ok;
  }
  else if (numModels != 2 && !singleModel)
  {
    std::cerr << "ERROR: Set to generate a model per block, but " << numModels
              << " present in output.\n";
    return ok;
  }

  ok = true;
  auto numPoints =
    static_cast<unsigned long long>(reader->GetOutput()->GetNumberOfElements(vtkDataObject::POINT));
  unsigned long long numPointsAllRanks;
  if (numberOfRanks > 1)
  {
    controller->Reduce(&numPoints, &numPointsAllRanks, 1,
      vtkCommunicator::StandardOperations::SUM_OP, /*destProcessId*/ 0);
  }
  else
  {
    numPointsAllRanks = numPoints;
  }

  if (rank == 0)
  {
    if (numPointsAllRanks != 24)
    {
      vtkLogF(ERROR, "Expected 24 points (including ghost points), got %llu.", numPointsAllRanks);
      ok = false;
    }

    for (unsigned int ii = 0; ii < numModels; ++ii)
    {
      auto* model = vtkStatisticalModel::SafeDownCast(models->GetPartitionAsDataObject(ii, 0));
      if (!model)
      {
        vtkLogF(ERROR, "Partition %u is not a statistical model.", ii);
        ok = false;
        return ok;
      }
      if (model->IsEmpty())
      {
        vtkLogF(ERROR, "Model %u is empty.", ii);
        ok = false;
        continue;
      }
      auto* learnTab = model->GetTable(vtkStatisticalModel::TableType::Learned, 0);
      auto* derivTab = model->GetTable(vtkStatisticalModel::TableType::Derived, 0);
      if (!learnTab || !derivTab)
      {
        vtkLogF(ERROR, "One or more model tables 0 for node %u are null.", ii);
        ok = false;
        return ok;
      }

      std::cout << "  Model " << ii << " learn-table 0\n";
      learnTab->Dump(10, -1, 4);
      std::cout << "  Model " << ii << " derived-table 0\n";
      derivTab->Dump(10, -1, 4);
      std::cout.flush();

      auto cardinality = learnTab->GetValueByName(0, "Cardinality").ToInt();
      if (singleModel && trainingFraction >= 1. && cardinality != 20)
      {
        vtkLogF(ERROR, "Expecting 10088 samples, got %i (not counting ghosts).", cardinality);
        ok = false;
      }
      else if (!singleModel)
      {
        int expectedCard = (ii == 0 ? 10 : 10);
        if (cardinality != expectedCard)
        {
          vtkLogF(ERROR, "Expected %i samples, got %i.", expectedCard, cardinality);
          ok = false;
        }
      }

      // If we are subsampling, the table values will vary from run to run. But if we
      // are not, we can compare to the expected values provided:
      if (trainingFraction >= 1.)
      {
        auto* expectedModel =
          vtkStatisticalModel::SafeDownCast(expectedModels->GetPartitionAsDataObject(ii, 0));
        if (!expectedModel)
        {
          vtkLogF(ERROR, "No matching model %i. Debug files will be written.", ii);
          ok = false;
        }
        else
        {
          auto* expectedLearnTab = expectedModel->GetTable(vtkStatisticalModel::Learned, 0);
          auto* expectedDerivTab = expectedModel->GetTable(vtkStatisticalModel::Derived, 0);
          constexpr double tolerance = 128.;
          if (!vtkTestUtilities::CompareDataObjects(learnTab, expectedLearnTab, tolerance))
          {
            vtkLogF(ERROR, "Learned table is not expected. Debug files will be written.");
            ok = false;
          }
          if (!vtkTestUtilities::CompareDataObjects(derivTab, expectedDerivTab, tolerance))
          {
            vtkLogF(ERROR, "Derived table is not expected. Debug files will be written.");
            ok = false;
          }
        }
      }
    }

    if (!ok)
    {
      // clang-format off
      vtkLogF(ERROR,
        "Writing statistical models to %s for debugging purposes. "
        "You may wish to use this output to update the 'expected' "
        "model tables if you fix a bug.", fname.c_str());
      // clang-format on
      vtkNew<vtkXMLPartitionedDataSetCollectionWriter> writer;
      writer->SetController(nullptr); // Only write one model out; all ranks share the same model.
      writer->SetDataModeToAscii();
      writer->SetInputConnection(stats->GetOutputPort());
      writer->SetFileName(fname.c_str());
      writer->Write();
    }

    std::cout << (ok ? "  Success\n" : "  FAILURE\n");
  }

  return ok;
}

} // anonymous namespace

int TestGenerateStatisticsSimple(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  rank = controller->GetLocalProcessId();
  numberOfRanks = controller->GetNumberOfProcesses();
  int retVal = EXIT_SUCCESS;

  // To debug one rank of this test, uncomment the following.
  // By default, rank 0 will be paused until you attach gdb
  // (using the printed command), run "p start = true" and
  // continue.
#if 0
  if (rank == 0)
  {
    std::cout << "gdb -p " << getpid() << "\n";
    volatile bool start = false;
    while (!start)
    {
      std::this_thread::sleep_for(100ms);
    }
  }
  controller->Barrier();
#endif

  // Find the temporary directory to write model data.
  char* tempRoot =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  tempDir = tempRoot;
  delete[] tempRoot;

  vtkNew<vtkXMLPartitionedDataSetCollectionReader> reader;
  const auto fname = GetFileName(argc, argv, "Data/simple-stat.vtpc");
  reader->SetFileName(fname.c_str());
  reader->UpdateInformation();

  // Load "expected" statistical models
  const auto e1name = GetFileName(argc, argv, "Data/simple-stat/TestModelSimple_B.vtpc");
  const auto e2name = GetFileName(argc, argv, "Data/simple-stat/TestModelSimple_C.vtpc");
  vtkNew<vtkPartitionedDataSetCollection> e1;
  vtkNew<vtkPartitionedDataSetCollection> e2;
  vtkNew<vtkXMLPartitionedDataSetCollectionReader> expectedReader;
  expectedReader->SetFileName(e1name.c_str());
  expectedReader->Update();
  e1->DeepCopy(expectedReader->GetOutputDataObject(0));
  expectedReader->SetFileName(e2name.c_str());
  expectedReader->Update();
  e2->DeepCopy(expectedReader->GetOutputDataObject(0));

  // Test with subsampling enabled and disabled.
  if (!RunStats(controller, reader, 0.25, /*singleModel*/ true, /*weight*/ false,
        "Subsampling enabled, single model, no per-cell weights.", nullptr))
  {
    retVal = EXIT_FAILURE;
  }
  if (!RunStats(controller, reader, 1.0, /*singleModel*/ true, /*weight*/ false,
        "Subsampling disabled, single model, no per-cell weights.", e1))
  {
    retVal = EXIT_FAILURE;
  }
  // Test per-block model output:
  if (!RunStats(controller, reader, 1.0, /*singleModel*/ false, /*weight*/ false,
        "Subsampling disabled, multiple models, no per-cell weights.", e2))
  {
    retVal = EXIT_FAILURE;
  }
  // Test weighting by cell measure:
  // Note that all the cells in the test data are equal-area, so this test
  // uses the same expected result as above. This is a test that the area-weighting
  // is equivalent to non-area weights when cell measures are uniform.
  // TestModelData tests the case of cells with unequal measure.
  if (!RunStats(controller, reader, 1.0, /*singleModel*/ false, /*weight*/ true,
        "Subsampling disabled, multiple models, volumetric per-cell weights.", e2))
  {
    retVal = EXIT_FAILURE;
  }

  controller->Finalize();

  return retVal;
}
