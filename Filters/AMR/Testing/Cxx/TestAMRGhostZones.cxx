/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRGhostZones.cxx

  Copyright (c) Kitware SAS
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestAMRGhostZones.cxx -- Regression test for AMR Ghost Zones
//
// .SECTION Description
//  Test that computing ghost zones does not erase previous ghost zones.

#include "vtkAMRBox.h"
#include "vtkAMRUtilities.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkSphere.h"
#include "vtkStructuredData.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

#include <cassert>
#include <sstream>

namespace
{

//------------------------------------------------------------------------------
void MakeScalars(int dims[3], double origin[3], double spacing[3], vtkFloatArray* scalars)
{
  // Implicit function used to compute scalars
  vtkNew<vtkSphere> sphere;
  sphere->SetRadius(3);
  sphere->SetCenter(5, 5, 5);

  scalars->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
  for (int k = 0; k < dims[2]; k++)
  {
    auto z = origin[2] + spacing[2] * k;
    for (int j = 0; j < dims[1]; j++)
    {
      auto y = origin[1] + spacing[1] * j;
      for (int i = 0; i < dims[0]; i++)
      {
        auto x = origin[0] + spacing[0] * i;
        scalars->SetValue(
          k * dims[0] * dims[1] + j * dims[0] + i, sphere->EvaluateFunction(x, y, z));
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkOverlappingAMR> CreateOverlappingAMR()
{
  // Create and populate the AMR dataset
  // The dataset should look like
  // Level 0
  //   uniform grid, dimensions 11, 11, 11, AMR box (0, 0, 0) - (9, 9, 9)
  // Level 1 - refinement ratio : 2
  //   uniform grid, dimensions 11, 11, 11, AMR box (0, 0, 0) - (9, 9, 9)
  //   uniform grid, dimensions 11, 11, 11, AMR box (10, 10, 10) - (19, 19, 19)
  // Use MakeScalars() above to fill the scalar arrays

  vtkSmartPointer<vtkOverlappingAMR> amr = vtkSmartPointer<vtkOverlappingAMR>::New();
  int blocksPerLevel[] = { 1, 2 };
  amr->Initialize(2, blocksPerLevel);

  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int dims[3] = { 11, 11, 11 };

  vtkNew<vtkUniformGrid> ug1;
  // Geometry
  ug1->SetOrigin(origin);
  ug1->SetSpacing(spacing);
  ug1->SetDimensions(dims);

  // Data
  vtkNew<vtkFloatArray> scalars;
  ug1->GetPointData()->SetScalars(scalars);
  MakeScalars(dims, origin, spacing, scalars);

  int lo[3] = { 0, 0, 0 };
  int hi[3] = { 9, 9, 9 };
  vtkAMRBox box1(lo, hi);
  amr->SetAMRBox(0, 0, box1);
  amr->SetDataSet(0, 0, ug1);

  double spacing2[3] = { 0.5, 0.5, 0.5 };

  vtkNew<vtkUniformGrid> ug2;
  // Geometry
  ug2->SetOrigin(origin);
  ug2->SetSpacing(spacing2);
  ug2->SetDimensions(dims);

  // Data
  vtkNew<vtkFloatArray> scalars2;
  ug2->GetPointData()->SetScalars(scalars2);
  MakeScalars(dims, origin, spacing2, scalars2);

  int lo2[3] = { 0, 0, 0 };
  int hi2[3] = { 9, 9, 9 };
  vtkAMRBox box2(lo2, hi2);
  amr->SetAMRBox(1, 0, box2);
  amr->SetDataSet(1, 0, ug2);

  double origin3[3] = { 5, 5, 5 };

  vtkNew<vtkUniformGrid> ug3;
  // Geometry
  ug3->SetOrigin(origin3);
  ug3->SetSpacing(spacing2);
  ug3->SetDimensions(dims);

  // Data
  vtkNew<vtkFloatArray> scalars3;
  ug3->GetPointData()->SetScalars(scalars3);
  MakeScalars(dims, origin3, spacing2, scalars3);

  int lo3[3] = { 10, 10, 10 };
  int hi3[3] = { 19, 19, 19 };
  vtkAMRBox box3(lo3, hi3);
  amr->SetAMRBox(1, 1, box3);
  amr->SetDataSet(1, 1, ug3);

  amr->SetRefinementRatio(0, 2);

  return amr;
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* GetGhostArray(const vtkSmartPointer<vtkOverlappingAMR>& amr)
{
  vtkUnsignedCharArray* ghostArray = nullptr;

  vtkUniformGrid* grid = amr->GetDataSet(0, 0);
  if (grid != nullptr)
  {
    vtkDataArray* dataArray = grid->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName());
    if (dataArray != nullptr)
    {
      ghostArray = vtkUnsignedCharArray::SafeDownCast(dataArray);
    }
  }

  return ghostArray;
}

} // namespace

//------------------------------------------------------------------------------
int TestAMRGhostZones(int, char*[])
{
  int rc = 0;

  vtkSmartPointer<vtkOverlappingAMR> amr = CreateOverlappingAMR();

  vtkAMRUtilities::BlankCells(amr);

  // Set some ghost cell as "hidden"
  vtkUnsignedCharArray* ghostArray = GetGhostArray(amr);
  if (ghostArray == nullptr)
  {
    std::cerr << "Unexpected error: ghostArray is nullptr!" << std::endl;
    ++rc;
  }
  else
  {
    unsigned char initialValue = ghostArray->GetValue(0);
    unsigned char newValue = vtkDataSetAttributes::HIDDENCELL;
    ghostArray->SetValue(0, newValue);

    // Blank again the AMR and check that the hidden cell is still present.
    vtkAMRUtilities::BlankCells(amr);
    vtkUnsignedCharArray* newGhostArray = GetGhostArray(amr);
    if (newGhostArray == nullptr)
    {
      std::cerr << "Unexpected error: newGhostArray is nullptr!" << std::endl;
      ++rc;
    }
    else
    {
      unsigned char newGhostValue = newGhostArray->GetValue(0);
      unsigned char expectedValue = initialValue | newValue;
      if (newGhostValue != expectedValue)
      {
        std::cerr << "Failure -- expected value: " << expectedValue
                  << ", actual value: " << newGhostValue << std::endl;
        ++rc;
      }
    }
  }

  return rc;
}
