/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButtonSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkButtonSource.h"
#include "vtkPoints.h"
#include "vtkMath.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkButtonSource, "1.1");
vtkStandardNewMacro(vtkButtonSource);

// Construct 
vtkButtonSource::vtkButtonSource()
{
  this->Width = 0.5;
  this->Height = 0.5;
  this->Depth = 0.05;

  this->CircumferentialResolution = 4;
  this->TextureResolution = 2;
  this->ShoulderResolution = 2;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->ShoulderTextureCoordinate[0] = 0.0;
  this->ShoulderTextureCoordinate[1] = 0.0;
  
  this->RadialRatio = 1.1;
  this->TextureStyle = VTK_TEXTURE_STYLE_PROPORTIONAL;
  this->TextureDimensions[0] = 100;
  this->TextureDimensions[1] = 100;
  this->TwoSided = 0;
}

void vtkButtonSource::Execute()
{
  int i, j;
  vtkPolyData *output = this->GetOutput();

  vtkDebugMacro(<<"Generating button");
  
  // Check input
  if ( this->Width <= 0.0 || this->Height <= 0.0 )
    {
    vtkErrorMacro(<<"Button must have non-zero height and width");
    return;
    }
  
  // Create the button in several steps. First, create the button in
  // the x-y plane, this requires creating the texture region and then
  // the shoulder region. After this, the z-depth is created. And if
  // it is a two-sided button, then a mirror reflection of the button
  // in the negative z-direction is created.
  int numPts = 1 + this->CircumferentialResolution *
               (this->TextureResolution + this->ShoulderResolution + 1);
  if ( this->TwoSided ) { numPts *= 2; }

  vtkPoints *newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

  vtkFloatArray *normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(numPts);

  vtkFloatArray *tcoords = vtkFloatArray::New();
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(numPts);

  vtkCellArray *newPolys = vtkCellArray::New();
  newPolys->Allocate(this->CircumferentialResolution*
                     (this->TextureResolution*this->ShoulderResolution));
  
  // Create the texture region.  --------------------------------------------
  // Start by determining the resolution in the width and height directions.
  // Setup the ellipsoid.
  float x[3], x0[3], x1[3], x2[3], x3[3], n[3];
  this->A = this->Width / 2.0;
  this->A2 = this->A * this->A;
  this->B = this->Height / 2.0;
  this->B2 = this->B*this->B;
  this->C = this->Depth;
  this->C2 = this->C*this->C;
  
  float *xP, dX, dY;
  if ( this->TextureStyle == VTK_TEXTURE_STYLE_FIT_IMAGE )
    {
    dX = (float)this->TextureDimensions[0];
    dY = (float)this->TextureDimensions[1];
    }
  else
    {
    dX = this->A;
    dY = this->B;
    }
  int hRes = ((int)ceil(this->CircumferentialResolution * (dY/(dY+dX)))) / 2;
  hRes = (hRes <= 0 ? 1 : hRes);
  int wRes = (this->CircumferentialResolution - 2*hRes) / 2;

  // Create the center point
  newPts->SetPoint(0,this->Origin[0],this->Origin[1],
                   this->Origin[2]+this->Depth);
  normals->SetTuple3(0, 0.0,0.0,1.0);
  tcoords->SetTuple2(0, 0.5, 0.5);
  
  // Set up for points interior to the texture 
  int offset = 1 + (this->TextureResolution-1)*this->CircumferentialResolution;

  // Determine the lower-left corner of the texture region
  float xe, ye;
  float a = this->A/this->RadialRatio;
  float b = this->B/this->RadialRatio;
  this->IntersectEllipseWithLine(a*a,b*b,dX,dY,xe,ye);
  
  x0[0] = this->Origin[0] - xe;
  x0[1] = this->Origin[1] - ye;
  x0[2] = this->ComputeDepth(1,x0[0],x0[1],n);
  newPts->SetPoint(offset, x0);
  normals->SetTuple(offset, n);
  tcoords->SetTuple2(offset, 0.0, 0.0);

  // Create the lower right point
  x1[0] = this->Origin[0] + xe;
  x1[1] = this->Origin[1] - ye;
  x1[2] = this->ComputeDepth(1,x1[0],x1[1],n);
  newPts->SetPoint(offset + wRes, x1);
  normals->SetTuple(offset + wRes, n);
  tcoords->SetTuple2(offset + wRes, 1.0, 0.0);
  
  // Create the upper right point
  x2[0] = this->Origin[0] + xe;
  x2[1] = this->Origin[1] + ye;
  x2[2] = this->ComputeDepth(1,x2[0],x2[1],n);
  newPts->SetPoint(offset + wRes + hRes, x2);
  normals->SetTuple(offset + wRes + hRes, n);
  tcoords->SetTuple2(offset + wRes + hRes, 1.0, 1.0);
  
  // Create the upper left point
  x3[0] = this->Origin[0] - xe;
  x3[1] = this->Origin[1] + ye;
  x3[2] = this->ComputeDepth(1,x3[0],x3[1],n);
  newPts->SetPoint(offset + 2*wRes + hRes, x3);
  normals->SetTuple(offset + 2*wRes + hRes, n);
  tcoords->SetTuple2(offset + 2*wRes + hRes, 0.0, 1.0);
  
  // Okay, now fill in the points along the edges
  float t;
  for (i=1; i < wRes; i++) //x0 -> x1
    {
    t = (float)i/wRes;
    x[0] = x0[0] + t * (x1[0]-x0[0]);
    x[1] = x0[1];
    x[2] = this->ComputeDepth(1,x[0],x[1],n);
    newPts->SetPoint(offset+i,x);
    normals->SetTuple(offset+i,n);
    tcoords->SetTuple2(offset+i, t,0.0);
    }
  for (i=1; i < hRes; i++) //x1 -> x2
    {
    t = (float)i/hRes;
    x[0] = x1[0];
    x[1] = x1[1] + t * (x2[1]-x1[1]);
    x[2] = this->ComputeDepth(1,x[0],x[1],n);
    newPts->SetPoint(offset+wRes+i,x);
    normals->SetTuple(offset+wRes+i,n);
    tcoords->SetTuple2(offset+wRes+i, 1.0,t);
    }
  for (i=1; i < wRes; i++) //x2 -> x3
    {
    t = (float)i/wRes;
    x[0] = x2[0] + t * (x3[0]-x2[0]);
    x[1] = x2[1];
    x[2] = this->ComputeDepth(1,x[0],x[1],n);
    newPts->SetPoint(offset+wRes+hRes+i,x);
    normals->SetTuple(offset+wRes+hRes+i,n);
    tcoords->SetTuple2(offset+wRes+hRes+i, (1.0-t),1.0);
    }
  for (i=1; i < hRes; i++) //x3 -> x0
    {
    t = (float)i/hRes;
    x[0] = x3[0];
    x[1] = x3[1] + t * (x0[1]-x3[1]);
    x[2] = this->ComputeDepth(1,x[0],x[1],n);
    newPts->SetPoint(offset+2*wRes+hRes+i,x);
    normals->SetTuple(offset+2*wRes+hRes+i,n);
    tcoords->SetTuple2(offset+2*wRes+hRes+i, 0.0,(1.0-t));
    }

  // Fill in the inside of the texture region
  vtkIdType pts[3];
  pts[0] = 0;
  for (i=0; i<(this->CircumferentialResolution-1); i++)
    {
    pts[1] = i + 1;
    pts[2] = i + 2;
    newPolys->InsertNextCell(3,pts);
    }
  pts[1] = this->CircumferentialResolution;
  pts[2] = 1;
  newPolys->InsertNextCell(3,pts);

  if ( this->TextureResolution >= 1 )
    {
    this->InterpolateCurve(1, newPts, this->CircumferentialResolution,
                           normals, tcoords, this->TextureResolution,
                           0, 0, offset, 1, 1, 1);
    this->CreatePolygons(newPolys, this->CircumferentialResolution,
                         this->TextureResolution-1, 1);
    }

  // Create the shoulder region.  --------------------------------------------
  // Start by duplicating points around the texture region. These are 
  // copied to avoid texture interpolation pollution.
  int c1Start = offset + this->CircumferentialResolution;
  for ( i=0; i < this->CircumferentialResolution; i++)
    {
    newPts->SetPoint(c1Start+i, newPts->GetPoint(offset+i));
    normals->SetTuple(c1Start+i, normals->GetTuple(offset+i));
    tcoords->SetTuple(c1Start+i, this->ShoulderTextureCoordinate);
    }
  
  // Now create points around the perimeter of the button. The locations
  // of the points (i.e., angles) are taken from the texture region.
  int c2Start = offset + (this->ShoulderResolution+1) *
                      this->CircumferentialResolution;
  for ( i=0; i < this->CircumferentialResolution; i++)
    {
    //compute the angle
    xP = newPts->GetPoint(offset+i);
    dX = xP[0] - this->Origin[0];
    dY = xP[1] - this->Origin[1];

    this->IntersectEllipseWithLine(this->A2,this->B2,dX,dY,xe,ye);

    x[0] = this->Origin[0] + xe;
    x[1] = this->Origin[1] + ye;
    x[2] = this->ComputeDepth(0,x[0],x[1],n);
    newPts->SetPoint(c2Start+i, x);
    normals->SetTuple(c2Start+i, n);
    tcoords->SetTuple(c2Start+i, this->ShoulderTextureCoordinate);
    }
  
  // Interpolate points between the curves. Create polygons.
  this->InterpolateCurve(0, newPts, this->CircumferentialResolution,
                         normals, tcoords, this->ShoulderResolution,
                         c1Start, 1, c2Start, 1, 
                         c1Start+this->CircumferentialResolution, 1);
  this->CreatePolygons(newPolys, this->CircumferentialResolution,
                       this->ShoulderResolution, c1Start);

  // Create the other side of the button if requested.
  if ( this->TwoSided > 0.0 )
    {
    //do the points
    numPts /= 2;
    for (i=0; i<numPts; i++)
      {
      newPts->GetPoint(i, x);
      x[0] = -(x[0]-this->Origin[0]) + this->Origin[0];
      x[2] = -(x[2]-this->Origin[2]) + this->Origin[2];
      newPts->SetPoint(i+numPts, x);
      normals->GetTuple(i, x);
      x[0] = -x[0];
      x[2] = -x[2];
      normals->SetTuple(i+numPts, x);
      tcoords->SetTuple(i+numPts, tcoords->GetTuple(i));
      }
    //do the polygons
    vtkIdType *pts, opts[4];
    int npts, numPolys=newPolys->GetNumberOfCells();
    for ( j=0, newPolys->InitTraversal(); j < numPolys; j++ )
      {
      newPolys->GetNextCell(npts,pts);
      for (i=0; i<npts; i++)
        {
        opts[i] = pts[i] + numPts;
        }
      newPolys->InsertNextCell(npts,opts);
      }
    }

  // Clean up and get out
  output->SetPoints(newPts);
  output->GetPointData()->SetNormals(normals);
  output->GetPointData()->SetTCoords(tcoords);
  output->SetPolys(newPolys);

  newPts->Delete();
  tcoords->Delete();
  newPolys->Delete();
}

