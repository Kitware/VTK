/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyData - concrete dataset represents vertices, lines, polygons, and triangle strips
// .SECTION Description
// vtkPolyData is a data object that is a concrete implementation of
// vtkDataSet. vtkPolyData represents a geometric structure consisting of
// vertices, lines, polygons, and/or triangle strips. Point and cell
// attribute values (e.g., scalars, vectors, etc.) also are represented.
//
// The actual cell types (vtkCellType.h) supported by vtkPolyData are:
// vtkVertex, vtkPolyVertex, vtkLine, vtkPolyLine, vtkTriangle, vtkQuad,
// vtkPolygon, and vtkTriangleStrip.
//
// One important feature of vtkPolyData objects is that special traversal and
// data manipulation methods are available to process data. These methods are
// generally more efficient than vtkDataSet methods and should be used
// whenever possible. For example, traversing the cells in a dataset we would
// use GetCell(). To traverse cells with vtkPolyData we would retrieve the
// cell array object representing polygons (for example using GetPolys()) and
// then use vtkCellArray's InitTraversal() and GetNextCell() methods.
//
// .SECTION Caveats
// Because vtkPolyData is implemented with four separate instances of
// vtkCellArray to represent 0D vertices, 1D lines, 2D polygons, and 2D
// triangle strips, it is possible to create vtkPolyData instances that
// consist of a mixture of cell types. Because of the design of the class,
// there are certain limitations on how mixed cell types are inserted into
// the vtkPolyData, and in turn the order in which they are processed and
// rendered. To preserve the consistency of cell ids, and to insure that
// cells with cell data are rendered properly, users must insert mixed cells
// in the order of vertices (vtkVertex and vtkPolyVertex), lines (vtkLine and
// vtkPolyLine), polygons (vtkTriangle, vtkQuad, vtkPolygon), and triangle
// strips (vtkTriangleStrip).
//
// Some filters when processing vtkPolyData with mixed cell types may process
// the cells in differing ways. Some will convert one type into another
// (e.g., vtkTriangleStrip into vtkTriangles) or expect a certain type
// (vtkDecimatePro expects triangles or triangle strips; vtkTubeFilter
// expects lines). Read the documentation for each filter carefully to
// understand how each part of vtkPolyData is processed.

#ifndef vtkPolyData_h
#define vtkPolyData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkPointSet.h"

#include "vtkCellTypes.h" // Needed for inline methods
#include "vtkCellLinks.h" // Needed for inline methods

class vtkVertex;
class vtkPolyVertex;
class vtkLine;
class vtkPolyLine;
class vtkTriangle;
class vtkQuad;
class vtkPolygon;
class vtkTriangleStrip;
class vtkEmptyCell;
struct vtkPolyDataDummyContainter;

class VTKCOMMONDATAMODEL_EXPORT vtkPolyData : public vtkPointSet
{
public:
  static vtkPolyData *New();

  vtkTypeMacro(vtkPolyData,vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_POLY_DATA;}

