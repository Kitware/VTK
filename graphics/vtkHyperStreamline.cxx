/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperStreamline.cc
  Language:  C++
  Date:      06 Nov 1995
  Version:   1.16


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
#include "vtkHyperStreamline.h"
#include "vtkMath.h"

#define VTK_START_FROM_POSITION 0
#define VTK_START_FROM_LOCATION 1

vtkHyperPoint::vtkHyperPoint()
{
  this->v[0] = this->v0;
  this->v[1] = this->v1;
  this->v[2] = this->v2;
}

vtkHyperPoint& vtkHyperPoint::operator=(const vtkHyperPoint& hp)
{
  int i, j;

  for (i=0; i<3; i++) 
    {
    this->x[i] = hp.x[i];
    this->p[i] = hp.p[i];
    this->w[i] = hp.w[i];
    for (j=0; j<3; j++) v[j][i] = hp.v[j][i];
    }
  this->cellId = hp.cellId;
  this->subId = hp.subId;
  this->s = hp.s;
  this->d = hp.d;

  return *this;
}

vtkHyperArray::vtkHyperArray()
{
  this->MaxId = -1; 
  this->Array = new vtkHyperPoint[1000];
  this->Size = 1000;
  this->Extend = 5000;
  this->Direction = VTK_INTEGRATE_FORWARD;
}

