/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource.cxx
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
#include "vtkConeSource.h"
#include "vtkFloatArray.h"
#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkConeSource, "1.59");
vtkStandardNewMacro(vtkConeSource);

//----------------------------------------------------------------------------
// Construct with default resolution 6, height 1.0, radius 0.5, and capping
// on.
vtkConeSource::vtkConeSource(int res)
{
  res = (res < 0 ? 0 : res);
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;

  this->Center[0] = 0.0f;
  this->Center[1] = 0.0f;
  this->Center[2] = 0.0f;
  
  this->Direction[0] = 1.0f;
  this->Direction[1] = 0.0f;
  this->Direction[2] = 0.0f;
}

//----------------------------------------------------------------------------
void vtkConeSource::Execute()
{
  float angle;
  int numLines, numPolys, numPts;
  float x[3], xbot;
  int i;
  vtkIdType pts[VTK_CELL_SIZE];
  vtkPoints *newPoints; 
  vtkCellArray *newLines=0;
  vtkCellArray *newPolys=0;
  vtkPolyData *output = this->GetOutput();
  // for streaming
  int piece;
  int numPieces;
  int maxPieces;
  int start, end;
  int createBottom;
  
  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  maxPieces = this->Resolution != 0 ? this->Resolution : 1;
  if (numPieces > maxPieces)
    {
    numPieces = maxPieces;
    }
  if (piece >= maxPieces)
    {
    // Super class should do this for us, 
    // but I put this condition in any way.
    return;
    }
  start = maxPieces * piece / numPieces;
  end = (maxPieces * (piece+1) / numPieces) - 1;
  createBottom = (this->Capping && (start == 0));
  
  vtkDebugMacro("ConeSource Executing");
  
  if ( this->Resolution )
    {
    angle = 2.0*3.141592654/this->Resolution;
    }
  else
    {
    angle = 0.0;
    }

  // Set things up; allocate memory
  //
  switch ( this->Resolution )
  {
  case 0:
    numPts = 2;
    numLines =  1;
    newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numLines,numPts));
    break;
  
  case 1: case 2:
    numPts = 2*this->Resolution + 1;
    numPolys = this->Resolution;
    newPolys = vtkCellArray::New();
    newPolys->Allocate(newPolys->EstimateSize(numPolys,3));
    break;

  default:
    if (createBottom)
      {
      // piece 0 has cap.
      numPts = this->Resolution + 1;
      numPolys = end - start + 2;
      }
    else
      {
      numPts = end - start + 3;
      numPolys = end - start + 2;
      }
    newPolys = vtkCellArray::New();
    newPolys->Allocate(newPolys->EstimateSize(numPolys,this->Resolution));
    break;
  }
  newPoints = vtkPoints::New();
  newPoints->SetDataTypeToFloat(); //used later during transformation
  newPoints->Allocate(numPts);

  // Create cone
  //
  x[0] = this->Height / 2.0; // zero-centered
  x[1] = 0.0;
  x[2] = 0.0;
  pts[0] = newPoints->InsertNextPoint(x);

  xbot = -this->Height / 2.0;

  switch (this->Resolution) 
  {
  case 0:
    x[0] = xbot;
    x[1] = 0.0;
    x[2] = 0.0;
    pts[1] = newPoints->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    break;

  case 2:  // fall through this case to use the code in case 1
    x[0] = xbot;
    x[1] = 0.0;
    x[2] = -this->Radius;
    pts[1] = newPoints->InsertNextPoint(x);
    x[0] = xbot;
    x[1] = 0.0;
    x[2] = this->Radius;
    pts[2] = newPoints->InsertNextPoint(x);
    newPolys->InsertNextCell(3,pts);

  case 1:
    x[0] = xbot;
    x[1] = -this->Radius;
    x[2] = 0.0;
    pts[1] = newPoints->InsertNextPoint(x);
    x[0] = xbot;
    x[1] = this->Radius;
    x[2] = 0.0;
    pts[2] = newPoints->InsertNextPoint(x);
    newPolys->InsertNextCell(3,pts);
    break;

  default: // General case: create Resolution triangles and single cap
    // create the bottom.
    if ( createBottom )
      {
      for (i=0; i < this->Resolution; i++) 
        {
        x[0] = xbot;
        x[1] = this->Radius * cos ((double)i*angle);
        x[2] = this->Radius * sin ((double)i*angle);
        // Reverse the order
        pts[this->Resolution - i - 1] = newPoints->InsertNextPoint(x);
        }
      newPolys->InsertNextCell(this->Resolution,pts);
      }
    
    pts[0] = 0;
    if ( ! createBottom)
      {
      // we need to create the points also
      x[0] = xbot;
      x[1] = this->Radius * cos ((double)start*angle);
      x[2] = this->Radius * sin ((double)start*angle);
      pts[1] = newPoints->InsertNextPoint(x);
      for (i = start; i <= end; ++i)
        {
        x[1] = this->Radius * cos ((double)(i+1)*angle);
        x[2] = this->Radius * sin ((double)(i+1)*angle);
        pts[2] = newPoints->InsertNextPoint(x);
        newPolys->InsertNextCell(3,pts);
        pts[1] = pts[2];
        }
      }
    else
      {
      // bottom and points have already been created.
      for (i=start; i <= end; i++) 
        {
        pts[1] = i+1;
        pts[2] = i+2;
        if (pts[2] > this->Resolution)
          {
          pts[2] = 1;
          }
        newPolys->InsertNextCell(3,pts);
        }
      } // createBottom
    
  } //switch

  // A non-default origin and/or direction requires transformation
  //
  if ( this->Center[0] != 0.0f || this->Center[1] != 0.0f || 
       this->Center[2] != 0.0f || this->Direction[0] != 1.0f || 
       this->Direction[1] != 0.0f || this->Direction[2] != 0.0f )
    {
    vtkTransform *t = vtkTransform::New();
    t->Translate(this->Center[0], this->Center[1], this->Center[2]);
    float vMag = vtkMath::Norm(this->Direction);
    t->RotateWXYZ((float)180.0, this->Direction[0]+vMag/2.0,
                  this->Direction[1]/2.0, this->Direction[2]/2.0);
    float *pts = ((vtkFloatArray *)newPoints->GetData())->GetPointer(0);
    for (i=0; i<numPts; i++, pts+=3)
      {
      t->TransformPoint(pts,pts);
      }
    
    t->Delete();
    }
  
  // Update ourselves
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  if ( newPolys )
    {
    newPolys->Squeeze(); // we may have estimated size; reclaim some space
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  else
    {
    output->SetLines(newLines);
    newLines->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkConeSource::ExecuteInformation()
{
  this->GetOutput()->SetMaximumNumberOfPieces(this->Resolution);
}


//----------------------------------------------------------------------------
void vtkConeSource::SetAngle(float angle)
{
  this->SetRadius (this->Height * tan ((double) angle*vtkMath::DegreesToRadians()));
}

//----------------------------------------------------------------------------
float vtkConeSource::GetAngle()
{
  return atan2 (this->Radius, this->Height) / vtkMath::DegreesToRadians();
}

//----------------------------------------------------------------------------
void vtkConeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Center: (" << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << ")\n";
  os << indent << "Direction: (" << this->Direction[0] << ", " 
     << this->Direction[1] << ", " << this->Direction[2] << ")\n";
}
