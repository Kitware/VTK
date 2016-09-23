/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGrid
 * @brief   dataset represents arbitrary combinations of
 * all possible cell types
 *
 * vtkUnstructuredGrid is a data object that is a concrete implementation of
 * vtkDataSet. vtkUnstructuredGrid represents any combinations of any cell
 * types. This includes 0D (e.g., points), 1D (e.g., lines, polylines), 2D
 * (e.g., triangles, polygons), and 3D (e.g., hexahedron, tetrahedron,
 * polyhedron, etc.).
*/

#ifndef vtkUnstructuredGrid_h
#define vtkUnstructuredGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUnstructuredGridBase.h"

class vtkCellArray;
class vtkCellLinks;
class vtkConvexPointSet;
class vtkEmptyCell;
class vtkHexahedron;
class vtkIdList;
class vtkIdTypeArray;
class vtkLine;
class vtkPixel;
class vtkPolyLine;
class vtkPolyVertex;
class vtkPolygon;
class vtkPyramid;
class vtkPentagonalPrism;
class vtkHexagonalPrism;
class vtkQuad;
class vtkQuadraticEdge;
class vtkQuadraticHexahedron;
class vtkQuadraticWedge;
class vtkQuadraticPolygon;
class vtkQuadraticPyramid;
class vtkQuadraticQuad;
class vtkQuadraticTetra;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkTriangle;
class vtkTriangleStrip;
class vtkUnsignedCharArray;
class vtkVertex;
class vtkVoxel;
class vtkWedge;
class vtkTriQuadraticHexahedron;
class vtkQuadraticLinearWedge;
class vtkQuadraticLinearQuad;
class vtkBiQuadraticQuad;
class vtkBiQuadraticQuadraticWedge;
class vtkBiQuadraticQuadraticHexahedron;
class vtkBiQuadraticTriangle;
class vtkCubicLine;
class vtkPolyhedron;
class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGrid :
    public vtkUnstructuredGridBase
{
public:
  static vtkUnstructuredGrid *New();

  vtkTypeMacro(vtkUnstructuredGrid, vtkUnstructuredGridBase)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_UNSTRUCTURED_GRID;};

  /**
   * Method allocates initial storage for the cell connectivity. Use this
   * method before the method InsertNextCell(). The array capacity is
   * doubled when the inserting a cell exceeds the current capacity.
   * extSize is no longer used.
   */
  void Allocate(vtkIdType numCells=1000, int extSize=1000) VTK_OVERRIDE;

  /**
   * Insert/create cell in object by type and list of point ids defining
   * cell topology. Most cells require just a type which implicitly defines
   * a set of points and their ordering. For non-polyhedron cell type, npts
   * is the number of unique points in the cell. pts are the list of global
   * point Ids. For polyhedron cell, a special input format is required.
   * npts is the number of faces in the cell. ptIds is the list of face stream:
   * (numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
   * Make sure you have called Allocate() before calling this method
   */
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds) VTK_OVERRIDE;

  /**
   * Insert/create cell in object by a list of point ids defining
   * cell topology. Most cells require just a type which implicitly defines
   * a set of points and their ordering. For non-polyhedron cell type, ptIds
   * is the list of global Ids of unique cell points. For polyhedron cell,
   * a special ptIds input format is required:
   * (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
   * Make sure you have called Allocate() before calling this method
   */
  vtkIdType InsertNextCell(int type, vtkIdList *ptIds) VTK_OVERRIDE;

  // Desciption:
  // Insert/create a polyhedron cell. npts is the number of unique points in
  // the cell. pts is the list of the unique cell point Ids. nfaces is the
  // number of faces in the cell. faces is the face-stream
  // [numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...].
  // All point Ids are global.
  // Make sure you have called Allocate() before calling this method
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds,
                           vtkIdType nfaces, vtkIdType *faces) VTK_OVERRIDE;

  //@{
  /**
   * Standard vtkDataSet methods; see vtkDataSet.h for documentation.
   */
  void Reset();
  void CopyStructure(vtkDataSet *ds) VTK_OVERRIDE;
  vtkIdType GetNumberOfCells() VTK_OVERRIDE;
  vtkCell *GetCell(vtkIdType cellId) VTK_OVERRIDE;
  void GetCell(vtkIdType cellId, vtkGenericCell *cell) VTK_OVERRIDE;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) VTK_OVERRIDE;
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds) VTK_OVERRIDE;
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds) VTK_OVERRIDE;
  vtkCellIterator* NewCellIterator() VTK_OVERRIDE;
  //@}

  int GetCellType(vtkIdType cellId) VTK_OVERRIDE;
  vtkUnsignedCharArray* GetCellTypesArray() { return this->Types; }
  vtkIdTypeArray* GetCellLocationsArray() { return this->Locations; }
  void Squeeze() VTK_OVERRIDE;
  void Initialize() VTK_OVERRIDE;
  int GetMaxCellSize() VTK_OVERRIDE;
  void BuildLinks();
  vtkCellLinks *GetCellLinks() {return this->Links;};
  virtual void GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                             vtkIdType* &pts);

  /**
   * Get the face stream of a polyhedron cell in the following format:
   * (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...).
   * If the requested cell is not a polyhedron, then the standard GetCellPoints
   * is called to return a list of unique point ids (id1, id2, id3, ...).
   */
  void GetFaceStream(vtkIdType cellId, vtkIdList *ptIds);

  /**
   * Get the number of faces and the face stream of a polyhedral cell.
   * The output \a ptIds has the following format:
   * (numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...).
   * If the requested cell is not a polyhedron, then the standard GetCellPoints
   * is called to return the number of points and a list of unique point ids
   * (id1, id2, id3, ...).
   */
  void GetFaceStream(vtkIdType cellId, vtkIdType& nfaces, vtkIdType* &ptIds);

  //@{
  /**
   * Special methods specific to vtkUnstructuredGrid for defining the cells
   * composing the dataset. Most cells require just arrays of cellTypes,
   * cellLocations and cellConnectivities which implicitly define the set of
   * points in each cell and their ordering. In those cases the
   * cellConnectivities are of the format
   * (numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3...). However, some
   * cells like vtkPolyhedron require points plus a list of faces. To handle
   * vtkPolyhedron, SetCells() support a special input cellConnectivities format
   * (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
   * The functions use vtkPolyhedron::DecomposeAPolyhedronCell() to convert
   * polyhedron cells into standard format.
   */
  void SetCells(int type, vtkCellArray *cells);
  void SetCells(int *types, vtkCellArray *cells);
  void SetCells(vtkUnsignedCharArray *cellTypes, vtkIdTypeArray *cellLocations,
                vtkCellArray *cells);
  void SetCells(vtkUnsignedCharArray *cellTypes, vtkIdTypeArray *cellLocations,
                vtkCellArray *cells, vtkIdTypeArray *faceLocations,
                vtkIdTypeArray *faces);
  //@}

  vtkCellArray *GetCells() {return this->Connectivity;};
  void ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts) VTK_OVERRIDE;
  vtkIdType InsertNextLinkedCell(int type, int npts, vtkIdType *pts);
  void RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId);
  void AddReferenceToCell(vtkIdType ptId, vtkIdType cellId);
  void ResizeCellList(vtkIdType ptId, int size);

  /**
   * Topological inquiry to get all cells using list of points exclusive of
   * cell specified (e.g., cellId).
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                        vtkIdList *cellIds) VTK_OVERRIDE;

  //@{
  /**
   * Set / Get the piece and the number of pieces. Similar to extent in 3D.
   */
  virtual int GetPiece();
  virtual int GetNumberOfPieces();
  //@}

  /**
   * Get the ghost level.
   */
  virtual int GetGhostLevel();

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() VTK_OVERRIDE;

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject *src) VTK_OVERRIDE;
  void DeepCopy(vtkDataObject *src) VTK_OVERRIDE;
  //@}

  /**
   * Fill vtkIdTypeArray container with list of cell Ids.  This
   * method traverses all cells and, for a particular cell type,
   * inserts the cell Id into the container.
   */
  void GetIdsOfCellsOfType(int type, vtkIdTypeArray *array) VTK_OVERRIDE;

  /**
   * Traverse cells and determine if cells are all of the same type.
   */
  int IsHomogeneous() VTK_OVERRIDE;

  /**
   * This method will remove any cell that is marked as ghost
   * (has the vtkDataSetAttributes::DUPLICATECELL bit set).
   */
  void RemoveGhostCells();

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkUnstructuredGrid* GetData(vtkInformation* info);
  static vtkUnstructuredGrid* GetData(vtkInformationVector* v, int i=0);
  //@}

  /**
   * Special support for polyhedron. Return NULL for all other cell types.
   */
  vtkIdType      *GetFaces(vtkIdType cellId);

  //@{
  /**
   * Get pointer to faces and facelocations. Support for polyhedron cells.
   */
  vtkIdTypeArray* GetFaces(){return this->Faces;};
  vtkIdTypeArray* GetFaceLocations(){return this->FaceLocations;};
  //@}

  /**
   * Special function used by vtkUnstructuredGridReader.
   * By default vtkUnstructuredGrid does not contain face information, which is
   * only used by polyhedron cells. If so far no polyhedron cells have been
   * added, Faces and FaceLocations pointers will be NULL. In this case, need to
   * initialize the arrays and assign values to the previous non-polyhedron cells.
   */
  int InitializeFacesRepresentation(vtkIdType numPrevCells);

  /**
   * A static method for converting a polyhedron vtkCellArray of format
   * [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * into three components: (1) an integer indicating the number of faces
   * (2) a standard vtkCellArray storing point ids [nCell0Pts, i, j, k]
   * and (3) an vtkIdTypeArray storing face connectivity in format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * Note: input is assumed to contain only one polyhedron cell.
   * Outputs (2) and (3) will be stacked at the end of the input
   * cellArray and faces. The original data in the input will not
   * be touched.
   */
  static void DecomposeAPolyhedronCell(vtkCellArray *polyhedronCellArray,
                                       vtkIdType & nCellpts,
                                       vtkIdType & nCellfaces,
                                       vtkCellArray *cellArray,
                                       vtkIdTypeArray *faces);

  static void DecomposeAPolyhedronCell(vtkIdType * polyhedronCellStream,
                                       vtkIdType & nCellpts,
                                       vtkIdType & nCellfaces,
                                       vtkCellArray *cellArray,
                                       vtkIdTypeArray *faces);

  /**
   * A static method for converting an input polyhedron cell stream of format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * into three components: (1) an integer indicating the number of faces
   * (2) a standard vtkCellArray storing point ids [nCell0Pts, i, j, k]
   * and (3) an vtkIdTypeArray storing face connectivity in format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * Note: input is assumed to contain only one polyhedron cell.
   * Outputs (2) and (3) will be stacked at the end of the input
   * cellArray and faces. The original data in the input will not
   * be touched.
   */
  static void DecomposeAPolyhedronCell(vtkIdType nCellFaces,
                                       vtkIdType * inFaceStream,
                                       vtkIdType & nCellpts,
                                       vtkCellArray * cellArray,
                                       vtkIdTypeArray * faces);

  /**
   * Convert pid in a face stream into idMap[pid]. The face stream is of format
   * [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]. The user is
   * responsible to make sure all the Ids in faceStream do not exceed the
   * range of idMap.
   */
  static void ConvertFaceStreamPointIds(vtkIdList * faceStream,
                                        vtkIdType * idMap);

  /**
   * Convert pid in a face stream into idMap[pid]. The face stream is of format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]. The user is responsible to
   * make sure all the Ids in faceStream do not exceed the range of idMap.
   */
  static void ConvertFaceStreamPointIds(vtkIdType nfaces,
                                        vtkIdType * faceStream,
                                        vtkIdType * idMap);


