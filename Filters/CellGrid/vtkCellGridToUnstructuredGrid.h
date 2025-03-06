// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridToUnstructuredGrid
 * @brief   Create an unstructured grid that approximates a cell-grid.
 *
 * All cell-grid attributes are mapped to point-data arrays.
 * Averaging is used so that discontinuous cell-attributes are
 * turned into continuous approximations.
 *
 * Currently, all cells and point-data are linear.
 *
 * Novel function spaces (those other than HGRAD) are sampled at
 * cell corner points.
 *
 * Because the query is simple, it is simply a child class of
 * the algorithm.
 */

#ifndef vtkCellGridToUnstructuredGrid_h
#define vtkCellGridToUnstructuredGrid_h

#include "vtkCellGrid.h"              // For subclass
#include "vtkCellType.h"              // For initializer
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // For ivar
#include "vtkStringToken.h"           // For ivars
#include "vtkUnstructuredGridAlgorithm.h"

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkIncrementalOctreePointLocator;

class VTKFILTERSCELLGRID_EXPORT vtkCellGridToUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  /// A query corresponding to this algorithm.
  ///
  /// This query gets run on the input cell-grid.
  class Query : public vtkCellGridQuery
  {
  public:
    static Query* New();
    vtkTypeMacro(vtkCellGridToUnstructuredGrid::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /// A placeholder for space to be occupied in a vtkCellArray.
    struct OutputAllocation
    {
      int CellType{ VTK_EMPTY_CELL };
      vtkIdType NumberOfCells{ 0 };
      vtkIdType NumberOfConnectivityEntries{ 0 };
      vtkIdType CellOffset{ 0 };
      vtkIdType ConnOffset{ 0 };
    };

    /// A map used to allocate space for the unstructured-grid's vtkCellArray.
    ///
    /// This maps input cell typenames to OutputAllocation structures.
    using OutputAllocations = std::unordered_map<vtkStringToken, OutputAllocation>;

    /// Passes performed by the query.
    ///
    /// In between CountOutputs and GenerateConnectivity, the query class will
    /// allocate vtkCellArray storage.
    ///
    /// In between GenerateConnectivity and GeneratePointData, the query class will
    /// allocate array storage for all arrays in the output vtkPointData.
    enum PassType : int
    {
      CountOutputs = 0, //!< Responders should insert into GetOutputAllocations().
      GenerateConnectivity =
        1, //!< Responders should insert points into the locator, point-count map, and connectivity.
      GeneratePointData = 2 //!< Responders should populate point-data.
    };

    bool Initialize() override;
    void StartPass() override;
    bool Finalize() override;

    /// Force three passes through this query.
    bool IsAnotherPassRequired() override { return this->Pass < PassType::GeneratePointData; }

    /// Get the request's output cell-grid.
    vtkUnstructuredGrid* GetOutput() const { return this->Output; }
    /// Get the request's input cell-grid.
    vtkCellGrid* GetInput() const { return this->Input; }

    /// Return the data structure used in the CountOutputs pass.
    OutputAllocations& GetOutputAllocations() { return this->OutputOffsets; }
    const OutputAllocations& GetOutputAllocations() const { return this->OutputOffsets; }

    /// Return an output attribute (or null).
    vtkDataArray* GetOutputArray(vtkCellAttribute* inputAttribute);

    /// Return the point-locator.
    ///
    /// Responders should use this to transform any input connectivity
    /// they have to connectivity entries referencing the output points
    /// using this locator. Insert points in the CountOutputs pass and
    /// fetch point IDs in the GenerateOuputs pass.
    vtkIncrementalOctreePointLocator* GetLocator() { return this->Locator.GetPointer(); }

    /// Return a map of per-cell-type point IDs to unstructured-grid point IDs.
    ///
    /// Responders should insert values as they use the incremental
    /// point-locator to transform connectivity. These maps can be
    /// tested to avoid incremental point insertion when possible.
    using ConnectivityTransformType = std::unordered_map<vtkIdType, vtkIdType>;
    ConnectivityTransformType& GetConnectivityTransform(vtkStringToken cellType)
    {
      return this->ConnectivityTransforms[cellType];
    }

    /// This map is used to count the number of references to output points.
    ///
    /// During the GenerateConnectivity pass, responders should increment
    /// values so each entry corresponds to the number of cells that
    /// reference the point ID which serves as the key.
    using ConnectivityCountType = std::map<vtkIdType, int>;
    ConnectivityCountType& GetConnectivityCount() { return this->ConnectivityCount; }

    /// The reciprocal of this->GetConnectivityCount().
    ///
    /// This vector is only valid during the GeneratePointData pass.
    using ConnectivityWeightType = std::vector<float>;
    ConnectivityWeightType& GetConnectivityWeights() { return this->ConnectivityWeights; }

  protected:
    friend class vtkCellGridToUnstructuredGrid;
    Query() = default;
    ~Query() override = default;

    vtkCellGrid* Input{ nullptr };
    vtkUnstructuredGrid* Output{ nullptr };

    /// Map output cell-typename to input cell-typename to count (offset after the
    /// AllocateOutputs pass has run).
    OutputAllocations OutputOffsets;
    // Map input to output attributes:
    std::unordered_map<vtkCellAttribute*, vtkDataArray*> AttributeMap;
    /// A locator used to insert cell-grid points into a vtkPoints instance.
    vtkNew<vtkIncrementalOctreePointLocator> Locator;
    /// Connectivity transforms per input cell type.
    std::unordered_map<vtkStringToken, ConnectivityTransformType> ConnectivityTransforms;
    /// Number of cells referencing a given output point.
    ConnectivityCountType ConnectivityCount;
    /// The reciprocal of every entry in ConnectivityCount.
    ConnectivityWeightType ConnectivityWeights;

  private:
    Query(const Query&) = delete;
    void operator=(const Query&) = delete;
  };

  static vtkCellGridToUnstructuredGrid* New();
  vtkTypeMacro(vtkCellGridToUnstructuredGrid, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCellGridToUnstructuredGrid() = default;
  ~vtkCellGridToUnstructuredGrid() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<Query> Request;

private:
  vtkCellGridToUnstructuredGrid(const vtkCellGridToUnstructuredGrid&) = delete;
  void operator=(const vtkCellGridToUnstructuredGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridToUnstructuredGrid_h
