/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.cxx
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
#include "vtkDiskSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkDiskSource* vtkDiskSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDiskSource");
  if(ret)
    {
    return (vtkDiskSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDiskSource;
}




vtkDiskSource::vtkDiskSource()
{
  this->InnerRadius = 0.25;
  this->OuterRadius = 0.5;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;
}

void vtkDiskSource::Execute()
{
  int numPolys, numPts;
  float x[3];
  int i, j;
  vtkIdType pts[4];
  float theta, deltaRadius;
  float cosTheta, sinTheta;
  vtkPoints *newPoints; 
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
  //
  // Set things up; allocate memory
  //

  numPts = (this->RadialResolution + 1) * 
           (this->CircumferentialResolution + 1);
  numPolys = this->RadialResolution * this->CircumferentialResolution;
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Create disk
//
  theta = 2.0 * vtkMath::Pi() / ((float)this->CircumferentialResolution);
  deltaRadius = (this->OuterRadius - this->InnerRadius) / 
                       ((float)this->RadialResolution);

  for (i=0; i<=this->CircumferentialResolution; i++) 
    {
    cosTheta = cos((double)i*theta);
    sinTheta = sin((double)i*theta);
    for (j=0; j <= this->RadialResolution; j++)
      {
      x[0] = (this->InnerRadius + j*deltaRadius) * cosTheta;
      x[1] = (this->InnerRadius + j*deltaRadius) * sinTheta;
      x[2] = 0.0;
      newPoints->InsertNextPoint(x);
      }
    }
//
//  Create connectivity
//
    for (i=0; i < this->CircumferentialResolution; i++) 
      {
      for (j=0; j < this->RadialResolution; j++) 
        {
        pts[0] = i*(this->RadialResolution+1) + j;
        pts[1] = pts[0] + 1;
        pts[2] = pts[1] + this->RadialResolution + 1;
        pts[3] = pts[2] - 1;
        newPolys->InsertNextCell(4,pts);
        }
      }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkDiskSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
}
