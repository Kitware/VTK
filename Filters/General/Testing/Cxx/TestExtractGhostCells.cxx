// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExtractGhostCells.h"

#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStaticPointLocator.h"
#include "vtkStructuredData.h"
#include "vtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
bool TestValues(vtkDataSet* ref, vtkStaticPointLocator* locator, vtkDataSet* ds)
{
  vtkDataArray* refArray = ref->GetPointData()->GetArray("RTData");
  vtkDataArray* array = ds->GetPointData()->GetArray("RTData");

  for (vtkIdType pointId = 0; pointId < ds->GetNumberOfPoints(); ++pointId)
  {
    double* p = ds->GetPoint(pointId);
    vtkIdType refPointId = locator->FindClosestPoint(p);

    if (refArray->GetTuple1(refPointId) != array->GetTuple1(pointId))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
int TestExtractGhostCells(int, char*[])
{
  int retVal = EXIT_SUCCESS;

  vtkNew<vtkRTAnalyticSource> image;
  image->SetWholeExtent(-5, 5, -5, 5, -5, 5);

  vtkNew<vtkPointDataToCellData> point2cell;
  point2cell->SetInputConnection(image->GetOutputPort());
  point2cell->Update();

  vtkNew<vtkUnsignedCharArray> ghosts;

  vtkNew<vtkImageData> im;

  im->ShallowCopy(point2cell->GetOutputDataObject(0));

  ghosts->SetNumberOfValues(im->GetNumberOfCells());
  ghosts->FillValue(0);
  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());

  int kmin = 1, kmax = 6, jmin = 1, jmax = 5, imin = 1, imax = 3;
  int dims[3] = { 10, 10, 10 };
  int ijk[3];

  for (ijk[2] = kmin; ijk[2] < kmax; ++ijk[2])
  {
    for (ijk[1] = jmin; ijk[1] < jmax; ++ijk[1])
    {
      for (ijk[0] = imin; ijk[0] < imax; ++ijk[0])
      {
        vtkIdType cellId = vtkStructuredData::ComputeCellId(dims, ijk);
        ghosts->SetValue(cellId, vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL);
      }
    }
  }

  im->GetCellData()->AddArray(ghosts);

  vtkNew<vtkCellCenters> centers;
  centers->SetInputData(im);
  centers->Update();
  auto points = vtkDataSet::SafeDownCast(centers->GetOutputDataObject(0));

  vtkNew<vtkStaticPointLocator> locator;
  locator->SetDataSet(points);
  locator->BuildLocator();

  vtkNew<vtkExtractGhostCells> extract;
  extract->SetInputData(im);

  vtkNew<vtkCellCenters> outCenters;
  outCenters->SetInputConnection(extract->GetOutputPort());
  outCenters->Update();
  auto outPoints = vtkDataSet::SafeDownCast(outCenters->GetOutputDataObject(0));

  if (outPoints->GetNumberOfPoints() != (kmax - kmin) * (jmax - jmin) * (imax - imin))
  {
    vtkLog(ERROR, "Wrong number of ghost cells in output");
    retVal = EXIT_FAILURE;
  }

  if (!TestValues(points, locator, outPoints))
  {
    vtkLog(ERROR, "Extracting ghost cells failed for data set.");
    retVal = EXIT_FAILURE;
  }

  return retVal;
}
