/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageAnisotropicDiffusion2D.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageAnisotropicDiffusion2D);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageAnisotropicDiffusion2D fitler.
vtkImageAnisotropicDiffusion2D::vtkImageAnisotropicDiffusion2D()
{
  this->HandleBoundaries = 1;
  this->NumberOfIterations = 0;
  this->SetNumberOfIterations(4);
  this->DiffusionThreshold = 5.0;
  this->DiffusionFactor = 1;
  this->Faces = 0;
  this->FacesOn();
  this->Edges = 0;
  this->EdgesOn();
  this->Corners = 0;
  this->CornersOn();
  this->GradientMagnitudeThreshold = 1;
  this->GradientMagnitudeThresholdOff();
}

//----------------------------------------------------------------------------
void 
vtkImageAnisotropicDiffusion2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";
  os << indent << "DiffusionThreshold: " << this->DiffusionThreshold << "\n";
  os << indent << "DiffusionFactor: " << this->DiffusionFactor << "\n";

  os << indent << "Faces: " << this->Faces << "\n";

  if (this->Edges)
    {
    os << indent << "Edges: On\n";
    }
  else
    {
    os << indent << "Edges: Off\n";
    }

  if (this->Corners)
    {
    os << indent << "Corners: On\n";
    }
  else
    {
    os << indent << "Corners: Off\n";
    }

  if (this->GradientMagnitudeThreshold)
    {
    os << indent << "GradientMagnitudeThreshold: On\n";
    }
  else
    {
    os << indent << "GradientMagnitudeThreshold: Off\n";
    }
}

//----------------------------------------------------------------------------
// This method sets the number of inputs which also affects the
// input neighborhood needed to compute one output pixel.
void vtkImageAnisotropicDiffusion2D::SetNumberOfIterations(int num)
{
  int temp;
  
  vtkDebugMacro(<< "SetNumberOfIterations: " << num);
  if (this->NumberOfIterations == num)
    {
    return;
    }

  this->Modified();
  temp = num*2 + 1;
  this->KernelSize[0] = temp;
  this->KernelSize[1] = temp;
  this->KernelMiddle[0] = num;
  this->KernelMiddle[1] = num;

  this->NumberOfIterations = num;
}  
  
