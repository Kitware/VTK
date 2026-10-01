// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCellGridIsocontourPoints_h
#define vtkCellGridIsocontourPoints_h

#include "vtkCellGrid.h"                   // For subclass
#include "vtkCellGridQuery.h"              // For inheritance
#include "vtkDGBezierFindCriticalPoints.h" // For CriticalPoint
#include "vtkFiltersCellGridModule.h"      // For export macro
#include "vtkNew.h"                        // For ivar
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkStringToken.h"  // For vtkStringToken

#include <unordered_map> // For unordered_map
#include <vector>        // For vector

VTK_ABI_NAMESPACE_BEGIN

class vtkCellAttribute;
class vtkIdTypeArray;

/**
 * @class   vtkCellGridIsocontourPoints
 * @brief   Locate points on a cell-attribute's isocontour.
 *
 * This is the first stage of a Voronoi-based isocontouring algorithm for
 * curved cells; it produces the points that a later stage connects into
 * triangles.
 *
 * Cells of every dimension are searched by default; see Include1DCells,
 * Include2DCells and Include3DCells to restrict the search.
 *
 * The attribute must be a scalar attribute in the Bernstein-Bezier basis (see
 * vtkCellGridChangeBasis): a Bezier cell's coefficients bound its field within
 * their convex hull, so a cell whose coefficients do not bracket the isovalue
 * is rejected outright.
 *
 * For every remaining cell, the search gathers the cell's corners and its
 * critical points (see vtkCellGridFindCriticalPoints; those on the cell's
 * boundary are included). It then considers each line segment, in the cell's
 * parameter space, between a point whose value lies below the isovalue and
 * one whose value lies at or above it, as in marching cells. Every such
 * segment crosses the isocontour at least once, and one crossing is located
 * per segment; a segment ending exactly on the isovalue reports that end.
 *
 * The output is a vtkPolyData with one point per isocontour point found, at
 * its location in the grid's world-space geometry, and one poly-vertex per
 * source cell that found any, grouping that cell's points (a cell with none
 * gets no output cell). Point-data arrays report the originating cell's type
 * ("CellType", a vtkStringToken hash of its class name), its ID within that type
 * ("CellId"), the point's parametric coordinates ("RST"), the field's value
 * there ("Value", which equals the isovalue up to solver tolerance), and the
 * field's gradient there in world coordinates ("Gradient" if ComputeGradients
 * is on, as it is by default; zero for cells that are not volumetric, and where
 * the cell's shape is degenerate).
 *
 * When InterpolateAttributes is on (it is on by default), every cell-attribute
 * of the input but its shape is also evaluated at the points, in the cells that
 * found them, and added as a point-data array named like the attribute. An
 * attribute named like one of the arrays above ("CellType", "CellId", "RST",
 * "Value" or "Gradient") is not added.
 *
 * A point on a face or edge shared by several cells is found once per cell,
 * and a point on the isovalue may be the end of several segments of one cell,
 * so the output holds near-duplicates. They are left in place; use
 * vtkStaticCleanPolyData (without averaging, which would move points off the
 * isocontour) to remove them.
 */
