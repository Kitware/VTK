/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests vtkPolyData

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"

namespace
{
//------------------------------------------------------------------------------
bool TestRemoveGhostCells()
{
  vtkNew<vtkPolyData> pd;
  pd->SetVerts(vtkNew<vtkCellArray>());
  pd->SetLines(vtkNew<vtkCellArray>());
  pd->SetPolys(vtkNew<vtkCellArray>());
  pd->SetStrips(vtkNew<vtkCellArray>());
  vtkNew<vtkPoints> points;
  vtkNew<vtkIdList> pointIds;
  vtkNew<vtkUnsignedCharArray> ghosts;
  vtkNew<vtkIdTypeArray> ids;

  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
  ids->SetName("Ids");

  double p[3] = { 0.0, 0.0, 0.0 };
  points->SetNumberOfPoints(5);
  points->SetPoint(0, p);
  p[0] = -1.0;
  points->SetPoint(1, p);
  p[0] = 1.0;
  points->SetPoint(2, p);
  p[1] = 1.0;
  points->SetPoint(3, p);
  p[2] = 1.0;
  points->SetPoint(4, p);

  pd->SetPoints(points);

  pointIds->SetNumberOfIds(1);
  pointIds->SetId(0, 0);
  ghosts->InsertNextValue(vtkDataSetAttributes::DUPLICATECELL);
  pd->InsertNextCell(VTK_VERTEX, pointIds);
  pointIds->SetId(0, 1);
  ghosts->InsertNextValue(0);
  pd->InsertNextCell(VTK_VERTEX, pointIds);

  pointIds->SetNumberOfIds(2);
  pointIds->SetId(0, 3);
  pointIds->SetId(1, 1);
  ghosts->InsertNextValue(0);
  pd->InsertNextCell(VTK_LINE, pointIds);
  pointIds->SetId(1, 2);
  ghosts->InsertNextValue(vtkDataSetAttributes::DUPLICATECELL);
  pd->InsertNextCell(VTK_LINE, pointIds);

  pointIds->SetNumberOfIds(3);
  pointIds->SetId(0, 4);
  pointIds->SetId(1, 1);
  pointIds->SetId(2, 2);
  ghosts->InsertNextValue(vtkDataSetAttributes::DUPLICATECELL);
  pd->InsertNextCell(VTK_TRIANGLE, pointIds);
  pointIds->SetId(2, 3);
  ghosts->InsertNextValue(0);
  pd->InsertNextCell(VTK_TRIANGLE, pointIds);

  pointIds->SetNumberOfIds(3);
  pointIds->SetId(0, 1);
  pointIds->SetId(1, 2);
  pointIds->SetId(2, 3);
  ghosts->InsertNextValue(0);
  pd->InsertNextCell(VTK_TRIANGLE_STRIP, pointIds);
  pointIds->SetId(2, 4);
  ghosts->InsertNextValue(0);
  pd->InsertNextCell(VTK_TRIANGLE_STRIP, pointIds);
  pointIds->SetId(1, 3);
  ghosts->InsertNextValue(vtkDataSetAttributes::DUPLICATECELL);
  pd->InsertNextCell(VTK_TRIANGLE_STRIP, pointIds);

  pd->GetCellData()->AddArray(ghosts);

  ids->SetNumberOfValues(ghosts->GetNumberOfValues());
  for (vtkIdType cellId = 0; cellId < ids->GetNumberOfValues(); ++cellId)
  {
    ids->SetValue(cellId, cellId);
  }

  pd->GetCellData()->AddArray(ids);

  // ghosts: 1 0 0 1 1 0 0 0
  // ids:    0 1 2 3 4 5 6 7

  // Add dummy point data and field data
  vtkNew<vtkIdTypeArray> pointDataIds;
  pointDataIds->SetName("pointDataIds");
  for (vtkIdType pointId = 0; pointId < pd->GetNumberOfPoints(); ++pointId)
  {
    pointDataIds->InsertNextValue(pointId);
  }
  pd->GetPointData()->AddArray(pointDataIds);

  vtkNew<vtkIdTypeArray> field;
  field->SetName("field");
  field->InsertNextValue(17);
  pd->GetFieldData()->AddArray(field);

  pd->RemoveGhostCells();

  const int nVerts = 1, nLines = 1, nPolys = 1, nStrips = 2;

  if (pd->GetNumberOfVerts() != nVerts || pd->GetNumberOfLines() != nLines ||
    pd->GetNumberOfPolys() != nPolys || pd->GetNumberOfStrips() != nStrips)
  {
    vtkLog(ERROR, "Removing ghosts failed... Wrong number of cells.");
    vtkLog(INFO, << pd->GetNumberOfVerts() << ", " << pd->GetNumberOfLines() << ", "
                 << pd->GetNumberOfPolys() << ", " << pd->GetNumberOfStrips());
    return false;
  }

  vtkIdTypeArray* newIds =
    vtkArrayDownCast<vtkIdTypeArray>(pd->GetCellData()->GetAbstractArray(ids->GetName()));

  if (newIds->GetValue(0) != 1 || newIds->GetValue(1) != 2 || newIds->GetValue(2) != 5 ||
    newIds->GetValue(3) != 6 || newIds->GetValue(4) != 7)
  {
    vtkLog(ERROR, "Removing ghosts failed... Wrong cell mapping.");
    return false;
  }

  // The first point should have been removed.
  if (pd->GetNumberOfPoints() != 4)
  {
    vtkLog(ERROR, "Removing ghosts fails... Wrong number of points.");
    return false;
  }

  // Check point data is still present and of the expected size
  vtkAbstractArray* ptArray = pd->GetPointData()->GetAbstractArray(pointDataIds->GetName());
  if (!ptArray || ptArray->GetNumberOfValues() != 4)
  {
    vtkLog(ERROR, "Removing ghosts failed... Unexepected point data content.");
    return false;
  }
  vtkIdTypeArray* fArray =
    vtkArrayDownCast<vtkIdTypeArray>(pd->GetFieldData()->GetAbstractArray(field->GetName()));
  if (!fArray || fArray->GetNumberOfValues() != 1 || fArray->GetValue(0) != 17)
  {
    vtkLog(ERROR, "Removing ghosts failed... Unexepected field data content.");
    return false;
  }

  return true;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int otherPolyData(int, char*[])
{
  int retVal = EXIT_SUCCESS;

  if (!::TestRemoveGhostCells())
  {
    retVal = EXIT_FAILURE;
  }

  return retVal;
}
