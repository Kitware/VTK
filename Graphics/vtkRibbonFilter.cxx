/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRibbonFilter.cxx
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
#include "vtkMath.h"
#include "vtkRibbonFilter.h"
#include "vtkPoints.h"
#include "vtkPolyLine.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkRibbonFilter, "1.59");
vtkStandardNewMacro(vtkRibbonFilter);

// Construct ribbon so that width is 0.1, the width does 
// not vary with scalar values, and the width factor is 2.0.
vtkRibbonFilter::vtkRibbonFilter()
{
  this->Width = 0.5;
  this->Angle = 0.0;
  this->VaryWidth = 0;
  this->WidthFactor = 2.0;

  this->DefaultNormal[0] = this->DefaultNormal[1] = 0.0;
  this->DefaultNormal[2] = 1.0;
  
  this->UseDefaultNormal = 0;
}

void vtkRibbonFilter::Execute()
{
  vtkIdType i;
  int j;
  vtkPoints *inPts;
  vtkDataArray *inNormals;
  vtkPointData *pd, *outPD;
  vtkCellData *cd, *outCD;
  vtkCellArray *inLines;
  vtkIdType numPts = 0;
  vtkIdType numNewPts = 0;
  vtkPoints *newPts;
  vtkFloatArray *newNormals;
  vtkCellArray *newStrips;
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  float p[3], pNext[3];
  float *n;
  float s[3], sNext[3], sPrev[3], w[3];
  double BevelAngle;
  int deleteNormals=0;
  vtkIdType ptId;
  vtkPolyData *input= this->GetInput();
  vtkPolyData *output= this->GetOutput();
  vtkDataArray *inScalars=NULL;
  float sFactor=1.0, range[2];
  vtkIdType ptOffset=0;

  // Initialize
  //
  vtkDebugMacro(<<"Creating ribbon");

  inPts=input->GetPoints();
  inLines = input->GetLines();
  if ( !inPts || 
       (numNewPts=inPts->GetNumberOfPoints()*2) < 1 ||
       !inLines || inLines->GetNumberOfCells() < 1 )
    {
    vtkErrorMacro(<< "No input data!");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  
  // copy point scalars, vectors, tcoords. Normals may be computed here.
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd,numNewPts);

  // copy point scalars, vectors, tcoords.
  cd = input->GetCellData();
  outCD = output->GetCellData();
  outCD->CopyNormalsOff();
  outCD->CopyAllocate(cd, inLines->GetNumberOfCells());
  int inCellId, outCellId;

  if ( !(inNormals=pd->GetNormals()) || this->UseDefaultNormal )
    {
    vtkPolyLine *lineNormalGenerator = vtkPolyLine::New();
    deleteNormals = 1;
    inNormals = vtkFloatArray::New();
    inNormals->SetNumberOfComponents(3);
    inNormals->Allocate(3*numPts);
    if ( this->UseDefaultNormal )
      {
      for ( i=0; i < numPts; i++)
        {
        inNormals->SetTuple(i,this->DefaultNormal);
        }
      }
    else
      {
      if ( !lineNormalGenerator->GenerateSlidingNormals(inPts,inLines,
                                                        inNormals) )
        {
        vtkErrorMacro(<< "No normals for line!\n");
        inNormals->Delete();
        return;
        }
      }
    lineNormalGenerator->Delete();
    }
  this->UpdateProgress(0.10);

  // If varying width, get appropriate info.
  //
  if ( this->VaryWidth && (inScalars=pd->GetScalars()) )
    {
    inScalars->GetRange(range,0);
    }

  newPts = vtkPoints::New();
  newPts->Allocate(numNewPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numNewPts);
  newNormals->SetName("Normals");
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(1,numNewPts));

  //  Create pairs of points along the line that are later connected into a 
  //  triangle strip.
  //
  int abort=0;
  int numLines=inLines->GetNumberOfCells();

  for (inCellId=0, inLines->InitTraversal(); 
       inLines->GetNextCell(npts,pts) && !abort; ++inCellId)
    {
    this->UpdateProgress((float)inCellId/numLines);
    abort = this->GetAbortExecute();

    // Use "averaged" segment to create beveled effect. Watch out 
    // for first and last points.
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
        vtkMath::Normalize(sPrev);
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
        for (i=0; i<3; i++)
          {
          p[i] = pNext[i];
          }
        inPts->GetPoint(pts[j+1],pNext);
        for (i=0; i<3; i++)
          {
          sPrev[i] = sNext[i];
          sNext[i] = pNext[i] - p[i];
          }
        }

      n = inNormals->GetTuple(pts[j]);

      if ( vtkMath::Normalize(sNext) == 0.0 )
        {
        vtkErrorMacro(<<"Coincident points!");
        return;
        }

      for (i=0; i<3; i++)
        {
        s[i] = (sPrev[i] + sNext[i]) / 2.0; //average vector
        }
      vtkMath::Normalize(s);
      
      if ( (BevelAngle = vtkMath::Dot(sNext,sPrev)) > 1.0 )
        {
        BevelAngle = 1.0;
        }
      if ( BevelAngle < -1.0 )
        {
        BevelAngle = -1.0;
        }
      BevelAngle = acos((double)BevelAngle) / 2.0; //(0->90 degrees)
      if ( (BevelAngle = cos(BevelAngle)) == 0.0 )
        {
        BevelAngle = 1.0;
        }

      BevelAngle = this->Width / BevelAngle;

      vtkMath::Cross(s,n,w);
      if ( vtkMath::Normalize(w) == 0.0)
        {
        vtkErrorMacro(<<"Bad normal!");
        return;
        }
      
      if ( inScalars )
        {
        sFactor = 1.0 + ((this->WidthFactor - 1.0) * 
              (inScalars->GetComponent(pts[j],0) - range[0]) / 
                         (range[1]-range[0]));
        }
      for (i=0; i<3; i++)
        {
        s[i] = p[i] + w[i] * BevelAngle * sFactor;
        }
      ptId = newPts->InsertNextPoint(s);
      newNormals->InsertTuple(ptId,n);
      outPD->CopyData(pd,pts[j],ptId);

      for (i=0; i<3; i++)
        {
        s[i] = p[i] - w[i] * BevelAngle * sFactor;
        }
      ptId = newPts->InsertNextPoint(s);
      newNormals->InsertTuple(ptId,n);
      outPD->CopyData(pd,pts[j],ptId);
      }

    // Generate the strip topology
    //
    outCellId = newStrips->InsertNextCell(npts*2);
    for (i=0; i < npts; i++) 
      {//order important for consistent normals
      newStrips->InsertCellPoint(ptOffset+2*i+1);
      newStrips->InsertCellPoint(ptOffset+2*i);
      }
    outCD->CopyData(cd,inCellId,outCellId);

    ptOffset += npts*2;
    } //for this line

  // Update ourselves
  //
  if ( deleteNormals )
    {
    inNormals->Delete();
    }

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetStrips(newStrips);
  newStrips->Delete();

  outPD->SetNormals(newNormals);
  newNormals->Delete();

  output->Squeeze();
}

void vtkRibbonFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Width: " << this->Width << "\n";
  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "VaryWidth: " << (this->VaryWidth ? "On\n" : "Off\n");
  os << indent << "Width Factor: " << this->WidthFactor << "\n";
  os << indent << "Use Default Normal: " << this->UseDefaultNormal << "\n";
  os << indent << "Default Normal: " << "( " << 
        this->DefaultNormal[0] << ", " <<
        this->DefaultNormal[1] << ", " <<
        this->DefaultNormal[2] << " )\n";
}

