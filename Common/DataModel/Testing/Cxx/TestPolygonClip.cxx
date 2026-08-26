// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .SECTION Description
// Regression test for vtkPolygon::Clip(). Verifies that a polygon lying
// entirely on one side of the clip value is either passed through unchanged
// or dropped, without being triangulated, and that a polygon straddling the
// clip value is still clipped correctly.

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMathUtilities.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"

#include <iostream>

namespace
{
// Clips a 4-point polygon (given by its point coordinates and per-point
// scalars) at the given value, returning the number of output cells and
// their total area.
void ClipSquare(vtkPoints* inputPoints, vtkDoubleArray* scalars, double value, int insideOut,
  vtkIdType& numberOfOutputCells, double& totalArea)
{
  vtkNew<vtkPolygon> polygon;
  vtkIdType numPts = inputPoints->GetNumberOfPoints();
  polygon->GetPointIds()->SetNumberOfIds(numPts);
  polygon->GetPoints()->SetNumberOfPoints(numPts);
  for (vtkIdType i = 0; i < numPts; ++i)
  {
    polygon->GetPointIds()->SetId(i, i);
    double p[3];
    inputPoints->GetPoint(i, p);
    polygon->GetPoints()->SetPoint(i, p);
  }

  vtkNew<vtkPoints> outputPoints;
  vtkNew<vtkMergePoints> locator;
  locator->InitPointInsertion(outputPoints, inputPoints->GetBounds());

  vtkNew<vtkCellArray> outputPolys;

  vtkNew<vtkPointData> inPD;
  inPD->SetScalars(scalars);
  vtkNew<vtkPointData> outPD;
  outPD->CopyAllocate(inPD);

  vtkNew<vtkCellData> inCD;
  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetName("cellIds");
  cellIds->InsertNextValue(0);
  inCD->AddArray(cellIds);
  vtkNew<vtkCellData> outCD;
  outCD->CopyAllocate(inCD);

  polygon->Clip(value, scalars, locator, outputPolys, inPD, outPD, inCD, 0, outCD, insideOut);

  numberOfOutputCells = outputPolys->GetNumberOfCells();

  totalArea = 0.0;
  vtkIdType npts;
  const vtkIdType* pts;
  outputPolys->InitTraversal();
  while (outputPolys->GetNextCell(npts, pts))
  {
    double normal[3];
    totalArea += vtkPolygon::ComputeArea(outputPoints, npts, pts, normal);
  }
}
}

int TestPolygonClip(int, char*[])
{
  int status = EXIT_SUCCESS;

  // A 2x2 square, area 4, used for all the cases below.
  vtkNew<vtkPoints> square;
  square->SetNumberOfPoints(4);
  square->SetPoint(0, 0.0, 0.0, 0.0);
  square->SetPoint(1, 2.0, 0.0, 0.0);
  square->SetPoint(2, 2.0, 2.0, 0.0);
  square->SetPoint(3, 0.0, 2.0, 0.0);

  vtkNew<vtkDoubleArray> allKept;
  allKept->SetNumberOfComponents(1);
  allKept->SetNumberOfTuples(4);
  allKept->FillComponent(0, 2.0);

  vtkNew<vtkDoubleArray> allDiscarded;
  allDiscarded->SetNumberOfComponents(1);
  allDiscarded->SetNumberOfTuples(4);
  allDiscarded->FillComponent(0, 0.0);

  vtkNew<vtkDoubleArray> straddling;
  straddling->SetNumberOfComponents(1);
  straddling->SetNumberOfTuples(4);
  straddling->SetValue(0, 0.0);
  straddling->SetValue(1, 0.0);
  straddling->SetValue(2, 2.0);
  straddling->SetValue(3, 2.0);

  // Case 1: the polygon lies entirely on the kept side (all scalars >=
  // value), so it should be passed through unchanged as a single cell,
  // rather than triangulated.
  {
    vtkIdType numberOfOutputCells;
    double totalArea;
    ClipSquare(square, allKept, 1.0, /*insideOut=*/0, numberOfOutputCells, totalArea);

    if (numberOfOutputCells != 1)
    {
      std::cerr << "ERROR: expected the fully-kept polygon to be passed through as a single "
                   "cell, got "
                << numberOfOutputCells << " cells" << std::endl;
      status = EXIT_FAILURE;
    }
    if (!vtkMathUtilities::NearlyEqual<double>(totalArea, 4.0))
    {
      std::cerr << "ERROR: expected the fully-kept polygon's area to be unchanged (4.0), got "
                << totalArea << std::endl;
      status = EXIT_FAILURE;
    }
  }

  // Case 2: the polygon lies entirely on the discarded side (all scalars <
  // value), so no cells should be output.
  {
    vtkIdType numberOfOutputCells;
    double totalArea;
    ClipSquare(square, allDiscarded, 1.0, /*insideOut=*/0, numberOfOutputCells, totalArea);

    if (numberOfOutputCells != 0)
    {
      std::cerr << "ERROR: expected the fully-discarded polygon to produce no output cells, got "
                << numberOfOutputCells << " cells" << std::endl;
      status = EXIT_FAILURE;
    }
  }

  // Case 3: same scalars as Case 1, but with insideOut on, so now the
  // polygon is entirely on the discarded side and should produce no cells.
  {
    vtkIdType numberOfOutputCells;
    double totalArea;
    ClipSquare(square, allKept, 1.0, /*insideOut=*/1, numberOfOutputCells, totalArea);

    if (numberOfOutputCells != 0)
    {
      std::cerr << "ERROR: expected insideOut to discard the fully-kept polygon, got "
                << numberOfOutputCells << " cells" << std::endl;
      status = EXIT_FAILURE;
    }
  }

  // Case 4: the polygon straddles the clip value, so it is triangulated and
  // clipped as before, producing fragments whose combined area is exactly
  // half of the square.
  {
    vtkIdType numberOfOutputCells;
    double totalArea;
    ClipSquare(square, straddling, 1.0, /*insideOut=*/0, numberOfOutputCells, totalArea);

    if (numberOfOutputCells == 0)
    {
      std::cerr << "ERROR: expected the straddling polygon to produce clipped output cells"
                << std::endl;
      status = EXIT_FAILURE;
    }
    if (!vtkMathUtilities::NearlyEqual<double>(totalArea, 2.0))
    {
      std::cerr << "ERROR: expected the straddling polygon's clipped area to be 2.0, got "
                << totalArea << std::endl;
      status = EXIT_FAILURE;
    }
  }

  return status;
}
