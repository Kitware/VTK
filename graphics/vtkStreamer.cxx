/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkStreamer.h"
#include "vtkMath.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStreamer* vtkStreamer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStreamer");
  if(ret)
    {
    return (vtkStreamer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStreamer;
}




#define VTK_START_FROM_POSITION 0
#define VTK_START_FROM_LOCATION 1

vtkStreamArray::vtkStreamArray()
{
  this->MaxId = -1; 
  this->Array = new vtkStreamPoint[1000];
  this->Size = 1000;
  this->Extend = 5000;
  this->Direction = VTK_INTEGRATE_FORWARD;
}

vtkStreamPoint *vtkStreamArray::Resize(int sz)
{
  vtkStreamPoint *newArray;
  int newSize;

  if (sz >= this->Size)
    {
    newSize = this->Size + 
      this->Extend*(((sz-this->Size)/this->Extend)+1);
    }
  else
    {
    newSize = sz;
    }

  newArray = new vtkStreamPoint[newSize];

  memcpy(newArray, this->Array,
         (sz < this->Size ? sz : this->Size) * sizeof(vtkStreamPoint));

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

// Construct object to start from position (0,0,0); integrate forward; terminal
// speed 0.0; vorticity computation off; integrations step length 0.2; and
// maximum propagation time 100.0.
vtkStreamer::vtkStreamer()
{
  this->StartFrom = VTK_START_FROM_POSITION;

  this->StartCell = 0;
  this->StartSubId = 0;
  this->StartPCoords[0] = this->StartPCoords[1] = this->StartPCoords[2] = 0.5;
  this->StartPosition[0] = this->StartPosition[1] = this->StartPosition[2] = 0.0;
  this->Streamers = NULL;
  this->MaximumPropagationTime = 100.0;
  this->IntegrationDirection = VTK_INTEGRATE_FORWARD;
  this->IntegrationStepLength = 0.2;
  this->Vorticity = 0;
  this->TerminalSpeed = 0.0;
  this->SpeedScalars = 0;
  this->NumberOfStreamers = 0;
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
}

vtkStreamer::~vtkStreamer()
{
  if ( this->Streamers )
    {
    delete [] this->Streamers;
    }

  this->SetSource(NULL);
  if (this->Threader)
    {
    this->Threader->Delete();
    }
}

void vtkStreamer::SetSource(vtkDataSet *source)
{
  this->vtkProcessObject::SetNthInput(1, source);
}

vtkDataSet *vtkStreamer::GetSource()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[1]);
}

// Specify the start of the streamline in the cell coordinate system. That is,
// cellId and subId (if composite cell), and parametric coordinates.
void vtkStreamer::SetStartLocation(int cellId, int subId, float pcoords[3])
{
  if ( cellId != this->StartCell || subId != this->StartSubId ||
  pcoords[0] !=  this->StartPCoords[0] || 
  pcoords[1] !=  this->StartPCoords[1] || 
  pcoords[2] !=  this->StartPCoords[2] )
    {
    this->Modified();
    this->StartFrom = VTK_START_FROM_LOCATION;

    this->StartCell = cellId;
    this->StartSubId = subId;
    this->StartPCoords[0] = pcoords[0];
    this->StartPCoords[1] = pcoords[1];
    this->StartPCoords[2] = pcoords[2];
    }
}

// Specify the start of the streamline in the cell coordinate system. That is,
// cellId and subId (if composite cell), and parametric coordinates.
void vtkStreamer::SetStartLocation(int cellId, int subId, float r, float s, float t)
{
  float pcoords[3];
  pcoords[0] = r;
  pcoords[1] = s;
  pcoords[2] = t;

  this->SetStartLocation(cellId, subId, pcoords);
}

// Get the starting location of the streamline in the cell coordinate system.
int vtkStreamer::GetStartLocation(int& subId, float pcoords[3])
{
  subId = this->StartSubId;
  pcoords[0] = this->StartPCoords[0];
  pcoords[1] = this->StartPCoords[1];
  pcoords[2] = this->StartPCoords[2];
  return this->StartCell;
}

