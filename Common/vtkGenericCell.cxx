/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCell.cxx
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
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkGenericCell* vtkGenericCell::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGenericCell");
  if(ret)
    {
    return (vtkGenericCell*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGenericCell;
}




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

void vtkGenericCell::Contour(float value, vtkDataArray *cellScalars, 
                             vtkPointLocator *locator, vtkCellArray *verts, 
                             vtkCellArray *lines, vtkCellArray *polys, 
                             vtkPointData *inPd, vtkPointData *outPd,
                             vtkCellData *inCd, vtkIdType cellId,
                             vtkCellData *outCd)
{
  this->Cell->Contour(value, cellScalars, locator, verts, lines, polys,
		      inPd, outPd, inCd, cellId, outCd);
}

void vtkGenericCell::Clip(float value, vtkDataArray *cellScalars, 
                          vtkPointLocator *locator, vtkCellArray *connectivity,
                          vtkPointData *inPd, vtkPointData *outPd,
                          vtkCellData *inCd, vtkIdType cellId,
                          vtkCellData *outCd, int insideOut)
{
  this->Cell->Clip(value, cellScalars, locator, connectivity, inPd,
		   outPd, inCd, cellId, outCd, insideOut);
}

int vtkGenericCell::IntersectWithLine(float p1[3], float p2[3], float tol,
                                      float& t, float x[3], float pcoords[3],
                                      int& subId)
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
    this->Points->UnRegister(this);
    this->PointIds->UnRegister(this);
    this->PointIds = NULL;
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








