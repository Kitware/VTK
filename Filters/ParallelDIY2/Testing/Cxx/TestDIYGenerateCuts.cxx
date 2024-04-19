// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBoundingBox.h"
#include "vtkDIYKdTreeUtilities.h"
#include "vtkDataSet.h"
#include "vtkNew.h"
#include "vtkRTAnalyticSource.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include <limits>
#include <vector>

namespace
{
void PrintError(const vtkBoundingBox& box1, const vtkBoundingBox& box2)
{
  double bds1[6], bds2[6];
  box1.GetBounds(bds1);
  box2.GetBounds(bds2);
  cerr << "Error comparing bounding boxes." << endl;
  cerr << "Expected: " << bds1[0] << " " << bds1[1] << " " << bds1[2] << " " << bds1[3] << " "
       << bds1[4] << " " << bds1[5] << " " << endl;
  cerr << "Got: " << bds2[0] << " " << bds2[1] << " " << bds2[2] << " " << bds2[3] << " " << bds2[4]
       << " " << bds2[5] << " " << endl;
}

// Test if the reduced cuts equals the reduced dataObj bounding box, taking into account the given
// epsilon. Also return true if dataObj bounds equals zero and cuts is empty.
bool TestCuts(vtkDataObject* dataObj, const std::vector<vtkBoundingBox>& cuts,
  std::array<double, 3>& epsilon, vtkMPIController* contr)
{
  double dataBounds[6];
  vtkDataSet::SafeDownCast(dataObj)->GetBounds(dataBounds);
  vtkBoundingBox dataBbox(dataBounds), allDataBbox;
  contr->AllReduce(dataBbox, allDataBbox);

  allDataBbox.Inflate(epsilon[0], epsilon[1], epsilon[2]);

  vtkBoundingBox allCut = { 0., 0., 0., 0., 0., 0. };
  for (const auto& cut : cuts)
  {
    double cutBounds[6];
    cut.GetBounds(cutBounds);
    allCut.AddBounds(cutBounds);
  }

  if (allDataBbox != allCut)
  {
    ::PrintError(allDataBbox, allCut);
    return false;
  }

  return true;
}

const int NUMBER_OF_PARTITIONS = 7;
const double EPSILON = std::numeric_limits<double>::epsilon();
}

int TestDIYGenerateCuts(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  bool status = EXIT_SUCCESS;

  // Generate 3D data cuts
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 63, 0, 63, 0, 63);
  wavelet->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  auto cuts = vtkDIYKdTreeUtilities::GenerateCuts(
    wavelet->GetOutputDataObject(0), ::NUMBER_OF_PARTITIONS, false, contr);

  // Reduced cuts should be strictly equal to the input data bounding box
  std::array<double, 3> epsilon = { 0., 0., 0. };
  if (!::TestCuts(wavelet->GetOutputDataObject(0), cuts, epsilon, contr))
  {
    status = EXIT_FAILURE;
  }

  // Generate 2D data cuts (zero-dimension on Z axis)
  wavelet->SetWholeExtent(0, 63, 0, 63, 0, 0);
  wavelet->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  cuts = vtkDIYKdTreeUtilities::GenerateCuts(
    wavelet->GetOutputDataObject(0), ::NUMBER_OF_PARTITIONS, false, contr);

  // Reduced cuts should have an epsilon width on Z axis
  epsilon = { 0., 0., ::EPSILON };
  if (!::TestCuts(wavelet->GetOutputDataObject(0), cuts, epsilon, contr))
  {
    status = EXIT_FAILURE;
  }

  // Generate 1D data cuts (zero-dimension on Y and Z axis)
  wavelet->SetWholeExtent(0, 63, 0, 0, 0, 0);
  wavelet->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  cuts = vtkDIYKdTreeUtilities::GenerateCuts(
    wavelet->GetOutputDataObject(0), ::NUMBER_OF_PARTITIONS, false, contr);

  // Reduced cuts should have an epsilon width on Y and Z axis
  epsilon = { 0., ::EPSILON, ::EPSILON };
  if (!::TestCuts(wavelet->GetOutputDataObject(0), cuts, epsilon, contr))
  {
    status = EXIT_FAILURE;
  }

  // Generate empty data
  wavelet->SetWholeExtent(0, 0, 0, 0, 0, 0);
  wavelet->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  cuts = vtkDIYKdTreeUtilities::GenerateCuts(
    wavelet->GetOutputDataObject(0), ::NUMBER_OF_PARTITIONS, false, contr);

  // Cuts should be empty
  epsilon = { 0., 0., 0. };
  if (!::TestCuts(wavelet->GetOutputDataObject(0), cuts, epsilon, contr))
  {
    status = EXIT_FAILURE;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();

  return status;
}
