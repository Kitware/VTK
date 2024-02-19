// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkDecimatePro.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

namespace
{
void DecimatePro(vtkPolyData* inputPolyData, double targetReduction, bool preserveTopology,
  double featureAngle, bool boundaryVertexDeletion)
{
  vtkNew<vtkDecimatePro> decimatePro;
  decimatePro->SetInputData(inputPolyData);
  decimatePro->SetTargetReduction(targetReduction);
  decimatePro->SetPreserveTopology(preserveTopology);
  decimatePro->SetFeatureAngle(featureAngle);
  decimatePro->SetBoundaryVertexDeletion(boundaryVertexDeletion);

  decimatePro->Update();
}
}

int TestDecimateProDegenerateTriangles(int argc, char* argv[])
{
  char* data_dir = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    cerr << "Could not determine data directory." << endl;
    return EXIT_FAILURE;
  }

  std::string dataPath = std::string(data_dir) + "/Data/degenerate_triangles.vtp";
  delete[] data_dir;

  vtkNew<vtkXMLPolyDataReader> inputReader;
  inputReader->SetFileName(dataPath.c_str());
  inputReader->Update();

  vtkPolyData* inputPolyData = inputReader->GetOutput();

  // Ensure there is no crash, with multiple filter settings
  // No need to check the output here

  DecimatePro(inputPolyData, 0.9, true, 45, true);

  DecimatePro(inputPolyData, 0.9, false, 15, true);

  return EXIT_SUCCESS;
}
