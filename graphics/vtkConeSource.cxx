/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkUnstructuredInformation.h"
#include "vtkMath.h"

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
}

//----------------------------------------------------------------------------
void vtkConeSource::Execute()
{
  float angle;
  int numLines, numPolys, numPts;
  float x[3], xbot;
  int i;
  int pts[VTK_CELL_SIZE];
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
  maxPieces = output->GetUnstructuredInformation()->GetMaximumNumberOfPieces();
  start = maxPieces * piece / numPieces;
  end = (maxPieces * (piece+1) / numPieces) - 1;
  createBottom = (this->Capping && (start == 0));
  
  vtkDebugMacro("ConeSource Executing");
  
  if ( this->Resolution )
    {
    angle = 2.0*3.141592654/this->Resolution;
    }
  //
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
  newPoints->Allocate(numPts);
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

//----------------------------------------------------------------------------
void vtkConeSource::ExecuteInformation()
{
  vtkUnstructuredInformation *info;
  int numTris, numPts;
  unsigned long size;
  
  numPts = this->Resolution + 1;
  numTris = this->Resolution;
  size = numPts * 3 * sizeof(float);
  size += numTris * 4 * sizeof(int);
  // one more polygon if capping
  if (this->Capping)
    {
    size += (this->Resolution + 1) * sizeof(int);
    }
  
  // convert to kilobytes
  size = (size / 1000) + 1;
  
  
  info = this->GetOutput()->GetUnstructuredInformation();
  info->SetEstimatedWholeMemorySize(size);
  if (this->Resolution < 3)
    {
    info->SetMaximumNumberOfPieces(1);
    }
  else
    {
    info->SetMaximumNumberOfPieces(this->Resolution);
    }  
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
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
}