  // Description:
  // Copy the geometric and topological structure of an input poly data object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Standard vtkDataSet interface.
  vtkIdType GetNumberOfCells();
  vtkCell *GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  int GetCellType(vtkIdType cellId);
  void GetCellBounds(vtkIdType cellId, double bounds[6]);
  void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                        vtkIdList *cellIds);

  // Description:
  // Copy cells listed in idList from pd, including points, point data,
  // and cell data.  This method assumes that point and cell data have
  // been allocated.  If you pass in a point locator, then the points
  // won't be duplicated in the output.
  void CopyCells(vtkPolyData *pd, vtkIdList *idList,
                 vtkPointLocator *locator = NULL);

  // Description:
  // Copy a cells point ids into list provided. (Less efficient.)
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);

  // Description:
  // Efficient method to obtain cells using a particular point. Make sure that
  // routine BuildLinks() has been called.
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);

  // Description:
  // Compute the (X, Y, Z)  bounds of the data.
  void ComputeBounds();

  // Description:
  // Recover extra allocated memory when creating data whose initial size
  // is unknown. Examples include using the InsertNextCell() method, or
  // when using the CellArray::EstimateSize() method to create vertices,
  // lines, polygons, or triangle strips.
  void Squeeze();

  // Description:
  // Return the maximum cell size in this poly data.
  int GetMaxCellSize();

  // Description:
  // Set the cell array defining vertices.
  void SetVerts (vtkCellArray* v);

  // Description:
  // Get the cell array defining vertices. If there are no vertices, an
  // empty array will be returned (convenience to simplify traversal).
  vtkCellArray *GetVerts();

  // Description:
  // Set the cell array defining lines.
  void SetLines (vtkCellArray* l);

  // Description:
  // Get the cell array defining lines. If there are no lines, an
  // empty array will be returned (convenience to simplify traversal).
  vtkCellArray *GetLines();

  // Description:
  // Set the cell array defining polygons.
  void SetPolys (vtkCellArray* p);

  // Description:
  // Get the cell array defining polygons. If there are no polygons, an
  // empty array will be returned (convenience to simplify traversal).
  vtkCellArray *GetPolys();

  // Description:
  // Set the cell array defining triangle strips.
  void SetStrips (vtkCellArray* s);

  // Description:
  // Get the cell array defining triangle strips. If there are no
  // triangle strips, an empty array will be returned (convenience to
  // simplify traversal).
  vtkCellArray *GetStrips();

  // Description:
  // Return the number of primitives of a particular type held..
  vtkIdType GetNumberOfVerts();
  vtkIdType GetNumberOfLines();
  vtkIdType GetNumberOfPolys();
  vtkIdType GetNumberOfStrips();

  // Description:
  // Method allocates initial storage for vertex, line, polygon, and
  // triangle strip arrays. Use this method before the method
  // PolyData::InsertNextCell(). (Or, provide vertex, line, polygon, and
  // triangle strip cell arrays.) The array capacity is doubled when the
  // inserting a cell exceeds the current capacity. extSize is no longer used.
  void Allocate(vtkIdType numCells=1000, int extSize=1000);

  // Description:
  // Similar to the method above, this method allocates initial storage for
  // vertex, line, polygon, and triangle strip arrays. It does this more
  // intelligently, examining the supplied inPolyData to determine whether to
  // allocate the verts, lines, polys, and strips arrays.  (These arrays are
  // allocated only if there is data in the corresponding arrays in the
  // inPolyData.)  Caution: if the inPolyData has no verts, and after
  // allocating with this method an PolyData::InsertNextCell() is invoked
  // where a vertex is inserted, bad things will happen.
  void Allocate(vtkPolyData *inPolyData, vtkIdType numCells=1000,
                int extSize=1000);

  // Description:
  // Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
  // VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
  // the PolyData::Allocate() function has been called first or that vertex,
  // line, polygon, and triangle strip arrays have been supplied.
  // Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
  vtkIdType InsertNextCell(int type, int npts, vtkIdType *pts);

  // Description:
  // Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
  // VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
  // the PolyData::Allocate() function has been called first or that vertex,
  // line, polygon, and triangle strip arrays have been supplied.
  // Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
  vtkIdType InsertNextCell(int type, vtkIdList *pts);

  // Description:
  // Begin inserting data all over again. Memory is not freed but otherwise
  // objects are returned to their initial state.
  void Reset();

  // Description:
  // Create data structure that allows random access of cells.
  void BuildCells();

  // Description:
  // Create upward links from points to cells that use each point. Enables
  // topologically complex queries. Normally the links array is allocated
  // based on the number of points in the vtkPolyData. The optional
  // initialSize parameter can be used to allocate a larger size initially.
  void BuildLinks(int initialSize=0);

  // Description:
  // Release data structure that allows random access of the cells. This must
  // be done before a 2nd call to BuildLinks(). DeleteCells implicitly deletes
  // the links as well since they are no longer valid.
  void DeleteCells();

  // Description:
  // Release the upward links from point to cells that use each point.
  void DeleteLinks();

  // Description:
  // Special (efficient) operations on poly data. Use carefully.
  void GetPointCells(vtkIdType ptId, unsigned short& ncells,
                     vtkIdType* &cells);

  // Description:
  // Get the neighbors at an edge. More efficient than the general
  // GetCellNeighbors(). Assumes links have been built (with BuildLinks()),
  // and looks specifically for edge neighbors.
  void GetCellEdgeNeighbors(vtkIdType cellId, vtkIdType p1, vtkIdType p2,
                            vtkIdList *cellIds);

  // Description:
  // Return a pointer to a list of point ids defining cell. (More efficient.)
  // Assumes that cells have been built (with BuildCells()).
  void GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType* &pts);

  // Description:
  // Given three vertices, determine whether it's a triangle. Make sure
  // BuildLinks() has been called first.
  int IsTriangle(int v1, int v2, int v3);

  // Description:
  // Determine whether two points form an edge. If they do, return non-zero.
  // By definition PolyVertex and PolyLine have no edges since 1-dimensional
  // edges are only found on cells 2D and higher.
  // Edges are defined as 1-D boundary entities to cells.
  // Make sure BuildLinks() has been called first.
  int IsEdge(vtkIdType p1, vtkIdType p2);

  // Description:
  // Determine whether a point is used by a particular cell. If it is, return
  // non-zero. Make sure BuildCells() has been called first.
  int IsPointUsedByCell(vtkIdType ptId, vtkIdType cellId);

  // Description:
  // Replace the points defining cell "cellId" with a new set of points. This
  // operator is (typically) used when links from points to cells have not been
  // built (i.e., BuildLinks() has not been executed). Use the operator
  // ReplaceLinkedCell() to replace a cell when cell structure has been built.
  void ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts);

  // Description:
  // Replace a point in the cell connectivity list with a different point.
  void ReplaceCellPoint(vtkIdType cellId, vtkIdType oldPtId,
                        vtkIdType newPtId);

  // Description:
  // Reverse the order of point ids defining the cell.
  void ReverseCell(vtkIdType cellId);

  // Description:
  // Mark a point/cell as deleted from this vtkPolyData.
  void DeletePoint(vtkIdType ptId);
  void DeleteCell(vtkIdType cellId);

  // Description:
  // The cells marked by calls to DeleteCell are stored in the Cell Array
  // VTK_EMPTY_CELL, but they still exist in the cell arrays.
  // Calling RemoveDeletedCells will traverse the cell arrays and remove/compact
  // the cell arrays as well as any cell data thus truly removing the cells
  // from the polydata object.
  void RemoveDeletedCells();

  // Description:
  // Add a point to the cell data structure (after cell pointers have been
  // built). This method adds the point and then allocates memory for the
  // links to the cells.  (To use this method, make sure points are available
  // and BuildLinks() has been invoked.) Of the two methods below, one inserts
  // a point coordinate and the other just makes room for cell links.
  vtkIdType InsertNextLinkedPoint(int numLinks);
  vtkIdType InsertNextLinkedPoint(double x[3], int numLinks);

  // Description:
  // Add a new cell to the cell data structure (after cell pointers have been
  // built). This method adds the cell and then updates the links from the
  // points to the cells. (Memory is allocated as necessary.)
  vtkIdType InsertNextLinkedCell(int type, int npts, vtkIdType *pts);

  // Description:
  // Replace one cell with another in cell structure. This operator updates the
  // connectivity list and the point's link list. It does not delete references
  // to the old cell in the point's link list. Use the operator
  // RemoveCellReference() to delete all references from points to (old) cell.
  // You may also want to consider using the operator ResizeCellList() if the
  // link list is changing size.
  void ReplaceLinkedCell(vtkIdType cellId, int npts, vtkIdType *pts);

  // Description:
  // Remove all references to cell in cell structure. This means the links from
  // the cell's points to the cell are deleted. Memory is not reclaimed. Use the
  // method ResizeCellList() to resize the link list from a point to its using
  // cells. (This operator assumes BuildLinks() has been called.)
  void RemoveCellReference(vtkIdType cellId);

  // Description:
  // Add references to cell in cell structure. This means the links from
  // the cell's points to the cell are modified. Memory is not extended. Use the
  // method ResizeCellList() to resize the link list from a point to its using
  // cells. (This operator assumes BuildLinks() has been called.)
  void AddCellReference(vtkIdType cellId);

  // Description:
  // Remove a reference to a cell in a particular point's link list. You may
  // also consider using RemoveCellReference() to remove the references from
  // all the cell's points to the cell. This operator does not reallocate
  // memory; use the operator ResizeCellList() to do this if necessary.
  void RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId);

  // Description:
  // Add a reference to a cell in a particular point's link list. (You may also
  // consider using AddCellReference() to add the references from all the
  // cell's points to the cell.) This operator does not realloc memory; use the
  // operator ResizeCellList() to do this if necessary.
  void AddReferenceToCell(vtkIdType ptId, vtkIdType cellId);

  // Description:
  // Resize the list of cells using a particular point. (This operator assumes
  // that BuildLinks() has been called.)
  void ResizeCellList(vtkIdType ptId, int size);

  // Description:
  // Restore object to initial state. Release memory back to system.
  virtual void Initialize();

  // Description:
  // Get the piece and the number of pieces. Similar to extent in 3D.
  virtual int GetPiece();
  virtual int GetNumberOfPieces();

  // Description:
  // Get the ghost level.
  virtual int GetGhostLevel();

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();

  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);
  void DeepCopy(vtkDataObject *src);

  // Description:
  // This method will remove any cell that has a ghost level array value
  // greater or equal to level.  It does not remove unused points (yet).
  void RemoveGhostCells(int level);

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkPolyData* GetData(vtkInformation* info);
  static vtkPolyData* GetData(vtkInformationVector* v, int i=0);
  //ETX

