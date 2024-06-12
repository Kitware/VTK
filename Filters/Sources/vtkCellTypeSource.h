// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellTypeSource
 * @brief   Create cells of a given type
 *
 * vtkCellTypeSource is a source object that creates cells of the given
 * input type. BlocksDimensions specifies the number of cell "blocks" in each
 * direction. A cell block may be divided into multiple cells based on
 * the chosen cell type (e.g. 6 pyramid cells make up a single cell block).
 * If a 1D cell is selected then only the first dimension is
 * used to specify how many cells are generated. If a 2D cell is
 * selected then only the first and second dimensions are used to
 * determine how many cells are created. The source respects pieces.
 */

#ifndef vtkCellTypeSource_h
#define vtkCellTypeSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMergePoints;

class VTKFILTERSSOURCES_EXPORT vtkCellTypeSource : public vtkUnstructuredGridAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type and printing instance values.
   */
  static vtkCellTypeSource* New();
  vtkTypeMacro(vtkCellTypeSource, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the type of cells to be generated.
   */
  void SetCellType(int cellType);
  vtkGetMacro(CellType, int);
  ///@}

  ///@{
  /**
   * Set/Get the order of Lagrange interpolation to be used.
   *
   * This is only used when the cell type is a Lagrange element.
   * The default is cubic (order 3).
   * Lagrange elements are the same order along all axes
   * (i.e., you cannot specify a different interpolation order
   * for the i, j, and k axes of a hexahedron).
   */
  vtkSetMacro(CellOrder, int);
  vtkGetMacro(CellOrder, int);
  ///@}

  ///@{
  /**
   * Set/Get whether quadratic cells with simplicial shapes should be "completed".
   *
   * By default, quadratic Lagrange cells with simplicial shapes
   * do not completely span the basis of all polynomial of the maximal
   * degree. This can be corrected by adding mid-face and body-centered
   * nodes. Setting this option to true will generate cells with these
   * additional nodes.
   *
   * This is only used when
   * (1) the cell type is a Lagrange triangle, tetrahedron, or wedge;
   * and (2) \a CellOrder is set to 2 (quadratic elements).
   * The default is false.
   *
   * When true, generated
   * (1) triangles will have 7 nodes instead of 6;
   * (2) tetrahedra will have 15 nodes instead of 10;
   * (3) wedges will have 21 nodes instead of 18.
   */
  vtkSetMacro(CompleteQuadraticSimplicialElements, bool);
  vtkGetMacro(CompleteQuadraticSimplicialElements, bool);
  vtkBooleanMacro(CompleteQuadraticSimplicialElements, bool);
  ///@}

  ///@{
  /**
   * Set/Get the polynomial order of the "Polynomial" point field.
   * The default is 1.
   */
  vtkSetClampMacro(PolynomialFieldOrder, int, 0, VTK_INT_MAX);
  vtkGetMacro(PolynomialFieldOrder, int);
  ///@}

  ///@{
  /**
   * Get the dimension of the cell blocks to be generated
   */
  int GetCellDimension();
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION (0) - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION (1) - Output double-precision floating point.
   */
  vtkSetClampMacro(OutputPrecision, int, 0, 1);
  vtkGetMacro(OutputPrecision, int);
  ///@}

  ///@{
  /**
   * Set the number of cells in each direction. If a 1D cell type is
   * selected then only the first dimension is used and if a 2D cell
   * type is selected then the first and second dimensions are used.
   * Default is (1, 1, 1), which results in a single block of cells.
   */
  void SetBlocksDimensions(int*);
  void SetBlocksDimensions(int, int, int);
  vtkGetVector3Macro(BlocksDimensions, int);
  ///@}

protected:
  vtkCellTypeSource();
  ~vtkCellTypeSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void GenerateTriangles(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuads(vtkUnstructuredGrid*, int extent[6]);
  void GeneratePolygons(vtkUnstructuredGrid*, int extent[6]);
  void GeneratePixels(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticTriangles(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticQuads(vtkUnstructuredGrid*, int extent[6]);
  void GenerateTetras(vtkUnstructuredGrid*, int extent[6]);
  void GenerateHexahedron(vtkUnstructuredGrid*, int extent[6]);
  void GeneratePolyhedron(vtkUnstructuredGrid*, int extent[6]);
  void GenerateVoxels(vtkUnstructuredGrid*, int extent[6]);
  void GenerateWedges(vtkUnstructuredGrid*, int extent[6]);
  void GeneratePyramids(vtkUnstructuredGrid*, int extent[6]);
  void GeneratePentagonalPrism(vtkUnstructuredGrid*, int extent[6]);
  void GenerateHexagonalPrism(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticTetras(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticHexahedron(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticWedges(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticPyramids(vtkUnstructuredGrid*, int extent[6]);
  void GenerateTriQuadraticPyramids(vtkUnstructuredGrid*, int extent[6]);

  void GenerateHighOrderQuads(
    vtkUnstructuredGrid* output, int extent[6], int cellType, int cellOrder);
  void GenerateHighOrderHexes(
    vtkUnstructuredGrid* output, int extent[6], int cellType, int cellOrder);
  void GenerateHighOrderCurves(vtkUnstructuredGrid*, int extent[6], int cellType, int cellOrder);
  void GenerateHighOrderTris(
    vtkUnstructuredGrid*, int extent[6], int cellType, int cellOrder, bool complete);
  void GenerateHighOrderTets(
    vtkUnstructuredGrid*, int extent[6], int cellType, int cellOrder, bool complete);
  void GenerateHighOrderWedges(
    vtkUnstructuredGrid*, int extent[6], int cellType, int cellOrder, bool complete);

  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderCurve instead.")
  void GenerateLagrangeCurves(vtkUnstructuredGrid*, int extent[6]);
  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderTris instead.")
  void GenerateLagrangeTris(vtkUnstructuredGrid*, int extent[6]);
  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderTets instead.")
  void GenerateLagrangeTets(vtkUnstructuredGrid*, int extent[6]);
  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderWedges instead.")
  void GenerateLagrangeWedges(vtkUnstructuredGrid*, int extent[6]);

  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderCurve instead.")
  void GenerateBezierCurves(vtkUnstructuredGrid*, int extent[6]);
  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderTris instead.")
  void GenerateBezierTris(vtkUnstructuredGrid*, int extent[6]);
  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderTets instead.")
  void GenerateBezierTets(vtkUnstructuredGrid*, int extent[6]);
  VTK_DEPRECATED_IN_9_4_0("Use GenerateHighOrderWedges instead.")
  void GenerateBezierWedges(vtkUnstructuredGrid*, int extent[6]);

  virtual void ComputeFields(vtkUnstructuredGrid*);
  double GetValueOfOrder(int order, double coords[3]);

  int BlocksDimensions[3];
  int CellType;
  int CellOrder;
  bool CompleteQuadraticSimplicialElements;
  int OutputPrecision;
  int PolynomialFieldOrder;
  vtkMergePoints* Locator; // Only valid during RequestData.

private:
  vtkCellTypeSource(const vtkCellTypeSource&) = delete;
  void operator=(const vtkCellTypeSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
