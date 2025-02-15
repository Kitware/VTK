// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObject.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyData.h"

bool TestGetAssociationTypeFromString()
{
  bool ret = true;
  ret &= (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::FIELD_ASSOCIATION_POINTS") ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);
  ret &= (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::FIELD_ASSOCIATION_CELLS") ==
    vtkDataObject::FIELD_ASSOCIATION_CELLS);
  ret &= (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::FIELD_ASSOCIATION_NONE") ==
    vtkDataObject::FIELD_ASSOCIATION_NONE);
  ret &= (vtkDataObject::GetAssociationTypeFromString(
            "vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS") ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS);
  ret &=
    (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::FIELD_ASSOCIATION_VERTICES") ==
      vtkDataObject::FIELD_ASSOCIATION_VERTICES);
  ret &= (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::FIELD_ASSOCIATION_EDGES") ==
    vtkDataObject::FIELD_ASSOCIATION_EDGES);
  ret &= (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::FIELD_ASSOCIATION_ROWS") ==
    vtkDataObject::FIELD_ASSOCIATION_ROWS);

  ret &=
    (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::POINT") == vtkDataObject::POINT);
  ret &=
    (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::CELL") == vtkDataObject::CELL);
  ret &=
    (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::FIELD") == vtkDataObject::FIELD);
  ret &= (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::POINT_THEN_CELL") ==
    vtkDataObject::POINT_THEN_CELL);
  ret &=
    (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::VERTEX") == vtkDataObject::VERTEX);
  ret &=
    (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::EDGE") == vtkDataObject::EDGE);
  ret &= (vtkDataObject::GetAssociationTypeFromString("vtkDataObject::ROW") == vtkDataObject::ROW);

  ret &= (vtkDataObject::GetAssociationTypeFromString(nullptr) == -1);
  ret &= (vtkDataObject::GetAssociationTypeFromString("") == -1);
  ret &= (vtkDataObject::GetAssociationTypeFromString("INVALID") == -1);

  if (!ret)
  {
    vtkLog(ERROR, "Unexpected GetAssociationTypeFromString result.");
  }
  return ret;
}

bool TestGhostArray()
{
  bool ret = true;

  // Test using a concrete vtkPolyData instance
  vtkNew<vtkPolyData> pd;
  for (int type = 0; type < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; type++)
  {
    ret &= (!pd->vtkDataObject::SupportsGhostArray(type));
    ret &= (pd->vtkDataObject::GetGhostArray(type) == nullptr);
  }

  if (!ret)
  {
    vtkLog(ERROR, "Unexpected ghost array result.");
  }
  return ret;
}

int TestDataObject(int, char*[])
{
  bool ret = true;
  ret &= TestGetAssociationTypeFromString();
  ret &= TestGhostArray();
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
