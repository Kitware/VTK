/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
//
// Methods for Cone generator
//
#include <math.h>
#include "vtkConeSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkConeSource* vtkConeSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkConeSource");
  if(ret)
    {
    return (vtkConeSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkConeSource;
}




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
  maxPieces = this->Resolution;
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
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
}
