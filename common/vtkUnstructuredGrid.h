/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
#include "vtkCellTypes.h"
#include "vtkCellLinks.h"
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

class VTK_EXPORT vtkUnstructuredGrid : public vtkPointSet {
public:
  static vtkUnstructuredGrid *New();

  const char *GetClassName() {return "vtkUnstructuredGrid";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  int GetDataObjectType() {return VTK_UNSTRUCTURED_GRID;};
  void Allocate(int numCells=1000, int extSize=1000);
  int InsertNextCell(int type, int npts, int *pts);
  int InsertNextCell(int type, vtkIdList *ptIds);
  void Reset();
  void SetCells(int *types, vtkCellArray *cells);
  vtkCellArray *GetCells() {return this->Connectivity;};
  vtkDataObject *MakeObject() {return new vtkUnstructuredGrid;};
  void CopyStructure(vtkDataSet *ds);
  int GetNumberOfCells();
  vtkCell *GetCell(int cellId);
  void GetCell(int cellId, vtkGenericCell *cell);
  void GetCellBounds(int cellId, float bounds[6]);
  void GetCellPoints(int cellId, vtkIdList *ptIds);
  void GetPointCells(int ptId, vtkIdList *cellIds);

  int GetCellType(int cellId);
  void Squeeze();
  void Initialize();
  int GetMaxCellSize();
  void BuildLinks();
  vtkCellLinks *GetCellLinks() {return this->Links;};
  void GetCellPoints(int cellId, int& npts, int* &pts);
  void ReplaceCell(int cellId, int npts, int *pts);
  int InsertNextLinkedCell(int type, int npts, int *pts); 
  void RemoveReferenceToCell(int ptId, int cellId);
  void AddReferenceToCell(int ptId, int cellId);
  void ResizeCellList(int ptId, int size);

  // Description:
  // For legacy compatibility. Do not use.
  void GetCellPoints(int cellId, vtkIdList &ptIds)
    {this->GetCellPoints(cellId, &ptIds);}
  void GetPointCells(int ptId, vtkIdList &cellIds)
    {this->GetPointCells(ptId, &cellIds);}
  int InsertNextCell(int type, vtkIdList &pts) {return this->InsertNextCell(type, &pts);}
  
  // Description:
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId).
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellNeighbors(int cellId, vtkIdList *ptIds, 
				vtkIdList *cellIds);

  // Description:
  // For streaming.  User/next filter specifies which piece the want updated.
  // The source of this poly data has to return exactly this piece.
  void SetUpdateExtent(int piece, int numPieces);
  void GetUpdateExtent(int &piece, int &numPieces);

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
  // Set the maximum number of pieces. Similar to WholeExtent in 3D.
  vtkSetMacro( MaximumNumberOfPieces, int );

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();
  
protected:
  vtkUnstructuredGrid();
  ~vtkUnstructuredGrid();
  vtkUnstructuredGrid(const vtkUnstructuredGrid& ug);
  void operator=(const vtkUnstructuredGrid&) {};

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
  vtkCellTypes *Cells;
  vtkCellArray *Connectivity;
  vtkCellLinks *Links;

 private:
  // Hide these from the user and the compiler.
  
  // Description:
  // For legacy compatibility. Do not use.
  void GetCellNeighbors(int cellId, vtkIdList& ptIds, vtkIdList& cellIds)
    {this->GetCellNeighbors(cellId, &ptIds, &cellIds);}
};

#endif







