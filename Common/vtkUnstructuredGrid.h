/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkIdList.h"
#include "vtkCellArray.h"
#include "vtkCellLinks.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"
class vtkVertex;
class vtkPolyVertex;
class vtkLine;
class vtkPolyLine;
class vtkTriangle;
class vtkTriangleStrip;
class vtkPixel;
class vtkQuad;
class vtkPolygon;
class vtkTetra;
class vtkVoxel;
class vtkHexahedron;
class vtkWedge;
class vtkPyramid;

class VTK_COMMON_EXPORT vtkUnstructuredGrid : public vtkPointSet {
private:
  vtkUnstructuredGrid(const vtkUnstructuredGrid&);  // Not implemented.
  void operator=(const vtkUnstructuredGrid&);  // Not implemented.
public:
  static vtkUnstructuredGrid *New();

  vtkTypeMacro(vtkUnstructuredGrid,vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  int GetDataObjectType() {return VTK_UNSTRUCTURED_GRID;};
  virtual void Allocate(vtkIdType numCells=1000, int extSize=1000);
  int InsertNextCell(int type, int npts, vtkIdType *pts);
  int InsertNextCell(int type, vtkIdList *ptIds);
  void Reset();
  void SetCells(int *types, vtkCellArray *cells);
  void SetCells(vtkUnsignedCharArray *cellTypes, vtkIntArray *cellLocations, 
                vtkCellArray *cells);
  vtkCellArray *GetCells() {return this->Connectivity;};
  vtkDataObject *MakeObject() {return vtkUnstructuredGrid::New();};
  virtual void CopyStructure(vtkDataSet *ds);
  vtkIdType GetNumberOfCells();
  virtual vtkCell *GetCell(vtkIdType cellId);
  virtual void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  virtual void GetCellBounds(vtkIdType cellId, float bounds[6]);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);

  int GetCellType(vtkIdType cellId);
  void Squeeze();
  void Initialize();
  int GetMaxCellSize();
  void BuildLinks();
  vtkCellLinks *GetCellLinks() {return this->Links;};
  virtual void GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                             vtkIdType* &pts);
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
  void SetUpdateExtent(int piece, int numPieces, int ghostLevel);
  void SetUpdateExtent(int piece, int numPieces)
    {this->SetUpdateExtent(piece, numPieces, 0);}
  void GetUpdateExtent(int &piece, int &numPieces, int &ghostLevel);

  // Description:
  // We need this here to keep from hiding superclass method
  vtkGetVector6Macro( UpdateExtent, int );

  // Description:
  // Call superclass method to avoid hiding
  // Since this data type does not use 3D extents, this set method
  // is useless but necessary since vtkDataSetToDataSetFilter does not
  // know what type of data it is working on.
  void SetUpdateExtent( int x1, int x2, int y1, int y2, int z1, int z2 )
    { this->vtkPointSet::SetUpdateExtent( x1, x2, y1, y2, z1, z2 ); };
  void SetUpdateExtent( int ext[6] )
    { this->vtkPointSet::SetUpdateExtent( ext ); };


  // Description:
  // Set / Get the piece and the number of pieces. Similar to extent in 3D.
  vtkGetMacro( Piece, int );
  vtkGetMacro( NumberOfPieces, int );

  // Description:
  // Set / Get the ghost level.
  vtkGetMacro( GhostLevel, int );
  
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
  // Fill vtkUnsignedCharArray container with list of unique cell types.  This
  // method traverses all cells and, for each unique cell type it encounters,
  // inserts the type into the container.
  void GetListOfUniqueCellTypes(vtkUnsignedCharArray *uniqueTypes);

  // Description:
  // Fill vtkIntArray container with list of cell Ids.  This
  // method traverses all cells and, for a particular cell type,
  // inserts the cell Id into the container.
  void GetIdsOfCellsOfType(int type, vtkIntArray *array);

  // Description:
  // Traverse cells and determine if cells are all of the same type.
  int IsHomogeneous();

protected:
  vtkUnstructuredGrid();
  ~vtkUnstructuredGrid();

  // used by GetCell method
  vtkVertex *Vertex;
  vtkPolyVertex *PolyVertex;
  vtkLine *Line;
  vtkPolyLine *PolyLine;
  vtkTriangle *Triangle;
  vtkTriangleStrip *TriangleStrip;
  vtkPixel *Pixel;
  vtkQuad *Quad;
  vtkPolygon *Polygon;
  vtkTetra *Tetra;
  vtkVoxel *Voxel;
  vtkHexahedron *Hexahedron;
  vtkWedge *Wedge;
  vtkPyramid *Pyramid;
  
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vtkCellArray *Connectivity;
  vtkCellLinks *Links;
  vtkUnsignedCharArray *Types;
  vtkIntArray *Locations;

 private:
  // Hide these from the user and the compiler.
  
  // Description:
  // For legacy compatibility. Do not use.
  void GetCellNeighbors(vtkIdType cellId, vtkIdList& ptIds, vtkIdList& cellIds)
    {this->GetCellNeighbors(cellId, &ptIds, &cellIds);}
};

#endif







