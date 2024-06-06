// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Override this if your query-result state requires initialization.
  ///
  /// You may override this method to do additional work, but you must
  /// be careful to call the base method from your override.
  ///
  /// Returning false will abort processing of the query.
  /// No error message will be printed.
  virtual bool Initialize();

  /// Mark the start of a pass through each cell type.
  /// This increments the \a Pass ivar which responders can access.
  ///
  /// You may override this method to do additional work, but you must
  /// be careful to call the base method from your override.
  virtual void StartPass();

  /// Return the current pass (the number of times each responder has been evaluated so far).
  vtkGetMacro(Pass, int);

  /// Override this if your query allows responders to execute in multiple phases.
  /// This method may do work in addition to returning whether another pass is needed.
  virtual bool IsAnotherPassRequired() { return false; }

  /// Override this if your query-result state requires finalization.
  virtual bool Finalize() { return true; }

protected:
  vtkCellGridQuery() = default;
  ~vtkCellGridQuery() override = default;

  int Pass{ -1 };

private:
  vtkCellGridQuery(const vtkCellGridQuery&) = delete;
  void operator=(const vtkCellGridQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridQuery_h
