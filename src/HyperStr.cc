/*=========================================================================

  Program:   Visualization Library
  Module:    HyperStr.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "HyperStr.hh"
#include "vtkMath.hh"

vtkHyperArray::vtkHyperArray()
{
  this->MaxId = -1; 
  this->Array = new vtkHyperPoint[1000];
  this->Size = 1000;
  this->Extend = 5000;
  this->Direction = INTEGRATE_FORWARD;
}

vtkHyperPoint *vtkHyperArray::Resize(int sz)
{
  vtkHyperPoint *newArray;
  int newSize;

  if (sz >= this->Size) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  newArray = new vtkHyperPoint[newSize];

  memcpy(newArray, this->Array,
         (sz < this->Size ? sz : this->Size) * sizeof(vtkHyperPoint));

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}


vtkHyperStreamline::vtkHyperStreamline()
{
  this->StartFrom = START_FROM_POSITION;

  this->StartCell = 0;
  this->StartSubId = 0;
  this->StartPCoords[0] = this->StartPCoords[1] = this->StartPCoords[2] = 0.5;
  this->StartPosition[0] = this->StartPosition[1] = this->StartPosition[2] = 0.0;
  this->Streamers = NULL;
  this->MaximumPropagationTime = 100.0;
  this->IntegrationDirection = INTEGRATE_FORWARD;
  this->IntegrationStepLength = 0.2;
  this->TerminalSpeed = 0.0;
}

// Description:
// Specify the start of the streamline in the cell coordinate system. That is,
// cellId and subId (if composite cell), and parametric coordinates.
void vtkHyperStreamline::SetStartLocation(int cellId, int subId, float pcoords[3])
{
  if ( cellId != this->StartCell || subId != this->StartSubId ||
  pcoords[0] !=  this->StartPCoords[0] || 
  pcoords[1] !=  this->StartPCoords[1] || 
  pcoords[2] !=  this->StartPCoords[2] )
    {
    this->Modified();
    this->StartFrom = START_FROM_LOCATION;

    this->StartCell = cellId;
    this->StartSubId = subId;
    this->StartPCoords[0] = pcoords[0];
    this->StartPCoords[1] = pcoords[1];
    this->StartPCoords[2] = pcoords[2];
    }
}

// Description:
// Specify the start of the streamline in the cell coordinate system. That is,
// cellId and subId (if composite cell), and parametric coordinates.
void vtkHyperStreamline::SetStartLocation(int cellId, int subId, float r, float s, float t)
{
  float pcoords[3];
  pcoords[0] = r;
  pcoords[1] = s;
  pcoords[2] = t;

  this->SetStartLocation(cellId, subId, pcoords);
}

// Description:
// Get the starting location of the streamline in the cell corrdinate system.
int vtkHyperStreamline::GetStartLocation(int& subId, float pcoords[3])
{
  subId = this->StartSubId;
  pcoords[0] = this->StartPCoords[0];
  pcoords[1] = this->StartPCoords[1];
  pcoords[2] = this->StartPCoords[2];
  return this->StartCell;
}

// Description:
// Specify the start of the streamline in the global coordinate system. Search
// must be performed to find initial cell to strart integration from.
void vtkHyperStreamline::SetStartPosition(float x[3])
{
  if ( x[0] != this->StartPosition[0] || x[1] != this->StartPosition[1] || 
  x[2] != this->StartPosition[2] )
    {
    this->Modified();
    this->StartFrom = START_FROM_POSITION;

    this->StartPosition[0] = x[0];
    this->StartPosition[1] = x[1];
    this->StartPosition[2] = x[2];
    }
}

// Description:
// Specify the start of the streamline in the global coordinate system. Search
// must be performed to find initial cell to strart integration from.
void vtkHyperStreamline::SetStartPosition(float x, float y, float z)
{
  float pos[3];
  pos[0] = x;
  pos[1] = y;
  pos[2] = z;

  this->SetStartPosition(pos);
}

// Description:
// Get the start position in global x-y-z coordinates.
float *vtkHyperStreamline::GetStartPosition()
{
  return this->StartPosition;
}

void vtkHyperStreamline::Execute()
{
  vtkDataSet *input=this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkScalars *inScalars;
  vtkVectors *inVectors;
  vtkHyperPoint *sNext, *sPtr;
  int i, j, ptId, offset, numSteps, subId;
  vtkCell *cell;
  vtkFloatVectors cellVectors(MAX_CELL_SIZE);
  vtkFloatScalars cellScalars(MAX_CELL_SIZE);
  float *v, x[3], xNext[3];
  vtkMath math;
  float d, step, dir, vNext[3], tol2, p[3];
  float w[MAX_CELL_SIZE], dist2;
  float closestPoint[3], stepLength;
  
  vtkDebugMacro(<<"Generating streamers");
  this->Initialize();
  this->NumberOfStreamers = 0;

  if ( ! (inVectors=pd->GetVectors()) )
    {
    vtkErrorMacro(<<"No vector data defined!");
    return;
    }

  inScalars = pd->GetScalars();
  tol2 = input->GetLength()/1000; tol2 = tol2*tol2;
//
// Create starting points
//
  this->NumberOfStreamers = offset = 1;
 
  if ( this->IntegrationDirection == INTEGRATE_BOTH_DIRECTIONS )
    {
    offset = 2;
    this->NumberOfStreamers *= 2;
    }

  this->Streamers = new vtkHyperArray[this->NumberOfStreamers];

  if ( this->StartFrom == START_FROM_POSITION )
    {
    sPtr = this->Streamers[0].InsertNextHyperPoint();
    for (i=0; i<3; i++) sPtr->x[i] = this->StartPosition[i];
    sPtr->cellId = input->FindCell(this->StartPosition, NULL, 0.0, 
                                   sPtr->subId, sPtr->p, w);
    }

  else //START_FROM_LOCATION
    {
    sPtr = this->Streamers[0].InsertNextHyperPoint();
    cell =  input->GetCell(sPtr->cellId);
    cell->EvaluateLocation(sPtr->subId, sPtr->p, sPtr->x, w);
    }
//
// Finish initializing each streamer
//
  this->Streamers[0].Direction = 1.0;
  sPtr = this->Streamers[0].GetHyperPoint(0);
  sPtr->d = 0.0;
  sPtr->t = 0.0;
  if ( sPtr->cellId >= 0 ) //starting point in dataset
    {
    cell = input->GetCell(sPtr->cellId);
    cell->EvaluateLocation(sPtr->subId, sPtr->p, xNext, w);

    inVectors->GetVectors(cell->PointIds,cellVectors);
    sPtr->v[0]  = sPtr->v[1] = sPtr->v[2] = 0.0;
    for (i=0; i < cell->GetNumberOfPoints(); i++)
      {
      v =  cellVectors.GetVector(i);
      for (j=0; j<3; j++) sPtr->v[j] += v[j] * w[i];
      }
    sPtr->speed = math.Norm(sPtr->v);

    if ( this->IntegrationDirection == INTEGRATE_BOTH_DIRECTIONS )
      {
      this->Streamers[1].Direction = -1.0;
      sNext = this->Streamers[1].InsertNextHyperPoint();
      *sNext = *sPtr;
      }
    } //for each streamer
//
// For each streamer, integrate in appropriate direction (using RK2)
//
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
    {
    //get starting step
    sPtr = this->Streamers[ptId].GetHyperPoint(0);
    if ( sPtr->cellId < 0 ) continue;

    dir = this->Streamers[ptId].Direction;
    cell = input->GetCell(sPtr->cellId);
    cell->EvaluateLocation(sPtr->subId, sPtr->p, xNext, w);
    step = this->IntegrationStepLength * sqrt((double)cell->GetLength2());
    inVectors->GetVectors(cell->PointIds,cellVectors);
    if ( inScalars ) inScalars->GetScalars(cell->PointIds,cellScalars);

    //integrate until time has been exceeded
    while ( sPtr->cellId >= 0 && sPtr->speed > this->TerminalSpeed &&
    sPtr->t < this->MaximumPropagationTime )
      {

      //compute updated position using this step (Euler integration)
      //use normalized velocity vector (to keep integration in cell)
      for (i=0; i<3; i++)
        xNext[i] = sPtr->x[i] + dir * step * sPtr->v[i] / sPtr->speed;

      //compute updated position using updated step
      cell->EvaluatePosition(xNext, closestPoint, subId, p, dist2, w);

      //interpolate velocity
      vNext[0] = vNext[1] = vNext[2] = 0.0;
      for (i=0; i < cell->GetNumberOfPoints(); i++)
        {
        v = cellVectors.GetVector(i);
        for (j=0; j < 3; j++) vNext[j] += v[j] * w[i];
        }

      //now compute final position
      for (i=0; i<3; i++)
        xNext[i] = sPtr->x[i] +   
                   dir * (step/2.0) * (sPtr->v[i] + vNext[i]) / sPtr->speed;

      sNext = this->Streamers[ptId].InsertNextHyperPoint();

      if ( cell->EvaluatePosition(xNext, closestPoint, sNext->subId, 
      sNext->p, dist2, w) )
        { //integration still in cell
        for (i=0; i<3; i++) sNext->x[i] = closestPoint[i];
        sNext->cellId = sPtr->cellId;
        sNext->subId = sPtr->subId;
        }
      else
        { //integration has passed out of cell
        sNext->cellId = input->FindCell(xNext, cell, tol2, 
                                        sNext->subId, sNext->p, w);
        if ( sNext->cellId >= 0 ) //make sure not out of dataset
          {
          for (i=0; i<3; i++) sNext->x[i] = xNext[i];
          cell = input->GetCell(sNext->cellId);
          inVectors->GetVectors(cell->PointIds,cellVectors);
          if ( inScalars ) inScalars->GetScalars(cell->PointIds,cellScalars);
          step = this->IntegrationStepLength * sqrt((double)cell->GetLength2());
          }
        }

      if ( sNext->cellId >= 0 )
        {
        cell->EvaluateLocation(sNext->subId, sNext->p, xNext, w);
        sNext->v[0] = sNext->v[1] = sNext->v[2] = 0.0;
        for (i=0; i < cell->GetNumberOfPoints(); i++)
          {
          v = cellVectors.GetVector(i);
          for (j=0; j < 3; j++) sNext->v[j] += v[j] * w[i];
          }
        sNext->speed = math.Norm(sNext->v);
        if ( inScalars )
          for (sNext->s=0.0, i=0; i < cell->GetNumberOfPoints(); i++)
            sNext->s += cellScalars.GetScalar(i) * w[i];

        d = sqrt((double)math.Distance2BetweenPoints(sPtr->x,sNext->x));
        sNext->d = sPtr->d + d;
        sNext->t = sPtr->t + (2.0 * d / (sPtr->speed + sNext->speed));
        }

      sPtr = sNext;

      }//for elapsed time

    } //for each streamer

  this->BuildTube();
}

void vtkHyperStreamline::BuildTube()
{
}

void vtkHyperStreamline::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  if ( this->StartFrom == START_FROM_POSITION )
    {
    os << indent << "Starting Position: (" << this->StartPosition[0] << ","
       << this->StartPosition[1] << ", " << this->StartPosition[2] << ")\n";
    }
  else 
    {
    os << indent << "Starting Location:\n\tCell: " << this->StartCell 
       << "\n\tSubId: " << this->StartSubId << "\n\tP.Coordinates: ("
       << this->StartPCoords[0] << ", " 
       << this->StartPCoords[1] << ", " 
       << this->StartPCoords[2] << ")\n";
    }

  os << indent << "Maximum Propagation Time: " 
     << this->MaximumPropagationTime << "\n";

  if ( this->IntegrationStepLength == INTEGRATE_FORWARD )
    os << indent << "Integration Direction: FORWARD\n";
  else if ( this->IntegrationStepLength == INTEGRATE_BACKWARD )
    os << indent << "Integration Direction: BACKWARD\n";
  else
    os << indent << "Integration Direction: FORWARD & BACKWARD\n";

  os << indent << "Integration Step Length: " << this->IntegrationStepLength << "\n";

  os << indent << "Terminal Speed: " << this->TerminalSpeed << "\n";
}


