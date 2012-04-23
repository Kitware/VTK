/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrderedTriangulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// be as fast as possible (since the algorithm can be slow) and uses block
// memory allocations to support rapid triangulation generation. Also, the
// assumption behind the class is that a maximum of hundreds of points are to
// be triangulated. If you desire more robust triangulation methods use
// vtkPolygon::Triangulate(), vtkDelaunay2D, or vtkDelaunay3D.
//
// .SECTION Background
// This work is documented in the technical paper: W.J. Schroeder, B. Geveci,
// M. Malaterre. Compatible Triangulations of Spatial Decompositions. In
// Proceedings of Visualization 2004, IEEE Press October 2004.
//
// Delaunay triangulations are unique assuming a random distribution of input
// points. The 3D Delaunay criterion is as follows: the circumsphere of each
// tetrahedron contains no other points of the triangulation except for the
// four points defining the tetrahedron.  In application this property is
// hard to satisfy because objects like cubes are defined by eight points all
// sharing the same circumsphere (center and radius); hence the Delaunay
// triangulation is not unique.  These so-called degenerate situations are
// typically resolved by arbitrary selecting a triangulation. This code does
// something different: it resolves degenerate triangulations by modifying
// the "InCircumsphere" method to use a slightly smaller radius. Hence,
// degenerate points are always considered "out" of the circumsphere. This,
// in combination with an ordering (based on id) of the input points,
// guarantees a unique triangulation.
//
// There is another related characteristic of Delaunay triangulations. Given
// a N-dimensional Delaunay triangulation, points laying on a (N-1) dimensional
// plane also form a (N-1) Delaunay triangulation. This means for example,
// that if a 3D cell is defined by a set of (2D) planar faces, then the
// face triangulations are Delaunay. Combining this with the method to
// generate unique triangulations described previously, the triangulations
// on the face are guaranteed unique. This fact can be used to triangulate
// 3D objects in such a way to guarantee compatible face triangulations.
// This is a very useful fact for parallel processing, or performing
// operations like clipping that require compatible triangulations across
// 3D cell faces. (See vtkClipVolume for an example.)
//
// A special feature of this class is that it can generate triangulation
// templates on the fly. If template triangulation is enabled, then the
// ordered triangulator will first triangulate the cell using the slower
// ordered Delaunay approach, and then store the result as a template.
// Later, if the same cell type and cell configuration is encountered,
// then the template is reused which greatly speeds the triangulation.

// .SECTION Caveats
// Duplicate vertices will be ignored, i.e., if two points have the same
// coordinates the second one is discarded. The implications are that the
// user of this class must prevent duplicate points. Because the precision
// of this algorithm is double, it's also a good idea to merge points
// that are within some epsilon of one another.
//
// The triangulation is performed using the parametric coordinates of the
// inserted points. Therefore the bounds (see InitTriangulation()) should
// represent the range of the parametric coordinates of the inserted points.

// .SECTION See Also
// vtkDelaunay2D vtkDelaunay3D vtkPolygon

#ifndef __vtkOrderedTriangulator_h
#define __vtkOrderedTriangulator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class  vtkUnstructuredGrid;
class  vtkCellArray;
class  vtkHeap;
class  vtkIdList;
class  vtkPoints;
class  vtkTetra;
class  vtkDataArray;
class  vtkDoubleArray;
struct vtkOTMesh;
struct vtkOTTemplates;
class vtkIncrementalPointLocator;
class vtkPointData;
class vtkCellData;

// Template ID's must be 32-bits. See .cxx file for more information.
#if VTK_SIZEOF_SHORT == 4
typedef unsigned short  TemplateIDType;
#elif VTK_SIZEOF_INT == 4
typedef unsigned int    TemplateIDType;
#elif VTK_SIZEOF_LONG == 4
typedef unsigned long   TemplateIDType;
#endif