vtkHyperPoint *vtkHyperArray::Resize(int sz)
{
  vtkHyperPoint *newArray;
  int newSize, i;

  if (sz >= this->Size) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  newArray = new vtkHyperPoint[newSize];

  for (i=0; i<sz; i++)
    newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

// Description:
// Construct object with initial starting position (0,0,0); integration step 
// length 0.2; step length 0.01; forward integration; terminal eigenvalue 0.0;
// number of sides 6; radius 0.5; and logarithmic scaling off.
vtkHyperStreamline::vtkHyperStreamline()
{
  this->StartFrom = VTK_START_FROM_POSITION;
  this->StartPosition[0] = this->StartPosition[1] = this->StartPosition[2] = 0.0;

  this->StartCell = 0;
  this->StartSubId = 0;
  this->StartPCoords[0] = this->StartPCoords[1] = this->StartPCoords[2] = 0.5;

  this->Streamers = NULL;

  this->MaximumPropagationDistance = 100.0;
  this->IntegrationStepLength = 0.2;
  this->StepLength = 0.01;
  this->IntegrationDirection = VTK_INTEGRATE_FORWARD;
  this->TerminalEigenvalue = 0.0;
  this->NumberOfSides = 6;
  this->Radius = 0.5;
  this->LogScaling = 0;
  this->IntegrationEigenvector = 0; //Major eigenvector
}

vtkHyperStreamline::~vtkHyperStreamline()
{
  if ( this->Streamers ) delete [] this->Streamers;
}

// Description:
// Specify the start of the hyperstreamline in the cell coordinate system. 
// That is, cellId and subId (if composite cell), and parametric coordinates.
void vtkHyperStreamline::SetStartLocation(int cellId, int subId, float pcoords[3])
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

// Description:
// Specify the start of the hyperstreamline in the cell coordinate system. 
// That is, cellId and subId (if composite cell), and parametric coordinates.
void vtkHyperStreamline::SetStartLocation(int cellId, int subId, float r, float s, float t)
{
  float pcoords[3];
  pcoords[0] = r;
  pcoords[1] = s;
  pcoords[2] = t;

  this->SetStartLocation(cellId, subId, pcoords);
}

// Description:
// Get the starting location of the hyperstreamline in the cell coordinate
// system. Returns the cell that the starting point is in.
int vtkHyperStreamline::GetStartLocation(int& subId, float pcoords[3])
{
  subId = this->StartSubId;
  pcoords[0] = this->StartPCoords[0];
  pcoords[1] = this->StartPCoords[1];
  pcoords[2] = this->StartPCoords[2];
  return this->StartCell;
}

// Description:
// Specify the start of the hyperstreamline in the global coordinate system. 
// Starting from position implies that a search must be performed to find 
// initial cell to start integration from.
void vtkHyperStreamline::SetStartPosition(float x[3])
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

// Description:
// Specify the start of the hyperstreamline in the global coordinate system. 
// Starting from position implies that a search must be performed to find 
// initial cell to start integration from.
void vtkHyperStreamline::SetStartPosition(float x, float y, float z)
{
  float pos[3];
  pos[0] = x;
  pos[1] = y;
  pos[2] = z;

  this->SetStartPosition(pos);
}

// Description:
// Get the start position of the hyperstreamline in global x-y-z coordinates.
float *vtkHyperStreamline::GetStartPosition()
{
  return this->StartPosition;
}

// Description:
// Use the major eigenvector field as the vector field through which to 
// integrate. The major eigenvector is the eigenvector whose corresponding
// eigenvalue is closest to positive infinity.
void vtkHyperStreamline::IntegrateMajorEigenvector()
{
  if ( this->IntegrationEigenvector != 0 )
    {
    this->Modified();
    this->IntegrationEigenvector = 0;
  }
}

// Description:
// Use the major eigenvector field as the vector field through which to 
// integrate. The major eigenvector is the eigenvector whose corresponding
// eigenvalue is between the major and minor eigenvalues.
void vtkHyperStreamline::IntegrateMediumEigenvector()
{
  if ( this->IntegrationEigenvector != 1 )
    {
    this->Modified();
    this->IntegrationEigenvector = 1;
  }
}

// Description:
// Use the major eigenvector field as the vector field through which to 
// integrate. The major eigenvector is the eigenvector whose corresponding
// eigenvalue is closest to negative infinity.
void vtkHyperStreamline::IntegrateMinorEigenvector()
{
  if ( this->IntegrationEigenvector != 2 )
    {
    this->Modified();
    this->IntegrationEigenvector = 2;
  }
}

// Make sure coordinate systems are consistent
static void FixVectors(float **prev, float **current, int iv, int ix, int iy)
{
  float p0[3], p1[3], p2[3];
  float v0[3], v1[3], v2[3];
  float temp[3];
  int i;

  for (i=0; i<3; i++)
    {
    v0[i] = current[i][iv];
    v1[i] = current[i][ix];
    v2[i] = current[i][iy];
    }

  if ( prev == NULL ) //make sure coord system is right handed
    {
    vtkMath::Cross(v0,v1,temp);
    if ( vtkMath::Dot(v2,temp) < 0.0 )
      {
      for (i=0; i<3; i++) current[i][iy] *= -1.0;
      }
    }

  else //make sure vectors consistent from one point to the next
    {
    for (i=0; i<3; i++)
      {
      p0[i] = prev[i][iv];
      p1[i] = prev[i][ix];
      p2[i] = prev[i][iy];
      }
    if ( vtkMath::Dot(p0,v0) < 0.0 )
      {
      for (i=0; i<3; i++) current[i][iv] *= -1.0;
      }
    if ( vtkMath::Dot(p1,v1) < 0.0 )
      {
      for (i=0; i<3; i++) current[i][ix] *= -1.0;
      }
    if ( vtkMath::Dot(p2,v2) < 0.0 )
      {
      for (i=0; i<3; i++) current[i][iy] *= -1.0;
      }
    }
}

void vtkHyperStreamline::Execute()
{
  vtkDataSet *input=this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkScalars *inScalars;
  vtkTensors *inTensors;
  vtkTensor *tensor;
  vtkHyperPoint *sNext, *sPtr;
  int i, j, k, ptId, offset, subId, iv, ix, iy;
  vtkCell *cell;
  float ev[3], xNext[3];
  float d, step, dir, tol2, p[3];
  float *w=new float[input->GetMaxCellSize()], dist2;
  float closestPoint[3];
  float *m[3], *v[3];
  float m0[3], m1[3], m2[3];
  float v0[3], v1[3], v2[3];
  vtkFloatTensors cellTensors(VTK_CELL_SIZE);
  vtkFloatScalars cellScalars(VTK_CELL_SIZE);
  cellTensors.ReferenceCountingOff();
  cellScalars.ReferenceCountingOff();
  // set up working matrices
  v[0] = v0; v[1] = v1; v[2] = v2; 
  m[0] = m0; m[1] = m1; m[2] = m2; 

  vtkDebugMacro(<<"Generating hyperstreamline(s)");
  this->NumberOfStreamers = 0;

  if ( ! (inTensors=pd->GetTensors()) )
    {
    vtkErrorMacro(<<"No tensor data defined!");
    return;
    }

  inScalars = pd->GetScalars();
  tol2 = input->GetLength() / 1000.0;
  tol2 = tol2 * tol2;
  iv = this->IntegrationEigenvector;
  ix = (iv + 1) % 3;
  iy = (iv + 2) % 3;
//
// Create starting points
//
  this->NumberOfStreamers = offset = 1;
 
  if ( this->IntegrationDirection == VTK_INTEGRATE_BOTH_DIRECTIONS )
    {
    offset = 2;
    this->NumberOfStreamers *= 2;
    }

  this->Streamers = new vtkHyperArray[this->NumberOfStreamers];

  if ( this->StartFrom == VTK_START_FROM_POSITION )
    {
    sPtr = this->Streamers[0].InsertNextHyperPoint();
    for (i=0; i<3; i++) sPtr->x[i] = this->StartPosition[i];
    sPtr->cellId = input->FindCell(this->StartPosition, NULL, (-1), 0.0, 
                                   sPtr->subId, sPtr->p, w);
    }

  else //VTK_START_FROM_LOCATION
    {
    sPtr = this->Streamers[0].InsertNextHyperPoint();
    cell =  input->GetCell(sPtr->cellId);
    cell->EvaluateLocation(sPtr->subId, sPtr->p, sPtr->x, w);
    }
//
// Finish initializing each hyperstreamline
//
  this->Streamers[0].Direction = 1.0;
  sPtr = this->Streamers[0].GetHyperPoint(0);
  sPtr->d = 0.0;
  if ( sPtr->cellId >= 0 ) //starting point in dataset
    {
    cell = input->GetCell(sPtr->cellId);
    cell->EvaluateLocation(sPtr->subId, sPtr->p, xNext, w);

    inTensors->GetTensors(cell->PointIds,cellTensors);

    // interpolate tensor, compute eigenfunctions
    for (j=0; j<3; j++) for (i=0; i<3; i++) m[i][j] = 0.0;
    for (k=0; k < cell->GetNumberOfPoints(); k++)
      {
      tensor = cellTensors.GetTensor(k);
      for (j=0; j<3; j++) 
        {
        for (i=0; i<3; i++) 
          {
          m[i][j] += tensor->GetComponent(i,j) * w[k];
          }
        }
      }

    vtkMath::Jacobi(m, sPtr->w, sPtr->v);
    FixVectors(NULL, sPtr->v, iv, ix, iy);

    if ( inScalars ) 
      {
      inScalars->GetScalars(cell->PointIds,cellScalars);
      for (sPtr->s=0, i=0; i < cell->GetNumberOfPoints(); i++)
        sPtr->s += cellScalars.GetScalar(i) * w[i];
      }

    if ( this->IntegrationDirection == VTK_INTEGRATE_BOTH_DIRECTIONS )
      {
      this->Streamers[1].Direction = -1.0;
      sNext = this->Streamers[1].InsertNextHyperPoint();
      *sNext = *sPtr;
      }
    else if ( this->IntegrationDirection == VTK_INTEGRATE_BACKWARD )
      {
      this->Streamers[0].Direction = -1.0;
      }
    } //for hyperstreamline in dataset
//
// For each hyperstreamline, integrate in appropriate direction (using RK2).
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
    inTensors->GetTensors(cell->PointIds,cellTensors);
    if ( inScalars ) inScalars->GetScalars(cell->PointIds,cellScalars);

    //integrate until distance has been exceeded
    while ( sPtr->cellId >= 0 && fabs(sPtr->w[0]) > this->TerminalEigenvalue &&
    sPtr->d < this->MaximumPropagationDistance )
      {

      //compute updated position using this step (Euler integration)
      for (i=0; i<3; i++)
        xNext[i] = sPtr->x[i] + dir * step * sPtr->v[i][iv];

      //compute updated position using updated step
      cell->EvaluatePosition(xNext, closestPoint, subId, p, dist2, w);

      //interpolate tensor
      for (j=0; j<3; j++) for (i=0; i<3; i++) m[i][j] = 0.0;
      for (k=0; k < cell->GetNumberOfPoints(); k++)
        {
        tensor = cellTensors.GetTensor(k);
        for (j=0; j<3; j++) 
          {
          for (i=0; i<3; i++) 
            {
            m[i][j] += tensor->GetComponent(i,j) * w[k];
            }
          }
        }

      vtkMath::Jacobi(m, ev, v);
      FixVectors(sPtr->v, v, iv, ix, iy);

      //now compute final position
      for (i=0; i<3; i++)
        xNext[i] = sPtr->x[i] + 
                   dir * (step/2.0) * (sPtr->v[i][iv] + v[i][iv]);

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
        sNext->cellId = input->FindCell(xNext, cell, sPtr->cellId, tol2, 
                                        sNext->subId, sNext->p, w);
        if ( sNext->cellId >= 0 ) //make sure not out of dataset
          {
          for (i=0; i<3; i++) sNext->x[i] = xNext[i];
          cell = input->GetCell(sNext->cellId);
          inTensors->GetTensors(cell->PointIds,cellTensors);
          if ( inScalars ) inScalars->GetScalars(cell->PointIds,cellScalars);
          step = this->IntegrationStepLength * sqrt((double)cell->GetLength2());
          }
        }

      if ( sNext->cellId >= 0 )
        {
        cell->EvaluateLocation(sNext->subId, sNext->p, xNext, w);
        for (j=0; j<3; j++) for (i=0; i<3; i++) m[i][j] = 0.0;
        for (k=0; k < cell->GetNumberOfPoints(); k++)
          {
          tensor = cellTensors.GetTensor(k);
          for (j=0; j<3; j++) 
            {
            for (i=0; i<3; i++) 
              {
              m[i][j] += tensor->GetComponent(i,j) * w[k];
              }
            }
          }

        vtkMath::Jacobi(m, sNext->w, sNext->v);
        FixVectors(sPtr->v, sNext->v, iv, ix, iy);

        if ( inScalars )
          for (sNext->s=0.0, i=0; i < cell->GetNumberOfPoints(); i++)
            sNext->s += cellScalars.GetScalar(i) * w[i];

        d = sqrt((double)vtkMath::Distance2BetweenPoints(sPtr->x,sNext->x));
        sNext->d = sPtr->d + d;
        }

      sPtr = sNext;

      }//for elapsed time

    } //for each hyperstreamline

  this->BuildTube();

  delete [] w;
}

