/*=========================================================================

  Program:   Visualization Library
  Module:    TubeF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TubeF.hh"
#include "PolyLine.hh"
#include "vlMath.hh"

vlTubeFilter::vlTubeFilter()
{
  this->Radius = 0.5;
  this->VaryRadius = 1;
  this->NumberOfSides = 3;
  this->RadiusFactor = 10;
}

void vlTubeFilter::Execute()
{
  int i, j, k;
  vlPoints *inPts;
  vlNormals *inNormals;
  vlPointData *pd;
  vlCellArray *inLines;
  int numPts, numNewPts;
  vlFloatPoints *newPts;
  vlFloatNormals *newNormals;
  vlCellArray *newStrips;
  vlMath math;
  int npts, *pts, i1, i2, ptOffset=0;
  float p[3], pNext[3];
  float *n, normal[3], nP[3];
  float s[3], sNext[3], sPrev[3], w[3];
  double BevelAngle;
  float theta=2.0*math.Pi()/this->NumberOfSides;
  int deleteNormals=0, ptId;
  vlPolyData *input=(vlPolyData *)this->Input;
  vlScalars *inScalars=NULL;
  float sFactor=1.0, range[2];
//
// Initialize
//
  vlDebugMacro(<<"Creating ribbon");
  this->Initialize();

  if ( !(inPts=input->GetPoints()) || 
  (numPts = inPts->GetNumberOfPoints()) < 1 ||
  !(inLines = input->GetLines()) || inLines->GetNumberOfCells() < 1 )
    {
    vlErrorMacro(<< ": No input data!\n");
    return;
    }
  numNewPts = numPts * this->NumberOfSides;

  // copy scalars, vectors, tcoords. Normals may be computed here.
  pd = input->GetPointData();
  this->PointData.CopyNormalsOff();
  this->PointData.CopyAllocate(pd,numNewPts);

  if ( !(inNormals=pd->GetNormals()) )
    {
    vlPolyLine lineNormalGenerator;
    deleteNormals = 1;
    inNormals = new vlFloatNormals(numNewPts);
    if ( !lineNormalGenerator.GenerateSlidingNormals(inPts,inLines,(vlFloatNormals*)inNormals) )
      {
      vlErrorMacro(<< "No normals for line!\n");
      delete inNormals;
      return;
      }
    }
//
// If varying width, get appropriate info.
//
  if ( this->VaryRadius && (inScalars=pd->GetScalars()) )
    {
    inScalars->GetRange(range);
    }

  newPts = new vlFloatPoints(numNewPts);
  newNormals = new vlFloatNormals(numNewPts);
  newStrips = new vlCellArray;
  newStrips->Allocate(newStrips->EstimateSize(1,numNewPts));
//
//  Create points along the line that are later connected into a 
//  triangle strip.
//
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
//
// Use "averaged" segment to create beveled effect. Watch out for first and 
// last points.
//
    for (j=0; j < npts; j++)
      {

      if ( j == 0 ) //first point
        {
        inPts->GetPoint(pts[0],p);
        inPts->GetPoint(pts[1],pNext);
        for (i=0; i<3; i++) 
          {
          sNext[i] = pNext[i] - p[i];
          sPrev[i] = sNext[i];
          }
        }

      else if ( j == (npts-1) ) //last point
        {
        for (i=0; i<3; i++)
          {
          sPrev[i] = sNext[i];
          p[i] = pNext[i];
          }
        }

      else
        {
        for (i=0; i<3; i++) p[i] = pNext[i];
        inPts->GetPoint(pts[j+1],pNext);
        for (i=0; i<3; i++)
          {
          sPrev[i] = sNext[i];
          sNext[i] = pNext[i] - p[i];
          }
        }

      n = inNormals->GetNormal(pts[j]);

      if ( math.Normalize(sNext) == 0.0 )
        {
        vlErrorMacro(<<"Coincident points!");
        return;
        }

      for (i=0; i<3; i++) s[i] = (sPrev[i] + sNext[i]) / 2.0; //average vector
      math.Normalize(s);
      
      if ( (BevelAngle = math.Dot(sNext,sPrev)) > 1.0 ) BevelAngle = 1.0;
      if ( BevelAngle < -1.0 ) BevelAngle = -1.0;
      BevelAngle = acos((double)BevelAngle) / 2.0; //(0->90 degrees)
      if ( (BevelAngle = cos(BevelAngle)) == 0.0 ) BevelAngle = 1.0;

      BevelAngle = this->Radius / BevelAngle; //keep ribbon constant width

      math.Cross(s,n,w);
      if ( math.Normalize(w) == 0.0)
        {
        vlErrorMacro(<<"Bad normal!");
        return;
        }
      
      math.Cross(w,s,nP); //create orthogonal coordinate system
      math.Normalize(nP);

      if ( inScalars )
        {
        if ( range[0] <= 0.0 ) // have to use straight linear
          {
          sFactor = 1.0 + ((this->RadiusFactor - 1.0) * 
                  (inScalars->GetScalar(j) - range[0]) / (range[1]-range[0]));
          }
        else // use flux preserving relationship
          {
          sFactor = sqrt((double)inScalars->GetScalar(j)/range[0]);
          if ( sFactor > this->RadiusFactor ) sFactor = this->RadiusFactor;
          }
        }

      //create points around line
      for (k=0; k < this->NumberOfSides; k++)
        {

        for (i=0; i<3; i++) 
          {
          normal[i] = w[i]*cos((double)k*theta) + nP[i]*sin((double)k*theta);
          s[i] = p[i] + this->Radius * sFactor * normal[i];
          }
        ptId = newPts->InsertNextPoint(s);
        newNormals->InsertNormal(ptId,normal);
        this->PointData.CopyData(pd,pts[j],ptId);
        }
      }
//
// Generate the strips
//
    for (k=0; k<this->NumberOfSides; k++)
      {
      i1 = (k+1) % this->NumberOfSides;
      newStrips->InsertNextCell(npts*2);
      for (i=0; i < npts; i++) 
        {
        i2 = i*this->NumberOfSides;
        newStrips->InsertCellPoint(ptOffset+i2+k);
        newStrips->InsertCellPoint(ptOffset+i2+i1);
        }
      } //for this line

    ptOffset += this->NumberOfSides*npts;
    }
//
// Update ourselves
//
  if ( deleteNormals ) delete inNormals;

  this->SetPoints(newPts);

  this->SetStrips(newStrips);
  this->PointData.SetNormals(newNormals);
  this->Squeeze();
}

void vlTubeFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Vary Radius: " << (this->VaryRadius ? "On\n" : "Off\n");
  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";
}

