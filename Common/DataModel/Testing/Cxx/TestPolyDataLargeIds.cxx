/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataRemoveCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkSmartPointer.h"


int TestPolyDataLargeIds(int , char *[])
{
  vtkIdType large_number = std::numeric_limits<int>::max();
  large_number += 1;
  std::cerr << "large_number: " << large_number << "\n";

  auto points = vtkSmartPointer<vtkPoints>::New();
  auto verts = vtkSmartPointer<vtkCellArray>::New();
  auto polydata = vtkSmartPointer<vtkPolyData>::New();

  points->SetDataTypeToFloat();
  points->SetNumberOfPoints(large_number);
  verts->Allocate(2 * large_number);
  for (vtkIdType i=0; i<large_number; ++i)
  {
    verts->InsertNextCell(1, &i);
  }

  polydata->SetPoints(points);
  polydata->SetVerts(verts);
  polydata->BuildCells();

  int num_errors = (polydata->GetNumberOfCells() == polydata->GetNumberOfVerts()) ? 0 : 1;

  if (num_errors == 0)
  {
    std::cerr << "Testing BuildCells" << std::endl;
    // check result of BuildCells
    vtkIdType npts, *pts;
    for (vtkIdType i=0; i<large_number && num_errors==0; ++i)
    {
      polydata->GetCellPoints(i, npts, pts);

      if (npts != 1 || pts[0] != i)
      {
        num_errors++;
      }
    }
    std::cerr << "BuildCells: " << (num_errors==0?"OK":"FAIL") << std::endl;
  }

  if (num_errors == 0)
  {
    polydata->BuildLinks();

    // check result of BuildLinks
    std::cerr << "Testing BuildLinks" << std::endl;
    auto cellids = vtkSmartPointer<vtkIdList>::New();
    for (vtkIdType i=0; i<large_number && num_errors==0; ++i)
    {
      polydata->GetPointCells(i, cellids);

      if (cellids->GetNumberOfIds() != 1 || cellids->GetId(0) != i)
      {
        num_errors++;
      }
    }
    std::cerr << "BuildLinks: " << (num_errors==0?"OK":"FAIL") << std::endl;
  }


  return (num_errors==0 ? 0 : 1);
}