void vtkButtonSource::InterpolateCurve(int inTextureRegion,
                                       vtkPoints *newPts, int numPts,
                                       vtkFloatArray *normals,
                                       vtkFloatArray *tcoords, int res,
                                       int c1StartPt, int c1Incr,
                                       int c2StartPt, int c2Incr, 
                                       int startPt, int incr)
{
  int i, j, idx;
  float *x0, *x1, *tc0, *tc1, t, x[3], tc[2], n[3];

  //walk around the curves interpolating new points between them
  for ( i=0; i < numPts; 
        i++, c1StartPt+=c1Incr, c2StartPt+=c2Incr, startPt+=incr)
    {
    x0 = newPts->GetPoint(c1StartPt);
    x1 = newPts->GetPoint(c2StartPt);
    tc0 = tcoords->GetTuple(c1StartPt);
    tc1 = tcoords->GetTuple(c2StartPt);

    //do the interpolations along this radius
    for ( j=1; j < res; j++ )
      {
      idx = startPt+(j-1)*numPts;
      t = (float)j / res;
      x[0] = x0[0] + t * (x1[0] - x0[0]);
      x[1] = x0[1] + t * (x1[1] - x0[1]);
      x[2] = this->ComputeDepth(inTextureRegion,x[0],x[1],n);
      newPts->SetPoint(idx, x);
      normals->SetTuple(idx, n);
      tc[0] = tc0[0] + t * (tc1[0] - tc0[0]);
      tc[1] = tc0[1] + t * (tc1[1] - tc0[1]);
      tcoords->SetTuple(idx, tc);
      }
    }//for all points
}

