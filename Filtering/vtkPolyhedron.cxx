/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPolyhedron.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyhedron.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkPolygon.h"
#include "vtkLine.h"


vtkCxxRevisionMacro(vtkPolyhedron, "$Revision: 1.7 $");
vtkStandardNewMacro(vtkPolyhedron);

//----------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkPolyhedron::vtkPolyhedron()
{
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
  this->Polygon = vtkPolygon::New();
  this->Tetra = vtkTetra::New();
  this->Faces = vtkCellArray::New();
}

//----------------------------------------------------------------------------
vtkPolyhedron::~vtkPolyhedron()
{
  this->Line->Delete();
  this->Triangle->Delete();
  this->Polygon->Delete();
  this->Tetra->Delete();
  this->Faces->Delete();
}

//----------------------------------------------------------------------------
// Should be called by GetCell() prior to any other method invocation
void vtkPolyhedron::Initialize()
{
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfEdges()
{
  return 0;
}

//----------------------------------------------------------------------------
vtkCell *vtkPolyhedron::GetEdge(int edgeId)
{
  return this->Line;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfFaces()
{
  return 0;
}

//----------------------------------------------------------------------------
vtkCell *vtkPolyhedron::GetFace(int faceId)
{
  return this->Polygon;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                                   vtkPoints *pts)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::Contour(double value,
                                vtkDataArray *cellScalars,
                                vtkIncrementalPointLocator *locator,
                                vtkCellArray *verts, vtkCellArray *lines,
                                vtkCellArray *polys,
                                vtkPointData *inPd, vtkPointData *outPd,
                                vtkCellData *inCd, vtkIdType cellId,
                                vtkCellData *outCd)
{
}


//----------------------------------------------------------------------------
void vtkPolyhedron::Clip(double value,
                             vtkDataArray *cellScalars,
                             vtkIncrementalPointLocator *locator, vtkCellArray *tets,
                             vtkPointData *inPD, vtkPointData *outPD,
                             vtkCellData *inCD, vtkIdType cellId,
                             vtkCellData *outCD, int insideOut)
{
}

//----------------------------------------------------------------------------
int vtkPolyhedron::CellBoundary(int subId, double pcoords[3],
                                    vtkIdList *pts)
{ 
  return 0;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::EvaluatePosition( double x[3],
                                         double * vtkNotUsed(closestPoint),
                                         int & subId, double pcoords[3],
                                         double & minDist2, double * weights )
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::EvaluateLocation( int &  subId, double   pcoords[3], 
                                          double x[3],  double * weights )
{
}

//----------------------------------------------------------------------------
int vtkPolyhedron::IntersectWithLine(double p1[3], double p2[3], double tol, 
                                         double& minT, double x[3], 
                                         double pcoords[3], int& subId)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::Derivatives(int subId, double pcoords[3],
                                    double *values, int dim, double *derivs)
{
}

//----------------------------------------------------------------------------
double *vtkPolyhedron::GetParametricCoords()
{
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateFunctions(double pcoords[3], double *sf)
{
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateDerivs(double pcoords[3], double *derivs)
{
}

//----------------------------------------------------------------------------
// Specify the faces for this cell.
void vtkPolyhedron::SetFaces(vtkIdType *faces)
{
}

//----------------------------------------------------------------------------
// Return the list of faces for this cell.
vtkIdType *vtkPolyhedron::GetFaces()
{
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Polygon:\n";
  this->Polygon->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Tetra:\n";
  this->Tetra->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Faces:\n";
  this->Faces->PrintSelf(os,indent.GetNextIndent());

}
