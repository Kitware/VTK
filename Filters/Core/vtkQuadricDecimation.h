// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadricDecimation
 * @brief   reduce the number of triangles in a mesh
 *
 * vtkQuadricDecimation is a filter to reduce the number of triangles in
 * a triangle mesh, forming a good approximation to the original geometry.
 * The input to vtkQuadricDecimation is a vtkPolyData object, and only
 * triangles are treated. If you desire to decimate polygonal meshes, first
 * triangulate the polygons with vtkTriangleFilter.
 *
 * The algorithm is based on repeated edge collapses until the requested mesh
 * reduction is achieved. Edges are placed in a priority queue based on the
 * "cost" to delete the edge. The cost is an approximate measure of error
 * (distance to the original surface)--described by the so-called quadric
 * error measure. The quadric error measure is associated with each vertex of
 * the mesh and represents a matrix of planes incident on that vertex. The
 * distance of the planes to the vertex is the error in the position of the
 * vertex (originally the vertex error iz zero). As edges are deleted, the
 * quadric error measure associated with the two end points of the edge are
 * summed (this combines the plane equations) and an optimal collapse point
 * can be computed. Edges connected to the collapse point are then reinserted
 * into the queue after computing the new cost to delete them. The process
 * continues until the desired reduction level is reached or topological
 * constraints prevent further reduction. Note that this basic algorithm can
 * be extended to higher dimensions by
 * taking into account variation in attributes (i.e., scalars, vectors, and
 * so on). Attributes are interpolated during the edge collapse,
 * except for vtkIdType arrays: values of removed points are discarded.
 *
 * This paper is based on the work of Garland and Heckbert who first
 * presented the quadric error measure at Siggraph '97 "Surface
 * Simplification Using Quadric Error Metrics". For details of the algorithm
 * Michael Garland's Ph.D. thesis is also recommended. Hughues Hoppe's Vis
 * '99 paper, "New Quadric Metric for Simplifying Meshes with Appearance
 * Attributes" is also a good take on the subject especially as it pertains
 * to the error metric applied to attributes.
 *
 * @par Thanks:
 * Thanks to Bradley Lowekamp of the National Library of Medicine/NIH for
 * contributing this class.
 */

#ifndef vtkQuadricDecimation_h
#define vtkQuadricDecimation_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkEdgeTable;
class vtkIdList;
class vtkPointData;
class vtkPriorityQueue;
class vtkDoubleArray;

