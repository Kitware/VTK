// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridIsocontour
 * @brief   Extract an isosurface of a scalar cell-attribute of curved cells.
 *
 * Select the attribute with SetInputAttributeToProcess() and the value with
 * SetIsovalue(). The output is a cell-grid of linear triangles (vtkDGTri)
 * approximating the isosurface.
 *
 * The filter works in four steps:
 * 1. The attribute is converted to the Bernstein-Bezier basis if it is not in
 *    it already (see vtkCellGridChangeBasis). The field lies within the convex
 *    hull of its Bezier control values, so only the cells whose control values
 *    are not all on one side of the isovalue can hold the isosurface; the
 *    filter's Query finds them.
 * 2. Those cells alone are transcribed into linear cells (see
 *    vtkCellGridToUnstructuredGrid), each subdivided (see SetSubdivisionLevel),
 *    and contoured with vtkContourFilter. The cells are transcribed apart, so
 *    each point of the contour lies in the input cell whose linear cells it was
 *    found in, at the parametric coordinates interpolated along the edge it
 *    cuts. The points that cells sharing a face find on it are then merged into
 *    one (see SetTolerance).
 * 3. Every point is placed at its parametric coordinates in its cell, on the
 *    input's geometry, and the attribute and its gradient are evaluated there
 *    with vtkCellGridEvaluator.
 * 4. As in vtkContourFilter, each triangle's normal points toward the lower
 *    values: the triangles are oriented against the gradients at their points.
 *
 * The cells of any dimension are contoured: the output has triangles (vtkDGTri)
 * from volumetric cells, lines (vtkDGEdge) from surfaces and vertices
 * (vtkDGVert) from curves.
 *
 * The output points carry the cell of the input each is in: "CellType" (the
 * hash of the cell type's name), "CellId" and "RST" (its parametric
 * coordinates there).
 *
 * When InterpolateAttributes is on (the default, as in vtkContourFilter),
 * every cell-attribute of the input but its shape is evaluated at the output
 * points, in their cells, and output as a cell-attribute of the cells,
 * interpolated linearly across each.
 *
 * Each cell is contoured with its own values. Where the attribute is continuous
 * (neighboring cells agree on the faces they share, as when they share degrees
 * of freedom) the pieces found in neighboring cells meet and the surface is
 * closed. Where it is discontinuous, as a Discontinuous Galerkin field is, the
 * surface is that of the field cell by cell: it has gaps where the field jumps
 * between cells, and a larger Tolerance joins only the smaller ones.
 *
 * The second output is empty unless OutputIsocontourPoints is on, and then it
 * is the points vtkCellGridIsocontourPoints finds on the isocontour, as a
 * vtkPolyData.
 *
 * @sa vtkCellGridIsocontourPoints vtkCellGridToUnstructuredGrid vtkContourFilter
 * vtkCellGridEvaluator
 */

#ifndef vtkCellGridIsocontour_h
#define vtkCellGridIsocontour_h

#include "vtkCellGridAlgorithm.h"
#include "vtkCellGridQuery.h"         // For inheritance
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // For ivar
#include "vtkSmartPointer.h"          // For vtkSmartPointer
#include "vtkStringToken.h"           // For vtkStringToken

#include <unordered_map> // For unordered_map

VTK_ABI_NAMESPACE_BEGIN

class vtkCellAttribute;
class vtkIdTypeArray;
class vtkPolyData;

