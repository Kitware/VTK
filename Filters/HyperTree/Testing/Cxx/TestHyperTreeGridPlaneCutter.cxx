// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridPlaneCutter.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkPolyData.h"

int TestHyperTreeGridPlaneCutter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(1);
  htGrid->SetDimensions(2, 2, 2);
  htGrid->SetGridScale(1., 1., 1.);
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor(".");

  // Plane cutters
  vtkNew<vtkHyperTreeGridPlaneCutter> cut1;
  cut1->SetInputConnection(htGrid->GetOutputPort());
  cut1->SetPlane(0, 1, 0, 1);
  cut1->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(cut1->GetOutput());

  vtkNew<vtkPoints> expectedPoints;
  expectedPoints->InsertNextPoint(0, 1, 0);
  expectedPoints->InsertNextPoint(1, 1, 0);
  expectedPoints->InsertNextPoint(1, 1, 1);
  expectedPoints->InsertNextPoint(0, 1, 1);

  for (int i = 0; i < output->GetNumberOfPoints(); i++)
  {
    double point[3];
    output->GetPoint(i, point);

    double expectedPoint[3];
    expectedPoints->GetPoint(i, expectedPoint);

    for (int j = 0; j < 3; j++)
    {
      if (point[j] != expectedPoint[j])
      {
        vtkLog(ERROR,
          "Invalid point at index " << i << ". "
                                    << "Expected (" << expectedPoint[0] << ", " << expectedPoint[1]
                                    << ", " << expectedPoint[2] << "), but got (" << point[0]
                                    << ", " << point[1] << ", " << point[2] << ")");
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