void vtkButtonSource::CreatePolygons(vtkCellArray *newPolys, int num, int res, 
                                     int startIdx)
{
  int i, j;
  vtkIdType idx, pts[4];
  
  for (i=0; i < res; i++, startIdx+=num)
    {
    idx = startIdx;
    for (j=0; j < num; j++, idx++)
      {
      pts[0] = idx;
      pts[1] = idx + num;
      if ( j == (num-1) )
        {
        pts[2] = startIdx + num;
        pts[3] = startIdx;
        }
      else
        {
        pts[2] = idx + num + 1;
        pts[3] = idx + 1;
        }
      newPolys->InsertNextCell(4,pts);
      }
    }
}

void vtkButtonSource::IntersectEllipseWithLine(float a2, float b2, float dX, 
                                               float dY, float& xe, float& ye)
{
  double m;
  
  if ( fabs(dY) <= fabs(dX) )
    {
    m = dY/dX;
    xe = sqrt( a2*b2/(b2 + m*m*a2) );
    if ( dX < 0.0 ) xe = -xe;
    ye = m*xe;
    }
  else
    {
    m = dX/dY;
    ye = sqrt( a2*b2/(m*m*b2 + a2) );
    if ( dY < 0.0 ) ye = -ye;
    xe = m*ye;
    }
}