class VTKFILTERSCORE_EXPORT vtkQuadricDecimation : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkQuadricDecimation, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkQuadricDecimation* New();

  ///@{
  /**
   * Set/Get the desired reduction (expressed as a fraction of the original
   * number of triangles). The actual reduction may be less depending on
   * triangulation, topological constraints and optional maximum error limit.
   * @see SetMaximumError
   */
  vtkSetClampMacro(TargetReduction, double, 0.0, 1.0);
  vtkGetMacro(TargetReduction, double);
  ///@}

  ///@{
  /**
   * Decide whether to include data attributes in the error metric. If off,
   * then only geometric error is used to control the decimation. By default
   * the attribute errors are off.
   */
  vtkSetMacro(AttributeErrorMetric, vtkTypeBool);
  vtkGetMacro(AttributeErrorMetric, vtkTypeBool);
  vtkBooleanMacro(AttributeErrorMetric, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Decide whether to activate volume preservation which greatly reduces errors
   * in triangle normal direction. If off, volume preservation is disabled and
   * if AttributeErrorMetric is active, these errors can be large.
   * By default VolumePreservation is off
   * the attribute errors are off.
   */
  vtkSetMacro(VolumePreservation, vtkTypeBool);
  vtkGetMacro(VolumePreservation, vtkTypeBool);
  vtkBooleanMacro(VolumePreservation, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Maximum allowed absolute error for stopping criteria.
   * VTK_DOUBLE_MAX by default (no limit).
   * This limit takes precedence over the TargetReduction.
   */
  vtkSetMacro(MaximumError, double);
  vtkGetMacro(MaximumError, double);
  ///@}

  ///@{
  /**
   * Parameters related to adding a probabilistic uncertainty to the position and normals of the
   * quadrics following [1]. The goal using these parameters is to regularize the point finding
   * algorithm so as to have better quality mesh elements at the cost of a slightly lower precision
   * on the geometry potentially (mostly at sharp edges). Can also be useful for decimating meshes
   * that have been triangulated on noisy data.
   *
   * Regularize: boolean property determining whether or not to use the regularization method
   * Regularization: user defined variable that can be used to adjust the level of
   * regularization. One can think of it as the standard deviation of the probability distribution
   * of normals in the context of noisy data.
   *
   * [1] P. Trettner and L. Kobbelt, Fast and Robust QEF Minimization using Probabilistic Quadrics,
   * EUROGRAPHICS, Volume 39, Number 2 (2020)
   */
  vtkSetMacro(Regularize, vtkTypeBool);
  vtkGetMacro(Regularize, vtkTypeBool);
  vtkBooleanMacro(Regularize, vtkTypeBool);
  vtkSetMacro(Regularization, double);
  vtkGetMacro(Regularization, double);
  ///@}

  ///@{
  /**
   * Parameters related to the treatment of the boundary of the mesh during decimation.
   *
   * WeighBoundaryConstraintsByLength: When this boolean is set to true, use the legacy weighting by
   * boundary_edge_length instead of by boundary_edge_length^2 for backwards compatibility (default
   * to false) BoundaryWeightFactor: A floating point factor to weigh the boundary quadric
   * constraints by: higher factors further constrain the boundary.
   */
  vtkSetMacro(WeighBoundaryConstraintsByLength, vtkTypeBool);
  vtkGetMacro(WeighBoundaryConstraintsByLength, vtkTypeBool);
  vtkBooleanMacro(WeighBoundaryConstraintsByLength, vtkTypeBool);
  vtkSetMacro(BoundaryWeightFactor, double);
  vtkGetMacro(BoundaryWeightFactor, double);
  ///@}

  ///@{
  /**
   * Getter/Setter for mapping point data to the output during decimation.
   * Attributes are interpolated during edge collapses, except for
   * vtkIdType array where each collapse lead to a single id being kept.
   */
  vtkGetMacro(MapPointData, bool);
  vtkSetMacro(MapPointData, bool);
  vtkBooleanMacro(MapPointData, bool);
  ///@}

  ///@{
  /**
   * If attribute errors are to be included in the metric (i.e.,
   * AttributeErrorMetric is on), then the following flags control which
   * attributes are to be included in the error calculation. By default all
   * of these are on.
   */
  vtkSetMacro(ScalarsAttribute, vtkTypeBool);
  vtkGetMacro(ScalarsAttribute, vtkTypeBool);
  vtkBooleanMacro(ScalarsAttribute, vtkTypeBool);
  vtkSetMacro(VectorsAttribute, vtkTypeBool);
  vtkGetMacro(VectorsAttribute, vtkTypeBool);
  vtkBooleanMacro(VectorsAttribute, vtkTypeBool);
  vtkSetMacro(NormalsAttribute, vtkTypeBool);
  vtkGetMacro(NormalsAttribute, vtkTypeBool);
  vtkBooleanMacro(NormalsAttribute, vtkTypeBool);
  vtkSetMacro(TCoordsAttribute, vtkTypeBool);
  vtkGetMacro(TCoordsAttribute, vtkTypeBool);
  vtkBooleanMacro(TCoordsAttribute, vtkTypeBool);
  vtkSetMacro(TensorsAttribute, vtkTypeBool);
  vtkGetMacro(TensorsAttribute, vtkTypeBool);
  vtkBooleanMacro(TensorsAttribute, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the scaling weight contribution of the attribute. These
   * values are used to weight the contribution of the attributes
   * towards the error metric.
   */
  vtkSetMacro(ScalarsWeight, double);
  vtkSetMacro(VectorsWeight, double);
  vtkSetMacro(NormalsWeight, double);
  vtkSetMacro(TCoordsWeight, double);
  vtkSetMacro(TensorsWeight, double);
  vtkGetMacro(ScalarsWeight, double);
  vtkGetMacro(VectorsWeight, double);
  vtkGetMacro(NormalsWeight, double);
  vtkGetMacro(TCoordsWeight, double);
  vtkGetMacro(TensorsWeight, double);
  ///@}

  ///@{
  /**
   * Get the actual reduction. This value is only valid after the
   * filter has executed.
   */
  vtkGetMacro(ActualReduction, double);
  ///@}

protected:
  vtkQuadricDecimation();
  ~vtkQuadricDecimation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Do the dirty work of eliminating the edge; return the number of
   * triangles deleted.
   */
  int CollapseEdge(vtkIdType pt0Id, vtkIdType pt1Id);

  /**
   * Compute quadric for all vertices
   */
  void InitializeQuadrics(vtkIdType numPts);

  /**
   * Free boundary edges are weighted
   */
  void AddBoundaryConstraints();

  /**
   * Compute quadric for this vertex.
   */
  void ComputeQuadric(vtkIdType pointId);

  /**
   * Add the quadrics for these 2 points since the edge between them has
   * been collapsed.
   */
  void AddQuadric(vtkIdType oldPtId, vtkIdType newPtId);

  ///@{
  /**
   * Compute cost for contracting this edge and the point that gives us this
   * cost.
   */
  double ComputeCost(vtkIdType edgeId, double* x);
  double ComputeCost2(vtkIdType edgeId, double* x);
  ///@}

  /**
   * Find all edges that will have an endpoint change ids because of an edge
   * collapse.  p1Id and p2Id are the endpoints of the edge.  p2Id is the
   * pointId being removed.
   */
  void FindAffectedEdges(vtkIdType p1Id, vtkIdType p2Id, vtkIdList* edges);

  /**
   * Find a cell that uses this edge.
   */
  vtkIdType GetEdgeCellId(vtkIdType p1Id, vtkIdType p2Id);

  int IsGoodPlacement(vtkIdType pt0Id, vtkIdType pt1Id, const double* x);
  int TrianglePlaneCheck(
    const double t0[3], const double t1[3], const double t2[3], const double* x);
  void ComputeNumberOfComponents();
  void UpdateEdgeData(vtkIdType pt0Id, vtkIdType pt1Id);

  ///@{
  /**
   * Helper function to set the new point coordinates.
   * @see SetPointActiveAttributes, SetPointAttributeArray
   * @see MapPointData ComputeCost2 ComputeCost
   */
  void SetPointCoordinates(vtkIdType ptId, const double* x);

  /**
   * Helper function to set the point coordinates and it's attributes.
   *
   * New point attributes are stored in `x` after point coordinates.
   * Not all point attributes are set, only activate attribute arrays.
   * @see SetPointCoordinates, SetPointAttributeArray
   * @see MapPointData
   */
  void SetPointActiveAttributes(vtkIdType ptId, const double* x);

  /**
   * Helper function to set and get the point and it's attributes as an array
   *
   * The setter needs the entire edge for interpolation of point data.
   * Attributes in `x` are ignored.
   *
   * FIXME a linear interpolation is used instead of a quadratic interpolation.
   * FIXME new point coordinates may be far away from the edge.
   * FIXME shouldn't the attributes be scaled with AttributeScale?
   * @see SetPointCoordinates, SetPointActiveAttributes
   * @see MapPointData
   */
  void SetPointAttributeArray(vtkIdType ptId[2], const double* x);
  void GetPointAttributeArray(vtkIdType ptId, double* x);
  ///@}

  /**
   * Find out how many components there are for each attribute for this
   * poly data.
   */
  void GetAttributeComponents();

  double TargetReduction;
  double ActualReduction;
  vtkTypeBool AttributeErrorMetric;
  vtkTypeBool VolumePreservation;

  bool MapPointData = false;

  vtkTypeBool ScalarsAttribute;
  vtkTypeBool VectorsAttribute;
  vtkTypeBool NormalsAttribute;
  vtkTypeBool TCoordsAttribute;
  vtkTypeBool TensorsAttribute;

  double ScalarsWeight;
  double VectorsWeight;
  double NormalsWeight;
  double TCoordsWeight;
  double TensorsWeight;

  int NumberOfEdgeCollapses;
  vtkEdgeTable* Edges;
  vtkIdList* EndPoint1List;
  vtkIdList* EndPoint2List;
  vtkPriorityQueue* EdgeCosts;
  vtkDoubleArray* TargetPoints;
  int NumberOfComponents;
  vtkPolyData* Mesh;

  struct ErrorQuadric
  {
    double* Quadric;
  };

  // One ErrorQuadric per point
  ErrorQuadric* ErrorQuadrics;

  // Controlling regularization behavior
  vtkTypeBool Regularize = false;
  double Regularization = 0.05;

  // Controlling the boundary weighting behavior
  vtkTypeBool WeighBoundaryConstraintsByLength = false;
  double BoundaryWeightFactor = 1.0;

  // Contains 4 doubles per point. Length = nPoints * 4
  double* VolumeConstraints;
  int AttributeComponents[6];
  double AttributeScale[6];

  // Temporary variables for performance
  vtkIdList* CollapseCellIds;
  double* TempX;
  double* TempQuad;
  double* TempB;
  double** TempA;
  double* TempData;

private:
  vtkQuadricDecimation(const vtkQuadricDecimation&) = delete;
  void operator=(const vtkQuadricDecimation&) = delete;

  // Maximum allowed absolute error for stopping criteria.
  double MaximumError = VTK_DOUBLE_MAX;
};

VTK_ABI_NAMESPACE_END
#endif
