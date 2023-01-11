/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellGridBoundsQuery.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellGridBoundsQuery
 * @brief   Perform an operation on cells in a vtkCellMetadata instance.
 *
 * This is an empty base class that all query types must inherit.
 */

#ifndef vtkCellGridBoundsQuery_h
#define vtkCellGridBoundsQuery_h

#include "vtkCellGridQuery.h"

#include <array> // For Bounds ivar.

VTK_ABI_NAMESPACE_BEGIN
class vtkBoundingBox;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridBoundsQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridBoundsQuery* New();
  vtkTypeMacro(vtkCellGridBoundsQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize() override;
  void GetBounds(double* bds) VTK_SIZEHINT(6);
  void AddBounds(vtkBoundingBox& bbox);

protected:
  vtkCellGridBoundsQuery() = default;
  ~vtkCellGridBoundsQuery() override = default;

  std::array<double, 6> Bounds;

private:
  vtkCellGridBoundsQuery(const vtkCellGridBoundsQuery&) = delete;
  void operator=(const vtkCellGridBoundsQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridBoundsQuery_h
