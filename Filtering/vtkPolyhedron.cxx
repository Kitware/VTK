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
#include "vtkIdTypeArray.h"
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

#include <vtkstd/map>

vtkCxxRevisionMacro(vtkPolyhedron, "$Revision: 1.7 $");
vtkStandardNewMacro(vtkPolyhedron);

// Special typedef for point id map
struct vtkPointIdMap : public vtkstd::map<vtkIdType,vtkIdType> {};
typedef vtkstd::map<vtkIdType,vtkIdType*>::iterator PointIdMapIterator;

//----------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkPolyhedron::vtkPolyhedron()
{
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
  this->Polygon = vtkPolygon::New();
  this->Tetra = vtkTetra::New();
  this->Faces = vtkIdTypeArray::New();
  this->FaceLocations = vtkIdTypeArray::New();
  this->PointIdMap = new vtkPointIdMap;
}

//----------------------------------------------------------------------------
vtkPolyhedron::~vtkPolyhedron()
{
  this->Line->Delete();
  this->Triangle->Delete();
  this->Polygon->Delete();
  this->Tetra->Delete();
  this->Faces->Delete();
  this->FaceLocations->Delete();
  delete this->PointIdMap;
}

//----------------------------------------------------------------------------
// Should be called by GetCell() prior to any other method invocation. Here we
// assume that the points and faces have already been loaded into the polyhedron.
void vtkPolyhedron::Initialize()
{
  // We need to create a reverse map from the point ids to their canonical cell
  // ids. This is a fancy way of saying that we have to be able to rapidly go
  // from a PointId[i] to the location i in the cell.
  vtkIdType i, id, numPointIds = this->PointIds->GetNumberOfIds();
  for (i=0; i < numPointIds; ++i)
    {
    id = this->PointIds->GetId(i);
    (*this->PointIdMap)[id] = i;
    }
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
  return this->Faces->GetValue(0);
}

//----------------------------------------------------------------------------
vtkCell *vtkPolyhedron::GetFace(int faceId)
{
  if ( !this->Faces ||
       faceId < 0 || faceId > this->Faces->GetValue(0) )
    {
    return NULL;
    }
  
  // Okay load up the polygon
  vtkIdType i, p, loc = this->FaceLocations->GetValue(faceId);
  vtkIdType *face = this->Faces->GetPointer(loc);
  
  this->Polygon->PointIds->SetNumberOfIds(face[0]);
  this->Polygon->Points->SetNumberOfPoints(face[0]);
  for (i=0; i < face[0]; ++i)
    {
    this->Polygon->PointIds->SetId(i,face[i+1]);
    p = (*this->PointIdMap)[face[i+1]];
    this->Polygon->Points->SetPoint(i,this->Points->GetPoint(p));
    }

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
  // Set up face structure
  this->Faces->Reset();
  this->FaceLocations->Reset();
  this->PointIdMap->clear();
  
  vtkIdType nfaces = faces[0];
  this->FaceLocations->SetNumberOfValues(nfaces);

  this->Faces->InsertNextValue(nfaces);
  vtkIdType *face = faces + 1;
  vtkIdType faceLoc = 1;
  vtkIdType i, fid, npts;

  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    this->Faces->InsertNextValue(npts);
    for (i=1; i<=npts; ++i)
      {
      this->Faces->InsertNextValue(face[i]);
      }
    this->FaceLocations->SetValue(fid,faceLoc);

    faceLoc += face[0] + 1;
    face = faces + faceLoc;
    } //for all faces
}

//----------------------------------------------------------------------------
// Return the list of faces for this cell.
vtkIdType *vtkPolyhedron::GetFaces()
{
  return this->Faces->GetPointer(0);
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
