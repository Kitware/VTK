/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToBox.cxx
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
#include "vtkTextureMapToBox.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkTextureMapToBox* vtkTextureMapToBox::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTextureMapToBox");
  if(ret)
    {
    return (vtkTextureMapToBox*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTextureMapToBox;
}




// Construct with r-s-t range=(0,1) and automatic box generation turned on.
vtkTextureMapToBox::vtkTextureMapToBox()
{
  this->Box[0] = this->Box[2] = this->Box[4] = 0.0;
  this->Box[1] = this->Box[3] = this->Box[5] = 1.0;

  this->RRange[0] = 0.0;
  this->RRange[1] = 1.0;

  this->SRange[0] = 0.0;
  this->SRange[1] = 1.0;

  this->TRange[0] = 0.0;
  this->TRange[1] = 1.0;

  this->AutomaticBoxGeneration = 1;
}


void vtkTextureMapToBox::Execute()
{
  float tc[3];
  vtkIdType numPts, i;
  vtkFloatArray *newTCoords;
  int j;
  float *box, *p;
  float min[3], max[3];
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  vtkDebugMacro(<<"Generating 3D texture coordinates!");
  //
  //  Allocate texture data
  //
  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No points to texture!");
    return;
    }

  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(3);
  newTCoords->SetNumberOfTuples(numPts);

  if ( this->AutomaticBoxGeneration ) 
    {
    box = input->GetBounds();
    }
  else
    {
    box = this->Box;
    }
  //
  // Loop over all points generating coordinates
  //
  min[0] = this->RRange[0]; min[1] = this->SRange[0]; min[2] = this->TRange[0]; 
  max[0] = this->RRange[1]; max[1] = this->SRange[1]; max[2] = this->TRange[1]; 

  for (i=0; i<numPts; i++) 
    {
    p = output->GetPoint(i);
    for (j=0; j<3; j++) 
      {
      tc[j] = min[j] + (max[j]-min[j]) * (p[j] - box[2*j]) / 
	(box[2*j+1] - box[2*j]);
      if ( tc[j] < min[j] )
	{
	tc[j] = min[j];
	}
      if ( tc[j] > max[j] )
	{
	tc[j] = max[j];
	}
      }
    newTCoords->SetTuple(i,tc);
    }
  //
  // Update ourselves
  //
  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

// Specify the bounding box to map into.
void vtkTextureMapToBox::SetBox(float xmin, float xmax, float ymin, float ymax,
                                float zmin, float zmax)
{
  if ( xmin != this->Box[0] || xmax != this->Box[1] ||
  ymin != this->Box[2] || ymax != this->Box[3] ||
  zmin != this->Box[4] || zmax != this->Box[5] )
    {
    this->Modified();

    this->Box[0] = xmin; this->Box[1] = xmax; 
    this->Box[2] = ymin; this->Box[3] = ymax; 
    this->Box[4] = zmin; this->Box[5] = zmax; 

    for (int i=0; i<3; i++)
      {
      if ( this->Box[2*i] > this->Box[2*i+1] )
	{
         this->Box[2*i] = this->Box[2*i+1];
	}
      }
    }
}

void vtkTextureMapToBox::SetBox(float *bounds)
{
  this->SetBox(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4],
               bounds[5]);
}

void vtkTextureMapToBox::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Box: " << "( " << this->Box[0] << ", " 
     << this->Box[1] << ", "
     << this->Box[2] << ", "
     << this->Box[3] << ", "
     << this->Box[4] << ", "
     << this->Box[5] << " )\n";

  os << indent << "R Range: (" << this->RRange[0] << ", "
                               << this->RRange[1] << ")\n";
  os << indent << "S Range: (" << this->SRange[0] << ", "
                               << this->SRange[1] << ")\n";
  os << indent << "T Range: (" << this->TRange[0] << ", "
                               << this->TRange[1] << ")\n";
  os << indent << "Automatic Box Generation: " << 
                  (this->AutomaticBoxGeneration ? "On\n" : "Off\n");
}

