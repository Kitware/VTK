/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedSphereSource.cxx
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
#include "vtkTexturedSphereSource.h"
#include "vtkPoints.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//----------------------------------------------------------------------------
vtkTexturedSphereSource* vtkTexturedSphereSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTexturedSphereSource");
  if(ret)
    {
    return (vtkTexturedSphereSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTexturedSphereSource;
}

// Construct sphere with radius=0.5 and default resolution 8 in both Phi
// and Theta directions.
vtkTexturedSphereSource::vtkTexturedSphereSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Radius = 0.5;
  this->ThetaResolution = res;
  this->PhiResolution = res;
  this->Theta = 0.0;
  this->Phi = 0.0;
}

void vtkTexturedSphereSource::Execute()
{
  int i, j;
  int numPts;
  int numPolys;
  vtkPoints *newPoints; 
  vtkFloatArray *newNormals;
  vtkFloatArray *newTCoords;
  vtkCellArray *newPolys;
  float x[3], deltaPhi, deltaTheta, phi, theta, radius, norm;
  vtkIdType pts[3];
  vtkPolyData *output = this->GetOutput();
  float tc[2];
  
  //
  // Set things up; allocate memory
  //

  numPts = (this->PhiResolution + 1) * (this->ThetaResolution + 1);
  // creating triangles
  numPolys = this->PhiResolution * 2 * this->ThetaResolution;

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numPts);
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numPts);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,3));
  //
  // Create sphere
  //
  // Create intermediate points
  deltaPhi = vtkMath::Pi() / this->PhiResolution;
  deltaTheta = 2.0 * vtkMath::Pi() / this->ThetaResolution;
  for (i=0; i <= this->ThetaResolution; i++)
    {
    theta = i * deltaTheta;
    tc[0] = theta/(2.0*3.1415926);
    for (j=0; j <= this->PhiResolution; j++)
      {
      phi = j * deltaPhi;
      radius = this->Radius * sin((double)phi);
      x[0] = radius * cos((double)theta);
      x[1] = radius * sin((double)theta);
      x[2] = this->Radius * cos((double)phi);
      newPoints->InsertNextPoint(x);

      if ( (norm = vtkMath::Norm(x)) == 0.0 )
	{
	norm = 1.0;
	}
      x[0] /= norm; x[1] /= norm; x[2] /= norm; 
      newNormals->InsertNextTuple(x);

      tc[1] = 1.0 - phi/3.1415926;
      newTCoords->InsertNextTuple(tc);
      }
    }
  //
  // Generate mesh connectivity
  //
  // bands inbetween poles
  for (i=0; i < this->ThetaResolution; i++)
    {
    for (j=0; j < this->PhiResolution; j++)
      {
      pts[0] = (this->PhiResolution+1)*i + j;
      pts[1] = pts[0] + 1;
      pts[2] = ((this->PhiResolution+1)*(i+1)+j) + 1;
      newPolys->InsertNextCell(3,pts);

      pts[1] = pts[2];
      pts[2] = pts[1] - 1;
      newPolys->InsertNextCell(3,pts);
      }
    }
//
// Update ourselves and release memeory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkTexturedSphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
  os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
  os << indent << "Theta: " << this->Theta << "\n";
  os << indent << "Phi: " << this->Phi << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
}
