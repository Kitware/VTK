/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDashedStreamLine.cxx
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
#include "vtkDashedStreamLine.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"


//------------------------------------------------------------------------------
vtkDashedStreamLine* vtkDashedStreamLine::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDashedStreamLine");
  if(ret)
    {
    return (vtkDashedStreamLine*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDashedStreamLine;
}




vtkDashedStreamLine::vtkDashedStreamLine()
{
  this->DashFactor = 0.75;
}

void vtkDashedStreamLine::Execute()
{
  vtkStreamPoint *sPrev, *sPtr;
  vtkPoints *newPts;
  vtkVectors *newVectors;
  vtkFloatArray *newScalars=NULL;
  vtkCellArray *newLines;
  int i, ptId, j;
  vtkIdType pts[2];
  float tOffset, x[3], v[3], r, xPrev[3], vPrev[3], scalarPrev;
  float s = 0;
  float xEnd[3], vEnd[3], sEnd;
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  this->SavePointInterval = this->StepLength;
  this->vtkStreamer::Integrate();
  if ( this->NumberOfStreamers <= 0 )
    {
    return;
    }
  //
  //  Convert streamer into lines. Lines may be dashed.
  //
  newPts = vtkPoints::New();
  newPts->Allocate(1000);
  newVectors = vtkVectors::New();
  newVectors->Allocate(1000);
  if ( input->GetPointData()->GetScalars() || this->SpeedScalars )
    {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(1000);
    }
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(2*this->NumberOfStreamers,VTK_CELL_SIZE));
  //
  // Loop over all streamers generating points
  //
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
    {
    if ( this->Streamers[ptId].GetNumberOfPoints() < 2 )
      {
      continue;
      }
    sPrev = this->Streamers[ptId].GetStreamPoint(0);
    sPtr = this->Streamers[ptId].GetStreamPoint(1);
    for (j=0; j<3; j++)
      {
      xPrev[j] = sPrev->x[j];
      vPrev[j] = sPrev->v[j];
      }
    scalarPrev = sPrev->s;

    if ( this->Streamers[ptId].GetNumberOfPoints() == 2 && sPtr->cellId < 0 ) 
      {
      continue;
      }

    tOffset = sPrev->t;

    for ( i=1;
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
      {
//
// Search for end of dash...create end of one dash, beginning of next
//
      while ( tOffset >= sPrev->t && tOffset < sPtr->t )
        {
        r = (tOffset - sPrev->t) / (sPtr->t - sPrev->t);

        for (j=0; j<3; j++)
          {
          x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
          v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
          xEnd[j] = xPrev[j] + this->DashFactor * (x[j] - xPrev[j]);
          vEnd[j] = vPrev[j] + this->DashFactor * (v[j] - vPrev[j]);
          }

        // create this dash
        pts[0] = newPts->InsertNextPoint(x);
        newVectors->InsertVector(pts[0],v);

        pts[1] = newPts->InsertNextPoint(xEnd);
        newVectors->InsertVector(pts[1],vEnd);

        if ( newScalars ) 
          {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          newScalars->InsertTuple(pts[0],&s);
          sEnd = scalarPrev + this->DashFactor * (s - scalarPrev);
          newScalars->InsertTuple(pts[1],&sEnd);
          }

        newLines->InsertNextCell(2,pts);

        for (j=0; j<3; j++)
          {
          xPrev[j] = x[j];
          vPrev[j] = v[j];
          }
        if ( newScalars )
	  {
	  scalarPrev = s;
	  }
        tOffset += this->StepLength;

        } // while
      } //for this streamer
    } //for all streamers
//
// Update ourselves and release memory
//
  vtkDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points, "
               << newLines->GetNumberOfCells() << " lines");

  output->SetPoints(newPts);
  newPts->Delete();

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();

  if ( newScalars )
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }

  output->SetLines(newLines);
  newLines->Delete();

  // Delete the streamers since they are no longer needed
  delete[] this->Streamers;
  this->Streamers = 0;
  this->NumberOfStreamers = 0;

  output->Squeeze();
}

void vtkDashedStreamLine::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStreamLine::PrintSelf(os,indent);

  os << indent << "Dash Factor: " << this->DashFactor << " <<\n";

}


