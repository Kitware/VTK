/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkStructuredGrid - topologically regular array of data
// .SECTION Description
// vtkStructuredGrid is a data object that is a concrete implementation of
// vtkDataSet. vtkStructuredGrid represents a geometric structure that is a
// topologically regular array of points. The topology is that of a cube that
// has been subdivided into a regular array of smaller cubes. Each point/cell
// can be addressed with i-j-k indices. Examples include finite difference 
// grids.

#ifndef __vtkStructuredGrid_h
#define __vtkStructuredGrid_h

#include "vtkPointSet.hh"
#include "vtkStructuredData.hh"

class vtkStructuredGrid : public vtkPointSet, public vtkStructuredData {
public:
  vtkStructuredGrid();
  vtkStructuredGrid(const vtkStructuredGrid& sg);
  ~vtkStructuredGrid();
  char *GetClassName() {return "vtkStructuredGrid";};
  char *GetDataType() {return "vtkStructuredGrid";};
  void PrintSelf(ostream& os, vtkIndent indent);
 
  unsigned long GetMtime();

  // dataset interface
  vtkDataSet *MakeObject() {return new vtkStructuredGrid(*this);};
  int GetNumberOfPoints() {return vtkPointSet::GetNumberOfPoints();};
  vtkCell *GetCell(int cellId);
  int GetCellType(int cellId);
  float *GetPoint(int ptId);
  void GetPoint(int ptId, float p[3]);
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pcoords[3],float weights[MAX_CELL_SIZE]);
  int GetNumberOfCells();
  void GetCellPoints(int cellId, vtkIdList& ptIds);
  void GetPointCells(int ptId, vtkIdList& cellIds);

  void Initialize();

protected:
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  // blanking information inherited
};

inline float *vtkStructuredGrid::GetPoint(int ptId) 
{
  return this->vtkPointSet::GetPoint(ptId);
}

inline void vtkStructuredGrid::GetPoint(int ptId, float p[3]) 
{
  this->vtkPointSet::GetPoint(ptId,p);
}

inline int vtkStructuredGrid::GetNumberOfCells() 
{
  return this->vtkStructuredData::_GetNumberOfCells();
}

inline void vtkStructuredGrid::GetCellPoints(int cellId, vtkIdList& ptIds) 
{
  this->vtkStructuredData::_GetCellPoints(cellId,ptIds);
}

inline void vtkStructuredGrid::GetPointCells(int ptId, vtkIdList& cellIds) 
{
  this->vtkStructuredData::_GetPointCells(ptId,cellIds);
}

inline int vtkStructuredGrid::FindCell(float x[3], vtkCell *cell, float tol2, 
                                      int& subId, float pcoords[3],
                                      float weights[MAX_CELL_SIZE])
{
  return this->vtkPointSet::FindCell(x,cell,tol2,subId,pcoords,weights);
}

#endif




