/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTubeFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkTubeFilter.h"
#include "vtkPolyLine.h"
#include "vtkMath.h"

// Description:
// Construct object with radius 0.5, radius variation turned off, the number 
// of sides set to 3, and radius factor of 10.
vtkTubeFilter::vtkTubeFilter()
{
  this->Radius = 0.5;
  this->VaryRadius = VTK_VARY_RADIUS_OFF;
  this->NumberOfSides = 3;
  this->RadiusFactor = 10;

  this->DefaultNormal[0] = this->DefaultNormal[1] = 0.0;
  this->DefaultNormal[2] = 1.0;
  
  this->UseDefaultNormal = 0;
}

void vtkTubeFilter::Execute()
{
  int i, j, k;
  vtkPoints *inPts;
  vtkCellArray *inLines = NULL;
  vtkNormals *inNormals;
  vtkScalars *inScalars=NULL;
  vtkVectors *inVectors=NULL;
  int numPts = 0;
  int numNewPts;
  vtkFloatPoints *newPts;
  vtkFloatNormals *newNormals;
  vtkCellArray *newStrips;
  int npts, *pts, i1, i2, ptOffset=0;
  float p[3], pNext[3];
  float maxSpeed = 0;
  float *n, normal[3], nP[3];
  float s[3], sNext[3], sPrev[3], w[3];
  double BevelAngle;
  float theta=2.0*vtkMath::Pi()/this->NumberOfSides;
  int deleteNormals=0, ptId;
  float sFactor=1.0, range[2];
  vtkPointData *pd, *outPD;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
//
// Initialize
//
  vtkDebugMacro(<<"Creating tube");

  if ( !(inPts=input->GetPoints()) || 
  (numPts = inPts->GetNumberOfPoints()) < 1 ||
  !(inLines = input->GetLines()) || inLines->GetNumberOfCells() < 1 )
    {
    vtkErrorMacro(<< ": No input data!\n");
    return;
    }
  numNewPts = numPts * this->NumberOfSides;

  // copy scalars, vectors, tcoords. Normals may be computed here.
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd,numNewPts);

  if ( !(inNormals=pd->GetNormals()) || this->UseDefaultNormal )
    {
    vtkPolyLine lineNormalGenerator;
    deleteNormals = 1;
    inNormals = vtkFloatNormals::New();
    inNormals->SetNumberOfNormals(numNewPts);

    if ( this->UseDefaultNormal )
      {
      for ( i=0; i < numPts; i++)
	{
	inNormals->SetNormal(i,this->DefaultNormal);
	}
      }
    else
      {
      if ( !lineNormalGenerator.GenerateSlidingNormals(inPts,inLines,(vtkFloatNormals*)inNormals) )
        {
        vtkErrorMacro(<< "No normals for line!\n");
        if (deleteNormals) inNormals->Delete();
        newPts->Delete();
        newNormals->Delete();
        newStrips->Delete();
        return;
        }
      }
    }
//
// If varying width, get appropriate info.
//
  if ( this->VaryRadius == VTK_VARY_RADIUS_BY_SCALAR && 
  (inScalars=pd->GetScalars()) )
    {
    inScalars->GetRange(range);
    }
  else if ( this->VaryRadius == VTK_VARY_RADIUS_BY_VECTOR && 
  (inVectors=pd->GetVectors()) )
    {
    maxSpeed = inVectors->GetMaxNorm();
    }

  newPts = vtkFloatPoints::New();
  newPts->Allocate(numNewPts);
  newNormals = vtkFloatNormals::New();
  newNormals->Allocate(numNewPts);
  newStrips = vtkCellArray::New();
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
      if (npts < 2)
	{
	vtkErrorMacro(<< "less than two points");
	}

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

      if ( vtkMath::Normalize(sNext) == 0.0 )
        {
        vtkErrorMacro(<<"Coincident points!");
        inNormals->Delete();
        newPts->Delete();
        newNormals->Delete();
        newStrips->Delete();
        return;
        }

      for (i=0; i<3; i++) s[i] = (sPrev[i] + sNext[i]) / 2.0; //average vector
      // if s is zero then just use sPrev cross n
      if (vtkMath::Normalize(s) == 0.0)
	{
	vtkWarningMacro(<< "using alternate bevel vector");
	vtkMath::Cross(sPrev,n,s);
	if (vtkMath::Normalize(s) == 0.0)
	  {
	  vtkErrorMacro(<< "using alternate bevel vector");
	  }
	}
      
      if ( (BevelAngle = vtkMath::Dot(sNext,sPrev)) > 1.0 ) BevelAngle = 1.0;
      if ( BevelAngle < -1.0 ) BevelAngle = -1.0;
      BevelAngle = acos((double)BevelAngle) / 2.0; //(0->90 degrees)
      if ( (BevelAngle = cos(BevelAngle)) == 0.0 ) BevelAngle = 1.0;

      BevelAngle = this->Radius / BevelAngle; //keep tube constant radius

      vtkMath::Cross(s,n,w);
      if ( vtkMath::Normalize(w) == 0.0)
        {
        vtkErrorMacro(<<"Bad normal s = " << s[0] << " " << s[1] << " " << s[2] << " n = " << n[0] << " " << n[1] << " " << n[2]);
        if (deleteNormals) inNormals->Delete();
        newPts->Delete();
        newNormals->Delete();
        newStrips->Delete();
        return;
        }
      
      vtkMath::Cross(w,s,nP); //create orthogonal coordinate system
      vtkMath::Normalize(nP);

      if ( inScalars ) // varying by scalar values
        {
        sFactor = 1.0 + ((this->RadiusFactor - 1.0) * 
                  (inScalars->GetScalar(j) - range[0]) / (range[1]-range[0]));
	if ((range[1] - range[0]) == 0.0)
	  {
	  vtkErrorMacro(<< "Dividing by zero");
	  }
        }
      else if ( inVectors ) // use flux preserving relationship
        {
        sFactor = 
	  sqrt((double)maxSpeed/vtkMath::Norm(inVectors->GetVector(j)));
        if ( sFactor > this->RadiusFactor ) sFactor = this->RadiusFactor;
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
        outPD->CopyData(pd,pts[j],ptId);
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
        newStrips->InsertCellPoint(ptOffset+i2+i1);
        newStrips->InsertCellPoint(ptOffset+i2+k);
        }
      } //for this line

    ptOffset += this->NumberOfSides*npts;
    }
//
// Update ourselves
//
  if ( deleteNormals ) inNormals->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetStrips(newStrips);
  newStrips->Delete();

  outPD->SetNormals(newNormals);
  newNormals->Delete();

  output->Squeeze();
}

void vtkTubeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Vary Radius: " << (this->VaryRadius ? "On\n" : "Off\n");
  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";
}