//BTX
  // Description:
  // Scalar field critical point classification (for manifold 2D meshes).
  // Reference: J. Milnor "Morse Theory", Princeton University Press, 1963.
  //
  // Given a pointId and an attribute representing a scalar field, this member
  // returns the index of the critical point:
  // vtkPolyData::MINIMUM (index 0): local minimum;
  // vtkPolyData::SADDLE  (index 1): local saddle;
  // vtkPolyData::MAXIMUM (index 2): local maximum.
  //
  // Other returned values are:
  // vtkPolyData::REGULAR_POINT: regular point (the gradient does not vanish);
  // vtkPolyData::ERR_NON_MANIFOLD_STAR: the star of the considered vertex is
  // not manifold (could not evaluate the index)
  // vtkPolyData::ERR_INCORRECT_FIELD: the number of entries in the scalar field
  // array is different form the number of vertices in the mesh.
  // vtkPolyData::ERR_NO_SUCH_FIELD: the specified scalar field does not exist.
  enum
    {
    ERR_NO_SUCH_FIELD = -4,
    ERR_INCORRECT_FIELD = -3,
    ERR_NON_MANIFOLD_STAR = -2,
    REGULAR_POINT = -1,
    MINIMUM = 0,
    SADDLE = 1,
    MAXIMUM = 2
    };
