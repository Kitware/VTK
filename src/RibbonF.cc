/*=========================================================================

  Program:   Visualization Library
  Module:    RibbonF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlMath.hh"
#include "RibbonF.hh"
#include "FPoints.hh"
#include "FNormals.hh"
#include "PolyLine.hh"

vlRibbonFilter::vlRibbonFilter()
{
  this->Radius = 0.5;
  this->Angle = 180.0;
}

void vlRibbonFilter::Execute()
{
  int i, j, k;
  vlPoints *inPts;
  vlNormals *inNormals;
  vlPointData *pd;
  vlCellArray *inLines;
  int numNewPts;
  vlFloatPoints *newPts;
  vlFloatNormals *newNormals;
  vlCellArray *newStrips;
  int npts, *pts;
  float *x1, *x2, norm, *n, normal[3];
  float l[3], pt[3], p[3];
  vlMath math;
  float theta;
  int deleteNormals=0;
//
// Initialize
//
  this->Initialize();

  inPts = this->Input->GetPoints();
  pd = this->Input->GetPointData();
  // copy scalars, vectors, tcoords.  Normals are computed here.
  this->PointData.CopyAllocate(pd,1,1,0,1);

  if ( !(inLines = this->Input->GetLines()) || 
  inLines->GetNumberOfCells() < 1 )
    {
    vlErrorMacro(<< ": No input data!\n");
    return;
    }

  if ( !(inNormals=pd->GetNormals()) )
    {
    vlPolyLine lineNormalGenerator;
    deleteNormals = 1;
    inNormals = new vlFloatNormals(inPts->GetNumberOfPoints());
    if ( !lineNormalGenerator.GenerateNormals(inPts,inLines,(vlFloatNormals*)inNormals) )
      {
      vlErrorMacro(<< ": No normals for line!\n");
      delete inNormals;
      return;
      }
    }

  numNewPts = inPts->GetNumberOfPoints() * 2;
  newPts = new vlFloatPoints(numNewPts);
  newNormals = new vlFloatNormals(numNewPts);
  newStrips = new vlCellArray;
  newStrips->Allocate(newStrips->EstimateSize(1,numNewPts));
//
//  Create pairs of points along the line that are later connected into a 
//  triangle strip.
//
  theta = this->Angle * math.DegreesToRadians();
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    for (j=0; j<npts; j++)
      {
        // compute first point
        x1 = inPts->GetPoint(pts[j]);
        n = inNormals->GetNormal(pts[j]);
        for (k=0; k<3; k++) pt[k] = x1[k] + this->Radius*n[k];
        newPts->InsertNextPoint(pt);
        newNormals->InsertNextNormal(n);

        // now next point
        if ( j < (npts - 1) )
          {
          x2 = inPts->GetPoint(pts[j+1]);
          for (k=0; k<3; k++) l[k] = x2[k] - x1[k];
          }
        else
          {
          ; // use old l[k]
          }
        math.Cross(n,l,p);
        if ((norm=math.Norm(p)) == 0.0) norm = 1.0;
        for (k=0; k<3; k++) p[k] /= norm;
        for (k=0; k<3; k++) normal[k] = n[k]*cos((double)theta) + p[k]*sin((double)theta);
        for (k=0; k<3; k++) pt[k] = x1[k] + this->Radius*normal[k];
        newPts->InsertNextPoint(pt);
        newNormals->InsertNextNormal(normal);
      }
    }
//
// Generate the strip topology
//
  newStrips->InsertNextCell(numNewPts);
  for (i=0; i < numNewPts; i++) 
    {
    newStrips->InsertCellPoint(i);
    }
//
// Update ourselves
//
  if ( deleteNormals ) delete inNormals;

  this->SetPoints(newPts);

  this->SetStrips(newStrips);
  this->PointData.SetNormals(newNormals);
}

void vlRibbonFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlRibbonFilter::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Radius: " << this->Radius << "\n";
    os << indent << "Angle: " << this->Angle << "\n";
    }
}

