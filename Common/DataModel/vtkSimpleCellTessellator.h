/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleCellTessellator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSimpleCellTessellator
 * @brief   helper class to perform cell tessellation
 *
 * vtkSimpleCellTessellator is a helper class to perform adaptive tessellation
 * of particular cell topologies. The major purpose for this class is to
 * transform higher-order cell types (e.g., higher-order finite elements)
 * into linear cells that can then be easily visualized by VTK. This class
 * works in conjunction with the vtkGenericDataSet and vtkGenericAdaptorCell
 * classes.
 *
 * This algorithm is based on edge subdivision. An error metric along each
 * edge is evaluated, and if the error is greater than some tolerance, the
 * edge is subdivided (as well as all connected 2D and 3D cells). The process
 * repeats until the error metric is satisfied. Since the algorithm is based
 * on edge subdivision it inherently avoid T-junctions.
 *
 * A significant issue addressed by this algorithm is to insure face
 * compatibility across neigboring cells. That is, diagonals due to face
 * triangulation must match to insure that the mesh is compatible. The
 * algorithm employs a precomputed table to accelerate the tessellation
 * process. The table was generated with the help of vtkOrderedTriangulator
 * the basic idea is that the choice of diagonal is made only by considering the
 * relative value of the point ids.
 *
 * @sa
 * vtkGenericCellTessellator vtkGenericSubdivisionErrorMetric vtkAttributesErrorMetric
 * vtkGeometricErrorMetric vtkViewDependentErrorMetric
*/

#ifndef vtkSimpleCellTessellator_h
#define vtkSimpleCellTessellator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkGenericCellTessellator.h"

class vtkTriangleTile;
class vtkTetraTile;
class vtkCellArray;
class vtkDoubleArray;
class vtkGenericEdgeTable;
class vtkGenericSubdivisionErrorMetric;
class vtkGenericAttributeCollection;
class vtkGenericAdaptorCell;
class vtkGenericCellIterator;
class vtkPointData;
class vtkOrderedTriangulator;
class vtkPolygon;
class vtkIdList;

