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

class VTKCOMMONDATAMODEL_EXPORT vtkPointData : public vtkDataSetAttributes
{
public:
  static vtkPointData *New();

  vtkTypeMacro(vtkPointData,vtkDataSetAttributes);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  void NullPoint(vtkIdType ptId);

protected:
  vtkPointData() {}
  ~vtkPointData() VTK_OVERRIDE {}

private:
  vtkPointData(const vtkPointData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointData&) VTK_DELETE_FUNCTION;
};

#endif