void vtkHyperStreamline::BuildTube()
{
  vtkHyperPoint *sPrev, *sPtr;
  vtkFloatPoints *newPts;
  vtkFloatVectors *newVectors;
  vtkFloatNormals *newNormals;
  vtkFloatScalars *newScalars=NULL;
  vtkCellArray *newStrips;
  int i, ptId, j, id, k, i1, i2;
  int npts, ptOffset=0;
  float dOffset, x[3], v[3], s, r, r1[3], r2[3], stepLength;
  float xT[3], sFactor, normal[3], w[3];
  float theta=2.0*vtkMath::Pi()/this->NumberOfSides;
  vtkPointData *outPD;
  vtkDataSet *input=this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  int iv, ix, iy, numIntPts;
//
// Initialize
//
  vtkDebugMacro(<<"Creating hyperstreamline tube");
  if ( this->NumberOfStreamers <= 0 ) return;

  stepLength = input->GetLength() * this->StepLength;
  outPD = output->GetPointData();

  iv = this->IntegrationEigenvector;
  ix = (iv+1) % 3;
  iy = (iv+2) % 3;
//
// Allocate
//
  newPts  = vtkFloatPoints::New();
  newPts ->Allocate(2500);
  if ( this->Input->GetPointData()->GetScalars() )
    {
    newScalars = vtkFloatScalars::New();
    newScalars->Allocate(2500);
    }
  newVectors = vtkFloatVectors::New();
  newVectors->Allocate(2500);
  newNormals = vtkFloatNormals::New();
  newNormals->Allocate(2500);
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(3*this->NumberOfStreamers,
                                              VTK_CELL_SIZE));
