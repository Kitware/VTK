/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReebGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   vtkReebGraph
 * @brief   Reeb graph computation for PL scalar fields.
 *
 *
 * vtkReebGraph is a class that computes a Reeb graph given a PL scalar
 * field (vtkDataArray) defined on a simplicial mesh.
 * A Reeb graph is a concise representation of the connectivity evolution of
 * the level sets of a scalar function.
 *
 * It is particularly useful in visualization (optimal seed set computation,
 * fast flexible isosurface extraction, automated transfer function design,
 * feature-driven visualization, etc.) and computer graphics (shape
 * deformation, shape matching, shape compression, etc.).
 *
 * Reference:
 * "Sur les points singuliers d'une forme de Pfaff completement integrable ou
 * d'une fonction numerique",
 * G. Reeb,
 * Comptes-rendus de l'Academie des Sciences, 222:847-849, 1946.
 *
 * vtkReebGraph implements one of the latest and most robust Reeb graph
 * computation algorithms.
 *
 * Reference:
 * "Robust on-line computation of Reeb graphs: simplicity and speed",
 * V. Pascucci, G. Scorzelli, P.-T. Bremer, and A. Mascarenhas,
 * ACM Transactions on Graphics, Proc. of SIGGRAPH 2007.
 *
 * vtkReebGraph provides methods for computing multi-resolution topological
 * hierarchies through topological simplification.
 * Topoligical simplification can be either driven by persistence homology
 * concepts (default behavior) or by application specific metrics (see
 * vtkReebGraphSimplificationMetric).
 * In the latter case, designing customized simplification metric evaluation
 * algorithms enables the user to control the definition of what should be
 * considered as noise or signal in the topological filtering process.
 *
 * References:
 * "Topological persistence and simplification",
 * H. Edelsbrunner, D. Letscher, and A. Zomorodian,
 * Discrete Computational Geometry, 28:511-533, 2002.
 *
 * "Extreme elevation on a 2-manifold",
 * P.K. Agarwal, H. Edelsbrunner, J. Harer, and Y. Wang,
 * ACM Symposium on Computational Geometry, pp. 357-365, 2004.
 *
 * "Simplifying flexible isosurfaces using local geometric measures",
 * H. Carr, J. Snoeyink, M van de Panne,
 * IEEE Visualization, 497-504, 2004
 *
 * "Loop surgery for volumetric meshes: Reeb graphs reduced to contour trees",
 * J. Tierny, A. Gyulassy, E. Simon, V. Pascucci,
 * IEEE Trans. on Vis. and Comp. Graph. (Proc of IEEE VIS), 15:1177-1184, 2009.
 *
 *
 *
 * Reeb graphs can be computed from 2D data (vtkPolyData, with triangles only)
 * or 3D data (vtkUnstructuredGrid, with tetrahedra only), sequentially (see
 * the "Build" calls) or in streaming (see the "StreamTriangle" and
 * "StreamTetrahedron" calls).
 *
 * vtkReebGraph inherits from vtkMutableDirectedGraph.
 *
 * Each vertex of a vtkReebGraph object represents a critical point of the
 * scalar field where the connectivity of the related level set changes
 * (creation, deletion, split or merge of connected components).
 * A vtkIdTypeArray (called "Vertex Ids") is associated with the VertexData of
 * a vtkReebGraph object, in order to retrieve if necessary the exact Ids of
 * the corresponding vertices in the input mesh.
 *
 * The edges of a vtkReebGraph object represent the regions of the input mesh
 * separated by the critical contours of the field, and where the connectivity
 * of the input field does not change.
 * A vtkVariantArray is associated with the EdgeDta of a vtkReebGraph object and
 * each entry of this array is a vtkAbstractArray containing the Ids of the
 * vertices of those regions, sorted by function value (useful for flexible
 * isosurface extraction or level set signature computation, for instance).
 *
 * See Graphics/Testing/Cxx/TestReebGraph.cxx for examples of traversals and
 * typical usages (customized simplification, skeletonization, contour spectra,
 *  etc.) of a vtkReebGraph object.
 *
 *
 * @sa
 *      vtkReebGraphSimplificationMetric
 *      vtkPolyDataToReebGraphFilter
 *      vtkUnstructuredGridToReebGraphFilter
 *      vtkReebGraphSimplificationFilter
 *      vtkReebGraphSurfaceSkeletonFilter
 *      vtkReebGraphVolumeSkeletonFilter
 *      vtkAreaContourSpectrumFilter
 *      vtkVolumeContourSpectrumFilter
 *
 * @par Tests:
 *      Graphics/Testing/Cxx/TestReebGraph.cxx
