/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricDecimation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadricDecimation - reduce the number of triangles in a mesh
// .SECTION Description
// vtkQuadricDecimation is a filter to reduce the number of triangles in
// a triangle mesh, forming a good approximation to the original geometry.
// The input to vtkQuadricDecimation is a vtkPolyData object, and only
// triangles are treated. If you desire to decimate polygonal meshes, first
// triangulate the polygons with vtkTriangleFilter.
//
// The algorithm is based on repeated edge collapses until the requested mesh
// reduction is achieved. Edges are placed in a priority queue based on the
// "cost" to delete the edge. The cost is an approximate measure of error
// (distance to the original surface)--described by the so-called quadric
// error measure. The quadric error measure is associated with each vertex of
// the mesh and represents a matrix of planes incident on that vertex. The
// distance of the planes to the vertex is the error in the position of the
// vertex (originally the vertex error iz zero). As edges are deleted, the
// quadric error measure associated with the two end points of the edge are
// summed (this combines the plane equations) and an optimal collapse point
// can be computed. Edges connected to the collapse point are then reinserted
// into the queue after computing the new cost to delete them. The process
// continues until the desired reduction level is reached or topological
// constraints prevent further reduction. Note that this basic algorithm can
// be extended to higher dimensions by
// taking into account variation in attributes (i.e., scalars, vectors, and
// so on).
//
// This paper is based on the work of Garland and Heckbert who first
// presented the quadric error measure at Siggraph '97 "Surface
// Simplification Using Quadric Error Metrics". For details of the algorithm
// Michael Garland's Ph.D. thesis is also recommended. Hughues Hoppe's Vis
// '99 paper, "New Quadric Metric for Simplifying Meshes with Appearance
// Attributes" is also a good take on the subject especially as it pertains
// to the error metric applied to attributes.
//
// .SECTION Thanks
// Thanks to Bradley Lowekamp of the National Library of Medicine/NIH for
// contributing this class.

#ifndef __vtkQuadricDecimation_h
#define __vtkQuadricDecimation_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkEdgeTable;
class vtkIdList;
class vtkPointData;
class vtkPriorityQueue;
class vtkDoubleArray;

class VTKFILTERSCORE_EXPORT vtkQuadricDecimation : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkQuadricDecimation, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkQuadricDecimation *New();

  // Description:
  // Set/Get the desired reduction (expressed as a fraction of the original
  // number of triangles). The actual reduction may be less depending on
  // triangulation and topological constraints.
  vtkSetClampMacro(TargetReduction, double, 0.0, 1.0);
  vtkGetMacro(TargetReduction, double);

  // Description:
  // Decide whether to include data attributes in the error metric. If off,
  // then only geometric error is used to control the decimation. By default
  // the attribute errors are off.
  vtkSetMacro(AttributeErrorMetric, int);
  vtkGetMacro(AttributeErrorMetric, int);
  vtkBooleanMacro(AttributeErrorMetric, int);

  // Description:
  // If attribute errors are to be included in the metric (i.e.,
  // AttributeErrorMetric is on), then the following flags control which
  // attributes are to be included in the error calculation. By default all
  // of these are on.
  vtkSetMacro(ScalarsAttribute, int);
  vtkGetMacro(ScalarsAttribute, int);
  vtkBooleanMacro(ScalarsAttribute, int);
  vtkSetMacro(VectorsAttribute, int);
  vtkGetMacro(VectorsAttribute, int);
  vtkBooleanMacro(VectorsAttribute, int);
  vtkSetMacro(NormalsAttribute, int);
  vtkGetMacro(NormalsAttribute, int);
  vtkBooleanMacro(NormalsAttribute, int);
  vtkSetMacro(TCoordsAttribute, int);
  vtkGetMacro(TCoordsAttribute, int);
  vtkBooleanMacro(TCoordsAttribute, int);
  vtkSetMacro(TensorsAttribute, int);
  vtkGetMacro(TensorsAttribute, int);
  vtkBooleanMacro(TensorsAttribute, int);

  // Description:
  // Set/Get the scaling weight contribution of the attribute. These
  // values are used to weight the contribution of the attributes
  // towards the error metric.
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

  // Description:
  // Get the actual reduction. This value is only valid after the
  // filter has executed.
  vtkGetMacro(ActualReduction, double);

protected:
  vtkQuadricDecimation();
  ~vtkQuadricDecimation();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Do the dirty work of eliminating the edge; return the number of
  // triangles deleted.
  int CollapseEdge(vtkIdType pt0Id, vtkIdType pt1Id);

  // Description:
  // Compute quadric for all vertices
  void InitializeQuadrics(vtkIdType numPts);

  // Description:
  // Free boundary edges are weighted
  void AddBoundaryConstraints(void);

  // Description:
  // Compute quadric for this vertex.
  void ComputeQuadric(vtkIdType pointId);

  // Description:
  // Add the quadrics for these 2 points since the edge between them has
  // been collapsed.
  void AddQuadric(vtkIdType oldPtId, vtkIdType newPtId);

  // Description:
  // Compute cost for contracting this edge and the point that gives us this
  // cost.
  double ComputeCost(vtkIdType edgeId, double *x);
  double ComputeCost2(vtkIdType edgeId, double *x);

  // Description:
  // Find all edges that will have an endpoint change ids because of an edge
  // collapse.  p1Id and p2Id are the endpoints of the edge.  p2Id is the
  // pointId being removed.
  void FindAffectedEdges(vtkIdType p1Id, vtkIdType p2Id, vtkIdList *edges);

  // Description:
  // Find a cell that uses this edge.
  vtkIdType GetEdgeCellId(vtkIdType p1Id, vtkIdType p2Id);

  int IsGoodPlacement(vtkIdType pt0Id, vtkIdType pt1Id, const double *x);
  int TrianglePlaneCheck(const double t0[3], const double t1[3],
                         const double t2[3],  const double *x);
  void ComputeNumberOfComponents(void);
  void UpdateEdgeData(vtkIdType ptoId, vtkIdType pt1Id);

  // Description:
  // Helper function to set and get the point and it's attributes as an array
  void SetPointAttributeArray(vtkIdType ptId, const double *x);
  void GetPointAttributeArray(vtkIdType ptId, double *x);

  // Description:
  // Find out how many components there are for each attribute for this
  // poly data.
  void GetAttributeComponents();

  double TargetReduction;
  double ActualReduction;
  int   AttributeErrorMetric;

  int ScalarsAttribute;
  int VectorsAttribute;
  int NormalsAttribute;
  int TCoordsAttribute;
  int TensorsAttribute;

  double ScalarsWeight;
  double VectorsWeight;
  double NormalsWeight;
  double TCoordsWeight;
  double TensorsWeight;

  int               NumberOfEdgeCollapses;
  vtkEdgeTable     *Edges;
  vtkIdList        *EndPoint1List;
  vtkIdList        *EndPoint2List;
  vtkPriorityQueue *EdgeCosts;
  vtkDoubleArray   *TargetPoints;
  int               NumberOfComponents;
  vtkPolyData      *Mesh;

  //BTX
  struct ErrorQuadric
  {
    double *Quadric;
  };
  //ETX

  ErrorQuadric *ErrorQuadrics;
  int           AttributeComponents[6];
  double        AttributeScale[6];

  // Temporary variables for performance
  vtkIdList *CollapseCellIds;
  double *TempX;
  double *TempQuad;
  double *TempB;
  double **TempA;
  double *TempData;

private:
  vtkQuadricDecimation(const vtkQuadricDecimation&);  // Not implemented.
  void operator=(const vtkQuadricDecimation&);  // Not implemented.
};

#endif
