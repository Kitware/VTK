/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamLine.cxx
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
#include "vtkStreamLine.h"
#include "vtkObjectFactory.h"
#include "vtkPolyLine.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"

//----------------------------------------------------------------------------
vtkStreamLine* vtkStreamLine::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStreamLine");
  if(ret)
    {
    return (vtkStreamLine*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStreamLine;
}

// Construct object with step size set to 1.0.
vtkStreamLine::vtkStreamLine()
{
  this->StepLength = 1.0;
  this->NumberOfStreamers = 0;
}

void vtkStreamLine::Execute()
{
  vtkStreamPoint *sPrev, *sPtr;
  vtkPoints *newPts;
  vtkFloatArray *newVectors;
  vtkFloatArray *newScalars=NULL;
  vtkCellArray *newLines;
  vtkIdType ptId, i, id;
  int j;
  vtkIdList *pts;
  float tOffset, x[3], v[3], s, r;
  float theta;
  vtkPolyLine* lineNormalGenerator = NULL;
  vtkFloatArray* normals = NULL;
  vtkFloatArray* rotation = 0;
  vtkPolyData *output=this->GetOutput();
  vtkFieldData *fd;

  this->SavePointInterval = this->StepLength;
  this->vtkStreamer::Integrate();
  if ( this->NumberOfStreamers <= 0 ) {return;}

  //fd = vtkFieldData::New();
  fd = output->GetPointData()->GetFieldData();

  pts = vtkIdList::New();
  pts->Allocate(2500);

  //
  //  Convert streamer into lines. Lines may be dashed.
  //
  newPts  = vtkPoints::New();
  newPts->Allocate(1000);
  newVectors  = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->Allocate(3000);
  if ( this->Vorticity )
    {
    lineNormalGenerator = vtkPolyLine::New();
    normals = vtkFloatArray::New();
    normals->SetNumberOfComponents(3);
    normals->Allocate(3000);
    rotation = vtkFloatArray::New();
    rotation->SetNumberOfComponents(1);
    rotation->Allocate(1000);
    rotation->SetName("Thetas");
    fd->AddArray(rotation);
    }

  if ( this->GetInput()->GetPointData()->GetScalars() || this->SpeedScalars
    || this->OrientationScalars)
    {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(1000);
    }
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(2*this->NumberOfStreamers,
					    VTK_CELL_SIZE));
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

    if ( this->Streamers[ptId].GetNumberOfPoints() == 2 && sPtr->cellId >= 0 )
      {
      continue;
      }

    tOffset = sPrev->t;

    for ( i=1; 
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
      {
      //
      // Create points for line
      //
      while ( tOffset >= sPrev->t && tOffset < sPtr->t )
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
        newVectors->InsertTuple(id,v);

        if ( newScalars ) 
          {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          newScalars->InsertTuple(id,&s);
          }
	
	if ( this->Vorticity )
	  {
	  // Store the rotation values. Used after all the streamlines
	  // are generated.
	  theta = sPrev->theta + r * (sPtr->theta - sPrev->theta);
	  rotation->InsertTuple(id, &theta);
	  }

        tOffset += this->StepLength;

        } // while
      } //for this streamer

    if ( pts->GetNumberOfIds() > 1 )
      {
      newLines->InsertNextCell(pts);
      pts->Reset();
      }
    } //for all streamers

  vtkDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points, "
               << newLines->GetNumberOfCells() << " lines");

  if (this->Vorticity)
    {
    // Rotate the normal vectors with stream vorticity
    int nPts=newPts->GetNumberOfPoints();
    float normal[3], local1[3], local2[3], length, costheta, sintheta;

    lineNormalGenerator->GenerateSlidingNormals(newPts,newLines,normals);
    
    for(i=0; i<nPts; i++)
      {
      normals->GetTuple(i, normal);
      newVectors->GetTuple(i, v);
      // obtain two unit orthogonal vectors on the plane perpendicular to
      // the streamline
      for(j=0; j<3; j++) { local1[j] = normal[j]; }
      length = vtkMath::Normalize(local1);
      vtkMath::Cross(local1, v, local2);
      vtkMath::Normalize(local2);
      // Rotate the normal with theta
      rotation->GetTuple(i, &theta);
      costheta = cos(theta);
      sintheta = sin(theta);
      for(j=0; j<3; j++)
	{
	normal[j] = length* (costheta*local1[j] + sintheta*local2[j]);
	}
      normals->SetTuple(i, normal);
      }
    output->GetPointData()->SetNormals(normals);
    normals->Delete();
    lineNormalGenerator->Delete();
    rotation->Delete();
    }
    
  output->SetPoints(newPts);
  newPts->Delete();

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();

  if ( newScalars ) 
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }

  pts->Delete();
  output->SetLines(newLines);
  newLines->Delete();

  // Delete the streamers since they are no longer needed
  delete[] this->Streamers;
  this->Streamers = 0;
  this->NumberOfStreamers = 0;

  //output->SetFieldData(fd);
  //fd->Delete();
  
  output->Squeeze();
}

void vtkStreamLine::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStreamer::PrintSelf(os,indent);

  os << indent << "Step Length: " << this->StepLength << "\n";

}


