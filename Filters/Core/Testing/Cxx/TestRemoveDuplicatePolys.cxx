// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCellArray.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkRemoveDuplicatePolys.h>
#include <vtkSmartPointer.h>

int TestRemoveDuplicatePolys(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence =
    vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();

  for (unsigned int i = 0; i < 3; ++i)
  {
    double point[3];
    for (unsigned int j = 0; j < 3; ++j)
    {
      randomSequence->Next();
      point[j] = randomSequence->GetValue();
    }
    points->InsertNextPoint(point);
  }

  verts->InsertNextCell(3);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(2);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(0);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(1);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(1);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(2);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(0);

  points->Squeeze();
  verts->Squeeze();

  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  inputPolyData->SetPoints(points);
  inputPolyData->SetPolys(verts);

  vtkSmartPointer<vtkRemoveDuplicatePolys> removePolyData =
    vtkSmartPointer<vtkRemoveDuplicatePolys>::New();
  removePolyData->SetInputData(inputPolyData);

  removePolyData->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = removePolyData->GetOutput();

  if (outputPolyData->GetNumberOfPolys() != 1)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