protected:
  vtkUnstructuredGrid();
  ~vtkUnstructuredGrid() VTK_OVERRIDE;

  // used by GetCell method
  vtkVertex                         *Vertex;
  vtkPolyVertex                     *PolyVertex;
  vtkLine                           *Line;
  vtkPolyLine                       *PolyLine;
  vtkTriangle                       *Triangle;
  vtkTriangleStrip                  *TriangleStrip;
  vtkPixel                          *Pixel;
  vtkQuad                           *Quad;
  vtkPolygon                        *Polygon;
  vtkTetra                          *Tetra;
  vtkVoxel                          *Voxel;
  vtkHexahedron                     *Hexahedron;
  vtkWedge                          *Wedge;
  vtkPyramid                        *Pyramid;
  vtkPentagonalPrism                *PentagonalPrism;
  vtkHexagonalPrism                 *HexagonalPrism;
  vtkQuadraticEdge                  *QuadraticEdge;
  vtkQuadraticTriangle              *QuadraticTriangle;
  vtkQuadraticQuad                  *QuadraticQuad;
  vtkQuadraticPolygon               *QuadraticPolygon;
  vtkQuadraticTetra                 *QuadraticTetra;
  vtkQuadraticHexahedron            *QuadraticHexahedron;
  vtkQuadraticWedge                 *QuadraticWedge;
  vtkQuadraticPyramid               *QuadraticPyramid;
  vtkQuadraticLinearQuad            *QuadraticLinearQuad;
  vtkBiQuadraticQuad                *BiQuadraticQuad;
  vtkTriQuadraticHexahedron         *TriQuadraticHexahedron;
  vtkQuadraticLinearWedge           *QuadraticLinearWedge;
  vtkBiQuadraticQuadraticWedge      *BiQuadraticQuadraticWedge;
  vtkBiQuadraticQuadraticHexahedron *BiQuadraticQuadraticHexahedron;
  vtkBiQuadraticTriangle            *BiQuadraticTriangle;
  vtkCubicLine                      *CubicLine;
  vtkConvexPointSet                 *ConvexPointSet;
  vtkPolyhedron                     *Polyhedron;
  vtkEmptyCell                      *EmptyCell;

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vtkCellArray *Connectivity;
  vtkCellLinks *Links;
  vtkUnsignedCharArray *Types;
  vtkIdTypeArray *Locations;

  // Special support for polyhedra/cells with explicit face representations.
  // The Faces class represents polygonal faces using a modified vtkCellArray
  // structure. Each cell face list begins with the total number of faces in
  // the cell, followed by a vtkCellArray data organization
  // (n,i,j,k,n,i,j,k,...).
  vtkIdTypeArray *Faces;
  vtkIdTypeArray *FaceLocations;

private:
  // Hide these from the user and the compiler.
  vtkUnstructuredGrid(const vtkUnstructuredGrid&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGrid&) VTK_DELETE_FUNCTION;

  void Cleanup();
};

#endif
