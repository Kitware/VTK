/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <array>
#include "vtkAppendArcLength.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

namespace
{
void InitializePolyData(vtkPolyData *polyData)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(VTK_DOUBLE);
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(3);
  lines->InsertCellPoint(points->InsertNextPoint(0, 0, 0));
  lines->InsertCellPoint(points->InsertNextPoint(1.1, 0, 0));
  lines->InsertCellPoint(points->InsertNextPoint(3.3, 0, 0));

  lines->InsertNextCell(2);
  lines->InsertCellPoint(points->InsertNextPoint(0, 1, 0));
  lines->InsertCellPoint(points->InsertNextPoint(2.2, 1, 0));

  polyData->SetPoints(points);
  polyData->SetLines(lines);
}
}

/**
 * Tests if vtkAppendArcLength adds a point array called arc_length which,
 * computes the distance from the first point in the polyline.
 */
int TestAppendArcLength(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkPolyData> inputData
    = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputData);

  vtkSmartPointer<vtkAppendArcLength> arcLengthFilter
    = vtkSmartPointer<vtkAppendArcLength>::New();
  arcLengthFilter->SetInputDataObject(inputData);
  arcLengthFilter->Update();
  vtkDataSet* data =
    vtkDataSet::SafeDownCast(arcLengthFilter->GetOutputDataObject(0));

  std::array<double, 5> expected =
    {
      {0, 1.1, 3.3, 0, 2.2}
    };

  vtkDataArray* arcLength = data->GetPointData()->GetArray("arc_length");
  if (arcLength == nullptr)
  {
    std::cerr << "No arc_length array.\n";
    return EXIT_FAILURE;
  }
  if (arcLength->GetNumberOfComponents () != 1 ||
      static_cast<size_t>(arcLength->GetNumberOfTuples ()) != expected.size())
  {
    std::cerr << "Invalid size or number of components.\n";
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < expected.size(); ++i)
    if (arcLength->GetTuple(static_cast<vtkIdType>(i))[0] != expected[i])
    {
      std::cerr << "Invalid value: " << arcLength->GetTuple(static_cast<vtkIdType>(i))[0]
                << " expecting: " << expected[i] << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