class VTKFILTERSCELLGRID_EXPORT vtkCellGridIsocontour : public vtkCellGridAlgorithm
{
public:
  /** @class Query
   * @brief A cell-grid query for the cells that may hold an isocontour.
   *
   * Responders report, for their cell type, the IDs of the cells where a
   * scalar cell-attribute may take the isovalue, judged by the bound that the
   * attribute's basis gives on its values in each cell, and the order of the
   * basis there. A cell type no responder can judge is left out.
   */
  class Query : public vtkCellGridQuery
  {
  public:
    static Query* New();
    vtkTypeMacro(vtkCellGridIsocontour::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /// The IDs of the cells that may hold the isocontour, by cell type.
    using CandidateCells = std::unordered_map<vtkStringToken, vtkSmartPointer<vtkIdTypeArray>>;

    /// Forget the cells found by a previous run.
    bool Initialize() override;

    ///@{
    /// Set/get the scalar cell-attribute whose isocontour is sought.
    virtual void SetCellAttribute(vtkCellAttribute*);
    vtkGetObjectMacro(CellAttribute, vtkCellAttribute);
    ///@}

    ///@{
    /// Set/get the isovalue.
    vtkSetMacro(Isovalue, double);
    vtkGetMacro(Isovalue, double);
    ///@}

    /// Called by a responder: the cells \a cellIds of type \a cellType may hold
    /// the isocontour, and the attribute's basis has the order \a order there. A
    /// cell type with no such cell need not be reported.
    void AddCandidateCells(vtkStringToken cellType, vtkIdTypeArray* cellIds, int order);

    /// Return the cells that may hold the isocontour, by cell type.
    const CandidateCells& GetCandidateCells() const { return this->Candidates; }

    /// Return the highest order of the attribute's basis among the cell types
    /// that have candidate cells (0 when there are none).
    int GetMaximumOrder() const { return this->MaximumOrder; }

  protected:
    Query() = default;
    ~Query() override;

    vtkCellAttribute* CellAttribute{ nullptr };
    double Isovalue{ 0. };
    CandidateCells Candidates;
    int MaximumOrder{ 0 };

  private:
    Query(const Query&) = delete;
    void operator=(const Query&) = delete;
  };

  static vtkCellGridIsocontour* New();
  vtkTypeMacro(vtkCellGridIsocontour, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /// Set/get the isovalue.
  void SetIsovalue(double isovalue);
  double GetIsovalue() { return this->Request->GetIsovalue(); }
  ///@}

  ///@{
  /// Set/get the distance within which two points of the contour are the same
  /// point, as a fraction of the diagonal of the input's bounds. Cells that
  /// share a face each find the points of the contour on it, and these are
  /// merged into one. The default is 1e-6.
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /// Set/get how many times each cell that might hold the isosurface is halved
  /// along every parametric axis before it is contoured as linear cells (see
  /// vtkCellGridToUnstructuredGrid::SetSubdivisionLevel()). The number of linear
  /// cells grows as 8^level for volumetric cells (4^level for surfaces, 2^level
  /// for curves). A negative level, the default of -1, means the order of the
  /// cell-attribute's basis minus one (but not less than zero).
  vtkSetClampMacro(SubdivisionLevel, int, -1, VTK_INT_MAX);
  vtkGetMacro(SubdivisionLevel, int);
  ///@}

  ///@{
  /// Set/get whether the second output is the points on the isocontour that
  /// vtkCellGridIsocontourPoints finds in the input's cells (a vtkPolyData). They
  /// are not used to compute the first output; they get the input's attributes
  /// too if InterpolateAttributes is on. The default is off, and the
  /// second output is then empty.
  vtkSetMacro(OutputIsocontourPoints, bool);
  vtkGetMacro(OutputIsocontourPoints, bool);
  vtkBooleanMacro(OutputIsocontourPoints, bool);
  ///@}

  /// Return the second output: the isocontour points (see OutputIsocontourPoints).
  vtkPolyData* GetIsocontourPointsOutput();

  ///@{
  /// Set/get the number of particle relaxation iterations that spread the points
  /// on the isocontour more evenly inside each cell (see
  /// vtkCellGridIsocontourPoints). It applies to the points of the second output.
  /// Zero, the default, disables it.
  vtkSetClampMacro(ParticleRelaxationIterations, int, 0, 1000);
  vtkGetMacro(ParticleRelaxationIterations, int);
  ///@}

  ///@{
  /// Set/get whether the input's cell-attributes are evaluated at the output
  /// points and output as cell-attributes of the triangles. The default is on.
  vtkSetMacro(InterpolateAttributes, bool);
  vtkGetMacro(InterpolateAttributes, bool);
  vtkBooleanMacro(InterpolateAttributes, bool);
  ///@}

protected:
  vtkCellGridIsocontour();
  ~vtkCellGridIsocontour() override = default;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<Query> Request;
  int SubdivisionLevel{ -1 };
  double Tolerance{ 1e-6 };
  int ParticleRelaxationIterations{ 0 };
  bool OutputIsocontourPoints{ false };
  bool InterpolateAttributes{ true };

private:
  vtkCellGridIsocontour(const vtkCellGridIsocontour&) = delete;
  void operator=(const vtkCellGridIsocontour&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridIsocontour_h
