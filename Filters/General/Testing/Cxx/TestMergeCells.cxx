// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*
Test designed to verify the capability of vtkMergeCells to merge points
within a double precision tolerance.
Two hexahedrons cells are generated. One of them is randomly perturbed
with a small double precision amplitude.
vtkMergeCells is then tested with a tolerance smaller than the perturbation's amplitude
(in this case no points should be merged)
At last, vtkMergeCells is tested with a tolerance bigger than the perturbation's amplitude
(in this case 4 points should be merged)
*/

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkHexahedron.h"
#include "vtkMergeCells.h"
#include "vtkUnstructuredGrid.h"
#include <cstdlib>

static vtkNew<vtkHexahedron> MakeHexahedron(double origin[3], double length);

vtkNew<vtkHexahedron> MakeHexahedron(double origin[3], double length)
{
  vtkNew<vtkHexahedron> aHexahedron;
  // Ensure double precision points
  aHexahedron->GetPoints()->SetDataType(VTK_DOUBLE);
  aHexahedron->GetPointIds()->SetId(0, 0);
  aHexahedron->GetPointIds()->SetId(1, 1);
  aHexahedron->GetPointIds()->SetId(2, 2);
  aHexahedron->GetPointIds()->SetId(3, 3);
  aHexahedron->GetPointIds()->SetId(4, 4);
  aHexahedron->GetPointIds()->SetId(5, 5);
  aHexahedron->GetPointIds()->SetId(6, 6);
  aHexahedron->GetPointIds()->SetId(7, 7);

  aHexahedron->GetPoints()->SetPoint(0, origin);
  aHexahedron->GetPoints()->SetPoint(1, origin[0] + length, origin[1], origin[2]);
  aHexahedron->GetPoints()->SetPoint(2, origin[0] + length, origin[1] + length, origin[2]);
  aHexahedron->GetPoints()->SetPoint(3, origin[0], origin[1] + length, origin[2]);

  aHexahedron->GetPoints()->SetPoint(4, origin[0], origin[1], origin[2] + length);
  aHexahedron->GetPoints()->SetPoint(5, origin[0] + length, origin[1], origin[2] + length);
  aHexahedron->GetPoints()->SetPoint(6, origin[0] + length, origin[1] + length, origin[2] + length);
  aHexahedron->GetPoints()->SetPoint(7, origin[0], origin[1] + length, origin[2] + length);

  return aHexahedron;
}

int TestMergeCells(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Amplitude of the perturbation to be added to the points
  // of a VTK_HEXAHEDRON cell
  double amplitudePerturbation = 1.0e-13;
  // Use always the same seed
  vtkMath::RandomSeed(8775070);
  // Origin of the first hexa
  double origin0[3] = { 0.0, 0.0, 0.0 };
  // Length of the edges of the hexa
  double length = 1.0;
  // Origin of the second hexa
  double origin1[3] = { 0.0, length, 0.0 };
  // Generate hexahedrons
  auto hexa0 = MakeHexahedron(origin0, length);
  auto hexa1 = MakeHexahedron(origin1, length);
  // Add a random perturbation to the second hexa
  for (int i = 0; i < hexa0->GetNumberOfPoints(); i++)
  {
    double pointPosition[3];
    hexa1->GetPoints()->GetPoint(i, pointPosition);
    for (int j = 0; j < 3; j++)
    {
      int randomSign = vtkMath::Random(-1, 1) >= 0 ? 1 : -1;
      double randomPerturbation = vtkMath::Random(0.5, 0.7) * amplitudePerturbation;
      pointPosition[j] += randomSign * randomPerturbation;
    }
    hexa1->GetPoints()->SetPoint(i, pointPosition);
  }
  hexa1->GetPoints()->Modified();

  // Generate two meshes, one containing hexa0 and the second one
  // containing hexa1
  vtkNew<vtkUnstructuredGrid> mesh0;
  vtkNew<vtkUnstructuredGrid> mesh1;

  mesh0->Allocate(1);
  mesh1->Allocate(1);

  mesh0->InsertNextCell(VTK_HEXAHEDRON, hexa0->GetPointIds());
  mesh1->InsertNextCell(VTK_HEXAHEDRON, hexa1->GetPointIds());

  mesh0->SetPoints(hexa0->GetPoints());
  mesh1->SetPoints(hexa1->GetPoints());

  // Ensure that the GlobalCellIds of each hexa are different
  vtkNew<vtkIdTypeArray> ids0;
  ids0->SetName("GlobalCellIds");
  ids0->SetNumberOfValues(1);
  mesh0->GetCellData()->SetGlobalIds(ids0);

  vtkNew<vtkIdTypeArray> ids1;
  ids1->SetName("GlobalCellIds");
  ids1->SetNumberOfValues(1);
  mesh1->GetCellData()->SetGlobalIds(ids1);

  mesh0->GetCellData()->GetGlobalIds()->SetTuple1(0, 0);
  mesh1->GetCellData()->GetGlobalIds()->SetTuple1(0, 1);

  // Test vtkMergeCells with a tolerance smaller than the
  // amplitude of the random perturbation
  // In this case no point should be merged
  {
    double tolerancePointMerge = amplitudePerturbation / 10;
    vtkNew<vtkMergeCells> mergeCells;
    mergeCells->SetTotalNumberOfPoints(mesh0->GetNumberOfPoints() + mesh1->GetNumberOfPoints());
    mergeCells->SetTotalNumberOfCells(mesh0->GetNumberOfCells() + mesh1->GetNumberOfCells());
    mergeCells->SetTotalNumberOfDataSets(2);
    mergeCells->SetPointMergeTolerance(tolerancePointMerge);
    mergeCells->SetUseGlobalCellIds(1);
    mergeCells->SetUseGlobalIds(0);
    // Ensure double precision output
    mergeCells->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

    vtkNew<vtkUnstructuredGrid> output;
    mergeCells->SetUnstructuredGrid(output);
    mergeCells->MergeDataSet(mesh0);
    mergeCells->MergeDataSet(mesh1);
    mergeCells->Finish();
    int finalPoints = mergeCells->GetUnstructuredGrid()->GetNumberOfPoints();
    if (finalPoints != 16)
    {
      cerr << "Found " << finalPoints << " after merge, expected 16" << endl;
      return EXIT_FAILURE;
    }
  }

  // Test vtkMergeCells with a tolerance bigger than the
  // amplitude of the random perturbation
  // In this case 4 points should be merged
  {
    double tolerancePointMerge = amplitudePerturbation * 10;
    vtkNew<vtkMergeCells> mergeCells;
    mergeCells->SetTotalNumberOfPoints(mesh0->GetNumberOfPoints() + mesh1->GetNumberOfPoints());
    mergeCells->SetTotalNumberOfCells(mesh0->GetNumberOfCells() + mesh1->GetNumberOfCells());
    mergeCells->SetTotalNumberOfDataSets(2);
    mergeCells->SetPointMergeTolerance(tolerancePointMerge);
    mergeCells->SetUseGlobalCellIds(1);
    mergeCells->SetUseGlobalIds(0);
    mergeCells->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

    vtkNew<vtkUnstructuredGrid> output;
    mergeCells->SetUnstructuredGrid(output);
    mergeCells->MergeDataSet(mesh0);
    mergeCells->MergeDataSet(mesh1);
    mergeCells->Finish();
    int finalPoints = mergeCells->GetUnstructuredGrid()->GetNumberOfPoints();
    if (finalPoints != 12)
    {
      cerr << "Found " << finalPoints << " after merge, expected 12" << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
