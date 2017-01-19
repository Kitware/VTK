/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCell.h"

#include "vtkMath.h"
#include "vtkPoints.h"

//----------------------------------------------------------------------------
// Construct cell.
vtkCell::vtkCell()
{
  this->Points = vtkPoints::New();
  this->Points->SetDataTypeToDouble();
  this->PointIds = vtkIdList::New();
  // Consistent Register/Deletes (ShallowCopy uses Register.)
  this->Points->Register(this);
  this->Points->Delete();
  this->PointIds->Register(this);
  this->PointIds->Delete();
}

//----------------------------------------------------------------------------
vtkCell::~vtkCell()
{
  this->Points->UnRegister(this);
  this->PointIds->UnRegister(this);
}

//----------------------------------------------------------------------------
// Instantiate cell from outside
//
void vtkCell::Initialize(int npts, vtkIdType *pts, vtkPoints *p)
{
  this->PointIds->Reset();
  this->Points->Reset();

  for (int i=0; i<npts; i++)
  {
    this->PointIds->InsertId(i,pts[i]);
    this->Points->InsertPoint(i,p->GetPoint(pts[i]));
  }
}

//----------------------------------------------------------------------------
// Instantiate cell from outside. A simplified version of
// vtkCell::Initialize() that assumes point ids are simply the index into the
// points. This is a convenience function.
//
void vtkCell::Initialize(int npts, vtkPoints *p)
{
  this->PointIds->Reset();
  this->Points->Reset();

  for (int i=0; i<npts; i++)
  {
    this->PointIds->InsertId(i,i);
    this->Points->InsertPoint(i,p->GetPoint(i));
  }
}

//----------------------------------------------------------------------------
void vtkCell::ShallowCopy(vtkCell *c)
{
  this->Points->ShallowCopy(c->Points);
  if ( this->PointIds )
  {
    this->PointIds->UnRegister(this);
    this->PointIds = c->PointIds;
    this->PointIds->Register(this);
  }
}

//----------------------------------------------------------------------------
void vtkCell::DeepCopy(vtkCell *c)
{
  this->Points->DeepCopy(c->Points);
  this->PointIds->DeepCopy(c->PointIds);
}

//----------------------------------------------------------------------------
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Return pointer
// to array of six double values.
double *vtkCell::GetBounds ()
{
  double x[3];
  int i, numPts=this->Points->GetNumberOfPoints();

  if (numPts)
  {
    this->Points->GetPoint(0, x);
    this->Bounds[0] = x[0];
    this->Bounds[2] = x[1];
    this->Bounds[4] = x[2];
    this->Bounds[1] = x[0];
    this->Bounds[3] = x[1];
    this->Bounds[5] = x[2];
    for (i=1; i<numPts; i++)
    {
      this->Points->GetPoint(i, x);
      this->Bounds[0] = (x[0] < this->Bounds[0] ? x[0] : this->Bounds[0]);
      this->Bounds[1] = (x[0] > this->Bounds[1] ? x[0] : this->Bounds[1]);
      this->Bounds[2] = (x[1] < this->Bounds[2] ? x[1] : this->Bounds[2]);
      this->Bounds[3] = (x[1] > this->Bounds[3] ? x[1] : this->Bounds[3]);
      this->Bounds[4] = (x[2] < this->Bounds[4] ? x[2] : this->Bounds[4]);
      this->Bounds[5] = (x[2] > this->Bounds[5] ? x[2] : this->Bounds[5]);
    }
  }
  else
  {
    vtkMath::UninitializeBounds(this->Bounds);
  }
  return this->Bounds;
}

//----------------------------------------------------------------------------
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Copy result into
// user provided array.
void vtkCell::GetBounds(double bounds[6])
{
  this->GetBounds();
  for (int i=0; i < 6; i++)
  {
    bounds[i] = this->Bounds[i];
  }
}

//----------------------------------------------------------------------------
// Compute Length squared of cell (i.e., bounding box diagonal squared).
double vtkCell::GetLength2 ()
{
  double diff, l=0.0;
  int i;

  this->GetBounds();
  for (i=0; i<3; i++)
  {
    diff = this->Bounds[2*i+1] - this->Bounds[2*i];
    l += diff * diff;
  }
  return l;
}

//----------------------------------------------------------------------------
// Return center of the cell in parametric coordinates.
// Note that the parametric center is not always located
// at (0.5,0.5,0.5). The return value is the subId that
// the center is in (if a composite cell). If you want the
// center in x-y-z space, invoke the EvaluateLocation() method.
int vtkCell::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return 0;
}

//----------------------------------------------------------------------------
// This method works fine for all "rectangular" cells, not triangular
// and tetrahedral topologies.
double vtkCell::GetParametricDistance(double pcoords[3])
{
  int i;
  double pDist, pDistMax=0.0;

  for (i=0; i<3; i++)
  {
    if ( pcoords[i] < 0.0 )
    {
      pDist = -pcoords[i];
    }
    else if ( pcoords[i] > 1.0 )
    {
      pDist = pcoords[i] - 1.0;
    }
    else //inside the cell in the parametric direction
    {
      pDist = 0.0;
    }
    if ( pDist > pDistMax )
    {
      pDistMax = pDist;
    }
  }
  return pDistMax;
}


//----------------------------------------------------------------------------
void vtkCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int numIds=this->PointIds->GetNumberOfIds();

  os << indent << "Number Of Points: " << numIds << "\n";

  if ( numIds > 0 )
  {
    const double *bounds=this->GetBounds();

    os << indent << "Bounds: \n";
    os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";

    os << indent << "  Point ids are: ";
    for (int i=0; i < numIds; i++)
    {
      os << this->PointIds->GetId(i);
      if ( i && !(i % 12) )
      {
        os << "\n\t";
      }
      else
      {
        if ( i != (numIds-1) )
        {
          os << ", ";
        }
      }
    }
    os << indent << "\n";
  }
}

// Usually overridden. Only composite cells do not override this.
double *vtkCell::GetParametricCoords()
{
  return NULL;
}
