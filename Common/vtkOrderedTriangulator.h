/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrderedTriangulator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOrderedTriangulator - helper class to generate triangulations
// .SECTION Description
// This class is used to generate unique triangulations of points. The
// uniqueness of the triangulation is controlled by the id of the inserted
// points in combination with a Delaunay criterion. The class is designed to
// be very fast and uses block memory allocations to support rapid
// triangulation generation. Also, the assumption behind the class is that a
// maximum of hundreds of points are to be triangulated. If you desire more
// robust triangulation methods use vtkPolygon::Triangulate(), vtkDelaunay2D,
// or vtkDelaunay3D.
//
// Background: Delaunay triangulations are unique assuming a random
// distribution of input points. The 3D Delaunay criterion is as follows:
// the circumsphere of each tetrahedron contains no other points of the
// triangulation except for the four points defining the tetrahedron.
// In application this property is hard to satisfy because objects like
// cubes are defined by eight points all sharing the same circumsphere
// (center and radius); hence the Delaunay triangulation is not unique.
// These so-called degenerate situations are typically resolved by
// arbitrary selecting a triangulation. This code does something different:
// it resolves degenerate triangulations by modifying the "InCircumsphere"
// method to use a slightly smaller radius. Hence, degenerate points are
// always considered "out" of the circumsphere. This, in combination with
// an ordering (based on id) of the input points, guarantees a unique
// triangulation.
//
// There is another related characteristic of Delaunay triangulations. Given
// a N-dimensional Delaunay triangulation, points lying on a (N-1) dimensional
// plane also form a (N-1) Delaunay triangulation. This means for example,
// that if a 3D cell is defined by a set of (2D) planar faces, then the
// face triangulations are Delaunay. Combinaing this with the method to
// generate unique triangulations described previously, the triangulations
// on the face are guaranteed unique. This fact can be used to triangulate
// 3D objects in such a way to guarantee compatible face triangulations.
// This is a very useful fact for parallel processing, or performing
// operations like clipping that require compatible triangulations across
// 3D cell faces. (See vtkClipVolume for an example.)

// .SECTION Caveats
// Duplicate vertices will be ignored, i.e., if two points have the same
// coordinates the second one is discarded. The implications are that the
// user of this class most prevent duplicate points. Because the precision
// of this algorithm is double, it's also a good idea to merge points
// that are within some epsilon of one another.

// .SECTION See Also
// vtkDelaunay2D vtkDelaunay3D vtkPolygon

#ifndef __vtkOrderedTriangulator_h
#define __vtkOrderedTriangulator_h

#include "vtkObject.h"

class vtkUnstructuredGrid;
class vtkOTMesh;
class vtkCellArray;
class vtkMemoryPool;
class vtkIdList;
class vtkPoints;