// Specify the start of the streamline in the global coordinate system. Search
// must be performed to find initial cell to start integration from.
void vtkStreamer::SetStartPosition(float x[3])
{
  if ( x[0] != this->StartPosition[0] || x[1] != this->StartPosition[1] || 
  x[2] != this->StartPosition[2] )
    {
    this->Modified();
    this->StartFrom = VTK_START_FROM_POSITION;

    this->StartPosition[0] = x[0];
    this->StartPosition[1] = x[1];
    this->StartPosition[2] = x[2];
    }
}

// Specify the start of the streamline in the global coordinate system. Search
// must be performed to find initial cell to start integration from.
void vtkStreamer::SetStartPosition(float x, float y, float z)
{
  float pos[3];
  pos[0] = x;
  pos[1] = y;
  pos[2] = z;

  this->SetStartPosition(pos);
}

// Get the start position in global x-y-z coordinates.
float *vtkStreamer::GetStartPosition()
{
  return this->StartPosition;
}

static VTK_THREAD_RETURN_TYPE vtkStreamer_ThreadedIntegrate( void *arg )
{
  vtkStreamer              *self;
  int                      thread_count;
  int                      thread_id;
  vtkStreamArray           *streamer;
  vtkStreamPoint           *sNext, *sPtr;
  int                      i, j, ptId, subId;
  int                      idx, idxNext;
  float                    d, step, dir, vNext[3], p[3];
  float                    *v, xNext[3];
  float                    *w, dist2, tol2;
  vtkDataSet               *input;
  vtkGenericCell           *cell;
  vtkPointData             *pd;
  vtkVectors               *inVectors;
  vtkScalars               *inScalars;
  float                    closestPoint[3];
  vtkVectors               *cellVectors;
  vtkScalars               *cellScalars;
  vtkGenericCell           *gencell = NULL;

  thread_id = ((ThreadInfoStruct *)(arg))->ThreadID;
  thread_count = ((ThreadInfoStruct *)(arg))->NumberOfThreads;
  self = (vtkStreamer *)(((ThreadInfoStruct *)(arg))->UserData);

  input     = self->GetInput();
  pd        = input->GetPointData();
  inVectors = pd->GetVectors();
  inScalars = pd->GetScalars();

  cell      = vtkGenericCell::New();

  cellVectors = vtkVectors::New();
  cellVectors->Allocate(VTK_CELL_SIZE);
  cellScalars = vtkScalars::New();
  cellScalars->Allocate(VTK_CELL_SIZE);

  w = new float[input->GetMaxCellSize()];

  tol2 = input->GetLength()/1000; 
  tol2 = tol2*tol2;

  // For each streamer, integrate in appropriate direction (using RK2)
  // Do only the streamers that this thread should handle.
  for (ptId=0; ptId < self->GetNumberOfStreamers(); ptId++)
    {
    if ( ptId % thread_count == thread_id )
      {
      //get starting step
      streamer = self->GetStreamers() + ptId;
      sPtr = streamer->GetStreamPoint(idx=0);
      if ( sPtr->cellId < 0 )
	{
        continue;
	}

      dir = streamer->Direction;
      input->GetCell(sPtr->cellId, cell);
      cell->EvaluateLocation(sPtr->subId, sPtr->p, xNext, w);
      step = self->GetIntegrationStepLength() * sqrt((double)cell->GetLength2());
      inVectors->GetVectors(cell->PointIds, cellVectors);
      if ( inScalars )
	{
	inScalars->GetScalars(cell->PointIds, cellScalars);
	}

      //integrate until time has been exceeded
      while ( sPtr->cellId >= 0 && sPtr->speed > self->GetTerminalSpeed() &&
	      sPtr->t < self->GetMaximumPropagationTime() )
	{

	//compute updated position using this step (Euler integration)
	//use normalized velocity vector (to keep integration in cell)
	for (i=0; i<3; i++)
	  {
	  xNext[i] = sPtr->x[i] + dir * step * sPtr->v[i] / sPtr->speed;
	  }

	//compute updated position using updated step
	cell->EvaluatePosition(xNext, closestPoint, subId, p, dist2, w);

	//interpolate velocity
	vNext[0] = vNext[1] = vNext[2] = 0.0;
	for (i=0; i < cell->GetNumberOfPoints(); i++)
	  {
	  v = cellVectors->GetVector(i);
	  for (j=0; j < 3; j++)
	    {
	    vNext[j] += v[j] * w[i];
	    }
	  }

	//now compute final position
	for (i=0; i<3; i++)
	  {
	  xNext[i] = sPtr->x[i] +   
	    dir * (step/2.0) * (sPtr->v[i] + vNext[i]) / sPtr->speed;
	  }

	idxNext = streamer->InsertNextStreamPoint();
	sNext = streamer->GetStreamPoint(idxNext);
	sPtr = streamer->GetStreamPoint(idx);

	if ( cell->EvaluatePosition(xNext, closestPoint, sNext->subId, 
				    sNext->p, dist2, w) == 1)
	  { //integration still in cell
	  for (i=0; i<3; i++)
	    {
	    sNext->x[i] = closestPoint[i];
	    }
	  sNext->cellId = sPtr->cellId;
	  sNext->subId = sPtr->subId;
	  }
	else
	  { //integration has passed out of cell
	  if ( gencell == NULL )
	    {
	    gencell = vtkGenericCell::New();
	    }
	  sNext->cellId = input->FindCell(xNext, cell, gencell,
					  sPtr->cellId, tol2, 
					  sNext->subId, sNext->p, w);
	  if ( sNext->cellId >= 0 ) //make sure not out of dataset
	    {
	    for (i=0; i<3; i++)
	      {
	      sNext->x[i] = xNext[i];
	      }
	    input->GetCell(sNext->cellId, cell);
	    inVectors->GetVectors(cell->PointIds, cellVectors);
	    if ( inScalars )
	      {
	      inScalars->GetScalars(cell->PointIds, cellScalars);
	      }
	    step = self->GetIntegrationStepLength() * sqrt((double)cell->GetLength2());
	    }
	  }

	if ( sNext->cellId >= 0 )
	  {
	  cell->EvaluateLocation(sNext->subId, sNext->p, xNext, w);
	  sNext->v[0] = sNext->v[1] = sNext->v[2] = 0.0;
	  for (i=0; i < cell->GetNumberOfPoints(); i++)
	    {
	    v = cellVectors->GetVector(i);
	    for (j=0; j < 3; j++)
	      {
	      sNext->v[j] += v[j] * w[i];
	      }
	    }
	  sNext->speed = vtkMath::Norm(sNext->v);
	  if ( inScalars )
	    {
	    for (sNext->s=0.0, i=0; i < cell->GetNumberOfPoints(); i++)
	      {
	      sNext->s += cellScalars->GetScalar(i) * w[i];
	      }
	    }
	  d = sqrt((double)vtkMath::Distance2BetweenPoints(sPtr->x,sNext->x));
	  sNext->d = sPtr->d + d;
	  sNext->t = sPtr->t + (2.0 * d / (sPtr->speed + sNext->speed));
	  }

	idx = idxNext;
	sPtr = sNext;

	}//for elapsed time
      } //for each streamer
    }

  cell->Delete();

  cellVectors->Delete();
  cellScalars->Delete();

  delete [] w;

  if ( gencell )
    {
    gencell->Delete();
    }

  return VTK_THREAD_RETURN_VALUE;
}

