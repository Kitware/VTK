/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSource.cxx
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
#include "vtkPointSource.h"
#include "vtkMath.h"
#include <float.h>
#include <math.h>
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkPointSource* vtkPointSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPointSource");
  if(ret)
    {
    return (vtkPointSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPointSource;
}

//----------------------------------------------------------------------------
vtkPointSource::vtkPointSource(vtkIdType numPts)
{
  this->NumberOfPoints = (numPts > 0 ? numPts : 10);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Radius = 0.5;

  this->Distribution = VTK_POINT_UNIFORM;
}

//----------------------------------------------------------------------------
void vtkPointSource::Execute()
{
  vtkIdType i;
  float theta, rho, cosphi, sinphi, radius;
  float x[3];
  vtkPoints *newPoints;
  vtkCellArray *newVerts;
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<< "Generating random cloud of points...");

  newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfPoints);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(newVerts->EstimateSize(1,this->NumberOfPoints));

  newVerts->InsertNextCell(this->NumberOfPoints);

  if (this->Distribution == VTK_POINT_SHELL)
    {  // only produce points on the surface of the sphere
    for (i=0; i<this->NumberOfPoints; i++)
      {
      cosphi = 1 - 2*vtkMath::Random();
      sinphi = sqrt(1 - cosphi*cosphi);
      radius = this->Radius * sinphi;
      theta = 6.2831853f * vtkMath::Random();
      x[0] = this->Center[0] + radius*cos(theta);
      x[1] = this->Center[1] + radius*sin(theta);
      x[2] = this->Center[2] + this->Radius*cosphi;
      newVerts->InsertCellPoint(newPoints->InsertNextPoint(x));
      }
    }
  else
    { // uniform distribution throughout the sphere volume
    for (i=0; i<this->NumberOfPoints; i++)
      {
      cosphi = 1 - 2*vtkMath::Random();
      sinphi = sqrt(1 - cosphi*cosphi);
      rho = this->Radius*pow(vtkMath::Random(),0.33333333f);
      radius = rho * sinphi;
      theta = 6.2831853f * vtkMath::Random();
      x[0] = this->Center[0] + radius*cos(theta);
      x[1] = this->Center[1] + radius*sin(theta);
      x[2] = this->Center[2] + rho*cosphi;
      newVerts->InsertCellPoint(newPoints->InsertNextPoint(x));
      }
    }
   //
   // Update ourselves and release memory
   //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();
}

//----------------------------------------------------------------------------
void vtkPointSource::ExecuteInformation()
{
}


//----------------------------------------------------------------------------
void vtkPointSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";
  os << indent << "Distribution: " << 
     ((this->Distribution == VTK_POINT_SHELL) ? "Shell\n" : "Uniform\n");
}