class VTKFILTERSCELLGRID_EXPORT vtkCellGridIsocontourPoints : public vtkPolyDataAlgorithm
{
public:
  /** @class Query
   * @brief A cell-grid query for locating points on a cell-attribute's isocontour.
   */
  class Query : public vtkCellGridQuery
  {
  public:
    static Query* New();
    vtkTypeMacro(vtkCellGridIsocontourPoints::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /// The passes a responder is run through: first search every cell of
    /// its type (caching the results), then - once every cell type's count
    /// is known - size the output arrays once, then write the cached
    /// results into their allotted slice, and last, if InterpolateAttributes is
    /// on, evaluate the input's other attributes at the points written.
    enum PassType : int
    {
      CountOutputs = 0,
      AllocateOutputs = 1,
      GenerateOutputs = 2,
      EvaluateAttributes = 3
    };

    bool Initialize() override;

    /// Force three passes through this query, and a fourth if the attributes are
    /// to be evaluated.
    bool IsAnotherPassRequired() override
    {
      return this->Pass <
        (this->InterpolateAttributes ? PassType::EvaluateAttributes : PassType::GenerateOutputs);
    }

    /// Return the MTime also considering the cell-attribute.
    vtkMTimeType GetMTime() override;

    ///@{
    /// Specify the cell-attribute whose isocontour should be located.
    virtual void SetCellAttribute(vtkCellAttribute*);
    vtkGetObjectMacro(CellAttribute, vtkCellAttribute);
    ///@}

    ///@{
    /// Specify the isovalue.
    vtkSetMacro(Isovalue, double);
    vtkGetMacro(Isovalue, double);
    ///@}

    ///@{
    /// Specify which cells to search, by their dimension: 1-D cells (such as
    /// edges), 2-D cells (such as triangles and quadrilaterals) and 3-D cells
    /// (such as tetrahedra and hexahedra). All are searched by default.
    vtkSetMacro(Include1DCells, bool);
    vtkGetMacro(Include1DCells, bool);
    vtkSetMacro(Include2DCells, bool);
    vtkGetMacro(Include2DCells, bool);
    vtkSetMacro(Include3DCells, bool);
    vtkGetMacro(Include3DCells, bool);
    ///@}

    ///@{
    /// Specify the number of Witkin-Heckbert particle relaxation iterations
    /// to run after the initial line-intersection search. Zero disables
    /// relaxation (the default). Each iteration moves the points of a cell
    /// apart by adaptive repulsion (every point has its own repulsion radius),
    /// and points split or die where the density is too low or too high, so
    /// the points of a cell end up evenly spread, about as dense as they
    /// started on average. Higher values give a more uniform distribution at
    /// the cost of extra computation.
    vtkSetClampMacro(ParticleRelaxationIterations, int, 0, 1000);
    vtkGetMacro(ParticleRelaxationIterations, int);
    ///@}

    ///@{
    /// Specify whether the input's cell-attributes (but its shape) are evaluated
    /// at the points, into the output's point-data arrays named like them (the
    /// filter adds them). On by default; when off, the pass that evaluates them
    /// does not run.
    vtkSetMacro(InterpolateAttributes, bool);
    vtkGetMacro(InterpolateAttributes, bool);
    ///@}

    ///@{
    /// Specify whether the world-space gradient of the field is computed at the
    /// points, into the output's "Gradient" array (the filter adds it). On by
    /// default; when off, the Jacobians it needs are not evaluated.
    vtkSetMacro(ComputeGradients, bool);
    vtkGetMacro(ComputeGradients, bool);
    ///@}

    /// Return whether cells of \a dimension are to be searched.
    bool IncludesDimension(int dimension) const;

    /// Get the request's output point set.
    vtkPolyData* GetOutput() const { return this->Output; }

    /// Called by a responder during the CountOutputs pass to cache the
    /// isocontour points found for every cell of \a cellTypeName, indexed by
    /// that cell type's local cell ID - so the GenerateOutputs pass can write
    /// them to the output without searching for them again.
    void SetIsocontourPoints(vtkStringToken cellTypeName,
      std::vector<std::vector<vtk::basis::bezier::CriticalPoint>> isocontourPointsByCell);

    /// Return the isocontour points SetIsocontourPoints() cached for
    /// \a cellTypeName, or an empty vector if that cell type cached none.
    const std::vector<std::vector<vtk::basis::bezier::CriticalPoint>>& GetIsocontourPoints(
      vtkStringToken cellTypeName) const;

    /// Size the output's points and point-data arrays, assign each cell
    /// type a contiguous slice of them (GetCellTypeOffset()), build
    /// CellOffsets (GetCellOffset()), and build one poly-vertex output
    /// cell per source cell that has an isocontour point. A no-op after the
    /// first call, so every cell type's AllocateOutputs pass may call it
    /// unconditionally.
    void Allocate();

    /// Return \a cellTypeName's own base index into GetCellOffset()'s dense
    /// numbering. A hash lookup - resolve it once per cell type and pass it
    /// to GetCellOffset() rather than looking it up again on every cell.
    vtkIdType GetCellTypeOffset(vtkStringToken cellTypeName) const;

    /// Return the output index Allocate() assigned to the isocontour points
    /// of cell \a cellId (local to its type), given that type's own
    /// \a cellTypeOffset (from GetCellTypeOffset()) - whether or not it has
    /// any (in which case this equals the next cell's own offset).
    vtkIdType GetCellOffset(vtkIdType cellTypeOffset, vtkIdType cellId) const;

  protected:
    friend class vtkCellGridIsocontourPoints;
    Query() = default;
    ~Query() override;

    vtkPolyData* Output{ nullptr };
    vtkCellAttribute* CellAttribute{ nullptr };
    double Isovalue{ 0. };
    bool Include1DCells{ true };
    bool Include2DCells{ true };
    bool Include3DCells{ true };
    int ParticleRelaxationIterations{ 0 };
    bool InterpolateAttributes{ true };
    bool ComputeGradients{ true };

    std::unordered_map<vtkStringToken, std::vector<std::vector<vtk::basis::bezier::CriticalPoint>>>
      IsocontourPoints;
    /// Each cell type's own base index into CellOffsets' dense, flattened
    /// (type, local cell ID) numbering.
    std::unordered_map<vtkStringToken, vtkIdType> CellIndexBase;
    /// Per-cell (dense, every type, including cells with none) prefix sum
    /// of isocontour-point counts, built by Allocate(); GetCellOffset() reads
    /// this.
    vtkSmartPointer<vtkIdTypeArray> CellOffsets;
    bool Allocated{ false };

  private:
    Query(const Query&) = delete;
    void operator=(const Query&) = delete;
  };