//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The input and output datas
// must have the same data type.
void vtkImageAnisotropicDiffusion2D::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData, 
  vtkImageData **outData,
  int outExt[6], int id)
{
  int inExt[6], wholeExt[6];
  double *ar;
  int idx;
  vtkImageData *temp;
  
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  this->InternalRequestUpdateExtent(inExt,outExt,wholeExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro("Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }
  
  ar = inData[0][0]->GetSpacing();

  // make the temporary regions to iterate over.
  vtkImageData *in = vtkImageData::New();
  in->SetExtent(inExt);
  in->SetNumberOfScalarComponents(inData[0][0]->GetNumberOfScalarComponents());
  in->SetScalarType(VTK_DOUBLE);
  in->CopyAndCastFrom(inData[0][0],inExt);
  
  vtkImageData *out = vtkImageData::New();
  out->SetExtent(inExt);
  out->SetNumberOfScalarComponents(
    inData[0][0]->GetNumberOfScalarComponents());
  out->SetScalarType(VTK_DOUBLE);
  out->AllocateScalars();

  // Loop performing the diffusion
  // Note: region extent could get smaller as the diffusion progresses
  // (but never get smaller than output region).
  for (idx = this->NumberOfIterations - 1; 
       !this->AbortExecute && idx >= 0; --idx)
    {
    if (!id)
      {
      this->UpdateProgress(static_cast<double>(this->NumberOfIterations - idx)
                           /this->NumberOfIterations);
      }
    this->Iterate(in, out, ar[0], ar[1], outExt, idx);
    temp = in;
    in = out;
    out = temp;
    }
  
  // copy results into output.
  outData[0]->CopyAndCastFrom(in,outExt);
  in->Delete();
  out->Delete();
}

//----------------------------------------------------------------------------
// This method performs one pass of the diffusion filter.
// The inData and outData are assumed to have data type double,
// and have the same extent.
void vtkImageAnisotropicDiffusion2D::Iterate(vtkImageData *inData, 
                                             vtkImageData *outData,
                                             double ar0, double ar1,
                                             int *coreExtent, int count)
{
  int idx0, idx1, idx2;
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outInc0, outInc1, outInc2;
  int inMin0, inMax0, inMin1, inMax1, inMin2, inMax2;
  int min0, max0, min1, max1, min2, max2;
  double *inPtr0, *inPtr1, *inPtr2;
  double *outPtr0, *outPtr1, *outPtr2;
  double th0, th1, th01;
  double df0, df1, df01;
  double temp, sum;
  int idxC, maxC;

  maxC = inData->GetNumberOfScalarComponents();
  inData->GetExtent(inMin0, inMax0, inMin1, inMax1, inMin2, inMax2);
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outData->GetIncrements(outInc0, outInc1, outInc2);

  // Avoid warnings.
  th0 = th1 = th01 = df0 = df1 = df01 = 0.0;
  
  // Compute direction specific diffusion thresholds and factors.
  sum = 0.0;
  if (this->Edges)
    {
    th0 = ar0 * this->DiffusionThreshold;
    df0 = 1.0 / ar0;
    th1 = ar1 * this->DiffusionThreshold;
    df1 = 1.0 / ar1;
    // two edges per direction.
    sum += 2.0 * (df0 + df1);
    }
  if (this->Corners)
    {
    temp = sqrt(ar0*ar0 + ar1*ar1);
    th01 = temp * this->DiffusionThreshold;
    df01 = 1 / temp;
    // four corners per plane
    sum += 4 * (df01);
    }

  if (sum > 0.0)
    {
    temp = this->DiffusionFactor / sum;
    df0 *= temp;
    df1 *= temp;
    df01 *= temp;
    }
  else
    {
    vtkWarningMacro(<< "Iterate: NO NEIGHBORS");
    return;
    }

  // Compute the shrinking extent to loop over.
  min0 = coreExtent[0] - count;
  max0 = coreExtent[1] + count;
  min1 = coreExtent[2] - count;
  max1 = coreExtent[3] + count;
  // intersection
  min0 = (min0 > inMin0) ? min0 : inMin0;
  max0 = (max0 < inMax0) ? max0 : inMax0;
  min1 = (min1 > inMin1) ? min1 : inMin1;
  max1 = (max1 < inMax1) ? max1 : inMax1;
  
  vtkDebugMacro(<< "Iteration count: " << count << " ("
  << min0 << ", " << max0 << ", " << min1 << ", " << max1 << ")");
  
  // I apologize for explicitly diffusing each neighbor, but it is the easiest
  // way to deal with the boundary conditions.  Besides it is fast.
  // (Are you sure every one is correct?!!!)
  min2 = inMin2;
  max2 = inMax2;
 
  for (idxC = 0; idxC < maxC; idxC++)
    {
    inPtr2 = static_cast<double *>(inData->GetScalarPointer(min0, min1, min2));
    outPtr2 =
      static_cast<double *>(outData->GetScalarPointer(min0, min1, min2));
    inPtr2 += idxC;
    outPtr2 += idxC;
    
    for (idx2 = min2; idx2 <= max2; ++idx2, inPtr2+=inInc2, outPtr2+=outInc2)
      {
      inPtr1 = inPtr2;
      outPtr1 = outPtr2;    
      for (idx1 = min1; idx1 <= max1; ++idx1, inPtr1+=inInc1, outPtr1+=outInc1)
        {
        inPtr0 = inPtr1;
        outPtr0 = outPtr1;    
        for (idx0 = min0; idx0 <= max0; ++idx0, inPtr0+=inInc0, outPtr0+=outInc0)
          {
          // Copy center
          *outPtr0 = *inPtr0;
        
          // Special case for gradient magnitude threhsold 
          if (this->GradientMagnitudeThreshold)
            {
            double d0, d1;
            // compute the gradient magnitude (central differences).
            d0  = (idx0 != inMax0) ? inPtr0[inInc0] : *inPtr0;
            d0 -= (idx0 != inMin0) ? inPtr0[-inInc0] : *inPtr0;
            d0 /= ar0;
            d1  = (idx1 != inMax1) ? inPtr0[inInc1] : *inPtr0;
            d1 -= (idx1 != inMin1) ? inPtr0[-inInc1] : *inPtr0;
            d1 /= ar1;
            // If magnitude is big, don't diffuse.
            d0 = sqrt(d0*d0 + d1*d1);
            if (d0 > this->DiffusionThreshold)
              {
              // hack to not diffuse
              th0 = th1 = th01 = 0.0;
              }
            else
              {
              // hack to diffuse
              th0 = th1 = th01 = VTK_DOUBLE_MAX;
              }
            }
        
          // Start diffusing
          if (this->Edges)
            {
            // left
            if (idx0 != inMin0)
              {
              temp = inPtr0[-inInc0] - *inPtr0;
              if (fabs(temp) < th0)
                {
                *outPtr0 += temp * df0;
                }
              }
            // right
            if (idx0 != inMax0)
              {
              temp = inPtr0[inInc0] - *inPtr0;
              if (fabs(temp) < th0)
                {
                *outPtr0 += temp * df0;
                }
              }
            // up
            if (idx1 != inMin1)
              {
              temp = inPtr0[-inInc1] - *inPtr0;
              if (fabs(temp) < th1)
                {
                *outPtr0 += temp * df1;
                }
              }
            // down
            if (idx1 != inMax1)
              {
              temp = inPtr0[inInc1] - *inPtr0;
              if (fabs(temp) < th1)
                {
                *outPtr0 += temp * df1;
                }
              }
            }
        
          if (this->Corners)
            {
            // left up
            if (idx0 != inMin0 && idx1 != inMin1)
              {
              temp = inPtr0[-inInc0-inInc1] - *inPtr0;
              if (fabs(temp) < th01)
                {
                *outPtr0 += temp * df01;
                }
              }
            // right up
            if (idx0 != inMax0 && idx1 != inMin1)
              {
              temp = inPtr0[inInc0-inInc1] - *inPtr0;
              if (fabs(temp) < th01)
                {
                *outPtr0 += temp * df01;
                }
              }
            // left down
            if (idx0 != inMin0 && idx1 != inMax1)
              {
              temp = inPtr0[-inInc0+inInc1] - *inPtr0;
              if (fabs(temp) < th01)
                {
                *outPtr0 += temp * df01;
                }
              }
            // right down
            if (idx0 != inMax0 && idx1 != inMax1)
              {
              temp = inPtr0[inInc0+inInc1] - *inPtr0;
              if (fabs(temp) < th01)
                {
                *outPtr0 += temp * df01;
                }
              }
            }
          }
        }
      }
    }
}
