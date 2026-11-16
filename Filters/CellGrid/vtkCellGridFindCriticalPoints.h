// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCellGridFindCriticalPoints_h
#define vtkCellGridFindCriticalPoints_h

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
 * @class   vtkCellGridFindCriticalPoints
 * @brief   Locate the interior critical points of a scalar cell-attribute.
 *
 * A curved (higher-order) cell's scalar field can have local minima, maxima,
 * and saddle points anywhere inside it, not only at its degrees of freedom.
 * A linear isocontouring algorithm only sees a cell's samples, so it misses
 * topology that occurs strictly between them - a loop in a 0-isocontour, for
 * instance. Locating these critical points is a prerequisite for subdividing
 * a cell so that a linear contouring algorithm produces a topologically
 * correct result.
 *
 * This filter requires the attribute be expressed in a Bernstein-Bezier
 * basis: a Bezier cell's coefficients bound the field within their convex
 * hull, which is what makes searching for every critical point tractable.
 * Convert an attribute with vtkCellGridChangeBasis first if it is not
 * already Bezier.
 *
 * The search covers every cell shape vtkDGCell supports - edges,
 * quadrilaterals and hexahedra, whose parameter space is a tensor product of
 * independent axes; triangles and tetrahedra, whose is a simplex; wedges,
 * which are one of each, a triangular cross-section extruded along an
 * independent axis; and pyramids, whose square cross-section collapses to
 * the apex - and only scalar (single-component) attributes; multi-component
 * attributes are left unprocessed for now.
 *
 * Critical points on a cell's faces and edges are located as well as those
 * in its interior; only its corners are skipped, being trivially critical
 * by construction.
 *
 * The output is a vtkPolyData holding one point per critical point found, at
 * its location in the grid's world-space geometry (the cell's shape
 * attribute evaluated at the critical point). Point-data arrays report the
 * originating cell's type ("CellType", a vtkStringToken hash of its class
 * name - see vtkCellGridEvaluator, which uses the same convention) and ID
 * within that type ("CellId"; a grid may hold more than one cell type, so
 * "CellId" alone does not identify a cell - "CellType" and "CellId"
 * together do), the point's parametric coordinates within that cell
 * ("RST"), the field's value there ("Value"), and its classification
 * ("Type": 0 = minimum, 1 = maximum, 2 = saddle).
 *
 * The output's vertex cells group a source cell's points together: one
 * poly-vertex per source cell that contributed at least one critical point
 * (a cell with none is skipped, not emitted as a degenerate zero-point
 * cell). See Query::GetCellOffset() to look up where a given cell's own
 * points begin, whether or not it has any.
 */
