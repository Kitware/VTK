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

#include <cmath>

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
  if (this->WholeExtent[0] > this->WholeExtent[1] ||
    this->WholeExtent[2] > this->WholeExtent[3] ||
    this->WholeExtent[4] > this->WholeExtent[5])
  {
    vtkErrorMacro("Invalid WholeExtent: "
      << this->WholeExtent[0] << ", " << this->WholeExtent[1] << ", "
      << this->WholeExtent[2] << ", " << this->WholeExtent[3] << ", "
      << this->WholeExtent[4] << ", " << this->WholeExtent[5]);
    return 0;
  }

  int tmpExt[6], i;
  for (i = 0; i < 3; i++)
  {
    tmpExt[2*i] = this->WholeExtent[2*i] / this->SubsampleRate;
    tmpExt[2*i+1] = this->WholeExtent[2*i+1] / this->SubsampleRate;
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               tmpExt,6);

  outInfo->Set(vtkDataObject::ORIGIN(),  0.0, 0.0, 0.0);
  outInfo->Set(vtkDataObject::SPACING(), this->SubsampleRate,
               this->SubsampleRate, this->SubsampleRate);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);

  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

void vtkRTAnalyticSource::ExecuteDataWithInformation(vtkDataObject *vtkNotUsed(output),
                                                     vtkInformation *outInfo)
{
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

  // Split the update extent further based on piece request.
  int* execExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  // For debugging
  /*
  int numPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numGhosts = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (piece == 0)
    {
    cout << "Piece:" << piece << " " << numPieces << " " << numGhosts << endl;
    cout << "Extent: "
         << execExt[0] << " "
         << execExt[1] << " "
         << execExt[2] << " "
         << execExt[3] << " "
         << execExt[4] << " "
         << execExt[5] << endl;
    }
  */

  vtkImageData *data = vtkImageData::GetData(outInfo);
  this->AllocateOutputData(data, outInfo, execExt);
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

  // Loop through output pixels
  temp2 = 1.0 / (2.0 * this->StandardDeviation * this->StandardDeviation);

  double x, y, z;
  const double xscale = (whlExt[1] > whlExt[0])? (1.0/(whlExt[1] - whlExt[0])) : 1.0;
  const double yscale = (whlExt[3] > whlExt[2])? (1.0/(whlExt[3] - whlExt[2])) : 1.0;
  const double zscale = (whlExt[5] > whlExt[4])? (1.0/(whlExt[5] - whlExt[4])) : 1.0;
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    if ((this->SubsampleRate > 1) && (idxZ % this->SubsampleRate))
    {
      continue;
    }
    z = this->Center[2] - (idxZ + newOutExt[4]);
    z *= zscale;
    zContrib = z * z;
    const float zfactor = static_cast<float>(this->ZMag*cos(this->ZFreq*z));
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
    {
      if ((this->SubsampleRate > 1) && (idxY % this->SubsampleRate))
      {
        continue;
      }
      if (!(count%target))
      {
        this->UpdateProgress(count/(50.0*target));
      }
      count++;
      y = this->Center[1] - (idxY + newOutExt[2]);
      y *= yscale;
      yContrib = y * y;
      const float yfactor = static_cast<float>(this->YMag*sin(this->YFreq*y));
      for (idxX = 0; idxX <= maxX; idxX++)
      {
        if ((this->SubsampleRate > 1) && (idxX % this->SubsampleRate))
        {
          continue;
        }
        // Pixel operation
        sum = zContrib + yContrib;
        x = this->Center[0] - (idxX + newOutExt[0]);
        x *= xscale;
        sum = sum + (x * x);
        const float xfactor = static_cast<float>(this->XMag*sin(this->XFreq*x));
        *outPtr = this->Maximum * exp(-sum * temp2)
          + xfactor /*this->XMag*sin(this->XFreq*x)*/
          + yfactor /*this->YMag*sin(this->YFreq*y)*/
          + zfactor /*this->ZMag*cos(this->ZFreq*z)*/;
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

int vtkRTAnalyticSource::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  if (!this->Superclass::FillOutputPortInformation(port, info))
  {
    return 0;
  }

  return 1;
}