//ETX
  int GetScalarFieldCriticalIndex (vtkIdType pointId,
                                   vtkDataArray *scalarField);
  int GetScalarFieldCriticalIndex (vtkIdType pointId, int fieldId);
  int GetScalarFieldCriticalIndex (vtkIdType pointId, const char* fieldName);

protected:
  vtkPolyData();
  ~vtkPolyData();

  // constant cell objects returned by GetCell called.
  vtkVertex *Vertex;
  vtkPolyVertex *PolyVertex;
  vtkLine *Line;
  vtkPolyLine *PolyLine;
  vtkTriangle *Triangle;
  vtkQuad *Quad;
  vtkPolygon *Polygon;
  vtkTriangleStrip *TriangleStrip;
  vtkEmptyCell *EmptyCell;

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vtkCellArray *Verts;
  vtkCellArray *Lines;
  vtkCellArray *Polys;
  vtkCellArray *Strips;

  // dummy static member below used as a trick to simplify traversal
  static vtkPolyDataDummyContainter DummyContainer;

  // supporting structures for more complex topological operations
  // built only when necessary
  vtkCellTypes *Cells;
  vtkCellLinks *Links;

private:
  // Hide these from the user and the compiler.

  // Description:
  // For legacy compatibility. Do not use.
  void GetCellNeighbors(vtkIdType cellId, vtkIdList& ptIds, vtkIdList& cellIds)
    {this->GetCellNeighbors(cellId, &ptIds, &cellIds);}

  void Cleanup();

