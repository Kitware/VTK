/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDGBoundsResponder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDGBoundsResponder
 * @brief   Respond to a query on one particular type of cell.
 *
 * This is pure virtual base class that all responder types must inherit.
 */

#ifndef vtkDGBoundsResponder_h
#define vtkDGBoundsResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridBoundsQuery.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;
class vtkDGBoundsResponders;

class VTKFILTERSCELLGRID_EXPORT vtkDGBoundsResponder
  : public vtkCellGridResponder<vtkCellGridBoundsQuery>
{
public:
  static vtkDGBoundsResponder* New();
  vtkTypeMacro(vtkDGBoundsResponder, vtkCellGridResponder<vtkCellGridBoundsQuery>);

  bool Query(vtkCellGridBoundsQuery* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGBoundsResponder() = default;
  ~vtkDGBoundsResponder() override = default;

private:
  vtkDGBoundsResponder(const vtkDGBoundsResponder&) = delete;
  void operator=(const vtkDGBoundsResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGBoundsResponder_h
// VTK-HeaderTest-Exclude: vtkDGBoundsResponder.h
