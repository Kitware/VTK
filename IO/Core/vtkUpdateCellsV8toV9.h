/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUpdateCellsV8toV9.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUpdateCellsV8toV9
 * @brief   Update cells from v8 node layout to v9 node layout
 */

#ifndef vtkUpdateCellsV8toV9_h
#define vtkUpdateCellsV8toV9_h

#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkCellTypes.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkUnstructuredGrid.h"

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
      int order =
        static_cast<int>(round(std::cbrt(static_cast<int>(oldpts->GetNumberOfIds())))) - 1;
      for (int j = 0; j < oldpts->GetNumberOfIds(); j++)
      {
        int newid = vtkHigherOrderHexahedron::NodeNumberingMappingFromVTK8To9(order, j);
        if (j != newid)
        {
          newpts->SetId(j, oldpts->GetId(newid));
        }
      }
      output->GetCells()->ReplaceCellAtId(i, newpts);
    }
  }
}

inline bool vtkNeedsNewFileVersionV8toV9(vtkCellTypes* cellTypes)
{
  int nCellTypes = cellTypes->GetNumberOfTypes();
  for (vtkIdType i = 0; i < nCellTypes; ++i)
  {
    unsigned char type = cellTypes->GetCellType(i);
    if (type == VTK_HIGHER_ORDER_HEXAHEDRON || type == VTK_LAGRANGE_HEXAHEDRON ||
      type == VTK_BEZIER_HEXAHEDRON)
    {
      return true;
    }
  }
  return false;
}

#endif // vtkUpdateCellsV8toV9_h
// VTK-HeaderTest-Exclude: vtkUpdateCellsV8toV9.h
