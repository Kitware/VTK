/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutMaterial.cxx
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
#include "vtkCutMaterial.h"
#include "vtkCutter.h"
#include "vtkThreshold.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkCutMaterial* vtkCutMaterial::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCutMaterial");
  if(ret)
    {
    return (vtkCutMaterial*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCutMaterial;
}

// Instantiate object with no input and no defined output.
vtkCutMaterial::vtkCutMaterial()
{
  this->MaterialArrayName = NULL;
  this->SetMaterialArrayName("material");
  this->Material = 0;
  this->ArrayName = NULL;
  
  this->UpVector[0] = 0.0;
  this->UpVector[1] = 0.0;
  this->UpVector[2] = 1.0;
  
  this->MaximumPoint[0] = 0.0;
  this->MaximumPoint[1] = 0.0;
  this->MaximumPoint[2] = 0.0;
 
  this->CenterPoint[0] = 0.0;
  this->CenterPoint[1] = 0.0;
  this->CenterPoint[2] = 0.0;
  
  this->Normal[0] = 0.0;
  this->Normal[1] = 1.0;
  this->Normal[2] = 0.0;
  
  this->PlaneFunction = vtkPlane::New();
}

vtkCutMaterial::~vtkCutMaterial()
{
  this->PlaneFunction->Delete();
  this->PlaneFunction = NULL;

  this->SetMaterialArrayName(NULL);
  this->SetArrayName(NULL);
}

void vtkCutMaterial::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkThreshold *thresh;
  vtkCutter *cutter;
  float *bds;
  
  // Check to see if we have the required field arrays.
  if (this->MaterialArrayName == NULL || this->ArrayName == NULL)
    {
    vtkErrorMacro("Material and Array names must be set.");
    return;
    }
  
  if (input->GetCellData()->GetArray(this->MaterialArrayName) == NULL)
    {
    vtkErrorMacro("Could not find cell array " << this->MaterialArrayName);
    return;
    }
  if (input->GetCellData()->GetArray(this->ArrayName) == NULL)
    {
    vtkErrorMacro("Could not find cell array " << this->ArrayName);
    return;
    }
  
  // It would be nice to get rid of this in the future.
  thresh = vtkThreshold::New();
  thresh->SetInput(input);
  thresh->SetArrayName(this->MaterialArrayName);
  thresh->SetAttributeModeToUseCellData();
  thresh->ThresholdBetween(this->Material-0.5, this->Material+0.5);
  thresh->Update();
  
  bds = thresh->GetOutput()->GetBounds();
  this->CenterPoint[0] = 0.5 * (bds[0] + bds[1]);
  this->CenterPoint[1] = 0.5 * (bds[2] + bds[3]);
  this->CenterPoint[2] = 0.5 * (bds[4] + bds[5]);
  
  this->ComputeMaximumPoint(thresh->GetOutput());
  this->ComputeNormal();
  
  this->PlaneFunction->SetOrigin(this->CenterPoint);
  this->PlaneFunction->SetNormal(this->Normal);
  
  cutter = vtkCutter::New();
  cutter->SetInput(thresh->GetOutput());
  cutter->SetCutFunction(this->PlaneFunction);
  cutter->SetValue(0, 0.0);
  cutter->Update();

  vtkDataSet* output = this->GetOutput();
  output->CopyStructure(cutter->GetOutput());
  output->GetPointData()->PassData(
           cutter->GetOutput()->GetPointData());
  output->GetCellData()->PassData(
           cutter->GetOutput()->GetCellData());

  cutter->Delete();
  thresh->Delete();
}

void vtkCutMaterial::ComputeNormal()
{
  float tmp[3];
  float mag;
  
  if (this->UpVector[0] == 0.0 && this->UpVector[1] == 0.0 && this->UpVector[2] == 0.0)
    {
    vtkErrorMacro("Zero magnitude UpVector.");
    this->UpVector[2] = 1.0;
    }
  
  tmp[0] = this->MaximumPoint[0] - this->CenterPoint[0];
  tmp[1] = this->MaximumPoint[1] - this->CenterPoint[1];
  tmp[2] = this->MaximumPoint[2] - this->CenterPoint[2];
  vtkMath::Cross(tmp, this->UpVector, this->Normal);
  mag = vtkMath::Normalize(this->Normal);
  // Rare singularity
  while (mag == 0.0)
    { 
    tmp[0] = vtkMath::Random();
    tmp[1] = vtkMath::Random();
    tmp[2] = vtkMath::Random();
    vtkMath::Cross(tmp, this->UpVector, this->Normal);
    mag = vtkMath::Normalize(this->Normal);
    }
}

void vtkCutMaterial::ComputeMaximumPoint(vtkDataSet *input)
{
  vtkDataArray *data;
  vtkIdType idx, bestIdx, num;
  float comp, best;
  vtkCell *cell;
  float *bds;
  
  // Find the maximum value.
  data = input->GetCellData()->GetArray(this->ArrayName);
  if (data == NULL)
    {
    vtkErrorMacro("What happened to the array " << this->ArrayName);
    return;
    }

  num = data->GetNumberOfTuples();
  if (num <= 0)
    {
    vtkErrorMacro("No values in array " << this->ArrayName);
    return;
    }
    
  best = data->GetComponent(0, 0);
  bestIdx = 0;
  for (idx = 1; idx < num; ++idx)
    {
    comp = data->GetComponent(idx, 0);
    if (comp > best)
      {
      best = comp;
      bestIdx = idx;
      }
    }
  
  // Get the cell with the larges value.
  cell = input->GetCell(bestIdx);
  bds = cell->GetBounds();
  this->MaximumPoint[0] = (bds[0] + bds[1]) * 0.5;
  this->MaximumPoint[1] = (bds[2] + bds[3]) * 0.5;
  this->MaximumPoint[2] = (bds[4] + bds[5]) * 0.5;  
}

void vtkCutMaterial::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "ArrayName: ";
  if ( this->ArrayName)
    {
    os << this->ArrayName << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
  os << indent << "MaterialArrayName: " << this->MaterialArrayName << endl;
  os << indent << "Material: " << this->Material << endl;
  
  os << indent << "UpVector: " << this->UpVector[0] << ", "
     << this->UpVector[1] << ", " << this->UpVector[2] << endl;
  
  os << indent << "MaximumPoint: " << this->MaximumPoint[0] << ", "
     << this->MaximumPoint[1] << ", " << this->MaximumPoint[2] << endl;
  os << indent << "CenterPoint: " << this->CenterPoint[0] << ", "
     << this->CenterPoint[1] << ", " << this->CenterPoint[2] << endl;
  os << indent << "Normal: " << this->Normal[0] << ", "
     << this->Normal[1] << ", " << this->Normal[2] << endl;
}
