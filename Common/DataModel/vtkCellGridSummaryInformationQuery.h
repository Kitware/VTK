// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridSummaryInformationQuery
 * @brief   Compute summary information for a vtkCellGrid in a single pass.
 *
 * This query combines two pieces of summary information that are computed
 * together over all vtkCellAttribute instances and all cell types:
 *
 * 1. **Range of polynomial orders** – the minimum and maximum value of
 *    vtkCellAttribute::CellTypeInfo::Order across all attributes and cell types.
 *    Call GetOrderRange() after running the query.
 *    An invalid range (range[0] > range[1]) means no attributes were found.
 *
 * 2. **Degrees of freedom (DOF) count** – per attribute, depending on whether
 *    the attribute uses DOF sharing:
 *    - DOFSharing valid (continuous field): number of unique point IDs in the
 *      connectivity array, minus any ghost point IDs.
 *    - DOFSharing invalid (discontinuous field): GetNumberOfValues() on the
 *      "values" array.
 *    Contributions from each cell type are summed (approximation for mixed grids).
 *    Call GetNumberOfDOF() or GetNumberOfDOF(attribute) after running the query.
 */

#ifndef vtkCellGridSummaryInformationQuery_h
#define vtkCellGridSummaryInformationQuery_h

#include "vtkCellGridQuery.h"
#include "vtkCommonDataModelModule.h" // for export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

#include <array>         // For std::array
#include <limits>        // For std::numeric_limits
#include <unordered_map> // For std::unordered_map

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkCellGridSummaryInformationQuery
  : public vtkCellGridQuery
{
public:
  static vtkCellGridSummaryInformationQuery* New();
  vtkTypeMacro(vtkCellGridSummaryInformationQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Reset all accumulators before responders are invoked.
  bool Initialize() override;

  ///@{
  /**
   * The name of the attribute for which summary information is being computed.
   *
   * If none is set, it will be computed for all.
   */
  vtkSetStringMacro(AttributeName)
  vtkGetStringMacro(AttributeName)
  ///@}

  /// Information to collect on a per-vtkCellAttribute basis.
  struct SummaryInformation
  {
    /// The range of polynomial interpolation orders this attribute employs.
    ///
    /// Initialized to {INT_MAX, INT_MIN} so the first AddOrder() call sets both bounds.
    std::array<int, 2> OrderRange{ std::numeric_limits<int>::max(),
      std::numeric_limits<int>::min() };

    /// The number of degrees of freedom used to represent the corresponding cell attribute.
    ///
    /// Note that DOF shared by cells of different types or across different cell grids will
    /// be counted multiple times to avoid excessive memory consumption.
    vtkIdType DOFCount{ 0 };
  };

  /// Return the full per-attribute DOF count map.
  const std::unordered_map<vtkCellAttribute*, SummaryInformation>& GetSummaryInformationMap() const
  {
    return this->SummaryInformationMap;
  }

  /// Return summary information
  const SummaryInformation& GetSummaryInformation(vtkCellAttribute* att) const;

  /// Called by responders to accumulate Summary Information per attribute.
  void AddSummaryInformation(vtkCellAttribute* att, const SummaryInformation& summaryInformation);

  // -----------------------------------------------------------------------
  // Polynomial order range
  // -----------------------------------------------------------------------

  /// Return the [min, max] range of polynomial orders found.
  ///
  /// An invalid range (range[0] > range[1]) means no attributes were encountered.
  void GetOrderRange(vtkCellAttribute*, int* range) const VTK_SIZEHINT(2);
  const std::array<int, 2>& GetOrderRange(vtkCellAttribute*) const;

  // -----------------------------------------------------------------------
  // Degrees of freedom
  // -----------------------------------------------------------------------

  /// Return the DOF count for a specific attribute (0 if not found).
  vtkIdType GetNumberOfDOF(vtkCellAttribute* att) const;

protected:
  vtkCellGridSummaryInformationQuery() = default;
  ~vtkCellGridSummaryInformationQuery() override;

private:
  vtkCellGridSummaryInformationQuery(const vtkCellGridSummaryInformationQuery&) = delete;
  void operator=(const vtkCellGridSummaryInformationQuery&) = delete;

  std::unordered_map<vtkCellAttribute*, SummaryInformation> SummaryInformationMap;

  char* AttributeName = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridSummaryInformationQuery_h
