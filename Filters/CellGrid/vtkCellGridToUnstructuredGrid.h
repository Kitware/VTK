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
 * The output cells are always linear, but you may increase the
 * SubdivisionLevel in order to approximate curved geometry and
 * higher-order attributes: each input cell is subdivided into
 * 2^SubdivisionLevel pieces along each of its parametric axes, so a
 * SubdivisionLevel of 0 (the default) produces a single linear cell
 * per input cell while a SubdivisionLevel of 2 produces 4 pieces
 * along each axis (i.e., 64 cells for a hexahedron).
 *
 * Note that this is h-refinement of the output mesh, not p-refinement:
 * the level says how many times each cell is halved, never what degree
 * the output cells have.
 *
 * Each input shape is subdivided into cells of the same shape
 * where possible (hexahedra into hexahedra, wedges into wedges,
 * etc.); pyramids are the exception, as they subdivide into a
 * mix of pyramids and tetrahedra.
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
      /// A cell type representative of the output.
      ///
      /// This is only indicative. A responder subdividing a cell may emit more
      /// than one type, since pyramids also produce tetrahedra, and a responder
      /// transcribing the sides of a cell emits the type of the side.
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

    ///@{
    /// Set/get how many times each input cell is halved along every
    /// parametric axis.
    ///
    /// A level of 0 (the default) subdivides each input cell into a single
    /// linear output cell. Each increment doubles the number of subdivisions
    /// along every parametric axis of the input cell (see GetSubdivisionCount()).
    /// Negative levels are treated as 0; there is no upper limit beyond what
    /// the output can represent and what memory allows.
    vtkSetClampMacro(SubdivisionLevel, int, 0, VTK_INT_MAX);
    vtkGetMacro(SubdivisionLevel, int);
    ///@}

    ///@{
    /// Set/get how close two samples must be before they are merged into one
    /// output point, as a fraction of the input's bounding-box diagonal.
    ///
    /// Samples that neighboring cells make of a shared face have to merge, or
    /// the output falls apart into disconnected cells. Samples of a single cell
    /// must not, or that cell collapses. The gap between a cell's own samples
    /// halves with every level, so this has to stay well below it.
    vtkSetClampMacro(PointMergeTolerance, double, 0., VTK_DOUBLE_MAX);
    vtkGetMacro(PointMergeTolerance, double);
    ///@}

    /// Return true if the user has asked the filter to stop.
    ///
    /// A high level can keep a responder busy for a long time, so it needs a
    /// way to notice and give up. The query has no notion of aborting on its
    /// own, so it borrows the algorithm's.
    bool IsAborted() { return this->Algorithm && this->Algorithm->CheckAbort(); }

    /// Return the number of subdivisions along each parametric axis.
    ///
    /// This is 2^GetSubdivisionLevel(). Responders should use it rather than the
    /// level itself when generating output cells. Levels too large to shift by
    /// saturate, since no such subdivision could be allocated anyway.
    vtkIdType GetSubdivisionCount() const
    {
      constexpr int maxShift = 8 * static_cast<int>(sizeof(vtkIdType)) - 2;
      unsigned shift = static_cast<unsigned>(
        this->SubdivisionLevel > maxShift ? maxShift : this->SubdivisionLevel);
      return static_cast<vtkIdType>(1) << shift;
    }

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

    /// How many times each cell is halved per axis (see SetSubdivisionLevel()).
    int SubdivisionLevel{ 0 };
    /// The relative distance within which samples merge (see SetPointMergeTolerance()).
    double PointMergeTolerance{ 1e-12 };
    /// The algorithm running this query, used only to poll for an abort.
    vtkCellGridToUnstructuredGrid* Algorithm{ nullptr };

    /// Map output cell-typename to input cell-typename to count (offset after the
    /// AllocateOutputs pass has run).
    OutputAllocations OutputOffsets;
    // Map input to output attributes:
    std::unordered_map<vtkCellAttribute*, vtkDataArray*> AttributeMap;
    /// A locator used to insert cell-grid points into a vtkPoints instance.
    vtkNew<vtkIncrementalOctreePointLocator> Locator;
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

  ///@{
  /// Set/get how many times each input cell is halved along every parametric
  /// axis.
  ///
  /// Each input cell is subdivided into 2^SubdivisionLevel pieces along each of
  /// its parametric axes. The default (0) produces one linear output cell
  /// per input cell. Negative values are treated as 0.
  ///
  /// Output cells are always linear; this refines the output mesh (h-refinement)
  /// rather than raising the degree of its cells, and increasing it improves the
  /// approximation of curved geometry and of higher-order cell-attributes.
  ///
  /// No upper limit is imposed, but be aware that the number of output cells
  /// grows as 8^SubdivisionLevel for three-dimensional input cells: a level that
  /// cannot be represented or allocated makes the filter fail with an error
  /// rather than silently producing a coarser result.
  virtual void SetSubdivisionLevel(int level);
  virtual int GetSubdivisionLevel() const;
  ///@}

  ///@{
  /// Set/get how close two output points must be before they are merged,
  /// measured as a fraction of the input's bounding-box diagonal.
  ///
  /// Neighboring cells sample the faces they share at the same locations, and
  /// those samples have to merge for the output to hold together. Samples
  /// within one cell must stay distinct, or that cell collapses. The default of
  /// 1e-12 separates the two cases at any practical level. Raise it only if an
  /// input's coordinates disagree between neighboring cells by more than
  /// rounding, and lower it (to 0, for exact matches only) if an input really
  /// does contain points that close together.
  virtual void SetPointMergeTolerance(double tolerance);
  virtual double GetPointMergeTolerance() const;
  ///@}

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
