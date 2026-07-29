// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkExpandMarkedElements.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

namespace
{

vtkSmartPointer<vtkDataSet> GetSphere(int part, int num_parts)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(6);
  sphere->SetPhiResolution(6);
  sphere->SetStartTheta(360.0 * part / num_parts);
  sphere->SetEndTheta(360.0 * (part + 1) / num_parts);
  sphere->Update();
  auto ds = sphere->GetOutput();

  vtkNew<vtkSignedCharArray> selectedCells;
  selectedCells->SetName("MarkedCells");
  selectedCells->SetNumberOfTuples(ds->GetNumberOfCells());
  selectedCells->FillComponent(0, 0);
  selectedCells->SetTypedComponent(20, 0, 1);
  ds->GetCellData()->AddArray(selectedCells);
  return ds;
}
}
int TestExpandMarkedElements(int argc, char* argv[])
{
  vtkNew<vtkMultiBlockDataSet> mb;
  for (int cc = 0; cc < 3; ++cc)
  {
    mb->SetBlock(cc, ::GetSphere(cc, 3));
  }

  vtkNew<vtkExpandMarkedElements> filter;
  filter->SetInputDataObject(mb);
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "MarkedCells");
  filter->RemoveIntermediateLayersOn();
  filter->RemoveSeedOn();
  filter->SetNumberOfLayers(3);
  filter->Update();

  return vtkTestUtilities::RegressionTest(
           argc, argv, filter->GetOutput(), "/Data/BaselineData/TestExpandMarkedElements.vtkhdf")
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