//
// Loop over all hyperstreamlines generating points
//
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
    {
    if ( (numIntPts=this->Streamers[ptId].GetNumberOfPoints()) < 2 ) continue;
    sPrev = this->Streamers[ptId].GetHyperPoint(0);
    sPtr = this->Streamers[ptId].GetHyperPoint(1);

    // compute scale factor
    i = (sPrev->w[ix] > sPrev->w[iy] ? ix : iy);
    if ( sPrev->w[i] == 0.0 ) sFactor = 1.0;
    else sFactor = this->Radius / sPrev->w[i];

    if ( numIntPts == 2 && sPtr->cellId < 0 ) continue;

    dOffset = sPrev->d;

    for ( npts=0, i=1; i < numIntPts && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetHyperPoint(i) )
      {
//
// Bracket steps and construct tube points
//
      while ( dOffset >= sPrev->d && dOffset < sPtr->d )
        {
        r = (dOffset - sPrev->d) / (sPtr->d - sPrev->d);

        for (j=0; j<3; j++) //compute point in center of tube
          {
          x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
          v[j] = sPrev->v[j][iv] + r * (sPtr->v[j][iv] - sPrev->v[j][iv]);
          r1[j] = sPrev->v[j][ix] + r * (sPtr->v[j][ix] - sPrev->v[j][ix]);
          r2[j] = sPrev->v[j][iy] + r * (sPtr->v[j][iy] - sPrev->v[j][iy]);
          w[j] = sPrev->w[j] + r * (sPtr->w[j] - sPrev->w[j]);
          }

        // construct points around tube
        for (k=0; k < this->NumberOfSides; k++)
          {
          for (j=0; j<3; j++) 
            {
            normal[j] = w[ix]*r1[j]*cos((double)k*theta) + 
                        w[iy]*r2[j]*sin((double)k*theta);
            xT[j] = x[j] + sFactor * normal[j];
            }
          id = newPts->InsertNextPoint(xT);
          newVectors->InsertVector(id,v);
          vtkMath::Normalize(normal);
          newNormals->InsertNormal(id,normal);
          }

        if ( newScalars ) //add scalars around tube
          {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          for (k=0; k<this->NumberOfSides; k++)
            newScalars->InsertNextScalar(s);
          }

        npts++;
        dOffset += stepLength;

        } //while
      } //for this hyperstreamline

    //
    // Generate the strips for this hyperstreamline
    //
    for (k=0; k<this->NumberOfSides; k++)
      {
      i1 = (k+1) % this->NumberOfSides;
      newStrips->InsertNextCell(npts*2);
      for (i=0; i < npts; i++) 
        {
        //make sure strip definition consistent with normals
        if (this->Streamers[ptId].Direction > 0.0) i2 = i*this->NumberOfSides;
        else i2 = (npts - i - 1) * this->NumberOfSides;
        newStrips->InsertCellPoint(ptOffset+i2+k);
        newStrips->InsertCellPoint(ptOffset+i2+i1);
        }
      }//for all tube sides

    ptOffset += this->NumberOfSides*npts;

    } //for all hyperstreamlines
