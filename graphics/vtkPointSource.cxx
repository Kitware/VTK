/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSource.cxx
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
#include "vtkPointSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
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
vtkPointSource::vtkPointSource(int numPts)
{
  this->NumberOfPoints = (numPts > 0 ? numPts : 10);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Radius = 0.5;
}

//----------------------------------------------------------------------------
void vtkPointSource::Execute()
{
  int i;
  float radius, theta, phi, x[3], rho;
  vtkPoints *newPoints;
  vtkCellArray *newVerts;
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<< "Generating random cloud of points...");

  newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfPoints);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(newVerts->EstimateSize(1,this->NumberOfPoints));

  newVerts->InsertNextCell(this->NumberOfPoints);
  for (i=0; i<this->NumberOfPoints; i++)
    {
    phi = vtkMath::Pi() * vtkMath::Random();
    rho = this->Radius * vtkMath::Random();
    radius = rho * sin((double)phi);
    theta = 2.0*vtkMath::Pi() * vtkMath::Random();
    x[0] = this->Center[0] + radius * cos((double)theta);
    x[1] = this->Center[1] + radius * sin((double)theta);
    x[2] = this->Center[2] + rho * cos((double)phi);
    newVerts->InsertCellPoint(newPoints->InsertNextPoint(x));
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

}
