/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradientMagnitude.cxx
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
#include "vtkImageGradientMagnitude.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageGradientMagnitude, "1.30");
vtkStandardNewMacro(vtkImageGradientMagnitude);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageGradientMagnitude fitler.
vtkImageGradientMagnitude::vtkImageGradientMagnitude()
{
  this->Dimensionality = 2;
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
void vtkImageGradientMagnitude::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HandleBoundaries: " << this->HandleBoundaries << "\n";
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

//----------------------------------------------------------------------------
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageGradientMagnitude::ExecuteInformation(vtkImageData *inData, 
                                                   vtkImageData *outData)
{  
  int extent[6];
  int idx;

  inData->GetWholeExtent(extent);
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < this->Dimensionality; ++idx)
      {
      extent[idx*2] += 1;
      extent[idx*2 + 1] -= 1;
      }
    }
  
  outData->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
void vtkImageGradientMagnitude::ComputeInputUpdateExtent(int inExt[6],
                                                         int outExt[6])
{
  int *wholeExtent;
  int idx;

  wholeExtent = this->GetInput()->GetWholeExtent();
  
  memcpy(inExt,outExt,6*sizeof(int));
  
  // grow input whole extent.
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    inExt[idx*2] -= 1;
    inExt[idx*2+1] += 1;
    if (this->HandleBoundaries)
      {
      // we must clip extent with whole extent is we hanlde boundaries.
      if (inExt[idx*2] < wholeExtent[idx*2])
        {
        inExt[idx*2] = wholeExtent[idx*2];
        }
      if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
        {
        inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
        }
      }
    }
}





//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
void vtkImageGradientMagnitudeExecute(vtkImageGradientMagnitude *self,
                                      vtkImageData *inData, T *inPtr,
                                      vtkImageData *outData, T *outPtr,
                                      int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int axesNum;
  int *wholeExtent, *inIncs;
  float r[3], d, sum;
  int useZMin, useZMax, useYMin, useYMax, useXMin, useXMax;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality();
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // The data spacing is important for computing the gradient.
  inData->GetSpacing(r);
  r[0] = 0.5 / r[0];
  r[1] = 0.5 / r[1];
  r[2] = 0.5 / r[2];

  // get some other info we need
  inIncs = inData->GetIncrements(); 
  wholeExtent = inData->GetExtent(); 

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    useZMin = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useZMax = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      useYMin = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useYMax = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        useXMin = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
        useXMax = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];
        for (idxC = 0; idxC < maxC; idxC++)
          {
          // do X axis
          d = (float)(inPtr[useXMin]);
          d -= (float)(inPtr[useXMax]);
          d *= r[0]; // multiply by the data spacing
          sum = d * d;
          // do y axis
          d = (float)(inPtr[useYMin]);
          d -= (float)(inPtr[useYMax]);
          d *= r[1]; // multiply by the data spacing
          sum += (d * d);
          if (axesNum == 3)
            {
            // do z axis
            d = (float)(inPtr[useZMin]);
            d -= (float)(inPtr[useZMax]);
            d *= r[2]; // multiply by the data spacing
            sum += (d * d);
            }
          *outPtr = (T)(sqrt(sum));
          outPtr++;
          inPtr++;
          }
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageGradientMagnitude::ThreadedExecute(vtkImageData *inData, 
                                         vtkImageData *outData,
                                         int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
                << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageGradientMagnitudeExecute, this, 
                      inData, (VTK_TT *)(inPtr), outData, 
                      (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}















