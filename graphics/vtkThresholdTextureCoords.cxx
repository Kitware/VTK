/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTextureCoords.cxx
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
#include "vtkThresholdTextureCoords.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkThresholdTextureCoords* vtkThresholdTextureCoords::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThresholdTextureCoords");
  if(ret)
    {
    return (vtkThresholdTextureCoords*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThresholdTextureCoords;
}

// Construct with lower threshold=0, upper threshold=1, threshold 
// function=upper, and texture dimension = 2. The "out" texture coordinate
// is (0.25,0,0); the "in" texture coordinate is (0.75,0,0).
vtkThresholdTextureCoords::vtkThresholdTextureCoords()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->TextureDimension = 2;

  this->ThresholdFunction = &vtkThresholdTextureCoords::Upper;

  this->OutTextureCoord[0] = 0.25;
  this->OutTextureCoord[1] = 0.0;
  this->OutTextureCoord[2] = 0.0;

  this->InTextureCoord[0] = 0.75;
  this->InTextureCoord[1] = 0.0;
  this->InTextureCoord[2] = 0.0;
}

// Criterion is cells whose scalars are less than lower threshold.
void vtkThresholdTextureCoords::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vtkThresholdTextureCoords::Lower;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are less than upper threshold.
void vtkThresholdTextureCoords::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vtkThresholdTextureCoords::Upper;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThresholdTextureCoords::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThresholdTextureCoords::Between;
    this->Modified();
    }
}
  
void vtkThresholdTextureCoords::Execute()
{
  vtkIdType numPts;
  vtkTCoords *newTCoords;
  vtkIdType ptId;
  vtkDataArray *inScalars;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  vtkDebugMacro(<< "Executing texture threshold filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( ! (inScalars = input->GetPointData()->GetActiveScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to texture threshold");
    return;
    }
     
  numPts = input->GetNumberOfPoints();
  newTCoords = vtkTCoords::New();
  newTCoords->Allocate(this->TextureDimension);

  // Check that the scalars of each point satisfy the threshold criterion
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( (this->*(this->ThresholdFunction))(inScalars->GetComponent(ptId,0)) )
      {
      newTCoords->InsertTCoord(ptId,this->InTextureCoord);
      }
    else //doesn't satisfy criterion
      {
      newTCoords->InsertTCoord(ptId,this->OutTextureCoord);
      }

    } //for all points

  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkThresholdTextureCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  if ( this->ThresholdFunction == &vtkThresholdTextureCoords::Upper )
    {
    os << indent << "Threshold By Upper\n";
    }

  else if ( this->ThresholdFunction == &vtkThresholdTextureCoords::Lower )
    {
    os << indent << "Threshold By Lower\n";
    }

  else if ( this->ThresholdFunction == &vtkThresholdTextureCoords::Between )
    {
    os << indent << "Threshold Between\n";
    }

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
  os << indent << "Texture Dimension: " << this->TextureDimension << "\n";;

  os << indent << "Out Texture Coordinate: (" << this->OutTextureCoord[0] 
     << ", " << this->OutTextureCoord[1]
     << ", " << this->OutTextureCoord[2] << ")\n";

  os << indent << "In Texture Coordinate: (" << this->InTextureCoord[0] 
     << ", " << this->InTextureCoord[1]
     << ", " << this->InTextureCoord[2] << ")\n";
}