void vtkStreamer::Integrate()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *source = this->GetSource();
  vtkPointData *pd=input->GetPointData();
  vtkScalars *inScalars;
  vtkVectors *inVectors;
  int numSourcePts, idx, idxNext;
  vtkStreamPoint *sNext, *sPtr;
  int i, j, ptId, offset;
  vtkCell *cell;
  float *v, xNext[3];
  float tol2;
  float *w=new float[input->GetMaxCellSize()];
  vtkVectors *cellVectors;
  vtkScalars *cellScalars;
  
  vtkDebugMacro(<<"Generating streamers");
  this->NumberOfStreamers = 0;
  if ( this->Streamers != NULL ) // reexecuting - delete old stuff
    {
    delete [] this->Streamers;
    this->Streamers = NULL;
    }

  if ( ! (inVectors=pd->GetVectors()) )
    {
    vtkErrorMacro(<<"No vector data defined!");
    return;
    }

  cellVectors = vtkVectors::New();
  cellVectors->Allocate(VTK_CELL_SIZE);
  cellScalars = vtkScalars::New();
  cellScalars->Allocate(VTK_CELL_SIZE);
  
  inScalars = pd->GetScalars();
  tol2 = input->GetLength()/1000; 
  tol2 = tol2*tol2;

  //
  // Create starting points
  //
  this->NumberOfStreamers = numSourcePts = offset = 1;
  if ( this->GetSource() )
    {
    this->NumberOfStreamers = numSourcePts = source->GetNumberOfPoints();
    }
 
  if ( this->IntegrationDirection == VTK_INTEGRATE_BOTH_DIRECTIONS )
    {
    offset = 2;
    this->NumberOfStreamers *= 2;
    }

  this->Streamers = new vtkStreamArray[this->NumberOfStreamers];

  if ( this->StartFrom == VTK_START_FROM_POSITION && !this->GetSource() )
    {
    idx = this->Streamers[0].InsertNextStreamPoint();
    sPtr = this->Streamers[0].GetStreamPoint(idx);
    for (i=0; i<3; i++)
      {
      sPtr->x[i] = this->StartPosition[i];
      }
    sPtr->cellId = input->FindCell(this->StartPosition, NULL, -1, 0.0, 
                                   sPtr->subId, sPtr->p, w);
    }

  else if ( this->StartFrom == VTK_START_FROM_LOCATION && !this->GetSource() )
    {
    idx = this->Streamers[0].InsertNextStreamPoint();
    sPtr = this->Streamers[0].GetStreamPoint(idx);
    cell =  input->GetCell(sPtr->cellId);
    cell->EvaluateLocation(sPtr->subId, sPtr->p, sPtr->x, w);
    }

  else //VTK_START_FROM_SOURCE
    {
    for (ptId=0; ptId < numSourcePts; ptId++)
      {
      idx = this->Streamers[offset*ptId].InsertNextStreamPoint();
      sPtr = this->Streamers[offset*ptId].GetStreamPoint(idx);
      source->GetPoint(ptId,sPtr->x);
      sPtr->cellId = input->FindCell(sPtr->x, NULL, -1, tol2,
                                     sPtr->subId, sPtr->p, w);
      }
    }

  // Finish initializing each streamer
  //
  for (idx=0, ptId=0; ptId < numSourcePts; ptId++)
    {
    this->Streamers[offset*ptId].Direction = 1.0;
    sPtr = this->Streamers[offset*ptId].GetStreamPoint(idx);
    sPtr->d = 0.0;
    sPtr->t = 0.0;
    if ( sPtr->cellId >= 0 ) //starting point in dataset
      {
      cell = input->GetCell(sPtr->cellId);
      cell->EvaluateLocation(sPtr->subId, sPtr->p, xNext, w);

      inVectors->GetVectors(cell->PointIds, cellVectors);
      sPtr->v[0]  = sPtr->v[1] = sPtr->v[2] = 0.0;
      for (i=0; i < cell->GetNumberOfPoints(); i++)
        {
        v =  cellVectors->GetVector(i);
        for (j=0; j<3; j++)
	  {
	  sPtr->v[j] += v[j] * w[i];
	  }
        }
      sPtr->speed = vtkMath::Norm(sPtr->v);

      if ( inScalars ) 
        {
        inScalars->GetScalars(cell->PointIds, cellScalars);
        for (sPtr->s=0, i=0; i < cell->GetNumberOfPoints(); i++)
	  {
          sPtr->s += cellScalars->GetScalar(i) * w[i];
	  }
        }
      }

    if ( this->IntegrationDirection == VTK_INTEGRATE_BOTH_DIRECTIONS )
      {
      this->Streamers[offset*ptId+1].Direction = -1.0;
      idxNext = this->Streamers[offset*ptId+1].InsertNextStreamPoint();
      sNext = this->Streamers[offset*ptId+1].GetStreamPoint(idxNext);
      sPtr = this->Streamers[offset*ptId].GetStreamPoint(idx);
      *sNext = *sPtr;
      }
    else if ( this->IntegrationDirection == VTK_INTEGRATE_BACKWARD )
      {
      this->Streamers[offset*ptId].Direction = -1.0;
      }
    } //for each streamer

  // Some data access methods must be called once from a single thread before they
  // can safely be used. Call those now
  vtkGenericCell *gcell = vtkGenericCell::New();
  input->GetCell(0,gcell);
  gcell->Delete();
  
  // Set up and execute the thread
  this->Threader->SetNumberOfThreads( this->NumberOfThreads );
  this->Threader->SetSingleMethod( vtkStreamer_ThreadedIntegrate, (void *)this );
  this->Threader->SingleMethodExecute();

  // Compute vorticity if desired.
  //
  if ( this->Vorticity )
    {
    this->ComputeVorticity();
    }
  //
  // Now create appropriate representation
  //
  if ( this->SpeedScalars )
    {
    for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
      {
      for ( sPtr=this->Streamers[ptId].GetStreamPoint(0), i=0; 
	    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0; 
	    i++, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
        {
        sPtr->s = sPtr->speed;
        }
      }
    }
  delete [] w;
  cellVectors->Delete();
  cellScalars->Delete();
}

