// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIdList.h"
#include "vtkStructuredCellArray.h"
#include "vtkStructuredData.h"

int TestStructuredCellArrayExtent(int extent[6])
{
  int dims[3] = { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1 };

  vtkNew<vtkStructuredCellArray> implicitCellArray;
  implicitCellArray->SetData(extent, true);

  vtkNew<vtkIdList> idList1, idList2;
  const auto dataDescription = vtkStructuredData::GetDataDescription(dims);
  for (vtkIdType i = 0, max = implicitCellArray->GetNumberOfCells(); i < max; ++i)
  {
    vtkStructuredData::GetCellPoints(i, idList1, dataDescription, dims);
    implicitCellArray->GetCellAtId(i, idList2);
    if (idList1->GetNumberOfIds() != idList2->GetNumberOfIds())
    {
      vtkErrorWithObjectMacro(nullptr,
        << "Error: cell " << i << " has " << idList1->GetNumberOfIds()
        << " points, but GetCellAtId() returned " << idList2->GetNumberOfIds() << " points.");
      return EXIT_FAILURE;
    }
    for (vtkIdType j = 0; j < idList1->GetNumberOfIds(); ++j)
    {
      if (idList1->GetId(j) != idList2->GetId(j))
      {
        vtkErrorWithObjectMacro(nullptr, << "Error: cell " << i << " has point " << j << " with id "
                                         << idList1->GetId(j) << ", but GetCellAtId() returned id "
                                         << idList2->GetId(j));
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

int TestStructuredCellArray(int, char*[])
{
  int res = EXIT_SUCCESS;
  int extentX[6] = { 0, 30, 0, 0, 0, 0 };
  res &= TestStructuredCellArrayExtent(extentX);
  int extentY[6] = { 0, 0, 0, 19, 0, 0 };
  res &= TestStructuredCellArrayExtent(extentY);
  int extentZ[6] = { 0, 0, 0, 0, 0, 38 };
  res &= TestStructuredCellArrayExtent(extentZ);
  int extentXZ[6] = { 0, 30, 0, 0, 0, 38 };
  res &= TestStructuredCellArrayExtent(extentXZ);
  int extentYZ[6] = { 0, 0, 0, 19, 0, 38 };
  res &= TestStructuredCellArrayExtent(extentYZ);
  int extentXY[6] = { 0, 30, 0, 19, 0, 0 };
  res &= TestStructuredCellArrayExtent(extentXY);
  int extentXYZ[6] = { 0, 30, 0, 19, 0, 38 };
  res &= TestStructuredCellArrayExtent(extentXYZ);

  return res;
}
