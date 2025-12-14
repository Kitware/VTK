// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridSampleQuery
 * @brief   Output a table of samples at zero or more points inside each cell.
 *
 * Attributes common to all input cell-types should be interpolated to each output vertex.
 *
 * Responders are free to choose the number of samples inside each cell but
 * in general, the samples should be chosen so that if used for quadrature they
 * will accurately estimate integral values over each cell.
 */

#ifndef vtkCellGridSampleQuery_h
#define vtkCellGridSampleQuery_h

#include "vtkCellGrid.h" // For API
#include "vtkCellGridQuery.h"
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkIdTypeArray.h"           // For API
#include "vtkSmartPointer.h"          // For API
#include "vtkStringToken.h"           // For API
#include "vtkTable.h"                 // For API

VTK_ABI_NAMESPACE_BEGIN

class vtkCellAttribute;
class vtkCellGrid;
class vtkDataArray;
class vtkIdTypeArray;
class vtkTable;
class vtkTypeUInt32Array;

class VTKFILTERSCELLGRID_EXPORT vtkCellGridSampleQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridSampleQuery* New();
  vtkTypeMacro(vtkCellGridSampleQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get whether the sample table should include cell type and cell ID columns.
   *
   * The default is to omit these columns.
   */
  vtkSetMacro(IncludeSourceCellInfo, bool);
  vtkGetMacro(IncludeSourceCellInfo, bool);
  vtkBooleanMacro(IncludeSourceCellInfo, bool);
  ///@}

  ///@{
  /**
   * Set/get whether the sample table should include a column for parametric coordinates.
   *
   * The default is to omit this column. If included, it will be a vtkDataArray with
   * as many components as largested parametric dimension of all input cell types.
   * For example, if a dataset has vertices, lines, and quadrilateral cells, this will have 2
   * components per tuple. If a dataset has triangular and tetrahedral cells, this will have 3
   * components per tuple. Note that cells are not required to have parametric coordinates, so
   * values may be meaningless.
   */
  vtkSetMacro(IncludeSourceCellSite, bool);
  vtkGetMacro(IncludeSourceCellSite, bool);
  vtkBooleanMacro(IncludeSourceCellSite, bool);
  ///@}

  /// A map used to allocate output table rows for various input cell types.
  using OutputAllocations = std::unordered_map<vtkStringToken, vtkIdType>;

  /// Passes used during processing of this query.
  enum PassType : int
  {
    CountOutputs = 0,   //!< Responders call AddOutputSamples with an output row count.
    GenerateOutputs = 1 //!< Responders insert samples into the output table.
  };

  /// Overridden
  bool Initialize() override;
  void StartPass() override;

  /// Force two passes through this query.
  bool IsAnotherPassRequired() override { return this->Pass < PassType::GenerateOutputs; }

  ///@{
  /**
   * Get/set the request's input cell-grid.
   */
  vtkSetSmartPointerMacro(Input, vtkCellGrid);
  vtkGetSmartPointerMacro(Input, vtkCellGrid);
  ///@}

  ///@{
  /** Set/get the request's output table.
   *
   * Users of this query must set the table they wish populated with samples.
   */
  vtkSetSmartPointerMacro(Output, vtkTable);
  vtkGetSmartPointerMacro(Output, vtkTable);
  ///@}

  /// Responders should call this method during the CountOutputs pass to
  /// allocate space in an output cell type.
  void AddOutputSamples(vtkStringToken inputCellType, vtkIdType numberOfOutputs);

  /// Responders should call this method during the GenerateOutputs pass to
  /// obtain the starting row of the table where they can write their samples.
  vtkIdType GetSampleOffset(vtkStringToken inputCellType);

  /// Return an output table column (or null) given an input cell attribute.
  ///
  /// Responders may use this method to obtain an output data array to hold
  /// sample values of a particular attribute.
  vtkDataArray* GetOutputAttributeColumn(vtkCellAttribute* inputAttribute);

  /// Return the data array holding the input cell ID of each sample (if requested).
  ///
  /// Responders are expected to set values in this array for their samples when
  /// this array is non-null.
  ///
  /// If the input cell ID is not requested by IncludeSourceCellInfo,
  /// this will return null.
  vtkIdTypeArray* GetSourceCellIdColumn() { return this->SourceCellId; }

  /// Return the data array holding the parametric coordinates (if applicable) of
  /// each sample in the output table.
  ///
  /// Responders are expected to set values in this array for their samples when
  /// this array is non-null.
  ///
  /// If the parametric coordinates are not requested by IncludeSourceCellSite,
  /// this will return null.
  vtkDataArray* GetSourceCellSiteColumn() { return this->SourceCellSite; }

  /// Return the data structure that AddOutputSamples() modifies in
  /// the PassType::CountOutputs pass.
  OutputAllocations& GetOutputAllocations() { return this->OutputOffsets; }
  const OutputAllocations& GetOutputAllocations() const { return this->OutputOffsets; }

  ///@{
  /**
   * Set/get the largest parametric dimension across all cells.
   *
   * Responders should call this during the CountOutputs pass if their cells' maximum
   * parametric dimension is larger than its current value.
   *
   * At the start of the GenerateOutputs pass, this is used to allocate
   * this->SourceCellSites array, if present.
   */
  vtkSetMacro(MaximumParametricDimension, int);
  vtkGetMacro(MaximumParametricDimension, int);
  ///@}

protected:
  vtkCellGridSampleQuery() = default;
  ~vtkCellGridSampleQuery() override = default;

private:
  vtkSmartPointer<vtkCellGrid> Input;
  vtkSmartPointer<vtkTable> Output;

  bool IncludeSourceCellInfo{ false };
  bool IncludeSourceCellSite{ false };
  int MaximumParametricDimension{ 0 };

  /// Map an input cell-typename to count (offset after the AllocateOutputs pass has run).
  OutputAllocations OutputOffsets;
  /// Map an input cell-attribute to an output table column:
  std::unordered_map<vtkCellAttribute*, vtkDataArray*> AttributeMap;
  /// If IncludeSourceCellInfo is true, this will hold the cell-type hash for each sample.
  vtkSmartPointer<vtkTypeUInt32Array> SourceCellType;
  /// If IncludeSourceCellInfo is true, this will hold the cell ID for each sample.
  vtkSmartPointer<vtkIdTypeArray> SourceCellId;
  /// If IncludeSourceCellSite is true, this will hold the parametric coordinates of each sample.
  vtkSmartPointer<vtkDataArray> SourceCellSite;

  vtkCellGridSampleQuery(const vtkCellGridSampleQuery&) = delete;
  void operator=(const vtkCellGridSampleQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridSampleQuery_h
