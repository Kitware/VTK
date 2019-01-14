/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageButterworthLowPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageButterworthLowPass.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkImageButterworthLowPass);

//----------------------------------------------------------------------------
vtkImageButterworthLowPass::vtkImageButterworthLowPass()
{
  this->CutOff[0] = this->CutOff[1] = this->CutOff[2] = VTK_DOUBLE_MAX;
  this->Order = 1;
}


//----------------------------------------------------------------------------
void vtkImageButterworthLowPass::SetXCutOff(double cutOff)
{
  if (cutOff == this->CutOff[0])
  {
    return;
  }
  this->CutOff[0] = cutOff;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageButterworthLowPass::SetYCutOff(double cutOff)
{
  if (cutOff == this->CutOff[1])
  {
    return;
  }
  this->CutOff[1] = cutOff;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageButterworthLowPass::SetZCutOff(double cutOff)
{
  if (cutOff == this->CutOff[2])
  {
    return;
  }
  this->CutOff[2] = cutOff;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageButterworthLowPass::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int ext[6], int id)
{
  int idx0, idx1, idx2;
  int min0, max0;
  double *inPtr;
  double *outPtr;
  int wholeExtent[6];
  double spacing[3];
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outInc0, outInc1, outInc2;
  double temp0, temp1, temp2, mid0, mid1, mid2;
  // normalization factors
  double norm0, norm1, norm2;
  double sum1, sum0;
  unsigned long count = 0;
  unsigned long target;

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // Error checking
  if (inData[0][0]->GetNumberOfScalarComponents() != 2)
  {
    vtkErrorMacro("Expecting 2 components not "
                  << inData[0][0]->GetNumberOfScalarComponents());
    return;
  }
  if (inData[0][0]->GetScalarType() != VTK_DOUBLE ||
      outData[0]->GetScalarType() != VTK_DOUBLE)
  {
    vtkErrorMacro("Expecting input and output to be of type double");
    return;
  }

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  inData[0][0]->GetSpacing(spacing);

  inPtr = static_cast<double *>(inData[0][0]->GetScalarPointerForExtent(ext));
  outPtr = static_cast<double *>(outData[0]->GetScalarPointerForExtent(ext));

  inData[0][0]->GetContinuousIncrements(ext, inInc0, inInc1, inInc2);
  outData[0]->GetContinuousIncrements(ext, outInc0, outInc1, outInc2);

  min0 = ext[0];
  max0 = ext[1];
  mid0 = static_cast<double>(wholeExtent[0] + wholeExtent[1] + 1) / 2.0;
  mid1 = static_cast<double>(wholeExtent[2] + wholeExtent[3] + 1) / 2.0;
  mid2 = static_cast<double>(wholeExtent[4] + wholeExtent[5] + 1) / 2.0;
  if ( this->CutOff[0] == 0.0)
  {
    norm0 = VTK_DOUBLE_MAX;
  }
  else
  {
    norm0 = 1.0 / ((spacing[0] * 2.0 * mid0) * this->CutOff[0]);
  }
  if ( this->CutOff[1] == 0.0)
  {
    norm1 = VTK_DOUBLE_MAX;
  }
  else
  {
    norm1 = 1.0 / ((spacing[1] * 2.0 * mid1) * this->CutOff[1]);
  }
  if ( this->CutOff[2] == 0.0)
  {
    norm2 = VTK_DOUBLE_MAX;
  }
  else
  {
    norm2 = 1.0 / ((spacing[2] * 2.0 * mid2) * this->CutOff[2]);
  }

  target = static_cast<unsigned long>(
    (ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  // loop over all the pixels (keeping track of normalized distance to origin.
  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
  {
    // distance to min (this axis' contribution)
    temp2 = static_cast<double>(idx2);
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
      temp1 = static_cast<double>(idx1);
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
        temp0 = static_cast<double>(idx0);
        // Wrap back to 0.
        if (temp0 > mid0)
        {
          temp0 = mid0 + mid0 - temp0;
        }
        // Convert location into cycles / world unit
        temp0 = temp0 * norm0;
        sum0 = sum1 + temp0 * temp0;

        // compute Butterworth1D function from sum = d^2
        if (this->Order == 1)
        {
          sum0 = 1.0 / (1.0 + sum0);
        }
        else
        {
          sum0 = 1.0 / (1.0 + pow(sum0, static_cast<double>(this->Order)));
        }

        // real component
        *outPtr++ = *inPtr++ * sum0;
        // imaginary component
        *outPtr++ = *inPtr++ * sum0;

      }
      inPtr += inInc1;
      outPtr += outInc1;
    }
    inPtr += inInc2;
    outPtr += outInc2;
  }
}

void vtkImageButterworthLowPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Order: " << this->Order << "\n";

  os << indent << "CutOff: ( "
     << this->CutOff[0] << ", "
     << this->CutOff[1] << ", "
     << this->CutOff[2] << " )\n";
}