//-----------------------------------------------------------------------------
//
// The tessellation object
class VTKCOMMONDATAMODEL_EXPORT vtkSimpleCellTessellator : public vtkGenericCellTessellator
{
public:
  static vtkSimpleCellTessellator *New();
  vtkTypeMacro(vtkSimpleCellTessellator,vtkGenericCellTessellator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the higher order cell in order to access the evaluation function.
   */
  vtkGetObjectMacro(GenericCell, vtkGenericAdaptorCell);
  //@}

  /**
   * Tessellate a face of a 3D `cell'. The face is specified by the
   * index value.
   * The result is a set of smaller linear triangles in `cellArray' with
   * `points' and point data `internalPd'.
   * \pre cell_exists: cell!=0
   * \pre valid_dimension: cell->GetDimension()==3
   * \pre valid_index_range: (index>=0) && (index<cell->GetNumberOfBoundaries(2))
   * \pre att_exists: att!=0
   * \pre points_exists: points!=0
   * \pre cellArray_exists: cellArray!=0
   * \pre internalPd_exists: internalPd!=0
   */
  void TessellateFace(vtkGenericAdaptorCell *cell,
                      vtkGenericAttributeCollection *att,
                      vtkIdType index,
                      vtkDoubleArray *points,
                      vtkCellArray *cellArray,
                      vtkPointData *internalPd) override;

  /**
   * Tessellate a 3D `cell'. The result is a set of smaller linear
   * tetrahedra in `cellArray' with `points' and point data `internalPd'.
   * \pre cell_exists: cell!=0
   * \pre valid_dimension: cell->GetDimension()==3
   * \pre att_exists: att!=0
   * \pre points_exists: points!=0
   * \pre cellArray_exists: cellArray!=0
   * \pre internalPd_exists: internalPd!=0
   */
  void Tessellate(vtkGenericAdaptorCell *cell,
                  vtkGenericAttributeCollection *att,
                  vtkDoubleArray *points,
                  vtkCellArray *cellArray,
                  vtkPointData *internalPd ) override;

  /**
   * Triangulate a 2D `cell'. The result is a set of smaller linear triangles
   * in `cellArray' with `points' and point data `internalPd'.
   * \pre cell_exists: cell!=0
   * \pre valid_dimension: cell->GetDimension()==2
   * \pre att_exists: att!=0
   * \pre points_exists: points!=0
   * \pre cellArray_exists: cellArray!=0
   * \pre internalPd_exists: internalPd!=0
   */
  void Triangulate(vtkGenericAdaptorCell *cell,
                   vtkGenericAttributeCollection *att,
                   vtkDoubleArray *points,
                   vtkCellArray *cellArray,
                   vtkPointData *internalPd) override;

  /**
   * Reset the output for repeated use of this class.
   */
  void Reset();


  /**
   * Initialize the tessellator with a data set `ds'.
   */
  void Initialize(vtkGenericDataSet *ds) override;

  /**
   * Return the number of fixed subdivisions. It is used to prevent from
   * infinite loop in degenerated cases. For order 3 or higher, if the
   * inflection point is exactly on the mid-point, error metric will not
   * detect that a subdivision is required. 0 means no fixed subdivision:
   * there will be only adaptive subdivisions.

   * The algorithm first performs `GetFixedSubdivisions' non adaptive
   * subdivisions followed by at most `GetMaxAdaptiveSubdivisions' adaptive
   * subdivisions. Hence, there are at most `GetMaxSubdivisionLevel'
   * subdivisions.
   * \post positive_result: result>=0 && result<=GetMaxSubdivisionLevel()
   */
  int GetFixedSubdivisions();

  /**
   * Return the maximum level of subdivision. It is used to prevent from
   * infinite loop in degenerated cases. For order 3 or higher, if the
   * inflection point is exactly on the mid-point, error metric will not
   * detect that a subdivision is required. 0 means no subdivision,
   * neither fixed nor adaptive.
   * \post positive_result: result>=GetFixedSubdivisions()
   */
  int GetMaxSubdivisionLevel();

  /**
   * Return the maximum number of adaptive subdivisions.
   * \post valid_result: result==GetMaxSubdivisionLevel()-GetFixedSubdivisions()
   */
  int GetMaxAdaptiveSubdivisions();

  /**
   * Set the number of fixed subdivisions. See GetFixedSubdivisions() for
   * more explanations.
   * \pre positive_level: level>=0 && level<=GetMaxSubdivisionLevel()
   * \post is_set: GetFixedSubdivisions()==level
   */
  void SetFixedSubdivisions(int level);

  /**
   * Set the maximum level of subdivision. See GetMaxSubdivisionLevel() for
   * more explanations.
   * \pre positive_level: level>=GetFixedSubdivisions()
   * \post is_set: level==GetMaxSubdivisionLevel()
   */
  void SetMaxSubdivisionLevel(int level);

  /**
   * Set both the number of fixed subdivisions and the maximum level of
   * subdivisions. See GetFixedSubdivisions(), GetMaxSubdivisionLevel() and
   * GetMaxAdaptiveSubdivisions() for more explanations.
   * \pre positive_fixed: fixed>=0
   * \pre valid_range: fixed<=maxLevel
   * \post fixed_is_set: fixed==GetFixedSubdivisions()
   * \post maxLevel_is_set: maxLevel==GetMaxSubdivisionLevel()
   */
  void SetSubdivisionLevels(int fixed,
                            int maxLevel);


protected:
  vtkSimpleCellTessellator();
  ~vtkSimpleCellTessellator() override;

  /**
   * Extract point `pointId' from the edge table to the output point and output
   * point data.
   */
  void CopyPoint(vtkIdType pointId);

  /**
   * HashTable instead of vtkPointLocator
   */
  vtkGenericEdgeTable *EdgeTable;

  void InsertEdgesIntoEdgeTable( vtkTriangleTile &tri );
  void RemoveEdgesFromEdgeTable( vtkTriangleTile &tri );
  void InsertPointsIntoEdgeTable( vtkTriangleTile &tri );

  void InsertEdgesIntoEdgeTable( vtkTetraTile &tetra );
  void RemoveEdgesFromEdgeTable( vtkTetraTile &tetra );

  /**
   * Initialize `root' with the sub-tetra defined by the `localIds' points on
   * the complex cell, `ids' are the global ids over the mesh of those points.
   * The sub-tetra is also defined by the ids of its edges and of its faces
   * relative to the complex cell. -1 means that the edge or the face of the
   * sub-tetra is not an original edge or face of the complex cell.
   * \pre cell_exists: this->GenericCell!=0
   * \pre localIds_exists: localIds!=0
   * \pre localIds_size: sizeof(localIds)==4
   * \pre ids_exists: ids!=0
   * \pre ids_size: sizeof(ids)==4
   * \pre edgeIds_exists: edgeIds!=0
   * \pre edgeIds_size: sizeof(edgeIds)==6
   * \pre faceIds_exists: faceIds!=0
   * \pre faceIds_size: sizeof(faceIds)==4
   */
  void InitTetraTile(vtkTetraTile &root,
                     vtkIdType *localIds,
                     vtkIdType *ids,
                     int *edgeIds,
                     int *faceIds);

  /**
   * Triangulate a triangle of `cell'. This triangle can be the top-level
   * triangle if the cell is a triangle or a toplevel sub-triangle is the cell
   * is a polygon, or a triangular face of a 3D cell or a top-level
   * sub-triangle of a face of a 3D cell if the face is not a triangle.
   * Arguments `localIds', `ids' and `edgeIds' have the same meaning than
   * for InitTetraTile.
   * \pre cell_exists: cell!=0
   * \pre localIds_exists: localIds!=0
   * \pre localIds_size: sizeof(localIds)==3
   * \pre ids_exists: ids!=0
   * \pre ids_size: sizeof(ids)==3
   * \pre edgeIds_exists: edgeIds!=0
   * \pre edgeIds_size: sizeof(edgeIds)==3
   */
  void TriangulateTriangle(vtkGenericAdaptorCell *cell,
                           vtkIdType *localIds,
                           vtkIdType *ids,
                           int *edgeIds,
                           vtkGenericAttributeCollection *att,
                           vtkDoubleArray *points,
                           vtkCellArray *cellArray,
                           vtkPointData *internalPd);

  /**
   * To access the higher order cell from third party library
   */
  vtkGenericAdaptorCell *GenericCell;

  /**
   * Allocate some memory if Scalars does not exists or is smaller than size.
   * \pre positive_size: size>0
   */
  void AllocateScalars(int size);

  /**
   * Scalar buffer used to save the interpolate values of the attributes
   * The capacity is at least the number of components of the attribute
   * collection ot the current cell.
   */

  // Scalar buffer that stores the global coordinates, parametric coordinates,
  // attributes at left, mid and right point. The format is:
  // lxlylz lrlslt [lalb lcldle...] mxmymz mrmsmt [mamb mcmdme...]
  // rxryrz rrrsrt [rarb rcrdre...]
  // The ScalarsCapacity>=(6+attributeCollection->GetNumberOfComponents())*3

  double *Scalars;
  int ScalarsCapacity;

  /**
   * Number of double value to skip to go to the next point into Scalars array
   * It is 6+attributeCollection->GetNumberOfComponents()
   */
  int PointOffset;

  /**
   * Used to iterate over edges boundaries in GetNumberOfCellsUsingEdges()
   */
  vtkGenericCellIterator    *CellIterator;

  /**
   * To access the higher order field from third party library
   */
  vtkGenericAttributeCollection *AttributeCollection;

  //@{
  /**
   * To avoid New/Delete
   */
  vtkDoubleArray     *TessellatePoints;  //Allow to use GetPointer
  vtkCellArray       *TessellateCellArray;
  vtkPointData *TessellatePointData;
  //@}

  int FindEdgeReferenceCount(double p1[3], double p2[3],
                             vtkIdType &e1, vtkIdType &e2);

  int GetNumberOfCellsUsingFace( int faceId );
  int GetNumberOfCellsUsingEdge( int edgeId );

  /**
   * Is the edge defined by vertices (`p1',`p2') in parametric coordinates on
   * some edge of the original tetrahedron? If yes return on which edge it is,
   * else return -1.
   * \pre p1!=p2
   * \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
   * \post valid_result: (result==-1) || ( result>=0 && result<=5 )
   */
  int IsEdgeOnFace(double p1[3], double p2[3]);

  /**
   * Return 1 if the parent of edge defined by vertices (`p1',`p2') in
   * parametric coordinates, is an edge; 3 if there is no parent (the edge is
   * inside). If the parent is an edge, return its id in `localId'.
   * \pre p1!=p2
   * \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
   * \post valid_result: (result==1)||(result==3)
   */
  int FindEdgeParent2D(double p1[3], double p2[3], int &localId);

  /**
   * Return 1 if the parent of edge defined by vertices (`p1',`p2') in
   * parametric coordinates, is an edge; 2 if the parent is a face, 3 if there
   * is no parent (the edge is inside). If the parent is an edge or a face,
   * return its id in `localId'.
   * \pre p1!=p2
   * \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
   * \post valid_result: result>=1 && result<=3
   */
  int FindEdgeParent(double p1[3], double p2[3], int &localId);

  /**
   * Allocate some memory if PointIds does not exist or is smaller than size.
   * \pre positive_size: size>0
   */
  void AllocatePointIds(int size);

  /**
   * Are the faces `originalFace' and `face' equal?
   * The result is independent from any order or orientation.
   * \pre originalFace_exists: originalFace!=0
   */
  int FacesAreEqual(int *originalFace,
                    int face[3]);

  /**
   * Number of points in the dataset to be tessellated.
   */
  vtkIdType NumberOfPoints;

  int FixedSubdivisions;
  int MaxSubdivisionLevel;
  int CurrentSubdivisionLevel;

  /**
   * For each edge (6) of the sub-tetra, there is the id of the original edge
   * or -1 if the edge is not an original edge
   */
  int *EdgeIds;
  /**
   * For each face (4) of the sub-tetra, there is the id of the original face
   * or -1 if the face is not an original face
   */
  int *FaceIds;

  // The following variables are for complex cells.

  // Used to create tetra from more complex cells, because the tessellator
  // is supposed to deal with simplices only.
  vtkOrderedTriangulator *Triangulator;

  // Used to store the sub-tetra during the tessellation of complex
  // cells.
  vtkCellArray *Connectivity;

  // Used to create triangles from a face of a complex cell.
  vtkPolygon *Polygon;

  // Used to store the sub-triangles during the tessellation of complex cells.
  vtkIdList *TriangleIds;

  vtkIdType *PointIds;
  int PointIdsCapacity;

private:
  vtkSimpleCellTessellator(const vtkSimpleCellTessellator&) = delete;
  void operator=(const vtkSimpleCellTessellator&) = delete;

  friend class vtkTetraTile;
  friend class vtkTriangleTile;

};

#endif
