// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGraphGeodesicPath
 * @brief   Abstract base for classes that generate a geodesic path on a graph (mesh).
 *
 * Serves as a base class for algorithms that trace a geodesic on a
 * polygonal dataset treating it as a graph. ie points connecting the
 * vertices of the graph
 */

#ifndef vtkGraphGeodesicPath_h
#define vtkGraphGeodesicPath_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkGeodesicPath.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDijkstraGraphInternals;
class vtkIdList;

class VTKFILTERSMODELING_EXPORT vtkGraphGeodesicPath : public vtkGeodesicPath
{
public:
  ///@{
  /**
   * Standard methods for printing and determining type information.
   */
  vtkTypeMacro(vtkGraphGeodesicPath, vtkGeodesicPath);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Fill the array with the cumulative weights.
   */
  void GetCumulativeWeights(vtkDoubleArray* weights);

  /**
   * The vertex ids (of the input polydata) on the shortest path.
   */
  vtkGetObjectMacro(IdList, vtkIdList);

  ///@{
  /**
   * Stop when the end vertex is reached
   * or calculate shortest path to all vertices.
   * (default: false)
   */
  vtkSetMacro(StopWhenEndReached, vtkTypeBool);
  vtkGetMacro(StopWhenEndReached, vtkTypeBool);
  vtkBooleanMacro(StopWhenEndReached, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The vertex at the start of the shortest path
   */
  vtkGetMacro(StartVertex, vtkIdType);
  vtkSetMacro(StartVertex, vtkIdType);
  ///@}

  ///@{
  /**
   * The vertex at the end of the shortest path
   */
  vtkGetMacro(EndVertex, vtkIdType);
  vtkSetMacro(EndVertex, vtkIdType);
  ///@}

  ///@{
  /**
   * Use scalar values in the edge weight.
   * If changed, the adjacency matrix is recomputed.
   * (default: false)
   */
  void SetUseScalarWeights(vtkTypeBool);
  vtkGetMacro(UseScalarWeights, vtkTypeBool);
  vtkBooleanMacro(UseScalarWeights, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Use the input point to repel the path by assigning high costs.
   * (default: false)
   */
  vtkSetMacro(RepelPathFromVertices, vtkTypeBool);
  vtkGetMacro(RepelPathFromVertices, vtkTypeBool);
  vtkBooleanMacro(RepelPathFromVertices, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify vtkPoints to use to repel the path from.
   */
  void SetRepelVertices(vtkPoints*);
  vtkGetObjectMacro(RepelVertices, vtkPoints);
  ///@}

  ///@{
  /**
   * Set/Get the name of the input array used for the edge weights.
   * It is effective only if UseScalarWeights is true.
   * If changed, the adjacency matrix is recomputed.
   (default: "")
   */
  void SetProcessedFieldArrayName(const std::string& name);
  vtkGetMacro(ProcessedFieldArrayName, const std::string&);
  ///@}

protected:
  vtkGraphGeodesicPath();
  ~vtkGraphGeodesicPath() override;

  /**
   * Helper to get the node (point or cell) position from its index.
   */
  virtual void GetNodeFromIndex(vtkDataSet* inData, vtkIdType u, double pt[3]);

  /**
   * Helper to get the number of nodes in the graph.
   */
  virtual vtkIdType GetNumberOfNodes(vtkDataSet* inData);

  /**
   * Helper to discard repelled vertices from the shortest path computation.
   */
  virtual void DiscardRepelVertices(vtkDataSet* inData, int startv, int endv);

  /**
   * Builds a graph description of the input.
   */
  virtual void BuildAdjacency(vtkDataSet* inData) = 0;

  /**
   * Computes the fixed cost going from vertex u to v.
   */
  virtual double CalculateStaticEdgeCost(vtkDataSet* inData, vtkIdType u, vtkIdType v) = 0;

  /**
   * Computes the dynamic cost cost going from vertex u to v, that may depend on one or more
   * vertices that precede u.
   */
  virtual double CalculateDynamicEdgeCost(vtkDataSet*, vtkIdType, vtkIdType) { return 0.0; }

  /**
   * Actual computation of the shortest path from vertex startv to endv.
   */
  virtual void ShortestPath(vtkDataSet* inData, int startv, int endv);

  /**
   * Initialize the main internal structures for shortest path computation.
   */
  void Initialize(vtkDataSet* inData);

  /**
   * Reset the main internal structures for shortest path computation.
   */
  void Reset();

  /**
   * Relax edge u,v with weight w.
   */
  void Relax(int u, int v, double w);

  /**
   * Backtrace the shortest path.
   */
  void TraceShortestPath(
    vtkDataSet* inData, vtkPolyData* outPoly, vtkIdType startv, vtkIdType endv);

  vtkTimeStamp AdjacencyBuildTime;
  vtkTimeStamp AdjacencyParametersTime;

  int NumberOfVertices = 0;

  // The vertex ids on the shortest path.
  vtkIdList* IdList;

  // Internalized STL containers.
  std::unique_ptr<vtkDijkstraGraphInternals> Internals;

  vtkTypeBool StopWhenEndReached = 0;
  vtkTypeBool UseScalarWeights = 0;
  vtkTypeBool RepelPathFromVertices = 0;

  vtkPoints* RepelVertices = nullptr;

  std::string ProcessedFieldArrayName;

  vtkIdType StartVertex = 0;
  vtkIdType EndVertex = 0;

private:
  vtkGraphGeodesicPath(const vtkGraphGeodesicPath&) = delete;
  void operator=(const vtkGraphGeodesicPath&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
