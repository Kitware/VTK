/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCell.h
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
// .NAME vtkGenericCell - provides thread-safe access to cells
// .SECTION Description
// vtkGenericCell is a class that provides access to concrete types of cells.
// It's main purpose is to allow thread-safe access to cells, supporting
// the vtkDataSet::GetCell(vtkGenericCell *) method. vtkGenericCell acts
// like any type of cell, it just dereferences an internal representation.
// .SECTION See Also
// vtkCell vtkDataSet

#ifndef __vtkGenericCell_h
#define __vtkGenericCell_h

#include "vtkCell.h"

class VTK_EXPORT vtkGenericCell : public vtkCell
{
public:
  // Description:
  // Create handle to any type of cell; by default a vtkEmptyCell.
  vtkGenericCell();
  ~vtkGenericCell();
  static vtkGenericCell *New() {return new vtkGenericCell;};
  const char *GetClassName() {return "vtkGenericCell";};

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  void ShallowCopy(vtkCell *c);
  void DeepCopy(vtkCell *c);
  int GetCellType();
  int GetCellDimension();
  int GetInterpolationOrder();
  int GetNumberOfEdges();
  int GetNumberOfFaces();
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  int EvaluatePosition(float x[3], float closestPoint[3], 
		       int& subId, float pcoords[3], 
		       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], 
                                float x[3], float *weights);
  void Contour(float value, vtkScalars *cellScalars, 
	       vtkPointLocator *locator, vtkCellArray *verts, 
	       vtkCellArray *lines, vtkCellArray *polys, 
	       vtkPointData *inPd, vtkPointData *outPd,
	       vtkCellData *inCd, int cellId, vtkCellData *outCd);
  void Clip(float value, vtkScalars *cellScalars, 
	    vtkPointLocator *locator, vtkCellArray *connectivity,
	    vtkPointData *inPd, vtkPointData *outPd,
	    vtkCellData *inCd, int cellId, vtkCellData *outCd, 
                    int insideOut);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
			float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
		   int dim, float *derivs);
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // This method is used to support the vtkDataSet::GetCell(vtkGenericCell *)
  // method. It allows vtkGenericCell to act like any cell type by
  // dereferencing an internal instance of a concrete cell type. When
  // you set the cell type, you are resetting a pointer to an internal
  // cell which is then used for computation.
  void SetCellType(int cellType);
  void SetCellTypeToEmptyCell() {this->SetCellType(VTK_EMPTY_CELL);};
  void SetCellTypeToVertex() {this->SetCellType(VTK_VERTEX);};
  void SetCellTypeToPolyVertex() {this->SetCellType(VTK_POLY_VERTEX);};
  void SetCellTypeToLine() {this->SetCellType(VTK_LINE);};
  void SetCellTypeToPolyLine() {this->SetCellType(VTK_POLY_LINE);};
  void SetCellTypeToTriangle() {this->SetCellType(VTK_TRIANGLE);};
  void SetCellTypeToTriangleStrip() {this->SetCellType(VTK_TRIANGLE_STRIP);};
  void SetCellTypeToPolygon() {this->SetCellType(VTK_POLYGON);};
  void SetCellTypeToPixel() {this->SetCellType(VTK_PIXEL);};
  void SetCellTypeToQuad() {this->SetCellType(VTK_QUAD);};
  void SetCellTypeToTetra() {this->SetCellType(VTK_TETRA);};
  void SetCellTypeToVoxel() {this->SetCellType(VTK_VOXEL);};
  void SetCellTypeToHexahedron() {this->SetCellType(VTK_HEXAHEDRON);};
  void SetCellTypeToWedge() {this->SetCellType(VTK_WEDGE);};
  void SetCellTypeToPyramid() {this->SetCellType(VTK_PYRAMID);};

protected:
  vtkCell *Cell;
};

#endif


