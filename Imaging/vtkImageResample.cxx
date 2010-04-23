/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageResample.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageResample);

//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
vtkImageResample::vtkImageResample()
{
  this->MagnificationFactors[0] = 1.0;
  this->MagnificationFactors[1] = 1.0;
  this->MagnificationFactors[2] = 1.0;
  this->OutputSpacing[0] = 0.0; // not specified
  this->OutputSpacing[1] = 0.0; // not specified
  this->OutputSpacing[2] = 0.0; // not specified
  this->InterpolationMode = VTK_RESLICE_LINEAR;
  this->Dimensionality = 3;
}

//----------------------------------------------------------------------------
void vtkImageResample::SetAxisOutputSpacing(int axis, double spacing)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return;
    }
  
  if (this->OutputSpacing[axis] != spacing)
    {
    this->OutputSpacing[axis] = spacing;
    this->Modified();
    if (spacing != 0.0)
      {
      // Delay computing the magnification factor.
      // Input might not be set yet.
      this->MagnificationFactors[axis] = 0.0; // Not computed yet.
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageResample::SetAxisMagnificationFactor(int axis, double factor)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return;
    }
  
  if (this->MagnificationFactors[axis] == factor)
    {
    return;
    }
  this->Modified();
  this->MagnificationFactors[axis] = factor;
  // Spacing is no longer valid.
  this->OutputSpacing[axis] = 0.0; // Not computed yet.
}

//----------------------------------------------------------------------------
double vtkImageResample::GetAxisMagnificationFactor(int axis,
                                                    vtkInformation *inInfo)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return 0.0;
    }
  
  if (this->MagnificationFactors[axis] == 0.0)
    {
    double *inputSpacing;
    if ( ! this->GetInput())
      {
      vtkErrorMacro("GetMagnificationFactor: Input not set.");
      return 0.0;
      }
    this->GetInput()->UpdateInformation();
    if (!inInfo)
      {
      inInfo = this->GetExecutive()->GetInputInformation(0, 0);
      }
    inputSpacing = inInfo->Get(vtkDataObject::SPACING());
    this->MagnificationFactors[axis] = 
      inputSpacing[axis] / this->OutputSpacing[axis];
    
    }

  vtkDebugMacro("Returning magnification factor " 
                <<  this->MagnificationFactors[axis] << " for axis "
                << axis);
  
  return this->MagnificationFactors[axis];
}


//----------------------------------------------------------------------------
// Computes any global image information associated with regions.
int vtkImageResample::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int wholeMin, wholeMax, axis, ext[6];
  double spacing[3], factor;

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
  inInfo->Get(vtkDataObject::SPACING(), spacing);
  
  for (axis = 0; axis < 3; axis++)
    {
    wholeMin = ext[axis*2];
    wholeMax = ext[axis*2+1];
    
    // Scale the output extent
    factor = 1.0;
    if (axis < this->Dimensionality)
      {
      factor = this->GetAxisMagnificationFactor(axis, inInfo);
      }

    wholeMin = static_cast<int>(ceil(static_cast<double>(wholeMin) * factor));
    wholeMax = static_cast<int>(floor(static_cast<double>(wholeMax) * factor));
    
    // Change the data spacing
    spacing[axis] /= factor;
    
    ext[axis*2] = wholeMin;
    ext[axis*2+1] = wholeMax;
    
    // just in case  the input spacing has changed.
    if (this->OutputSpacing[axis] != 0.0)
      {
      // Cause MagnificationFactor to recompute.
      this->MagnificationFactors[axis] = 0.0;
      }
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);

  return 1;
}

void vtkImageResample::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
  os << indent << "Interpolate: " << (this->GetInterpolate() ? "On\n" : "Off\n");
}
