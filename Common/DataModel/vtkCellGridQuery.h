/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellGridQuery.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellGridQuery
 * @brief   Perform an operation on cells in a vtkCellMetadata instance.
 *
 * This is an empty base class that all query types must inherit.
 *
 * The `vtkCellGrid::Query()` method calls the query's `Initialize()` implementation;
 * then loops over all its cell-types (calling the best-matching responder's `Query()`
 * method for that cell-type); then calls `Finalize()`.
 *
 * The responders have an opportunity to modify the state of the query object,
 * so these methods are a chance to prepare your query's state and then perform
 * reduce-like computations after all the cells have been handled.
 */

#ifndef vtkCellGridQuery_h
#define vtkCellGridQuery_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridQuery : public vtkObject
{
public:
  vtkTypeMacro(vtkCellGridQuery, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

  /// Override this if your query-result state requires initialization.
  virtual void Initialize() {}

  /// Override this if your query-result state requires finalization.
  virtual void Finalize() {}

protected:
  vtkCellGridQuery() = default;
  ~vtkCellGridQuery() override = default;

private:
  vtkCellGridQuery(const vtkCellGridQuery&) = delete;
  void operator=(const vtkCellGridQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridQuery_h
