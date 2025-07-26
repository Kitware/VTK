// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGenerateStatistics.h"

#include "vtkCompositeDataSet.h"
#include "vtkCorrelativeStatistics.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIOSSReader.h"
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

bool RunStats(vtkMultiProcessController* controller, vtkIOSSReader* reader, double trainingFraction,
  bool singleModel, bool weightByMeasure, const std::string& testType,
  vtkSmartPointer<vtkPartitionedDataSetCollection> expectedModels)
{
  bool ok = false;
  static char runNumber = 'A';
  std::string runFile = "TestModel_";
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

  // Correlate (point-centered) Z displacement to (cell-centered) EQPS.
  // This works by promoting EQPS to point centering (either by averaging or by
  // volume-weighted average, depending on \a weightByMeasure) and running
  // statistics over the resulting arrays.
  stats->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DISPL", 2);
  stats->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "EQPS");

  // An interesting alternative is to correlate Z displacement to the L₂-norm of displacement:
  // stats->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DISPL", -2);

  // Run the filter at this timestep (about midway through the simulation) so that the
  // displacement and EQPS fields are not uniformly zero:
  stats->UpdateTimeStep(0.00209993, rank, numberOfRanks);

  auto* models = vtkPartitionedDataSetCollection::SafeDownCast(stats->GetOutputDataObject(0));
  unsigned int numModels = models ? models->GetNumberOfPartitionedDataSets() : 0;
  if (numModels != 1 && singleModel)
  {
    vtkLogF(ERROR, "Set to generate a single model, but %i present in output.", numModels);
    return ok;
  }
  else if (numModels != 2 && !singleModel)
  {
    vtkLogF(ERROR, "Set to generate a model per block, but %i present in output.", numModels);
    return ok;
  }

  ok = true;
  auto numPoints = static_cast<unsigned long long>(
    reader->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::POINT));
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
    if (numPointsAllRanks != 10516)
    {
      vtkLogF(
        ERROR, "Expected 10516 points (including ghost points), got %llu.", numPointsAllRanks);
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
      if (singleModel && trainingFraction >= 1. && cardinality != 10088)
      {
        vtkLogF(ERROR, "Expecting 10088 samples, got %i (not counting ghosts).", cardinality);
        ok = false;
      }
      else if (singleModel && trainingFraction < 1.)
      {
        auto expected = static_cast<int>(10088 * trainingFraction);
        if (cardinality != expected)
        {
          vtkLogF(ERROR, "Number of samples: expecting %i got %i", expected, cardinality);
          ok = false;
        }
      }
      else if (!singleModel)
      {
        int expectedCard = (ii == 0 ? 6724 : 3364);
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
          // We use a relatively loose tolerance here due to single-precision communication of
          // model table data among ranks:
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

int TestGenerateStatistics(int argc, char* argv[])
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

  vtkNew<vtkIOSSReader> reader;
  const auto fname = GetFileName(argc, argv, "Data/Exodus/can.e.4/can.e.4.0");
  reader->SetFileName(fname.c_str());
  reader->SetController(controller);
  reader->UpdateInformation();
  reader->UpdateTimeStep(0.00209993, rank, numberOfRanks);

  // Load "expected" statistical models
  const auto e1name = GetFileName(argc, argv, "Data/Exodus/can.e.4/statistics/TestModel_B.vtpc");
  const auto e2name = GetFileName(argc, argv, "Data/Exodus/can.e.4/statistics/TestModel_C.vtpc");
  const auto e3name = GetFileName(argc, argv, "Data/Exodus/can.e.4/statistics/TestModel_D.vtpc");
  vtkNew<vtkPartitionedDataSetCollection> e1;
  vtkNew<vtkPartitionedDataSetCollection> e2;
  vtkNew<vtkPartitionedDataSetCollection> e3;
  vtkNew<vtkXMLPartitionedDataSetCollectionReader> expectedReader;
  expectedReader->SetFileName(e1name.c_str());
  expectedReader->Update();
  e1->DeepCopy(expectedReader->GetOutputDataObject(0));
  expectedReader->SetFileName(e2name.c_str());
  expectedReader->Update();
  e2->DeepCopy(expectedReader->GetOutputDataObject(0));
  expectedReader->SetFileName(e3name.c_str());
  expectedReader->Update();
  e3->DeepCopy(expectedReader->GetOutputDataObject(0));

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
  if (!RunStats(controller, reader, 1.0, /*singleModel*/ false, /*weight*/ true,
        "Subsampling disabled, multiple models, volumetric per-cell weights.", e3))
  {
    retVal = EXIT_FAILURE;
  }

  controller->Finalize();

  return retVal;
}