class VTK_COMMON_EXPORT vtkOrderedTriangulator : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkOrderedTriangulator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object.
  static vtkOrderedTriangulator *New();

  // Description:
  // Initialize the triangulation process. Provide a bounding box and
  // the maximum number of points to be inserted.
  void InitTriangulation(float bounds[6], int numPts);

  // Description:
  // For each point to be inserted, provide an id, a position x, and
  // whether the point is inside (type=0), outside (type=1), or on the
  // boundary (type=2). You must call InitTriangulation() prior to 
  // invoking this method. Make sure that the number of points inserted
  // does not exceed the numPts specified in InitTriangulation(). Also
  // note that the "id" can be any integer and can be greater than 
  // numPts. It is used to create tetras (in AddTetras() with the
  // appropriate connectivity ids. The method returns an internal id that
  // can be used prior to the Triangulate() method to update the type of
  // the point with UpdatePointType().
  vtkIdType InsertPoint(vtkIdType id, float x[3], int type);
  vtkIdType InsertPoint(vtkIdType id, vtkIdType sortid, float x[3], int type);
  vtkIdType InsertPoint(vtkIdType id, vtkIdType sortid,  vtkIdType sortid2, 
                        float x[3], int type);

  // Description:
  // Perform the triangulation. (Complete all calls to InsertPoint() prior
  // to invoking this method.)
  void Triangulate();

  // Description:
  // Update the point type. This is useful when the merging of nearly 
  // coincident points is performed. The id is the internal id returned
  // from InsertPoint(). The method should be invoked prior to the
  // Triangulate method. The type is specified as inside (type=0), 
  // outside (type=1), or on the boundary (type=2). 
  void UpdatePointType(vtkIdType internalId, int type);

  // Description:
  // Boolean indicates whether the points have been pre-sorted. If 
  // pre-sorted is enabled, the points are not sorted on point id.
  // By default, presorted is off. (The point id is defined in
  // InsertPoint().)
  vtkSetMacro(PreSorted,int);
  vtkGetMacro(PreSorted,int);
  vtkBooleanMacro(PreSorted,int);

  // Description:
  // Tells the triangulator that a second sort id is provided
  // for each point and should also be considered when sorting.
  vtkSetMacro(UseTwoSortIds,int);
  vtkGetMacro(UseTwoSortIds,int);
  vtkBooleanMacro(UseTwoSortIds,int);

  // Description:
  // Initialize and add the tetras and points from the triangulation to the
  // unstructured grid provided.  New points are created and the mesh is
  // allocated. (This method differs from AddTetras() in that it inserts
  // points and cells; AddTetras only adds the tetra cells.) The tetrahdera
  // added are of the type specified (0=inside,1=outside,2=all). Inside
  // tetrahedron are those whose points are classified "inside" or on the
  // "boundary."  Outside tetrahedron have at least one point classified
  // "outside."  The method returns the number of tetrahedrahedron of the
  // type requested.
  vtkIdType GetTetras(int classification, vtkUnstructuredGrid *ugrid);
  
  // Description:
  // Add the tetras to the unstructured grid provided. The unstructured
  // grid is assumed to have been initialized (with Allocate()) and
  // points set (with SetPoints()). The tetrahdera added are of the type
  // specified (0=inside,1=outside,2=all). Inside tetrahedron are 
  // those whose points are classified "inside" or on the "boundary." 
  // Outside tetrahedron have at least one point classified "outside." 
  // The method returns the number of tetrahedrahedron of the type 
  // requested.
  vtkIdType AddTetras(int classification, vtkUnstructuredGrid *ugrid);
  
  // Description:
  // Add the tetrahedra classified (0=inside,1=outside) to the connectivity
  // list provided. Inside tetrahedron are those whose points are all
  // classified "inside." Outside tetrahedron have at least one point
  // classified "outside." The method returns the number of tetrahedron
  // of the type requested.    
  vtkIdType AddTetras(int classification, vtkCellArray *connectivity);
  
  // Description:
  // Add the tetrahedra classified (0=inside,1=outside) to the list
  // of ids and coordinates provided. These assume that the first four points
  // form a tetrahedron, the next four the next, and so on.
  vtkIdType AddTetras(int classification, vtkIdList *ptIds, vtkPoints *pts);
  
  // Description:
  // Add the triangle faces classified (2=boundary) to the connectivity
  // list provided. The method returns the number of triangles.
  vtkIdType AddTriangles(vtkCellArray *connectivity);
  
protected:
  vtkOrderedTriangulator();
  ~vtkOrderedTriangulator();

private:
  vtkOTMesh  *Mesh;
  int NumberOfPoints; //number of points inserted
  int MaximumNumberOfPoints; //maximum possible number of points to be inserted
  int PreSorted;
  int UseTwoSortIds;
  vtkMemoryPool* Pool;
  
private:
  vtkOrderedTriangulator(const vtkOrderedTriangulator&);  // Not implemented.
  void operator=(const vtkOrderedTriangulator&);  // Not implemented.
};

#endif