float vtkButtonSource::ComputeDepth(int inTextureRegion, 
                                    float x, float y, float n[3])
{
  float z;

  x -= this->Origin[0];
  y -= this->Origin[1];
  z = 1.0 - (x*x)/this->A2 - (y*y)/this->B2;

  if ( z < 0.0 ) n[2] = z = 0.0;
  else n[2] = z = this->Depth * sqrt(z);

  n[0] = 2.0*x / this->A2;
  n[1] = 2.0*y / this->B2;
  n[2] = 2.0*z / this->C2;

  vtkMath::Normalize(n);

  return (z + this->Origin[2]);
}
  
void vtkButtonSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Width: " << this->Width << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Depth: " << this->Depth << "\n";

  os << indent << "Circumferential Resolution: " 
     << this->CircumferentialResolution << "\n";
  os << indent << "Texture Resolution: " << this->TextureResolution << "\n";
  os << indent << "Shoulder Resolution: " << this->ShoulderResolution << "\n";

  os << indent << "Origin: (" << this->Origin[0] << ", "
                              << this->Origin[1] << ", "
                              << this->Origin[2] << ")\n";

  os << indent << "Shoulder Texture Coordinate: (" 
     << this->ShoulderTextureCoordinate[0] << ", "
     << this->ShoulderTextureCoordinate[1] << ")\n";

  os << indent << "Radial Ratio: " << this->RadialRatio << "\n";

  os << indent << "Texture Style: ";
  if ( this->TextureStyle == VTK_TEXTURE_STYLE_FIT_IMAGE )
    {
    os << "Fit\n";
    }
  else
    {
    os << "Proportional\n";
    }
  
  os << indent << "Texture Dimensions: (" 
     << this->TextureDimensions[0] << ", " 
     << this->TextureDimensions[1] << ")\n";

  os << indent << "Two Sided: " 
     << (this->TwoSided ? "On\n" : "Off\n");
}