class VTKFILTERSCELLGRID_EXPORT vtkCellGridFindCriticalPoints : public vtkPolyDataAlgorithm
{
public:
  /** @class Query
   * @brief A cell-grid query for locating a cell-attribute's critical points.
   */
  class Query : public vtkCellGridQuery
  {
  public:
    static Query* New();
    vtkTypeMacro(vtkCellGridFindCriticalPoints::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /// The passes a responder is run through: first solve every cell of its
    /// type (caching the results, so the solve never runs twice), then -
    /// once every cell type's count is known - size the output arrays once,
    /// then write the cached results into their allotted slice.
    enum PassType : int
    {
      CountOutputs = 0,
      AllocateOutputs = 1,
      GenerateOutputs = 2
    };

    bool Initialize() override;

    /// Force three passes through this query.
    bool IsAnotherPassRequired() override { return this->Pass < PassType::GenerateOutputs; }

    /// Return the MTime also considering the cell-attribute.
    vtkMTimeType GetMTime() override;

    ///@{
    /// Specify the cell-attribute whose critical points should be located.
    virtual void SetCellAttribute(vtkCellAttribute*);
    vtkGetObjectMacro(CellAttribute, vtkCellAttribute);
    ///@}

    /// Get the request's output point set.
    vtkPolyData* GetOutput() const { return this->Output; }
    /// Get the request's input cell-grid.
    vtkCellGrid* GetInput() const { return this->Input; }

    /// Called by a responder during the CountOutputs pass to cache the
    /// critical points found for every cell of \a cellTypeName, indexed by
    /// that cell type's local cell ID - so the GenerateOutputs pass can
    /// write them to the output without solving for them again.
    void SetCriticalPoints(vtkStringToken cellTypeName,
      std::vector<std::vector<vtk::basis::bezier::CriticalPoint>> criticalPointsByCell);

    /// Return the critical points SetCriticalPoints() cached for
    /// \a cellTypeName, or an empty vector if that cell type cached none.
    const std::vector<std::vector<vtk::basis::bezier::CriticalPoint>>& GetCriticalPoints(
      vtkStringToken cellTypeName) const;

    /// Called by a responder during the CountOutputs pass to cache each
    /// critical point's classification (the output's own "Type": 0 =
    /// minimum, 1 = maximum, 2 = saddle), indexed the same way
    /// SetCriticalPoints() indexes its own points - so a cell's coefficients
    /// only ever need extracting once.
    void SetCriticalPointTypes(
      vtkStringToken cellTypeName, std::vector<std::vector<int>> typesByCell);

    /// Return the classifications SetCriticalPointTypes() cached for
    /// \a cellTypeName, or an empty vector if that cell type cached none.
    const std::vector<std::vector<int>>& GetCriticalPointTypes(vtkStringToken cellTypeName) const;

    /// Size the output's points and point-data arrays, assign each cell
    /// type a contiguous slice of them (GetCellTypeOffset()), build
    /// CellOffsets (GetCellOffset()), and build one poly-vertex output
    /// cell per source cell that has a critical point. A no-op after the
    /// first call, so every cell type's AllocateOutputs pass may call it
    /// unconditionally.
    void Allocate();

    /// Return \a cellTypeName's own base index into GetCellOffset()'s dense
    /// numbering. A hash lookup - resolve it once per cell type and pass it
    /// to GetCellOffset() rather than calling that overload of GetCellOffset()
    /// that looks it up again on every cell.
    vtkIdType GetCellTypeOffset(vtkStringToken cellTypeName) const;

    /// Return the output index Allocate() assigned to the critical points of
    /// cell \a cellId (local to its type), given that type's own
    /// \a cellTypeOffset (from GetCellTypeOffset()) - whether or not it has
    /// any (in which case this equals the next cell's own offset).
    vtkIdType GetCellOffset(vtkIdType cellTypeOffset, vtkIdType cellId) const;

  protected:
    friend class vtkCellGridFindCriticalPoints;
    Query() = default;
    ~Query() override;

    vtkCellGrid* Input{ nullptr };
    vtkPolyData* Output{ nullptr };
    vtkCellAttribute* CellAttribute{ nullptr };

    std::unordered_map<vtkStringToken, std::vector<std::vector<vtk::basis::bezier::CriticalPoint>>>
      CriticalPoints;
    /// Parallel to CriticalPoints - same keys, same per-cell indexing.
    std::unordered_map<vtkStringToken, std::vector<std::vector<int>>> Types;
    /// Each cell type's own base index into CellOffsets' dense, flattened
    /// (type, local cell ID) numbering.
    std::unordered_map<vtkStringToken, vtkIdType> CellIndexBase;
    /// Per-cell (dense, every type, including cells with none) prefix sum
    /// of critical-point counts, built by Allocate(); GetCellOffset() reads
    /// this.
    vtkSmartPointer<vtkIdTypeArray> CellOffsets;
    bool Allocated{ false };

  private:
    Query(const Query&) = delete;
    void operator=(const Query&) = delete;
  };

  static vtkCellGridFindCriticalPoints* New();
  vtkTypeMacro(vtkCellGridFindCriticalPoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Return the MTime also considering the query.
  vtkMTimeType GetMTime() override;

  /// Set/get the cell-attribute whose critical points should be located.
  void SetCellAttribute(vtkCellAttribute* attribute);

protected:
  vtkCellGridFindCriticalPoints();
  ~vtkCellGridFindCriticalPoints() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  vtkNew<Query> Request;

private:
  vtkCellGridFindCriticalPoints(const vtkCellGridFindCriticalPoints&) = delete;
  void operator=(const vtkCellGridFindCriticalPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
