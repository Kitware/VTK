/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellTessellator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericCellTessellator - helper class to perform cell tessellation
// .SECTION Description
// vtkGenericCellTessellator is a helper class to perform adaptive tessellation
// of particular cell topologies. The major purpose for this class is to
// transform higher-order cell types (e.g., higher-order finite elements)
// into linear cells that can then be easily visualized by VTK. This class
// works in conjunction with the vtkGenericDataSet and vtkGenericAdaptorCell
// classes.
//
// This algorithm is based on edge subdivision. An error metric along each
// edge is evaluated, and if the error is greater than some tolerance, the
// edge is subdivided (as well as all connected 2D and 3D cells). The process
// repeats until the error metric is satisfied. 
//
// A significant issue addressed by this algorithm is to insure face
// compatibility across neigboring cells. That is, diagaonals due to face
// triangulation must match to insure that the mesh is compatible. The
// algorithm employs a precomputed table to accelerate the ttessellation
// process. The table was generated with the help of  vtkOrderedTriangulator;
// the basic idea is that the choice of diagonal is made by considering the
// relative value of the point ids.


#ifndef __vtkGenericCellTessellator_h
#define __vtkGenericCellTessellator_h

#include "vtkObject.h"

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

//-----------------------------------------------------------------------------
//
// The tessellation object
class VTK_FILTERING_EXPORT vtkGenericCellTessellator : public vtkObject
{
public:
  static vtkGenericCellTessellator *New();
  vtkTypeRevisionMacro(vtkGenericCellTessellator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the higher order cell in order to access the evaluation function.
  vtkGetObjectMacro(GenericCell, vtkGenericAdaptorCell);

  // Description:
  // Tessellate a face of a tetrahedron cell. The face is specified by the
  // index value.
  // \pre cell_exists: cell!=0
  // \pre is_a_tetra: (cell->GetType()==VTK_TETRA)
  //                 ||(cell->GetType()==VTK_QUADRATIC_TETRA)
  //                 ||(cell->GetType()==VTK_HIGHER_ORDER_TETRAHEDRON)
  // \pre valid_index_range: (index>=0) && (index<4)
  virtual void TessellateTriangleFace(vtkGenericAdaptorCell *cell,
                                      vtkGenericAttributeCollection *att,
                                      vtkIdType index,
                                      vtkDoubleArray *points,
                                      vtkCellArray *cellArray,
                                      vtkPointData *internalPd);

  // Description:
  // Tessellate a tetrahedron `cell'. The result is a set of smaller linear
  // cells `cellArray' with `points' and point data `scalars'.
  // \pre cell_exists: cell!=0
  // \pre is_a_tetra: (cell->GetType()==VTK_TETRA)
  //                 ||(cell->GetType()==VTK_QUADRATIC_TETRA)
  //                 ||(cell->GetType()==VTK_HIGHER_ORDER_TETRAHEDRON)
  void Tessellate(vtkGenericAdaptorCell *cell,
                  vtkGenericAttributeCollection *att,
                  vtkDoubleArray *points,
                  vtkCellArray *cellArray,
                  vtkPointData *internalPd);

  // Description:
  // Triangulate a triangle `cell'.
  // \pre cell_exists: cell!=0
  // \pre is_a_triangle: (cell->GetType()==VTK_TRIANGLE)
  //                 ||(cell->GetType()==VTK_QUADRATIC_TRIANGLE)
  //                 ||(cell->GetType()==VTK_HIGHER_ORDER_TRIANGLE)
  void Triangulate(vtkGenericAdaptorCell *cell,
                   vtkGenericAttributeCollection *att,
                   vtkDoubleArray *points,
                   vtkCellArray *cellArray,
                   vtkPointData *internalPd);

  // Description:
  // Specify the object to use to compute the error metric.
  virtual void SetErrorMetric(vtkGenericSubdivisionErrorMetric *subAlgorithm);
  vtkGetObjectMacro(ErrorMetric,vtkGenericSubdivisionErrorMetric);

  // Description:
  // Reset the output for repeated use of this class.
  void Reset();

  // Description:
  // Initialize the internal hash table.
  void Initialize(vtkIdType numPts);

  // Description:
  // Return the internal edge table.
  vtkGenericEdgeTable *GetEdgeTable();
  
protected:
  vtkGenericCellTessellator();
  ~vtkGenericCellTessellator();
  
  // Description:
  // Extract point `pointId' from the edge table to the output point and output
  // point data.
  void CopyPoint(vtkIdType pointId);
  
  // Description:
  //HashTable instead of vtkPointLocator
  vtkGenericEdgeTable *EdgeTable;
  void InsertEdgesIntoEdgeTable( vtkTriangleTile &tri );
  void RemoveEdgesFromEdgeTable( vtkTriangleTile &tri );
  void InsertPointsIntoEdgeTable( vtkTriangleTile &tri );

  void InsertEdgesIntoEdgeTable( vtkTetraTile &tetra );
  void RemoveEdgesFromEdgeTable( vtkTetraTile &tetra );
  void InsertPointsIntoEdgeTable( vtkTetraTile &tetra );

  // Description:
  // To access the higher order cell from third party library
  vtkGenericAdaptorCell *GenericCell;

  // Description:
  // Allocate some memory if Scalars does not exists or is smaller than size.
  // \pre positive_size: size>0
  void AllocateScalars(int size);
  
  // Description:
  // Scalar buffer used to save the interpolate values of the attributes
  // The capacity is at least the number of components of the attribute
  // collection ot the current cell.
  
  // Scalar buffer that stores the global coordinates, parametric coordinates,
  // attributes at left, mid and right point. The format is:
  // lxlylz lrlslt [lalb lcldle...] mxmymz mrmsmt [mamb mcmdme...] 
  // rxryrz rrrsrt [rarb rcrdre...]
  // The ScalarsCapacity>=(6+attributeCollection->GetNumberOfComponents())*3
  
  double *Scalars;
  int ScalarsCapacity;
  
  // Description:
  // Number of double value to skip to go to the next point into Scalars array
  // It is 6+attributeCollection->GetNumberOfComponents()
  int PointOffset;
  
  // Description:
  // Used to iterate over edges boundaries in GetNumberOfCellsUsingEdges()
  vtkGenericCellIterator    *CellIterator;

  // Description:
  // To access the higher order field from third party library
  vtkGenericAttributeCollection *AttributeCollection;

  // Description:
  // To avoid New/Delete
  vtkDoubleArray     *TessellatePoints;  //Allow to use GetPointer
  vtkCellArray       *TessellateCellArray;
//  vtkDoubleArray     *TessellateScalars;
  vtkPointData *TessellatePointData;

  // Description:
  // Contains the error metric
  vtkGenericSubdivisionErrorMetric *ErrorMetric;

  // Description:
  // Internal function used to tessellate a triangle
  void InternalTessellateTriangle( vtkTriangleTile& root );

  int FindEdgeReferenceCount(double p1[3], double p2[3], 
                             vtkIdType &e1, vtkIdType &e2);

  int GetNumberOfCellsUsingFace( int faceId );
  int GetNumberOfCellsUsingEdge( int edgeId );
  
  // Description:
  // Is the edge defined by vertices (`p1',`p2') in parametric coordinates on
  // some edge of the original tetrahedron? If yes return on which edge it is,
  // else return -1.
  // \pre p1!=p2
  // \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
  // \post valid_result: (result==-1) || ( result>=0 && result<=5 )
  int IsEdgeOnFace(double p1[3], double p2[3]);
  
  // Description:
  // Return 1 if the parent of edge defined by vertices (`p1',`p2') in
  // parametric coordinates, is an edge; 3 if there is no parent (the edge is
  // inside). If the parent is an edge, return its id in `localId'.
  // \pre p1!=p2
  // \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
  // \post valid_result: (result==1)||(result==3)
  int FindEdgeParent2D(double p1[3], double p2[3], int &localId);
  
  // Description:
  // Return 1 if the parent of edge defined by vertices (`p1',`p2') in
  // parametric coordinates, is an edge; 2 if the parent is a face, 3 if there
  // is no parent (the edge is inside). If the parent is an edge or a face,
  // return its id in `localId'.
  // \pre p1!=p2
  // \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
  // \post valid_result: result>=1 && result<=3
  int FindEdgeParent(double p1[3], double p2[3], int &localId);
  
private:
  vtkGenericCellTessellator(const vtkGenericCellTessellator&);  // Not implemented.
  void operator=(const vtkGenericCellTessellator&);  // Not implemented.
  
  //BTX
  friend class vtkTetraTile;
  friend class vtkTriangleTile;
  //ETX
};

#endif
