// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnstructuredGridToCellGrid
 * @brief   Create a cell-grid that approximates a collection of vtkUnstructuredGrids.
 *
 * This filter takes a partitioned dataset collection (or a single unstructured grid)
 * as input, iterates each block's cells to find the shapes and orders of cells present;
 * then it creates empty vtkCellGrids and runs its internal TranscribeQuery to construct
 * cells and cell-attributes to match each input unstructured-grid.
 *
 * Note that this filter assumes all the cells of the same shape in
 * the vtkUnstructuredGrid are of the same order. This matches
 * assumptions in the IOSS reader but may not be true of arbitrary
 * unstructured grids.
 *
 * ## Design notes
 *
 * Even though this filter is named as though it processes a single unstructured grid
 * at a time, it is multiblock aware (and indeed, always outputs a partitioned dataset
 * collection). This was done in order to properly handle IOSS data, which includes
 * metadata on the parent composite dataset that is relevant to conversions of individual
 * child objects it contains.
 *
 * There is no reason – apart from the development time required – that
 * this filter couldn't accept any vtkDataSet as input (rather than just
 * unstructured grids). However, without some analogs to structured
 * dataset types, this would generally perform poorly. A set of spline
 * cells would make representing many structured datasets space-efficient
 * and amenable to smoothing/simplification.
 */
#ifndef vtkUnstructuredGridToCellGrid_h
#define vtkUnstructuredGridToCellGrid_h

#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include "vtkCellGrid.h"                         // for API + ivars
#include "vtkFiltersCellGridModule.h"            // For export macro
#include "vtkNew.h"                              // for ivar
#include "vtkStringToken.h"                      // for API + ivars
#include "vtkUnstructuredGridFieldAnnotations.h" // for API + ivars

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkDataSetAttributes;
class vtkUnstructuredGrid;
class vtkPartitionedDataSetCollection;

class VTKFILTERSCELLGRID_EXPORT vtkUnstructuredGridToCellGrid
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkUnstructuredGridToCellGrid* New();
  vtkTypeMacro(vtkUnstructuredGridToCellGrid, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Reset();
  virtual void AddPreferredOutputType(
    int inputCellType, vtkStringToken preferredOutputType, int priority);

  /// An internal query object for transcribing cells from input to output.
  ///
  /// Note that before this query is called, the \a Input grid has its
  /// cells counted by type and each type of cell has been claimed by
  /// a responder.
  class TranscribeQuery : public vtkCellGridQuery
  {
  public:
    vtkTypeMacro(vtkUnstructuredGridToCellGrid::TranscribeQuery, vtkCellGridQuery);
    static TranscribeQuery* New();

    bool Initialize() override;
    bool Finalize() override;

    /// Sum counts of all input cell types that generate the same output cell type.
    ///
    /// This returns true if all the input cells are claimed or false if some are
    /// unclaimed.
    bool SumOutputCounts();

    /// For each point- or cell-data array from the Input, create an output cell-attribute.
    void AddCellAttributes(vtkDataSetAttributes* attributes);

    /// A claim on input vtkCell types registered by an output vtkCellMetadata subclass.
    ///
    /// The vtkCellMetadata subclass with the highest priority replaces any prior entry
    /// with itself while subclasses with lower priorities take no action against
    /// higher-priority claims. Users may pre-populate vtkUnstructuredGridToCellGrid's
    /// TranscribeQuery with high-priority claims to affect the output.
    ///
    /// An invalid CellType token indicates the input cells are unclaimed.
    /// The filter may be configured to fail, warn, or quietly succeed when
    /// unsupported input-cell types are present.
    struct Claim
    {
      Claim() = default;
      Claim(const Claim&) = default;
      Claim& operator=(const Claim&) = default;
      Claim(vtkIdType numberOfCells, int priority, vtkStringToken cellType)
        : NumberOfCells(numberOfCells)
        , CellTypePriority(priority)
        , CellType(cellType)
      {
      }

      vtkIdType NumberOfCells{ 0 };
      int CellTypePriority{ 0 };
      vtkStringToken CellType;
    };

    /// The phase of the query: 0 → claiming input cells; 1 → transcribing claimed cells.
    int Phase{ 0 };
    /// The input dataset whose cells should be transcribed.
    vtkUnstructuredGrid* Input{ nullptr };
    /// The output cell-grid.
    vtkCellGrid* Output{ nullptr };
    /// The flat index of the current Input and Output data objects inside the collection.
    unsigned int FlatIndex{ 0 };
    /// The input point-coordinates (3-component) array, which is copied to the output.
    vtkDataArray* Coordinates{ nullptr };
    /// A map from input cell type to counts and the output cell type (if any).
    std::map<int, Claim> CellTypeMap;
    /// A map from output cell type-token to output count.
    std::unordered_map<vtkStringToken, vtkIdType> OutputAllocations;

    /// A key for indexing fields defined on a partitioned dataset collection entry.
    using BlockAttributesKey = vtkUnstructuredGridFieldAnnotations::BlockAttributesKey;

    /// Gloms of multiple field names that represent vectors or tensors.
    using FieldGlom = vtkUnstructuredGridFieldAnnotations::FieldGlom;

    /// Configuration hints for a partitioned dataset collection entry.
    using BlockAttributesValue = vtkUnstructuredGridFieldAnnotations::BlockAttributesValue;

    /// Container for field annotations capture from the input unstructured grid.
    vtkNew<vtkUnstructuredGridFieldAnnotations> Annotations;

  protected:
    TranscribeQuery() = default;
    ~TranscribeQuery() override = default;

  private:
    TranscribeQuery(const TranscribeQuery&) = delete;
    void operator=(const TranscribeQuery&) = delete;
  };

protected:
  vtkUnstructuredGridToCellGrid();
  ~vtkUnstructuredGridToCellGrid() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  /// Look for IOSS (or other) annotations that aggregate arrays and add cell-attributes
  /// for them (marking those arrays as ineligible for use in AddCellAttributes later.
  void AddAnnotatedAttributes(vtkPartitionedDataSetCollection* input);

  /// Transcribe a single unstructured grid from the input collection.
  virtual bool ProcessUnstructuredGrid(vtkUnstructuredGrid* input, vtkCellGrid* output);

  vtkNew<TranscribeQuery> Request;

private:
  vtkUnstructuredGridToCellGrid(const vtkUnstructuredGridToCellGrid&) = delete;
  void operator=(const vtkUnstructuredGridToCellGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkUnstructuredGridToCellGrid_h
// VTK-HeaderTest-Exclude: vtkUnstructuredGridToCellGrid.h