class VTKCOMMONDATAMODEL_EXPORT vtkOrderedTriangulator : public vtkObject
{
public:
  vtkTypeMacro(vtkOrderedTriangulator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object.
  static vtkOrderedTriangulator *New();

  // Description:
  // Initialize the triangulation process. Provide a bounding box and
  // the maximum number of points to be inserted. Note that since the
  // triangulation is performed using parametric coordinates (see
  // InsertPoint()) the bounds should be represent the range of the
  // parametric coordinates inserted.
  // \post no_point_inserted: GetNumberOfPoints()==0
  void InitTriangulation(double xmin, double xmax, double ymin, double ymax,
                         double zmin, double zmax, int numPts);
  void InitTriangulation(double bounds[6], int numPts);

  // Description:
  // For each point to be inserted, provide an id, a position x, parametric
  // coordinate p, and whether the point is inside (type=0), outside
  // (type=1), or on the boundary (type=2). You must call InitTriangulation()
  // prior to invoking this method. Make sure that the number of points
  // inserted does not exceed the numPts specified in
  // InitTriangulation(). Also note that the "id" can be any integer and can
  // be greater than numPts. It is used to create tetras (in AddTetras()) with
  // the appropriate connectivity ids. The method returns an internal id that
  // can be used prior to the Triangulate() method to update the type of the
  // point with UpdatePointType(). (Note: the algorithm triangulated with the
  // parametric coordinate p[3] and creates tetras with the global coordinate
  // x[3]. The parametric coordinates and global coordinates may be the same.)
  vtkIdType InsertPoint(vtkIdType id, double x[3], double p[3], int type);
  vtkIdType InsertPoint(vtkIdType id, vtkIdType sortid, double x[3],
                        double p[3], int type);
  vtkIdType InsertPoint(vtkIdType id, vtkIdType sortid,  vtkIdType sortid2,
                        double x[3], double p[3], int type);

  // Description:
  // Perform the triangulation. (Complete all calls to InsertPoint() prior
  // to invoking this method.) A special version is available when templates
  // should be used.
  void Triangulate();
  void TemplateTriangulate(int cellType, int numPts, int numEdges);

  // Description:
  // Update the point type. This is useful when the merging of nearly
  // coincident points is performed. The id is the internal id returned
  // from InsertPoint(). The method should be invoked prior to the
  // Triangulate method. The type is specified as inside (type=0),
  // outside (type=1), or on the boundary (type=2).
  // \pre valid_range: internalId>=0 && internalId<this->GetNumberOfPoints()
  void UpdatePointType(vtkIdType internalId, int type);

  // Description:
  // Return the parametric coordinates of point `internalId'.
  // It assumes that the point has already been inserted.
  // The method should be invoked prior to the Triangulate method.
  // \pre valid_range: internalId>=0 && internalId<this->GetNumberOfPoints()
  double *GetPointPosition(vtkIdType internalId);

  // Description:
  // Return the global coordinates of point `internalId'.
  // It assumes that the point has already been inserted.
  // The method should be invoked prior to the Triangulate method.
  // \pre valid_range: internalId>=0 && internalId<this->GetNumberOfPoints()
  double *GetPointLocation(vtkIdType internalId);

  // Description:
  // Return the Id of point `internalId'. This id is the one passed in
  // argument of InsertPoint.
  // It assumes that the point has already been inserted.
  // The method should be invoked prior to the Triangulate method.
  // \pre valid_range: internalId>=0 && internalId<this->GetNumberOfPoints()
  vtkIdType GetPointId(vtkIdType internalId);

  // Description:
  // Return the number of inserted points.
  vtkGetMacro(NumberOfPoints,int);

  // Description:
  // If this flag is set, then the ordered triangulator will create
  // and use templates for the triangulation. To use templates, the
  // TemplateTriangulate() method should be called when appropriate.
  // (Note: the TemplateTriangulate() method works for complete
  // (interior) cells without extra points due to intersection, etc.)
  vtkSetMacro(UseTemplates,int);
  vtkGetMacro(UseTemplates,int);
  vtkBooleanMacro(UseTemplates,int);

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
  // Assuming that all the inserted points come from a cell `cellId' to
  // triangulate, get the tetrahedra in outConnectivity, the points in locator
  // and copy point data and cell data. Return the number of added tetras.
  // \pre locator_exists: locator!=0
  // \pre outConnectivity: outConnectivity!=0
  // \pre inPD_exists: inPD!=0
  // \pre outPD_exists:  outPD!=0
  // \pre inCD_exists: inCD!=0
  // \pre outCD_exists: outCD!=0
  vtkIdType AddTetras(int classification,
                      vtkIncrementalPointLocator *locator,
                      vtkCellArray *outConnectivity,
                      vtkPointData *inPD,
                      vtkPointData *outPD,
                      vtkCellData *inCD,
                      vtkIdType cellId,
                      vtkCellData *outCD);

  // Description:
  // Add the tetrahedra classified (0=inside,1=outside) to the list
  // of ids and coordinates provided. These assume that the first four points
  // form a tetrahedron, the next four the next, and so on.
  vtkIdType AddTetras(int classification, vtkIdList *ptIds, vtkPoints *pts);

  // Description:
  // Add the triangle faces classified (2=boundary) to the connectivity
  // list provided. The method returns the number of triangles.
  vtkIdType AddTriangles(vtkCellArray *connectivity);

  // Description:
  // Add the triangle faces classified (2=boundary) and attached to the
  // specified point id to the connectivity list provided. (The id is the
  // same as that specified in InsertPoint().)
  vtkIdType AddTriangles(vtkIdType id, vtkCellArray *connectivity);

  // Description:
  // Methods to get one tetra at a time. Start with InitTetraTraversal()
  // and then invoke GetNextTetra() until the method returns 0.
  void InitTetraTraversal();

  // Description:
  // Methods to get one tetra at a time. Start with InitTetraTraversal()
  // and then invoke GetNextTetra() until the method returns 0.
  // cellScalars are point-centered scalars on the original cell.
  // tetScalars are point-centered scalars on the tetra: the values will be
  // copied from cellScalars.
  // \pre tet_exists: tet!=0
  // \pre cellScalars_exists: cellScalars!=0
  // \pre tetScalars_exists: tetScalars!=0
  // \pre tetScalars_valid_size: tetScalars->GetNumberOfTuples()==4
  int  GetNextTetra(int classification, vtkTetra *tet,
                    vtkDataArray *cellScalars, vtkDoubleArray *tetScalars);

protected:
  vtkOrderedTriangulator();
  ~vtkOrderedTriangulator();

private:
  void       Initialize();

  vtkOTMesh *Mesh;
  int        NumberOfPoints; //number of points inserted
  int        MaximumNumberOfPoints; //maximum possible number of points to be inserted
  double     Bounds[6];
  int        PreSorted;
  int        UseTwoSortIds;
  vtkHeap   *Heap;
  double     Quanta;

  int             UseTemplates;
  int             CellType;
  int             NumberOfCellPoints;
  int             NumberOfCellEdges;
  vtkHeap        *TemplateHeap;
  vtkOTTemplates *Templates;
  int             TemplateTriangulation();
  void            AddTemplate();
  TemplateIDType  ComputeTemplateIndex();

private:
  vtkOrderedTriangulator(const vtkOrderedTriangulator&);  // Not implemented.
  void operator=(const vtkOrderedTriangulator&);  // Not implemented.
};

#endif


