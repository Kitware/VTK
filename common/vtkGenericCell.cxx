/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCell.cxx
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
#include "vtkGenericCell.h"
#include "vtkEmptyCell.h"
#include "vtkVertex.h"
#include "vtkPolyVertex.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkQuad.h"
#include "vtkPixel.h"
#include "vtkPolygon.h"
#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkPyramid.h"

// Construct cell.
vtkGenericCell::vtkGenericCell()
{
  this->Cell = vtkEmptyCell::New();
}  

vtkGenericCell::~vtkGenericCell()
{
  this->Cell->Delete();
}

// The following methods dereference vtkCell virtual functions to allow
// vtkCell to act like a concrete object.
vtkCell *vtkGenericCell::MakeObject()
{
  return this->Cell->MakeObject();
}

void vtkGenericCell::ShallowCopy(vtkCell *c)
{
  this->Cell->ShallowCopy(c);
}

void vtkGenericCell::DeepCopy(vtkCell *c)
{
  this->Cell->DeepCopy(c);
}

int vtkGenericCell::GetCellType()
{
  return this->Cell->GetCellType();
}

int vtkGenericCell::GetCellDimension()
{
  return this->Cell->GetCellDimension();
}

int vtkGenericCell::GetInterpolationOrder()
{
  return this->Cell->GetInterpolationOrder();
}

int vtkGenericCell::GetNumberOfEdges()
{
  return this->Cell->GetNumberOfEdges();
}

int vtkGenericCell::GetNumberOfFaces()
{
  return this->Cell->GetNumberOfFaces();
}

vtkCell *vtkGenericCell::GetEdge(int edgeId)
{
  return this->Cell->GetEdge(edgeId);
}

vtkCell *vtkGenericCell::GetFace(int faceId)
{
  return this->Cell->GetFace(faceId);
}

int vtkGenericCell::CellBoundary(int subId, float pcoords[3], vtkIdList *pts)
{
  return this->Cell->CellBoundary(subId, pcoords, pts);
}

int vtkGenericCell::EvaluatePosition(float x[3], float closestPoint[3], 
				    int& subId, float pcoords[3], 
				    float& dist2, float *weights)
{
  return this->Cell->EvaluatePosition(x, closestPoint, subId, 
				      pcoords, dist2, weights);
}

void vtkGenericCell::EvaluateLocation(int& subId, float pcoords[3], 
				     float x[3], float *weights)
{
  this->Cell->EvaluateLocation(subId, pcoords, x, weights);
}

void vtkGenericCell::Contour(float value, vtkScalars *cellScalars, 
			    vtkPointLocator *locator, vtkCellArray *verts, 
			    vtkCellArray *lines, vtkCellArray *polys, 
			    vtkPointData *inPd, vtkPointData *outPd,
			    vtkCellData *inCd, int cellId, vtkCellData *outCd)
{
  this->Cell->Contour(value, cellScalars, locator, verts, lines, polys,
		      inPd, outPd, inCd, cellId, outCd);
}

void vtkGenericCell::Clip(float value, vtkScalars *cellScalars, 
			 vtkPointLocator *locator, vtkCellArray *connectivity,
			 vtkPointData *inPd, vtkPointData *outPd,
			 vtkCellData *inCd, int cellId, vtkCellData *outCd, 
			 int insideOut)
{
  this->Cell->Clip(value, cellScalars, locator, connectivity, inPd,
		   outPd, inCd, cellId, outCd, insideOut);
}

int vtkGenericCell::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
				     float x[3], float pcoords[3], int& subId)
{
  return this->Cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
}

int vtkGenericCell::Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts)
{
  return this->Cell->Triangulate(index, ptIds, pts);
}

void vtkGenericCell::Derivatives(int subId, float pcoords[3], float *values, 
				int dim, float *derivs)
{
  this->Cell->Derivatives(subId, pcoords, values, dim, derivs);
}

int vtkGenericCell::GetParametricCenter(float pcoords[3])
{
  return this->Cell->GetParametricCenter(pcoords);
}

// Set the type of dereferenced cell. Checks to see whether cell type
// has changed and creates a new cell only if necessary.
void vtkGenericCell::SetCellType(int cellType)
{
  if ( this->Cell->GetCellType() != cellType )
    {
    this->Points->Delete();
    this->PointIds->Delete();
    this->Cell->Delete();

    switch (cellType)
      {
      case VTK_EMPTY_CELL:
	this->Cell = vtkEmptyCell::New();
	break;
      case VTK_VERTEX:
	this->Cell = vtkVertex::New();
	break;
      case VTK_POLY_VERTEX:
	this->Cell = vtkPolyVertex::New();
	break;
      case VTK_LINE:
	this->Cell = vtkLine::New();
	break;
      case VTK_POLY_LINE:
	this->Cell = vtkPolyLine::New();
	break;
      case VTK_TRIANGLE:
	this->Cell = vtkTriangle::New();
	break;
      case VTK_TRIANGLE_STRIP:
	this->Cell = vtkTriangleStrip::New();
	break;
      case VTK_POLYGON:
	this->Cell = vtkPolygon::New();
	break;
      case VTK_PIXEL:
	this->Cell = vtkPixel::New();
	break;
      case VTK_QUAD:
	this->Cell = vtkQuad::New();
	break;
      case VTK_TETRA:
	this->Cell = vtkTetra::New();
	break;
      case VTK_VOXEL:
	this->Cell = vtkVoxel::New();
	break;
      case VTK_HEXAHEDRON:
	this->Cell = vtkHexahedron::New();
	break;
      case VTK_WEDGE:
	this->Cell = vtkWedge::New();
	break;
      case VTK_PYRAMID:
	this->Cell = vtkPyramid::New();
	break;
      default:
	vtkErrorMacro(<<"Unsupported cell type! Setting to vtkEmptyCell");
        this->SetCellType(VTK_EMPTY_CELL);
      }
    this->Points = this->Cell->Points;
    this->Points->Register(this);
    this->PointIds = this->Cell->PointIds;
    this->PointIds->Register(this);
    }
}








