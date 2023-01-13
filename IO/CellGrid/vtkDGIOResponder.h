// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGIOResponder
 * @brief   Read/write metadata specific to discontinuous Galerkin cells.
 *
 * vtkDGIOResponder is a concrete responder to vtkCellGridIOQuery objects.
 *
 * @sa
 * vtkCellGridIOQuery
 * vtkCellGridResponder
 */

#ifndef vtkDGIOResponder_h
#define vtkDGIOResponder_h

#include "vtkCellGridIOQuery.h" // For base class
#include "vtkCellGridResponder.h"
#include "vtkSmartPointer.h" // For SmartPointer

#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class VTKIOCELLGRID_EXPORT vtkDGIOResponder : public vtkCellGridResponder<vtkCellGridIOQuery>
{
public:
  static vtkDGIOResponder* New();
  vtkTypeMacro(vtkDGIOResponder, vtkCellGridResponder<vtkCellGridIOQuery>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Query(
    vtkCellGridIOQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches) override;

protected:
  vtkDGIOResponder() = default;
  ~vtkDGIOResponder() override = default;

private:
  vtkDGIOResponder(const vtkDGIOResponder&) = delete;
  void operator=(const vtkDGIOResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkDGIOResponder.h