*/

#ifndef vtkReebGraph_h
#define vtkReebGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include  "vtkMutableDirectedGraph.h"

class vtkDataArray;
class vtkDataSet;
class vtkIdList;
class vtkPolyData;
class vtkReebGraphSimplificationMetric;
class vtkUnstructuredGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkReebGraph : public vtkMutableDirectedGraph
{

public:

  static vtkReebGraph *New();

  vtkTypeMacro(vtkReebGraph, vtkMutableDirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  void PrintNodeData(ostream& os, vtkIndent indent);

  /**
   * Return class name of data type. This is one of VTK_STRUCTURED_GRID,
   * VTK_STRUCTURED_POINTS, VTK_UNSTRUCTURED_GRID, VTK_POLY_DATA, or
   * VTK_RECTILINEAR_GRID (see vtkSetGet.h for definitions).
   * THIS METHOD IS THREAD SAFE
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_REEB_GRAPH;}


  enum
  {
    ERR_INCORRECT_FIELD = -1,
    ERR_NO_SUCH_FIELD = -2,
    ERR_NOT_A_SIMPLICIAL_MESH = -3
  };

  /**
   * Build the Reeb graph of the field 'scalarField' defined on the surface
   * mesh 'mesh'.

   * Returned values:

   * vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
   * tuples as 'mesh' has vertices.

   * vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
   * simplicial mesh (for example, the surface mesh contains quads instead of
   * triangles).
   */
  int Build(vtkPolyData *mesh, vtkDataArray *scalarField);

  /**
   * Build the Reeb graph of the field 'scalarField' defined on the volume
   * mesh 'mesh'.

   * Returned values:

   * vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
   * tuples as 'mesh' has vertices.

   * vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
   * simplicial mesh.
   */
  int Build(vtkUnstructuredGrid *mesh, vtkDataArray *scalarField);


  /**
   * Build the Reeb graph of the field given by the Id 'scalarFieldId',
   * defined on the surface mesh 'mesh'.

   * Returned values:

   * vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
   * tuples as 'mesh' as vertices.

   * vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
   * simplicial mesh (for example, the surface mesh contains quads instead of
   * triangles).

   * vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the Id
   * 'scalarFieldId' does not exist.
   */
  int Build(vtkPolyData *mesh, vtkIdType scalarFieldId);

  /**
   * Build the Reeb graph of the field given by the Id 'scalarFieldId',
   * defined on the volume mesh 'mesh'.

   * Returned values:

   * vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
   * tuples as 'mesh' as vertices.

   * vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
   * simplicial mesh.

   * vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the Id
   * 'scalarFieldId' does not exist.
   */
  int Build(vtkUnstructuredGrid *mesh, vtkIdType scalarFieldId);


  /**
   * Build the Reeb graph of the field given by the name 'scalarFieldName',
   * defined on the surface mesh 'mesh'.

   * Returned values:

   * vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
   * tuples as 'mesh' as vertices.

   * vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
   * simplicial mesh (for example, the surface mesh contains quads instead of
   * triangles).

   * vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the name
   * 'scalarFieldName' does not exist.
   */
  int Build(vtkPolyData *mesh, const char* scalarFieldName);

  /**
   * Build the Reeb graph of the field given by the name 'scalarFieldName',
   * defined on the volume mesh 'mesh'.

   * Returned values:

   * vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
   * tuples as 'mesh' as vertices.

   * vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
   * simplicial mesh.

   * vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the name
   * 'scalarFieldName' does not exist.
   */
  int Build(vtkUnstructuredGrid *mesh, const char* scalarFieldName);

  /**
   * Streaming Reeb graph computation.
   * Add to the streaming computation the triangle of the vtkPolyData surface
   * mesh described by
   * vertex0Id, scalar0
   * vertex1Id, scalar1
   * vertex2Id, scalar2

   * where vertex<i>Id is the Id of the vertex in the vtkPolyData structure
   * and scalar<i> is the corresponding scalar field value.

   * IMPORTANT: The stream _must_ be finalized with the "CloseStream" call.
   */
  int StreamTriangle( vtkIdType vertex0Id, double scalar0,
                      vtkIdType vertex1Id, double scalar1,
                      vtkIdType vertex2Id, double scalar2);

  /**
   * Streaming Reeb graph computation.
   * Add to the streaming computation the tetrahedra of the vtkUnstructuredGrid
   * volume mesh described by
   * vertex0Id, scalar0
   * vertex1Id, scalar1
   * vertex2Id, scalar2
   * vertex3Id, scalar3

   * where vertex<i>Id is the Id of the vertex in the vtkUnstructuredGrid
   * structure and scalar<i> is the corresponding scalar field value.

   * IMPORTANT: The stream _must_ be finalized with the "CloseStream" call.
   */
  int StreamTetrahedron( vtkIdType vertex0Id, double scalar0,
                         vtkIdType vertex1Id, double scalar1,
                         vtkIdType vertex2Id, double scalar2,
                         vtkIdType vertex3Id, double scalar3);

  /**
   * Finalize internal data structures, in the case of streaming computations
   * (with StreamTriangle or StreamTetrahedron).
   * After this call, no more triangle or tetrahedron can be inserted via
   * StreamTriangle or StreamTetrahedron.
   * IMPORTANT: This method _must_ be called when the input stream is finished.
   * If you need to get a snapshot of the Reeb graph during the streaming
   * process (to parse or simplify it), do a DeepCopy followed by a
   * CloseStream on the copy.
   */
  void CloseStream();

  // Descrition:
  // Implements deep copy
  void DeepCopy(vtkDataObject *src) VTK_OVERRIDE;

  /**
   * Simplify the Reeb graph given a threshold 'simplificationThreshold'
   * (between 0 and 1).

   * This method is the core feature for Reeb graph multi-resolution hierarchy
   * construction.

   * Return the number of arcs that have been removed through the simplification
   * process.

   * 'simplificationThreshold' represents a "scale", under which each Reeb graph
   * feature is considered as noise. 'simplificationThreshold' is expressed as a
   * fraction of the scalar field overall span. It can vary from 0
   * (no simplification) to 1 (maximal simplification).

   * 'simplificationMetric' is an object in charge of evaluating the importance
   * of a Reeb graph arc at each step of the simplification process.
   * if 'simplificationMetric' is NULL, the default strategy (persitence of the
   * scalar field) is used.
   * Customized simplification metric evaluation algorithm can be designed (see
   * vtkReebGraphSimplificationMetric), enabling the user to control the
   * definition of what should be considered as noise or signal.

   * References:

   * "Topological persistence and simplification",
   * H. Edelsbrunner, D. Letscher, and A. Zomorodian,
   * Discrete Computational Geometry, 28:511-533, 2002.

   * "Extreme elevation on a 2-manifold",
   * P.K. Agarwal, H. Edelsbrunner, J. Harer, and Y. Wang,
   * ACM Symposium on Computational Geometry, pp. 357-365, 2004.

   * "Simplifying flexible isosurfaces using local geometric measures",
   * H. Carr, J. Snoeyink, M van de Panne,
   * IEEE Visualization, 497-504, 2004

   * "Loop surgery for volumetric meshes: Reeb graphs reduced to contour trees",
   * J. Tierny, A. Gyulassy, E. Simon, V. Pascucci,
   * IEEE Trans. on Vis. and Comp. Graph. (Proc of IEEE VIS), 15:1177-1184,2009.
   */
  int Simplify(double simplificationThreshold,
    vtkReebGraphSimplificationMetric *simplificationMetric);

  /**
   * Use a pre-defined Reeb graph (post-processing).
   * Use with caution!
   */
  void Set(vtkMutableDirectedGraph *g);

protected:

  vtkReebGraph();
  ~vtkReebGraph() VTK_OVERRIDE;

  class Implementation;
  Implementation* Storage;

private:
  vtkReebGraph(const vtkReebGraph&) VTK_DELETE_FUNCTION;
  void operator=(const vtkReebGraph&) VTK_DELETE_FUNCTION;

};

#endif
