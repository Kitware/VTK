/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamPoints.cxx
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
#include "vtkStreamPoints.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStreamPoints* vtkStreamPoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStreamPoints");
  if(ret)
    {
    return (vtkStreamPoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStreamPoints;
}

// Construct object with time increment set to 1.0.
vtkStreamPoints::vtkStreamPoints()
{
  this->TimeIncrement = 1.0;
  this->NumberOfStreamers = 0;
}

void vtkStreamPoints::Execute()
{
  vtkStreamPoint *sPrev, *sPtr;
  vtkPoints *newPts;
  vtkVectors *newVectors;
  vtkScalars *newScalars=NULL;
  vtkCellArray *newVerts;
  vtkIdType i, ptId, id;
  int j;
  float tOffset, x[3], v[3], s, r;
  vtkPolyData *output = this->GetOutput();
  vtkDataSet *input = this->GetInput();
  vtkIdList *pts;

  this->SavePointInterval = this->TimeIncrement;  
  this->vtkStreamer::Integrate();
  if ( this->NumberOfStreamers <= 0 )
    {
    return;
    }
  

  pts = vtkIdList::New();
  pts->Allocate(2500);
  newPts  = vtkPoints::New();
  newPts ->Allocate(1000);
  newVectors  = vtkVectors::New();
  newVectors ->Allocate(1000);
  if ( input->GetPointData()->GetScalars() || this->SpeedScalars )
    {
    newScalars = vtkScalars::New();
    newScalars->Allocate(1000);
    }
  newVerts = vtkCellArray::New();
  newVerts->Allocate(newVerts->EstimateSize(2*this->NumberOfStreamers,VTK_CELL_SIZE));
  
  //
  // Loop over all streamers generating points
  //
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
    {
    // tOffset is the time that the next point will have.
    tOffset = 0.0;

    for ( sPrev=sPtr=this->Streamers[ptId].GetStreamPoint(0), i=0; 
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
      {
      //
      // For each streamer, create points "time increment" apart
      //
      if ( tOffset < sPtr->t )
        {
        while ( tOffset < sPtr->t )
          {
          r = (tOffset - sPrev->t) / (sPtr->t - sPrev->t);

          for (j=0; j<3; j++)
            {
            x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
            v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
            }

          // add point to line
          id = newPts->InsertNextPoint(x);
	  pts->InsertNextId(id);
          newVectors->InsertVector(id,v);

          if ( newScalars ) 
            {
            s = sPrev->s + r * (sPtr->s - sPrev->s);
            newScalars->InsertScalar(id,s);
            }
  
          tOffset += this->TimeIncrement;
          } // while

        } //if points should be created

      } //for this streamer
    if ( pts->GetNumberOfIds() > 1 )
      {
      newVerts->InsertNextCell(pts);
      pts->Reset();
      }
    } //for all streamers
  //
  // Update ourselves
  //
  vtkDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points");

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetVerts(newVerts);
  newVerts->Delete();

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();

  if ( newScalars ) 
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }

  // Delete the streamers since they are no longer needed
  delete[] this->Streamers;
  this->Streamers = 0;
  this->NumberOfStreamers = 0;

  output->Squeeze();
  pts->Delete();
}

void vtkStreamPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStreamer::PrintSelf(os,indent);

  os << indent << "Time Increment: " << this->TimeIncrement << " <<\n";
}


