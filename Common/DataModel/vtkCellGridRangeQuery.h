// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridRangeQuery
 * @brief   Compute the range of a component of some vtkCellAttribute.
 *
 * If \a FiniteRange is true, then the range will omit any NaN or ±Inf
 * values present in the data. Otherwise (the default), the range may
 * contain these exceptional values.
 *
 * If \a Component is
 * + -2 (the default), the range of L₂-norms is computed.
 * + -1, the range of L₁-norms is computed.
 * + out of bounds, then an invalid range will be returned ([1, 0]).
 *
 * Note that this query is intended to be run by vtkCellGrid::GetRange()
 * since the cell-grid holds a cache of ranges. You may run it outside
 * of this method, but that may cause unnecessary re-computation of ranges.
 */

#ifndef vtkCellGridRangeQuery_h
#define vtkCellGridRangeQuery_h

#include "vtkCellAttribute.h" // For Attribute ivar.
#include "vtkCellGridQuery.h"

#include <array>  // For Ranges ivar.
#include <vector> // For Ranges ivar.

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridRangeQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridRangeQuery* New();
  vtkTypeMacro(vtkCellGridRangeQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set/get the component whose range should be computed.
  ///
  /// This must be modified before the query is evaluated.
  vtkSetMacro(Component, int);
  vtkGetMacro(Component, int);

  /// Set/get whether component whose range should be computed.
  ///
  /// This must be modified before the query is evaluated.
  vtkSetMacro(FiniteRange, vtkTypeBool);
  vtkGetMacro(FiniteRange, vtkTypeBool);

  /// Set/get the cell-grid that holds the cell-attribute's arrays.
  ///
  /// This must be set before the query is evaluated.
  virtual void SetCellGrid(vtkCellGrid* grid);
  vtkGetObjectMacro(CellGrid, vtkCellGrid);

  /// Set/get the cell-attribute whose range should be computed.
  ///
  /// This must be modified before the query is evaluated.
  vtkSetObjectMacro(CellAttribute, vtkCellAttribute);
  vtkGetObjectMacro(CellAttribute, vtkCellAttribute);

  /// Invoked during evaluation before any cell-grid responders are run.
  bool Initialize() override;

  /// Invoked during evaluation after all cell-grid responders are run.
  bool Finalize() override;

  /// Return the computed range (after the query is evaluated).
  void GetRange(int component, double* range) VTK_SIZEHINT(2);
  const std::array<double, 2>& GetRange(int component) const;
  void GetRange(double* range) VTK_SIZEHINT(2) { this->GetRange(this->Component, range); }
  const std::array<double, 2>& GetRange() const { return this->GetRange(this->Component); }

  /// Used by query-responders to update the range during evaluation.
  ///
  /// Calling \a AddRange() with an invalid range has no effect.
  void AddRange(const std::array<double, 2>& other);

  /// This is an additional call that responders can use to provide range
  /// for components not currently queried.
  void AddRange(int component, const std::array<double, 2>& other);

  /// Store the finite/entire range for a single component of a cell-attribute.
  ///
  /// Each vtkCellGrid instance holds a map of these structures to accelerate
  /// range lookups and passes in the address of this map to us before running
  /// the query. This way the object holding the cell-attributes holds their
  /// cached ranges.
  ///
  /// The vtkCellAttribute itself cannot hold its cached component-ranges because
  /// it may be referenced by multiple vtkCellGrid instances.
  struct ComponentRange
  {
    /// When was the finite range last computed?
    vtkTimeStamp FiniteRangeTime;
    /// What is the finite-valued range?
    std::array<double, 2> FiniteRange;

    /// When was the entire range last computed?
    vtkTimeStamp EntireRangeTime;
    /// What is the true range (including possible NaN or Inf values)?
    std::array<double, 2> EntireRange;
  };
  using CacheMap = std::map<vtkCellAttribute*, std::vector<ComponentRange>>;

protected:
  vtkCellGridRangeQuery() = default;
  ~vtkCellGridRangeQuery() override;

  int Component{ -2 };
  vtkTypeBool FiniteRange{ false };
  vtkCellGrid* CellGrid{ nullptr };
  vtkCellAttribute* CellAttribute{ nullptr };
  std::vector<std::array<double, 2>> Ranges;

private:
  vtkCellGridRangeQuery(const vtkCellGridRangeQuery&) = delete;
  void operator=(const vtkCellGridRangeQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridRangeQuery_h
