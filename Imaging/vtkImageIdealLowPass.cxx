/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIdealLowPass.cxx
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
#include "vtkImageIdealLowPass.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageIdealLowPass, "1.17");
vtkStandardNewMacro(vtkImageIdealLowPass);

//----------------------------------------------------------------------------
vtkImageIdealLowPass::vtkImageIdealLowPass()
{
  this->CutOff[0] = this->CutOff[1] = this->CutOff[2] = VTK_LARGE_FLOAT;
}


//----------------------------------------------------------------------------
void vtkImageIdealLowPass::SetXCutOff(float cutOff)
{
  if (cutOff == this->CutOff[0])
    {
    return;
    }
  this->CutOff[0] = cutOff;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageIdealLowPass::SetYCutOff(float cutOff)
{
  if (cutOff == this->CutOff[1])
    {
    return;
    }
  this->CutOff[1] = cutOff;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageIdealLowPass::SetZCutOff(float cutOff)
{
  if (cutOff == this->CutOff[2])
    {
    return;
    }
  this->CutOff[2] = cutOff;
  this->Modified();
}


//----------------------------------------------------------------------------
void vtkImageIdealLowPass::ThreadedExecute(vtkImageData *inData, 
                                           vtkImageData *outData,
                                           int ext[6], int id)
{
  int idx0, idx1, idx2;
  int min0, max0;
  float *inPtr;
  float *outPtr;
  int *wholeExtent;
  float *spacing;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  float temp0, temp1, temp2, mid0, mid1, mid2;
  // normalization factors
  float norm0, norm1, norm2;
  float sum1, sum0;
  unsigned long count = 0;
  unsigned long target;

  // Error checking
  if (inData->GetNumberOfScalarComponents() != 2)
    {
    vtkErrorMacro("Expecting 2 components not " 
                  << inData->GetNumberOfScalarComponents());
    return;
    }
  if (inData->GetScalarType() != VTK_FLOAT || 
      outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Expecting input and output to be of type float");
    return;
    }
  
  wholeExtent = this->GetInput()->GetWholeExtent();
  spacing = inData->GetSpacing();

  inPtr = (float *)(inData->GetScalarPointerForExtent(ext));
  outPtr = (float *)(outData->GetScalarPointerForExtent(ext));

  inData->GetContinuousIncrements(ext, inInc0, inInc1, inInc2);
  outData->GetContinuousIncrements(ext, outInc0, outInc1, outInc2);  
  
  min0 = ext[0];
  max0 = ext[1];
  mid0 = (float)(wholeExtent[0] + wholeExtent[1] + 1) / 2.0;
  mid1 = (float)(wholeExtent[2] + wholeExtent[3] + 1) / 2.0;
  mid2 = (float)(wholeExtent[4] + wholeExtent[5] + 1) / 2.0;
  if ( this->CutOff[0] == 0.0)
    {
    norm0 = VTK_LARGE_FLOAT;
    }
  else
    {
    norm0 = 1.0 / ((spacing[0] * 2.0 * mid0) * this->CutOff[0]);
    }
  if ( this->CutOff[1] == 0.0)
    {
    norm1 = VTK_LARGE_FLOAT;
    }
  else
    {
    norm1 = 1.0 / ((spacing[1] * 2.0 * mid1) * this->CutOff[1]);
    }
  if ( this->CutOff[2] == 0.0)
    {
    norm2 = VTK_LARGE_FLOAT;
    }
  else
    {
    norm2 = 1.0 / ((spacing[2] * 2.0 * mid2) * this->CutOff[2]);
    }
  
  target = (unsigned long)((ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  // loop over all the pixels (keeping track of normalized distance to origin.
  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
    {
    // distance to min (this axis' contribution)
    temp2 = (float)idx2;
    // Wrap back to 0.
    if (temp2 > mid2)
      {
      temp2 = mid2 + mid2 - temp2;
      }
    // Convert location into normalized cycles/world unit
    temp2 = temp2 * norm2;

    for (idx1 = ext[2]; !this->AbortExecute && idx1 <= ext[3]; ++idx1)
      {
      if (!id) 
        {
        if (!(count%target))
          {
          this->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      // distance to min (this axis' contribution)
      temp1 = (float)idx1;
      // Wrap back to 0.
      if (temp1 > mid1)
        {
        temp1 = mid1 + mid1 - temp1;
        }
      // Convert location into cycles / world unit
      temp1 = temp1 * norm1;
      sum1 = temp2 * temp2 + temp1 * temp1;
      
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        // distance to min (this axis' contribution)
        temp0 = (float)idx0;
        // Wrap back to 0.
        if (temp0 > mid0)
          {
          temp0 = mid0 + mid0 - temp0;
          }
        // Convert location into cycles / world unit
        temp0 = temp0 * norm0;
        sum0 = sum1 + temp0 * temp0;
        
        if (sum0 > 1.0)
          {
          // real component
          *outPtr++ = 0.0;
          ++inPtr;
          // imaginary component        
          *outPtr++ = 0.0;
          ++inPtr;
          }
        else
          {
          // real component
          *outPtr++ = *inPtr++;
          // imaginary component        
          *outPtr++ = *inPtr++;
          }
        }
      inPtr += inInc1;
      outPtr += outInc1;
      }
    inPtr += inInc2;
    outPtr += outInc2;    
    }
}

void vtkImageIdealLowPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CutOff: ( "
     << this->CutOff[0] << ", "
     << this->CutOff[1] << ", "
     << this->CutOff[2] << " )\n";
}

