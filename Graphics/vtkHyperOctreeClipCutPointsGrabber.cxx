/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeClipCutPointsGrabber.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeClipCutPointsGrabber.h"

#include "vtkObjectFactory.h"
#include "vtkOrderedTriangulator.h"
#include <vtkstd/set>
#include "vtkPolygon.h"
#include <assert.h>
#include "vtkPoints.h"

vtkStandardNewMacro(vtkHyperOctreeClipCutPointsGrabber);

class vtkHyperOctreeIdSet // Pimpl idiom
{
public:
  vtkstd::set<vtkIdType> Set;
};

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkHyperOctreeClipCutPointsGrabber::vtkHyperOctreeClipCutPointsGrabber()
{
  this->Triangulator=vtkOrderedTriangulator::New();
  this->IdSet=new vtkHyperOctreeIdSet;
  this->Polygon=0;
  this->Dimension=3;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
vtkHyperOctreeClipCutPointsGrabber::~vtkHyperOctreeClipCutPointsGrabber()
{
  if(this->Triangulator!=0)
    {
    this->Triangulator->UnRegister(this);
    delete this->IdSet;
    }
  if(this->Polygon!=0)
    {
    this->Polygon->UnRegister(this);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Set the dimension of the hyperoctree.
// \pre valid_dim: (dim==2 || dim==3)
// \post is_set: GetDimension()==dim
void vtkHyperOctreeClipCutPointsGrabber::SetDimension(int dim)
{
  assert("pre: valid_dim" && (dim==2 || dim==3));
  if(dim!=this->Dimension)
    {
    if(dim==3)
      {
      this->Polygon->UnRegister(this);
      this->Polygon=0;
      this->Triangulator=vtkOrderedTriangulator::New();
      this->IdSet=new vtkHyperOctreeIdSet;
      }
    else
      {
       this->Triangulator->UnRegister(this);
       this->Triangulator=0;
       delete this->IdSet;
       this->IdSet=0;
       this->Polygon=vtkPolygon::New();
      }
    this->Dimension=dim;
    }
  assert("post: is_set" && GetDimension()==dim);
}

//-----------------------------------------------------------------------------
// Description:
// Initialize the points insertion scheme.
// Actually, it is just a trick to initialize the IdSet from the filter.
// The IdSet class cannot be shared with the filter because it is a Pimpl.
// It is used by clip,cut and contour filters to build the points
// that lie on an hyperoctant.
// \pre only_in_3d: GetDimension()==3
void vtkHyperOctreeClipCutPointsGrabber::InitPointInsertion()
{
  assert("pre: only_in_3d" && this->GetDimension()==3);
  this->IdSet->Set.clear();
}

//-----------------------------------------------------------------------------
// Description:
// Insert a point, assuming the point is unique and does not require a
// locator. Tt does not mean it does not use a locator. It just mean that
// some implementation may skip the use of a locator.
void vtkHyperOctreeClipCutPointsGrabber::InsertPoint(vtkIdType ptId,
                                                     double pt[3],
                                                     double pcoords[3],
                                                     int vtkNotUsed(ijk)[3])
{
  this->Triangulator->InsertPoint(ptId,pt,pcoords,0);
}


//-----------------------------------------------------------------------------
// Description:
// Insert a point using a locator.
void vtkHyperOctreeClipCutPointsGrabber::InsertPointWithMerge(
  vtkIdType ptId,
  double pt[3],
  double pcoords[3],
  int vtkNotUsed(ijk)[3])
{
  if(this->IdSet->Set.find(ptId)==this->IdSet->Set.end()) // not find
    {
    this->IdSet->Set.insert(ptId);
    this->Triangulator->InsertPoint(ptId,pt,pcoords,0);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Insert a point in the quadtree case.
void vtkHyperOctreeClipCutPointsGrabber::InsertPoint2D(double pt[3],
                                                       int vtkNotUsed(ijk)[3])
{
  this->Polygon->GetPointIds()->InsertNextId(
    this->Polygon->GetPointIds()->GetNumberOfIds());
  this->Polygon->GetPoints()->InsertNextPoint(pt);
}

//-----------------------------------------------------------------------------
// Description:
// Return the ordered triangulator.
vtkOrderedTriangulator *vtkHyperOctreeClipCutPointsGrabber::GetTriangulator()
{
  return this->Triangulator;
}

//-----------------------------------------------------------------------------
// Description:
// Return the polygon.
vtkPolygon *vtkHyperOctreeClipCutPointsGrabber::GetPolygon()
{
  return this->Polygon;
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeClipCutPointsGrabber::PrintSelf(ostream& os,
                                                   vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