  static vtkCellGridIsocontourPoints* New();
  vtkTypeMacro(vtkCellGridIsocontourPoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Return the MTime also considering the query.
  vtkMTimeType GetMTime() override;

  /// Set/get the cell-attribute whose isocontour should be located.
  void SetCellAttribute(vtkCellAttribute* attribute);

  /// Set/get the isovalue.
  void SetIsovalue(double isovalue);
  double GetIsovalue() { return this->Request->GetIsovalue(); }

  ///@{
  /// Set/get whether the input's cell-attributes (but its shape) are evaluated at
  /// the output points and output as point-data arrays (see the class
  /// description). On by default.
  void SetInterpolateAttributes(bool interpolate);
  bool GetInterpolateAttributes() { return this->Request->GetInterpolateAttributes(); }
  vtkBooleanMacro(InterpolateAttributes, bool);
  ///@}

  ///@{
  /// Set/get whether the field's gradient in world coordinates is computed at the
  /// output points ("Gradient"). On by default.
  void SetComputeGradients(bool compute);
  bool GetComputeGradients() { return this->Request->GetComputeGradients(); }
  vtkBooleanMacro(ComputeGradients, bool);
  ///@}

  ///@{
  /// Set/get which cells to search, by their dimension: 1-D cells (such as
  /// edges), 2-D cells (such as triangles and quadrilaterals) and 3-D cells
  /// (such as tetrahedra and hexahedra). All are searched by default.
  void SetInclude1DCells(bool include);
  bool GetInclude1DCells() { return this->Request->GetInclude1DCells(); }
  vtkBooleanMacro(Include1DCells, bool);
  void SetInclude2DCells(bool include);
  bool GetInclude2DCells() { return this->Request->GetInclude2DCells(); }
  vtkBooleanMacro(Include2DCells, bool);
  void SetInclude3DCells(bool include);
  bool GetInclude3DCells() { return this->Request->GetInclude3DCells(); }
  vtkBooleanMacro(Include3DCells, bool);
  ///@}

  ///@{
  /// Set/get the number of Witkin-Heckbert particle relaxation iterations.
  /// Zero disables relaxation (the default). See the Query class for details.
  void SetParticleRelaxationIterations(int iterations);
  int GetParticleRelaxationIterations() { return this->Request->GetParticleRelaxationIterations(); }
  ///@}

protected:
  vtkCellGridIsocontourPoints();
  ~vtkCellGridIsocontourPoints() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  vtkNew<Query> Request;

private:
  vtkCellGridIsocontourPoints(const vtkCellGridIsocontourPoints&) = delete;
  void operator=(const vtkCellGridIsocontourPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridIsocontourPoints_h
