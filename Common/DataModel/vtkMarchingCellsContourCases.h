// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkMarchingCellsContourCases_h
#define vtkMarchingCellsContourCases_h

#include "vtkCommonDataModelModule.h"

#include <cstdint> // For uint8_t

/**
 * @class vtkMarchingCellsContourCases
 * @brief Lookup tables for marching cells contouring.
 *
 * vtkMarchingCellsContourCases provides the edge intersection tables used
 * by marching cells algorithms to generate contours (isosurfaces/isolines)
 * for each supported cell type.
 *
 * @section CaseIndex Case Index Computation
 *
 * For each cell, a case index is computed as a bitmask where bit i is set
 * if vertex i's scalar value is strictly greater than the isovalue. This
 * index is then used to look up the edges that the contour intersects.
 *
 * The number of cases per cell type is 2^N where N is the number of vertices:
 *
 * | Cell type      | Vertices | Cases |
 * |----------------|----------|-------|
 * | Line           |        2 |     4 |
 * | Triangle       |        3 |     8 |
 * | Pixel / Quad   |        4 |    16 |
 * | Tetra          |        4 |    16 |
 * | Voxel / Hex    |        8 |   256 |
 * | Wedge          |        6 |    64 |
 * | Pyramid        |        5 |    32 |
 *
 * @section TriangleFormat Triangle/Line Format
 *
 * The triangle and line tables (accessed via GetCellCase()) encode output
 * primitives as a flat array of cell-local edge indices, padded with -1:
 *
 * - 3D cells: edges grouped in triples, each triple defines one triangle.
 * - 2D cells: edges grouped in pairs, each pair defines one line segment.
 *
 * @section PolygonFormat Polygon Segment Format
 *
 * The polygon tables (accessed via GetCellCaseWithPolygons()) merge coplanar
 * triangles into polygons for 3D cells, reducing the output primitive count.
 *
 * In both formats, edge indices refer to the cell-local edge numbering
 * convention as returned by GetCellEdges().
 *
 * @sa vtkMarchingCellsClipCases
 */
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkMarchingCellsContourCases
{
public:
  ///@{
  /**
   * Case tables for a line cell (VTK_LINE).
   * Each case entry is an array of 3 ints: one edge index followed by -1,
   * or { -1, -1 } if the cell produces no output.
   * There are 4 cases (2^2 vertices).
   */
  using LineCase = int[3];
  static const LineCase* GetLineCases();
  static const LineCase& GetLineCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a triangle cell (VTK_TRIANGLE).
   * Each case entry is an array of 3 ints: one pair of edge indices defining
   * a line segment, followed by -1, or { -1, -1, -1 } if no output.
   * There are 8 cases (2^3 vertices).
   */
  using TriangleCase = int[3];
  static const TriangleCase* GetTriangleCases();
  static const TriangleCase& GetTriangleCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a pixel cell (VTK_PIXEL).
   * Each case entry is an array of 5 ints: edge indices grouped into line
   * segments (2 per segment), terminated by -1. There are 16 cases (2^4 vertices).
   * Note: vtkPixel uses a different point ordering than vtkQuad
   * (points 2 and 3 are swapped), so this table differs from QuadCases.
   */
  using PixelCase = int[5];
  static const PixelCase* GetPixelCases();
  static const PixelCase& GetPixelCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a quad cell (VTK_QUAD).
   * Each case entry is an array of 5 ints: edge indices grouped into line
   * segments (2 per segment), terminated by -1. There are 16 cases (2^4 vertices).
   */
  using QuadCase = int[5];
  static const QuadCase* GetQuadCases();
  static const QuadCase& GetQuadCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a tetrahedron cell (VTK_TETRA).
   * Each case entry is an array of 7 ints: edge indices grouped into triangles
   * (3 edges per triangle), terminated by -1. There are 16 cases (2^4 vertices).
   */
  using TetraCase = int[7];
  static const TetraCase* GetTetraCases();
  static const TetraCase& GetTetraCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a voxel cell (VTK_VOXEL).
   * Each case entry is an array of 16 ints: edge indices grouped into triangles
   * (3 edges per triangle), terminated by -1. There are 256 cases (2^8 vertices).
   * Note: vtkVoxel uses a different point ordering than vtkHexahedron
   * (points 2 and 3 are swapped, as are points 6 and 7), so this table
   * differs from HexahedronCases.
   */
  using VoxelCase = int[16];
  static const VoxelCase* GetVoxelCases();
  static const VoxelCase& GetVoxelCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a hexahedron cell (VTK_HEXAHEDRON).
   * Each case entry is an array of 16 ints: edge indices grouped into triangles
   * (3 edges per triangle), terminated by -1. There are 256 cases (2^8 vertices).
   */
  using HexahedronCase = int[16];
  static const HexahedronCase* GetHexahedronCases();
  static const HexahedronCase& GetHexahedronCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a wedge cell (VTK_WEDGE).
   * Each case entry is an array of 13 ints: edge indices grouped into triangles
   * (3 edges per triangle), terminated by -1. There are 64 cases (2^6 vertices).
   */
  using WedgeCase = int[13];
  static const WedgeCase* GetWedgeCases();
  static const WedgeCase& GetWedgeCase(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Case tables for a pyramid cell (VTK_PYRAMID).
   * Each case entry is an array of 13 ints: edge indices grouped into triangles
   * (3 edges per triangle), terminated by -1. There are 32 cases (2^5 vertices).
   */
  using PyramidCase = int[13];
  static const PyramidCase* GetPyramidCases();
  static const PyramidCase& GetPyramidCase(uint8_t caseIndex);
  ///@}

  /**
   * Generic interface to retrieve a contour case entry for any supported cell type.
   * @param cellType A VTK cell type constant (e.g. VTK_HEXAHEDRON, VTK_WEDGE).
   * @param caseIndex The case index computed from the vertex inside/outside bitmask.
   * @return A pointer to the first edge index in the case entry, terminated by -1,
   *         or a pointer to -1 if the cell type is not supported.
   */
  using CellCase = const int*;
  static CellCase GetCellCase(int cellType, uint8_t caseIndex);

  ///@{
  /**
   * Polygon case tables for a tetrahedron cell (VTK_TETRA).
   * Each case entry is an array of 6 ints encoding zero or one polygon
   * in the polygon format: { nPts, e0, ..., e(nPts-1), -1 } or { -1, ... }.
   * The maximum polygon size is a quad (nPts=4): { 4, e0, e1, e2, e3, -1 }.
   * There are 16 cases (2^4 vertices).
   */
  using TetraCaseWithPolygons = int[6];
  static const TetraCaseWithPolygons* GetTetraCasesWithPolygons();
  static const TetraCaseWithPolygons& GetTetraCaseWithPolygons(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Polygon case tables for a voxel cell (VTK_VOXEL).
   * Each case entry is an array of 17 ints encoding zero or more polygons
   * in the polygon format, terminated by -1. There are 256 cases (2^8 vertices).
   * Note: vtkVoxel uses a different point ordering than vtkHexahedron
   * (points 2 and 3 are swapped, as are points 6 and 7), so this table
   * differs from HexahedronCasesWithPolygons.
   */
  using VoxelCaseWithPolygons = int[17];
  static const VoxelCaseWithPolygons* GetVoxelCasesWithPolygons();
  static const VoxelCaseWithPolygons& GetVoxelCaseWithPolygons(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Polygon case tables for a hexahedron cell (VTK_HEXAHEDRON).
   * Each case entry is an array of 17 ints encoding zero or more polygons
   * in the polygon format, terminated by -1. There are 256 cases (2^8 vertices).
   */
  using HexahedronCaseWithPolygons = int[17];
  static const HexahedronCaseWithPolygons* GetHexahedronCasesWithPolygons();
  static const HexahedronCaseWithPolygons& GetHexahedronCaseWithPolygons(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Polygon case tables for a wedge cell (VTK_WEDGE).
   * Each case entry is an array of 10 ints encoding zero or more polygons
   * in the polygon format, terminated by -1.
   * The worst case is one quad and one triangle: { 4, e0..e3, 3, e4..e6, -1 }.
   * There are 64 cases (2^6 vertices).
   */
  using WedgeCaseWithPolygons = int[10];
  static const WedgeCaseWithPolygons* GetWedgeCasesWithPolygons();
  static const WedgeCaseWithPolygons& GetWedgeCaseWithPolygons(uint8_t caseIndex);
  ///@}

  ///@{
  /**
   * Polygon case tables for a pyramid cell (VTK_PYRAMID).
   * Each case entry is an array of 9 ints encoding zero or more polygons
   * in the polygon format, terminated by -1.
   * The worst case is two triangles: { 3, e0..e2, 3, e3..e5, -1 }.
   * There are 32 cases (2^5 vertices).
   */
  using PyramidCaseWithPolygons = int[9];
  static const PyramidCaseWithPolygons* GetPyramidCasesWithPolygons();
  static const PyramidCaseWithPolygons& GetPyramidCaseWithPolygons(uint8_t caseIndex);
  ///@}

  /**
   * Generic interface to retrieve a contour case entry that generates polygons
   * or line segments for any supported cell type.
   *
   * @param cellType   A VTK cell type constant (e.g. VTK_TETRA, VTK_HEXAHEDRON).
   * @param caseIndex  The marching case index, computed as a bitmask where bit i is set
   *                   if vertex i is above the isovalue.
   *
   * @return A pointer into the case table. The encoding is as follows:
   *
   *   Empty case (cell entirely inside or outside):
   *     [ -1, ... ]
   *
   *   Non-empty case — one or more output polygons/line segments:
   *     [ nPts0, e0, e1, ..., e(nPts0-1),
   *       nPts1, e0, e1, ..., e(nPts1-1),
   *       ...
   *       -1 ]
   *
   *   where each eN is a cell-local edge index.
   *
   * @note Returns nullptr for unsupported cell types (e.g. VTK_POLYGON, VTK_POLY_LINE).
   */
  using CellCaseWithPolygons = const int*;
  static CellCaseWithPolygons GetCellCaseWithPolygons(int cellType, uint8_t caseIndex);

  /**
   * Returns the edge definitions for the given cell type as an array of
   * point index pairs, where each pair defines one edge of the cell.
   * The array is indexed by edge id, matching the edge indices used in the
   * case tables above.
   * Returns nullptr if the cell type is not supported.
   */
  using Edge = int[2];
  using EdgeArray = const Edge*;
  static EdgeArray GetCellEdges(int cellType);
};
VTK_ABI_NAMESPACE_END

#endif // vtkMarchingCellsContourCases_h