//
// Update ourselves
//
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetStrips(newStrips);
  newStrips->Delete();

  if ( newScalars )
    {
    outPD->SetScalars(newScalars);
    newScalars->Delete();
    }

  outPD->SetNormals(newNormals);
  newNormals->Delete();

  outPD->SetVectors(newVectors);
  newVectors->Delete();

  output->Squeeze();
}

void vtkHyperStreamline::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  if ( this->StartFrom == VTK_START_FROM_POSITION )
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

  os << indent << "Maximum Propagation Distance: " 
     << this->MaximumPropagationDistance << "\n";

  if ( this->IntegrationDirection == VTK_INTEGRATE_FORWARD )
    os << indent << "Integration Direction: FORWARD\n";
  else if ( this->IntegrationDirection == VTK_INTEGRATE_BACKWARD )
    os << indent << "Integration Direction: BACKWARD\n";
  else
    os << indent << "Integration Direction: FORWARD & BACKWARD\n";

  os << indent << "Integration Step Length: " << this->IntegrationStepLength << "\n";
  os << indent << "Step Length: " << this->StepLength << "\n";

  os << indent << "Terminal Eigenvalue: " << this->TerminalEigenvalue << "\n";

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";
  os << indent << "Logarithmic Scaling: " << (this->LogScaling ? "On\n" : "Off\n");
  
  if ( this->IntegrationEigenvector == 0 )
    os << indent << "Integrate Along Major Eigenvector\n";
  else if ( this->IntegrationEigenvector == 1 )
    os << indent << "Integrate Along Medium Eigenvector\n";
  else
    os << indent << "Integrate Along Minor Eigenvector\n";
}


