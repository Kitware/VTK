/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRTAnalyticSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRTAnalyticSource.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

#include <math.h>

vtkStandardNewMacro(vtkRTAnalyticSource);

// ----------------------------------------------------------------------------
vtkRTAnalyticSource::vtkRTAnalyticSource()
{
  this->Maximum = 255.0;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->WholeExtent[0] = -10;  this->WholeExtent[1] = 10;
  this->WholeExtent[2] = -10;  this->WholeExtent[3] = 10;
  this->WholeExtent[4] = -10;  this->WholeExtent[5] = 10;
  this->StandardDeviation = 0.5;

  this->XFreq = 60;
  this->XMag = 10;
  this->YFreq = 30;
  this->YMag = 18;
  this->ZFreq = 40;
  this->ZMag = 5;

  this->SetNumberOfInputPorts(0);

  this->SubsampleRate = 1;
}

// ----------------------------------------------------------------------------
void vtkRTAnalyticSource::SetWholeExtent(int xMin, int xMax, 
                                         int yMin, int yMax,
                                         int zMin, int zMax)
{
  int modified = 0;
  
  if (this->WholeExtent[0] != xMin)
    {
    modified = 1;
    this->WholeExtent[0] = xMin ;
    }
  if (this->WholeExtent[1] != xMax)
    {
    modified = 1;
    this->WholeExtent[1] = xMax ;
    }
  if (this->WholeExtent[2] != yMin)
    {
    modified = 1;
    this->WholeExtent[2] = yMin ;
    }
  if (this->WholeExtent[3] != yMax)
    {
    modified = 1;
    this->WholeExtent[3] = yMax ;
    }
  if (this->WholeExtent[4] != zMin)
    {
    modified = 1;
    this->WholeExtent[4] = zMin ;
    }
  if (this->WholeExtent[5] != zMax)
    {
    modified = 1;
    this->WholeExtent[5] = zMax ;
    }
  if (modified)
    {
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
int vtkRTAnalyticSource::RequestInformation(
   vtkInformation *vtkNotUsed(request),
   vtkInformationVector **vtkNotUsed(inputVector),
   vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  int tmpExt[6], i;
  for (i = 0; i < 3; i++)
    {
    tmpExt[2*i] = this->WholeExtent[2*i] / this->SubsampleRate;
    tmpExt[2*i+1] = this->WholeExtent[2*i+1] / this->SubsampleRate;
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               tmpExt,6);

  outInfo->Set(vtkDataObject::SPACING(), this->SubsampleRate,
               this->SubsampleRate, this->SubsampleRate);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

void vtkRTAnalyticSource::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data;
  float *outPtr;
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int *outExt, *whlExt;
  int newOutExt[6];
  double sum;
  double yContrib, zContrib;
  double temp2;
  unsigned long count = 0;
  unsigned long target;

  data = this->AllocateOutputData(output);
  if (data->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: This source only outputs floats");
    return;
    }
  if (data->GetNumberOfPoints() <= 0)
    {
    return;
    }

  data->SetSpacing(this->SubsampleRate, this->SubsampleRate,
                   this->SubsampleRate);
  
  outExt = data->GetExtent();
  int i;
  for (i = 0; i < 3; i++)
    {
    newOutExt[2*i] = outExt[2*i] * this->SubsampleRate;
    newOutExt[2*i+1] = outExt[2*i+1] * this->SubsampleRate;
    }
  whlExt = this->GetWholeExtent();
  data->GetPointData()->GetScalars()->SetName("RTData");

  // find the region to loop over
  maxX = newOutExt[1] - newOutExt[0];
  maxY = newOutExt[3] - newOutExt[2]; 
  maxZ = newOutExt[5] - newOutExt[4];
  
  // Get increments to march through data 
  data->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outPtr = static_cast<float *>(data->GetScalarPointer(outExt[0],outExt[2],outExt[4]));
  
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Loop through ouput pixels
  temp2 = 1.0 / (2.0 * this->StandardDeviation * this->StandardDeviation);

  double x, y, z;
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    if (idxZ % this->SubsampleRate)
      {
      continue;
      }
    z = this->Center[2] - (idxZ + newOutExt[4]);
    if (whlExt[5] > whlExt[4])
      {
      z /= (whlExt[5] - whlExt[4]);
      }
    zContrib = z * z;
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      {
      if (idxY % this->SubsampleRate)
        {
        continue;
        }
      if (!(count%target))
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;
      y = this->Center[1] - (idxY + newOutExt[2]);
      if (whlExt[3] > whlExt[2])
        {
        y /= (whlExt[3] - whlExt[2]);
        }
      yContrib = y * y;
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        if (idxX % this->SubsampleRate)
          {
          continue;
          }
        // Pixel operation
        sum = zContrib + yContrib;
        x = this->Center[0] - (idxX + newOutExt[0]);
        if (whlExt[1] > whlExt[0])
          {
          x /= (whlExt[1] - whlExt[0]);
          }
        sum = sum + (x * x);
        *outPtr = this->Maximum * exp(-sum * temp2) 
          + this->XMag*sin(this->XFreq*x)
          + this->YMag*sin(this->YFreq*y)
          + this->ZMag*cos(this->ZFreq*z);
        outPtr++;
        }
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
}

void vtkRTAnalyticSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
  os << indent << "StandardDeviation: " << this->StandardDeviation << "\n";
  os << indent << "Center: ( "
     << this->Center[0] << ", "
     << this->Center[1] << ", "
     << this->Center[2] << " )\n";
  os << indent << "XFreq: " << this->XFreq << endl;
  os << indent << "YFreq: " << this->YFreq << endl;
  os << indent << "ZFreq: " << this->ZFreq << endl;
  os << indent << "XMag: " << this->XMag << endl;
  os << indent << "YMag: " << this->YMag << endl;
  os << indent << "ZMag: " << this->ZMag << endl;

  os << indent << "WholeExtent: " << this->WholeExtent[0]
     << ", " << this->WholeExtent[1] << ", " << this->WholeExtent[2]
     << ", " << this->WholeExtent[3] << ", " << this->WholeExtent[4]
     << ", " << this->WholeExtent[5] << endl;

  os << indent << "SubsampleRate: " << this->SubsampleRate << endl;
}
