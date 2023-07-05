// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUpdateCellsV8toV9
 * @brief   Update cells from v8 node layout to v9 node layout
 */

#ifndef vtkUpdateCellsV8toV9_h
#define vtkUpdateCellsV8toV9_h

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellTypes.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
inline void vtkUpdateCellsV8toV9(vtkUnstructuredGrid* output)
{
  vtkNew<vtkIdList> oldpts, newpts;

  for (vtkIdType i = 0; i < output->GetNumberOfCells(); ++i)
  {
    vtkIdType type = output->GetCellTypesArray()->GetTypedComponent(i, 0);
    if (type == VTK_HIGHER_ORDER_HEXAHEDRON || type == VTK_LAGRANGE_HEXAHEDRON ||
      type == VTK_BEZIER_HEXAHEDRON)
    {
      output->GetCells()->GetCellAtId(i, oldpts);
      newpts->DeepCopy(oldpts);

      int degs[3];
      if (output->GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        vtkDataArray* v = output->GetCellData()->GetHigherOrderDegrees();
        double degs_double[3];
        v->GetTuple(i, degs_double);
        for (int ii = 0; ii < 3; ii++)
          degs[ii] = static_cast<int>(degs_double[ii]);
      }
      else
      {
        int order =
          static_cast<int>(round(std::cbrt(static_cast<int>(oldpts->GetNumberOfIds())))) - 1;
        degs[0] = degs[1] = degs[2] = order;
      }
      for (int j = 0; j < oldpts->GetNumberOfIds(); j++)
      {
        int newid = vtkHigherOrderHexahedron::NodeNumberingMappingFromVTK8To9(degs, j);
        if (j != newid)
        {
          newpts->SetId(j, oldpts->GetId(newid));
        }
      }
      output->GetCells()->ReplaceCellAtId(i, newpts);
    }
  }
}

inline bool vtkNeedsNewFileVersionV8toV9(vtkUnsignedCharArray* distinctCellTypes)
{
  vtkIdType nCellTypes = distinctCellTypes->GetNumberOfValues();
  for (vtkIdType i = 0; i < nCellTypes; ++i)
  {
    unsigned char type = distinctCellTypes->GetValue(i);
    if (type == VTK_HIGHER_ORDER_HEXAHEDRON || type == VTK_LAGRANGE_HEXAHEDRON ||
      type == VTK_BEZIER_HEXAHEDRON)
    {
      return true;
    }
  }
  return false;
}

VTK_ABI_NAMESPACE_END
#endif // vtkUpdateCellsV8toV9_h
// VTK-HeaderTest-Exclude: vtkUpdateCellsV8toV9.h
