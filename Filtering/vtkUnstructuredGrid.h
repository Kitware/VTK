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
// .NAME vtkUnstructuredGrid - dataset represents arbitrary combinations of all possible cell types
// .SECTION Description
// vtkUnstructuredGrid is a data object that is a concrete implementation 
// of vtkDataSet. vtkUnstructuredGrid represents any combinations of any cell
// types. This includes 0D (e.g., points), 1D (e.g., lines, polylines), 2D 
// (e.g., triangles, polygons), and 3D (e.g., hexahedron, tetrahedron).

#ifndef __vtkUnstructuredGrid_h
#define __vtkUnstructuredGrid_h

#include "vtkPointSet.h"

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


class VTK_FILTERING_EXPORT vtkUnstructuredGrid : public vtkPointSet 
{
public:
  static vtkUnstructuredGrid *New();

  vtkTypeMacro(vtkUnstructuredGrid,vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  int GetDataObjectType() {return VTK_UNSTRUCTURED_GRID;};
  virtual void Allocate(vtkIdType numCells=1000, int extSize=1000);
  
  // Description:
  // Insert/create cell in object by type and list of point ids defining
  // cell topology.
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *pts);
  vtkIdType InsertNextCell(int type, vtkIdList *ptIds);
  
  void Reset();
  virtual void CopyStructure(vtkDataSet *ds);
  vtkIdType GetNumberOfCells();
  virtual vtkCell *GetCell(vtkIdType cellId);
  virtual void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  virtual void GetCellBounds(vtkIdType cellId, double bounds[6]);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);

  int GetCellType(vtkIdType cellId);
  vtkUnsignedCharArray* GetCellTypesArray() { return this->Types; }
  vtkIdTypeArray* GetCellLocationsArray() { return this->Locations; }
  void Squeeze();
  void Initialize();
  int GetMaxCellSize();
  void BuildLinks();
  vtkCellLinks *GetCellLinks() {return this->Links;};
  virtual void GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                             vtkIdType* &pts);

  // Description:
  // Special methods specific to vtkUnstructuredGrid for defining the cells
  // composing the dataset.
  void SetCells(int type, vtkCellArray *cells);
  void SetCells(int *types, vtkCellArray *cells);
  void SetCells(vtkUnsignedCharArray *cellTypes, vtkIdTypeArray *cellLocations, 
                vtkCellArray *cells);
  vtkCellArray *GetCells() {return this->Connectivity;};
  void ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts);
  int InsertNextLinkedCell(int type, int npts, vtkIdType *pts);
  void RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId);
  void AddReferenceToCell(vtkIdType ptId, vtkIdType cellId);
  void ResizeCellList(vtkIdType ptId, int size);

  // Description:
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId).
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds, 
                                vtkIdList *cellIds);

  // Description:
  // For streaming.  User/next filter specifies which piece the want updated.
  // The source of this poly data has to return exactly this piece.
  void GetUpdateExtent(int &piece, int &numPieces, int &ghostLevel);

  // Description:
  // We need this here to avoid hiding superclass method
  virtual int* GetUpdateExtent();
  virtual void GetUpdateExtent(int& x0, int& x1, int& y0, int& y1,
                               int& z0, int& z1);
  virtual void GetUpdateExtent(int extent[6]);

  // Description:
  // Set / Get the piece and the number of pieces. Similar to extent in 3D.
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
  virtual void ShallowCopy(vtkDataObject *src);  
  virtual void DeepCopy(vtkDataObject *src);

  // Description:
  // Fill vtkIdTypeArray container with list of cell Ids.  This
  // method traverses all cells and, for a particular cell type,
  // inserts the cell Id into the container.
  void GetIdsOfCellsOfType(int type, vtkIdTypeArray *array);

  // Description:
  // Traverse cells and determine if cells are all of the same type.
  int IsHomogeneous();

  // Description:
  // This method will remove any cell that has a ghost level array value
  // greater or equal to level.
  void RemoveGhostCells(int level);

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkUnstructuredGrid* GetData(vtkInformation* info);
  static vtkUnstructuredGrid* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  vtkUnstructuredGrid();
  ~vtkUnstructuredGrid();

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
  vtkEmptyCell                      *EmptyCell;
  
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vtkCellArray *Connectivity;
  vtkCellLinks *Links;
  vtkUnsignedCharArray *Types;
  vtkIdTypeArray *Locations;

 private:
  void Cleanup();
  
  // Hide these from the user and the compiler.
  
  // Description:
  // For legacy compatibility. Do not use.
  VTK_LEGACY(void GetCellNeighbors(vtkIdType cellId, vtkIdList& ptIds, vtkIdList& cellIds));

  vtkUnstructuredGrid(const vtkUnstructuredGrid&);  // Not implemented.
  void operator=(const vtkUnstructuredGrid&);  // Not implemented.
};

#endif







