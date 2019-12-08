/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkStaticCellLocator::FindClosestPoint
// .SECTION Description
// this program tests the FindClosestPoint method

#include "vtkCellLocator.h" // used as reference
#include "vtkCylinderSource.h"
#include "vtkGenericCell.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLocator.h"

int TestStaticCellLocator(int, char*[])
{
  auto source = vtkSmartPointer<vtkCylinderSource>::New();
  source->SetCapping(1);
  source->SetResolution(27);
  source->SetCenter(0.0, -1.0, 5.0);
  source->SetHeight(10.0);
  source->SetRadius(1.0);
  source->Update();

  auto static_loc = vtkSmartPointer<vtkStaticCellLocator>::New();
  static_loc->SetDataSet(source->GetOutput());
  static_loc->AutomaticOn();
  static_loc->BuildLocator();

  auto ref_loc = vtkSmartPointer<vtkCellLocator>::New();
  ref_loc->SetDataSet(source->GetOutput());
  ref_loc->AutomaticOn();
  ref_loc->BuildLocator();

  double test_points[10][3] = { { 0, -1, 0 }, { 0, -2, 1 }, { -1.7, -1, 0 }, { 7.0, -2, 1 },
    { 0, -1, 10 }, { 0, 1, 13 }, { -4, -1, 10 }, { 3, 1, 13 }, { 0.9, -1, 5 }, { 0.2, -0.9, 6 } };

  auto cell = vtkSmartPointer<vtkGenericCell>::New();
  int subId;
  vtkIdType static_cellId, ref_cellId;
  double static_dist2, ref_dist2, static_closest[3], ref_closest[3];
  int static_inside, ref_inside;
  vtkIdType static_ret, ref_ret;

  int num_failed = 0;
  for (int i = 0; i < 10; ++i)
  {
    double* p = test_points[i];

    static_loc->FindClosestPoint(p, static_closest, cell, static_cellId, subId, static_dist2);
    ref_loc->FindClosestPoint(p, ref_closest, cell, ref_cellId, subId, ref_dist2);

    // note that the cell id and even closest point are not always identical
    // but, the distance should be nearly identical.
    bool ok = (std::abs(static_dist2 - ref_dist2) < 1e-12);
    if (!ok)
    {
      std::cerr << "different closest point:\n";
      std::cerr << "\t" << static_cellId << " - " << ref_cellId << "\n";
      std::cerr << "\t" << static_dist2 << " - " << ref_dist2 << "\n";
      std::cerr << "\t(" << static_closest[0] << ", " << static_closest[1] << ", "
                << static_closest[2] << ") - (" << ref_closest[0] << ", " << ref_closest[1] << ", "
                << ref_closest[2] << ")\n";

      num_failed++;
    }

    static_ret = static_loc->FindClosestPointWithinRadius(
      p, 5.0, static_closest, cell, static_cellId, subId, static_dist2, static_inside);
    ref_ret = ref_loc->FindClosestPointWithinRadius(
      p, 5.0, ref_closest, cell, ref_cellId, subId, ref_dist2, ref_inside);

    if (static_ret != ref_ret)
    {
      std::cerr << "different closest point within radius result\n";
      num_failed++;
    }
    else if (static_ret)
    {
      // note that the cell id, inside and even closest point are not always identical
      // but, the distance should be nearly identical.
      ok = (std::abs(static_dist2 - ref_dist2) < 1e-12);
      if (!ok)
      {
        std::cerr << "different closest point within radius:\n";
        std::cerr << "\t" << static_cellId << " - " << ref_cellId << "\n";
        std::cerr << "\t" << static_dist2 << " - " << ref_dist2 << "\n";
        std::cerr << "\t(" << static_closest[0] << ", " << static_closest[1] << ", "
                  << static_closest[2] << ") - (" << ref_closest[0] << ", " << ref_closest[1]
                  << ", " << ref_closest[2] << ")\n";

        num_failed++;
      }
    }
  }

  return (num_failed == 0) ? 0 : 1;
}
