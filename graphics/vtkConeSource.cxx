/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource.cc
  Language:  C++
  Date:      09 Oct 1995
  Version:   1.22


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
//
// Methods for Cone generator
//
#include <math.h>
#include "vtkConeSource.h"
#include "vtkMath.h"

// Description:
// Construct with default resolution 6, height 1.0, radius 0.5, and capping
// on.
vtkConeSource::vtkConeSource(int res)
{
  res = (res < 0 ? 0 : res);
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;
}

void vtkConeSource::Execute()
{
  float angle;
  int numLines, numPolys, numPts;
  float x[3], xbot;
  int i;
  int pts[VTK_CELL_SIZE];
  vtkFloatPoints *newPoints; 
  vtkCellArray *newLines=0;
  vtkCellArray *newPolys=0;
  vtkPolyData *output = this->GetOutput();
  
  if ( this->Resolution ) angle = 2.0*3.141592654/this->Resolution;
  //
  // Set things up; allocate memory
  //

  switch ( this->Resolution )
  {
  case 0:
    numPts = 2;
    numLines =  1;
    newLines = new vtkCellArray;
    newLines->Allocate(newLines->EstimateSize(numLines,numPts));
    break;
  
  case 1: case 2:
    numPts = 2*this->Resolution + 1;
    numPolys = this->Resolution;
    newPolys = new vtkCellArray;
    newPolys->Allocate(newPolys->EstimateSize(numPolys,3));
    break;

  default:
    numPts = this->Resolution + 1;
    numPolys = this->Resolution + 1;
    newPolys = new vtkCellArray;
    newPolys->Allocate(newPolys->EstimateSize(numPolys,this->Resolution));
    break;
  }
  newPoints = new vtkFloatPoints(numPts);
//
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

    for (i=0; i<this->Resolution; i++) 
      {
      x[0] = xbot;
      x[1] = this->Radius * cos ((double)i*angle);
      x[2] = this->Radius * sin ((double)i*angle);
      pts[1] = newPoints->InsertNextPoint(x);
      pts[2] = (pts[1] % this->Resolution) + 1;
      newPolys->InsertNextCell(3,pts);
      }
//
// If capping, create last polygon
//
    if ( this->Capping )
      {
      for (i=0; i<this->Resolution; i++) pts[this->Resolution - i - 1] = i+1;
      newPolys->InsertNextCell(this->Resolution,pts);
      }
  } //switch
//
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

void vtkConeSource::SetAngle(float angle)
{
  this->SetRadius (this->Height * tan ((double) angle*vtkMath::DegreesToRadians()));
}

float vtkConeSource::GetAngle()
{
  return atan2 (this->Radius, this->Height) / vtkMath::DegreesToRadians();
}

void vtkConeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
}