private:
  vtkPolyData(const vtkPolyData&);  // Not implemented.
  void operator=(const vtkPolyData&);  // Not implemented.
};

inline void vtkPolyData::GetPointCells(vtkIdType ptId, unsigned short& ncells,
                                       vtkIdType* &cells)
{
  ncells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);
}

inline int vtkPolyData::IsTriangle(int v1, int v2, int v3)
{
  unsigned short int n1;
  int i, j, tVerts[3];
  vtkIdType *cells, *tVerts2, n2;

  tVerts[0] = v1;
  tVerts[1] = v2;
  tVerts[2] = v3;

  for (i=0; i<3; i++)
    {
    this->GetPointCells(tVerts[i], n1, cells);
    for (j=0; j<n1; j++)
      {
      this->GetCellPoints(cells[j], n2, tVerts2);
      if ( (tVerts[0] == tVerts2[0] || tVerts[0] == tVerts2[1] ||
            tVerts[0] == tVerts2[2]) &&
           (tVerts[1] == tVerts2[0] || tVerts[1] == tVerts2[1] ||
            tVerts[1] == tVerts2[2]) &&
           (tVerts[2] == tVerts2[0] || tVerts[2] == tVerts2[1] ||
            tVerts[2] == tVerts2[2]) )
        {
        return 1;
        }
      }
    }
  return 0;
}

inline int vtkPolyData::IsPointUsedByCell(vtkIdType ptId, vtkIdType cellId)
{
  vtkIdType *pts, npts;

  this->GetCellPoints(cellId, npts, pts);
  for (vtkIdType i=0; i < npts; i++)
    {
    if ( pts[i] == ptId )
      {
      return 1;
      }
    }

  return 0;
}

inline void vtkPolyData::DeletePoint(vtkIdType ptId)
{
  this->Links->DeletePoint(ptId);
}

inline void vtkPolyData::DeleteCell(vtkIdType cellId)
{
  this->Cells->DeleteCell(cellId);
}

inline void vtkPolyData::RemoveCellReference(vtkIdType cellId)
{
  vtkIdType *pts, npts;

  this->GetCellPoints(cellId, npts, pts);
  for (vtkIdType i=0; i<npts; i++)
    {
    this->Links->RemoveCellReference(cellId, pts[i]);
    }
}

inline void vtkPolyData::AddCellReference(vtkIdType cellId)
{
  vtkIdType *pts, npts;

  this->GetCellPoints(cellId, npts, pts);
  for (vtkIdType i=0; i<npts; i++)
    {
    this->Links->AddCellReference(cellId, pts[i]);
    }
}

inline void vtkPolyData::ResizeCellList(vtkIdType ptId, int size)
{
  this->Links->ResizeCellList(ptId,size);
}

inline void vtkPolyData::ReplaceCellPoint(vtkIdType cellId, vtkIdType oldPtId,
                                          vtkIdType newPtId)
{
  int i;
  vtkIdType *verts, nverts;

  this->GetCellPoints(cellId,nverts,verts);
  for ( i=0; i < nverts; i++ )
    {
    if ( verts[i] == oldPtId )
      {
      verts[i] = newPtId; // this is very nasty! direct write!
      return;
      }
    }
}

#endif
