/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToPlane.cxx
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
#include "vtkTextureMapToPlane.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkTextureMapToPlane* vtkTextureMapToPlane::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTextureMapToPlane");
  if(ret)
    {
    return (vtkTextureMapToPlane*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTextureMapToPlane;
}

// Construct with s,t range=(0,1) and automatic plane generation turned on.
vtkTextureMapToPlane::vtkTextureMapToPlane()
{
  // all zero - indicates that using normal is preferred and automatic is off
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Point1[0] = this->Point1[1] = this->Point1[2] = 0.0;
  this->Point2[0] = this->Point2[1] = this->Point2[2] = 0.0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->SRange[0] = 0.0;
  this->SRange[1] = 1.0;

  this->TRange[0] = 0.0;
  this->TRange[1] = 1.0;

  this->AutomaticPlaneGeneration = 1;
}

void vtkTextureMapToPlane::Execute()
{
  float tcoords[2];
  vtkIdType numPts;
  vtkFloatArray *newTCoords;
  vtkIdType i;
  int j;
  float *bounds;
  float proj, minProj, axis[3], sAxis[3], tAxis[3];
  int dir = 0;
  float s, t, sSf, tSf, *p;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  int abort=0;
  vtkIdType progressInterval;

  vtkDebugMacro(<<"Generating texture coordinates!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numPts=input->GetNumberOfPoints()) < 3 && 
  this->AutomaticPlaneGeneration )
    {
    vtkErrorMacro(<< "Not enough points for automatic plane mapping\n");
    return;
    }

  //  Allocate texture data
  //
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->SetNumberOfTuples(numPts);
  progressInterval = numPts/20 + 1;

  //  Compute least squares plane if on automatic mode; otherwise use
  //  normal specified or plane specified
  //
  if ( this->AutomaticPlaneGeneration && 
       (this->Origin[0] == 0.0 && this->Origin[1] == 0.0 && 
        this->Origin[2] == 0.0 && this->Point1[0] == 0.0 && 
        this->Point1[1] == 0.0 && this->Point1[2] == 0.0) )
    {
    if ( this->AutomaticPlaneGeneration )
      {
      this->ComputeNormal();
      }

    vtkMath::Normalize (this->Normal);

    //  Now project each point onto plane generating s,t texture coordinates
    //
    //  Create local s-t coordinate system.  Need to find the two axes on
    //  the plane and encompassing all the points.  Hence use the bounding
    //  box as a reference.
    //
    for (minProj=1.0, i=0; i<3; i++) 
      {
      axis[0] = axis[1] = axis[2] = 0.0;
      axis[i] = 1.0;
      if ( (proj=fabs(vtkMath::Dot(this->Normal,axis))) < minProj ) 
        {
        minProj = proj;
        dir = i;
        }
      }
    axis[0] = axis[1] = axis[2] = 0.0;
    axis[dir] = 1.0;

    vtkMath::Cross (this->Normal, axis, tAxis);
    vtkMath::Normalize (tAxis);

    vtkMath::Cross (tAxis, this->Normal, sAxis);

    //  Construct projection matrices
    //
    //  Arrange s-t axes so that parametric location of points will fall
    //  between s_range and t_range.  Simplest to do by projecting maximum
    //  corner of bounding box unto plane and backing out scale factors.
    //
    bounds = output->GetBounds();
    for (i=0; i<3; i++)
      {
      axis[i] = bounds[2*i+1] - bounds[2*i];
      }

    s = vtkMath::Dot(sAxis,axis);
    t = vtkMath::Dot(tAxis,axis);

    sSf = (this->SRange[1] - this->SRange[0]) / s;
    tSf = (this->TRange[1] - this->TRange[0]) / t;

    //  Now can loop over all points, computing parametric coordinates.
    //
    for (i=0; i<numPts && !abort; i++) 
      {
      if ( !(i % progressInterval) )
        {
        this->UpdateProgress((float)i/numPts);
        abort = this->GetAbortExecute();
        }

      p = output->GetPoint(i);
      for (j=0; j<3; j++)
        {
        axis[j] = p[j] - bounds[2*j];
        }

      tcoords[0] = this->SRange[0] + vtkMath::Dot(sAxis,axis) * sSf;
      tcoords[1] = this->TRange[0] + vtkMath::Dot(tAxis,axis) * tSf;

      newTCoords->SetTuple(i,tcoords);
      }
    } //compute plane and/or parametric range

  else //use the axes specified
    {
    float num, sDenom, tDenom;

    for ( i=0; i < 3; i++ ) //compute axes
      {
      sAxis[i] = this->Point1[i] - this->Origin[i];
      tAxis[i] = this->Point2[i] - this->Origin[i];
      }

    sDenom = vtkMath::Dot(sAxis,sAxis);
    tDenom = vtkMath::Dot(tAxis,tAxis);

    if ( sDenom == 0.0 || tDenom == 0.0 )
      {
      vtkErrorMacro("Bad plane definition");
      sDenom = tDenom = 1.0;
      }

    // compute s-t coordinates
    for (i=0; i < numPts && !abort; i++) 
      {
      if ( !(i % progressInterval) )
        {
        this->UpdateProgress((float)i/numPts);
        abort = this->GetAbortExecute();
        }
      p = output->GetPoint(i);
      for (j=0; j<3; j++)
        {
        axis[j] = p[j] - this->Origin[j];
        }

      //s-coordinate
      num = sAxis[0]*axis[0] + sAxis[1]*axis[1] + sAxis[2]*axis[2];
      tcoords[0] = num / sDenom;

      //t-coordinate
      num = tAxis[0]*axis[0] + tAxis[1]*axis[1] + tAxis[2]*axis[2];
      tcoords[1] = num / tDenom;

      newTCoords->SetTuple(i,tcoords);
      }
    }

  // Update ourselves
  //
  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

#define VTK_TOLERANCE 1.0e-03

void vtkTextureMapToPlane::ComputeNormal()
{
  vtkDataSet *output = this->GetOutput();
  vtkIdType numPts=output->GetNumberOfPoints();
  float m[9], v[3], *x;
  vtkIdType ptId;
  int dir = 0, i;
  float length, w, *c1, *c2, *c3, det;
  float *bounds;

  //  First thing to do is to get an initial normal and point to define
  //  the plane.  Then, use this information to construct better
  //  matrices.  If problem occurs, then the point and plane becomes the
  //  fallback value.
  //
  //  Get minimum width of bounding box.
  bounds = output->GetBounds();
  length = output->GetLength();

  for (w=length, i=0; i<3; i++)
    {
    this->Normal[i] = 0.0;
    if ( (bounds[2*i+1] - bounds[2*i]) < w ) 
      {
      dir = i;
      w = bounds[2*i+1] - bounds[2*i];
      }
    }

  //  If the bounds is perpendicular to one of the axes, then can
  //  quickly compute normal.
  //
  this->Normal[dir] = 1.0;
  if ( w <= (length*VTK_TOLERANCE) )
    {
    return;
    }

  //  Need to compute least squares approximation.  Depending on major
  //  normal direction (dir), construct matrices appropriately.
  //
  //  Compute 3x3 least squares matrix
  v[0] = v[1] = v[2] = 0.0;
  for (i=0; i<9; i++)
    {
    m[i] = 0.0;
    }

  for (ptId=0; ptId < numPts; ptId++) 
    {
    x = output->GetPoint(ptId);

    v[0] += x[0]*x[2];
    v[1] += x[1]*x[2];
    v[2] += x[2];

    m[0] += x[0]*x[0];
    m[1] += x[0]*x[1];
    m[2] += x[0];

    m[3] += x[0]*x[1];
    m[4] += x[1]*x[1];
    m[5] += x[1];

    m[6] += x[0];
    m[7] += x[1];
    }
  m[8] = numPts;

  //  Solve linear system using Kramers rule
  //
  c1 = m; c2 = m+3; c3 = m+6;
  if ( (det = vtkMath::Determinant3x3 (c1,c2,c3)) <= VTK_TOLERANCE )
    {
    return;
    }

  this->Normal[0] = vtkMath::Determinant3x3 (v,c2,c3) / det;
  this->Normal[1] = vtkMath::Determinant3x3 (c1,v,c3) / det;
  this->Normal[2] = -1.0; // because of the formulation

  return;
}

void vtkTextureMapToPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Origin: (" << this->Origin[0] << ", "
     << this->Origin[1] << ", " << this->Origin[2] << " )\n";

  os << indent << "Axis Point 1: (" << this->Point1[0] << ", "
     << this->Point1[1] << ", " << this->Point1[2] << " )\n";

  os << indent << "Axis Point 2: (" << this->Point2[0] << ", "
     << this->Point2[1] << ", " << this->Point2[2] << " )\n";

  os << indent << "S Range: (" << this->SRange[0] << ", "
                               << this->SRange[1] << ")\n";
  os << indent << "T Range: (" << this->TRange[0] << ", "
                               << this->TRange[1] << ")\n";
  os << indent << "Automatic Normal Generation: " << 
                  (this->AutomaticPlaneGeneration ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", "
                                << this->Normal[1] << ", "
                                << this->Normal[2] << ")\n";
}
