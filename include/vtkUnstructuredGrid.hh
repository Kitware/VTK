/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkPointSet.hh"
#include "vtkIdList.hh"
#include "vtkCellArray.hh"
#include "vtkCellList.hh"
#include "vtkLinkList.hh"

class vtkUnstructuredGrid : public vtkPointSet {
public:
  vtkUnstructuredGrid();
  vtkUnstructuredGrid(const vtkUnstructuredGrid& up);
  ~vtkUnstructuredGrid();
  char *GetClassName() {return "vtkUnstructuredGrid";};
  char *GetDataType() {return "vtkUnstructuredGrid";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // cell creation/manipulation methods
  void Allocate(int numCells=1000, int extSize=1000);
  int InsertNextCell(int type, int npts, int *pts);
  int InsertNextCell(int type, vtkIdList& ptIds);
  void Reset();
  void SetCells(int *types, vtkCellArray *cells);
  vtkCellArray *GetCells() {return this->Connectivity;};

  // dataset interface
  vtkDataSet *MakeObject() {return new vtkUnstructuredGrid(*this);};
  void CopyStructure(vtkDataSet *ds);
  int GetNumberOfCells();
  vtkCell *GetCell(int cellId);
  void GetCellPoints(int cellId, vtkIdList& ptIds);
  void GetPointCells(int ptId, vtkIdList& cellIds);
  int GetCellType(int cellId);
  void Squeeze();
  void Initialize();
  int GetMaxCellSize();

  // special cell structure methods
  void BuildLinks();
  void GetCellPoints(int cellId, int& npts, int* &pts);
  void ReplaceCell(int cellId, int npts, int *pts);
  int InsertNextLinkedCell(int type, int npts, int *pts); 
  void RemoveReferenceToCell(int ptId, int cellId);
  void AddReferenceToCell(int ptId, int cellId);
  void ResizeCellList(int ptId, int size);

protected:

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vtkCellList *Cells;
  vtkCellArray *Connectivity;
  vtkLinkList *Links;
};

#endif
