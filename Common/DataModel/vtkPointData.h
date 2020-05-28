/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointData
 * @brief   represent and manipulate point attribute data
 *
 * vtkPointData is a class that is used to represent and manipulate
 * point attribute data (e.g., scalars, vectors, normals, texture
 * coordinates, etc.) Most of the functionality is handled by
 * vtkDataSetAttributes
 */

#ifndef vtkPointData_h
#define vtkPointData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSetAttributes.h"
#include "vtkDeprecation.h" // for VTK_DEPRECATED_IN_9_0_0

class VTKCOMMONDATAMODEL_EXPORT vtkPointData : public vtkDataSetAttributes
{
public:
  static vtkPointData* New();
  static vtkPointData* ExtendedNew();

  vtkTypeMacro(vtkPointData, vtkDataSetAttributes);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  VTK_DEPRECATED_IN_9_1_0("Use vtkFieldData::NullData")
  void NullPoint(vtkIdType ptId);

protected:
  vtkPointData() = default;
  ~vtkPointData() override = default;

private:
  vtkPointData(const vtkPointData&) = delete;
  void operator=(const vtkPointData&) = delete;
};

#endif
