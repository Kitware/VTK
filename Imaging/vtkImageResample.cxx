/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageResample.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageResample, "1.36");
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
void vtkImageResample::SetAxisOutputSpacing(int axis, float spacing)
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
void vtkImageResample::SetAxisMagnificationFactor(int axis, float factor)
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
float vtkImageResample::GetAxisMagnificationFactor(int axis)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return 0.0;
    }
  
  if (this->MagnificationFactors[axis] == 0.0)
    {
    float *inputSpacing;
    if ( ! this->GetInput())
      {
      vtkErrorMacro("GetMagnificationFactor: Input not set.");
      return 0.0;
      }
    this->GetInput()->UpdateInformation();
    inputSpacing = this->GetInput()->GetSpacing();
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
void vtkImageResample::ExecuteInformation(vtkImageData *inData, 
                                          vtkImageData *outData) 
{
  int wholeMin, wholeMax, axis, ext[6];
  float spacing[3], factor;

  inData->GetWholeExtent(ext);
  inData->GetSpacing(spacing);
  
  for (axis = 0; axis < 3; axis++)
    {
    wholeMin = ext[axis*2];
    wholeMax = ext[axis*2+1];
    
    // Scale the output extent
    factor = 1.0;
    if (axis < this->Dimensionality)
      {
      factor = this->GetAxisMagnificationFactor(axis);
      }

    wholeMin = (int)(ceil((float)(wholeMin) * factor));
    wholeMax = (int)(floor((float)(wholeMax) * factor));
    
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

  outData->SetWholeExtent(ext);
  outData->SetSpacing(spacing);
}

void vtkImageResample::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
  os << indent << "Interpolate: " << (this->GetInterpolate() ? "On\n" : "Off\n");
}