void vtkStreamer::ComputeVorticity()
{
}

void vtkStreamer::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  if ( this->StartFrom == VTK_START_FROM_POSITION && !this->GetSource())
    {
    os << indent << "Starting Position: (" << this->StartPosition[0] << ","
       << this->StartPosition[1] << ", " << this->StartPosition[2] << ")\n";
    }
  else if ( this->StartFrom == VTK_START_FROM_LOCATION && !this->GetSource())
    {
    os << indent << "Starting Location:\n\tCell: " << this->StartCell 
       << "\n\tSubId: " << this->StartSubId << "\n\tP.Coordinates: ("
       << this->StartPCoords[0] << ", " 
       << this->StartPCoords[1] << ", " 
       << this->StartPCoords[2] << ")\n";
    }
  else
    {
    os << indent << "Starting Source: " << (void *)this->GetSource() << "\n";
    }

  os << indent << "Maximum Propagation Time: " 
     << this->MaximumPropagationTime << "\n";

  if ( this->IntegrationDirection == VTK_INTEGRATE_FORWARD )
    {
    os << indent << "Integration Direction: FORWARD\n";
    }
  else if ( this->IntegrationDirection == VTK_INTEGRATE_BACKWARD )
    {
    os << indent << "Integration Direction: BACKWARD\n";
    }
  else
    {
    os << indent << "Integration Direction: FORWARD & BACKWARD\n";
    }

  os << indent << "Integration Step Length: " << this->IntegrationStepLength << "\n";

  os << indent << "Vorticity: " << (this->Vorticity ? "On\n" : "Off\n");

  os << indent << "Terminal Speed: " << this->TerminalSpeed << "\n";

  os << indent << "Speed Scalars: " << (this->SpeedScalars ? "On\n" : "Off\n");

  os << indent << "Number Of Streamers: " << this->NumberOfStreamers << "\n";
  os << indent << "Number Of Threads: " << this->NumberOfThreads << "\n";
}


