// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDijkstraGraphGeodesicPath
 * @brief   Dijkstra algorithm to compute the graph geodesic.
 *
 * Takes as input a mesh and performs a single source shortest
 * path calculation. Dijkstra's algorithm is used. The implementation is
 * similar to the one described in Introduction to Algorithms (Second Edition)
 * by Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, and
 * Cliff Stein, published by MIT Press and McGraw-Hill. Some minor
 * enhancement are added though. All vertices are not pushed on the heap
 * at start, instead a front set is maintained. The heap is implemented as
 * a binary heap. The output of the filter is a set of lines describing
 * the shortest path from StartVertex to EndVertex. If a path cannot be found
 * the output will have no lines or points.
 *
 *
 * @par Thanks:
 * The class was contributed by Rasmus Paulsen.
 * www.imm.dtu.dk/~rrp/VTK . Also thanks to Alexandre Gouaillard and Shoaib
 * Ghias for bug fixes and enhancements.
 */

#ifndef vtkDijkstraGraphGeodesicPath_h
#define vtkDijkstraGraphGeodesicPath_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkGraphGeodesicPath.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSMODELING_EXPORT vtkDijkstraGraphGeodesicPath : public vtkGraphGeodesicPath
{
public:
  /**
   * Instantiate the class.
   */
  static vtkDijkstraGraphGeodesicPath* New();

  ///@{
  /**
   * Standard methods for printing and determining type information.
   */
  vtkTypeMacro(vtkDijkstraGraphGeodesicPath, vtkGraphGeodesicPath);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the start/end node as the closest point/cell in the input dataset from the given point.
   */
  vtkSetVector3Macro(StartPoint, double);
  vtkGetVector3Macro(StartPoint, double);
  vtkSetVector3Macro(EndPoint, double);
  vtkGetVector3Macro(EndPoint, double);
  ///@}

  ///@{
  /**
   * Define start and end node by their index if true or by closest points otherwise.
   * (default: true)
   */
  vtkSetMacro(UseNodeIndices, vtkTypeBool);
  vtkGetMacro(UseNodeIndices, vtkTypeBool);
  vtkBooleanMacro(UseNodeIndices, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get wether the path is compute on the vertices or the cells.
   * If changed, the adjacency matrix is recomputed.
   * Note this uses the vtkDataObject::AttributeTypes enum.
   * (valid range: [POINT, CELL], [0, 1], default: POINT)
   */
  void SetGraphType(int type);
  vtkGetMacro(GraphType, int);
  ///@}

protected:
  vtkDijkstraGraphGeodesicPath();
  ~vtkDijkstraGraphGeodesicPath() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  ///@{
  /**
   * Builds a graph description of the input.
   */
  void BuildAdjacency(vtkDataSet* inData) override;
  void BuildCellAdjacency(vtkDataSet* inData);
  ///@}

  ///@{
  /**
   * Computes the fixed cost going from vertex u to v.
   */
  double CalculateStaticEdgeCost(vtkDataSet* inData, vtkIdType u, vtkIdType v) override;
  double CalculateStaticEdgeCost(
    vtkDataSet* inData, vtkDataArray* scalars, vtkIdType u, vtkIdType v);
  ///@}

  /**
   * Computes the fixed cost going from cell c1 to c2.
   */
  double CalculateCellEdgeCost(
    vtkDataSet* inData, vtkDataArray* scalars, vtkIdType c1, vtkIdType c2);

  /**
   * Helper to get the node (point or cell) position from its index.
   */
  void GetNodeFromIndex(vtkDataSet* inData, vtkIdType u, double pt[3]) override;

  /**
   * Helper to get the number of nodes in the graph.
   */
  vtkIdType GetNumberOfNodes(vtkDataSet* inData) override;

  /**
   * Helper to discard repelled vertices from the shortest path computation.
   */
  void DiscardRepelVertices(vtkDataSet* inData, int startv, int endv) override;

  /**
   * Add the edge u->v and v->u to the adjacency table.
   */
  void AddBidirectionalEdge(vtkDataSet* inData, vtkDataArray* scalars,
    std::vector<std::map<int, double>>& adjacency, vtkIdType u, vtkIdType v);

  vtkTypeBool UseNodeIndices = 1;

  double StartPoint[3] = { 0.0, 0.0, 0.0 };
  double EndPoint[3] = { 0.0, 0.0, 0.0 };

  vtkNew<vtkPolyData> CellCenters;

  int GraphType = vtkDataObject::AttributeTypes::POINT;

private:
  vtkDijkstraGraphGeodesicPath(const vtkDijkstraGraphGeodesicPath&) = delete;
  void operator=(const vtkDijkstraGraphGeodesicPath&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
