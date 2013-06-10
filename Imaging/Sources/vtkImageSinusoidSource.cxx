/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSinusoidSource.h"

#include "vtkMath.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageSinusoidSource);

//----------------------------------------------------------------------------
vtkImageSinusoidSource::vtkImageSinusoidSource()
{
  this->Direction[0] = 1.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 0.0;

  this->Amplitude = 255.0;
  this->Phase = 0.0;
  this->Period = 20.0;

  this->WholeExtent[0] = 0;  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;  this->WholeExtent[5] = 0;

  this->SetNumberOfInputPorts(0);
}

void vtkImageSinusoidSource::SetDirection(double v[3])
{
  this->SetDirection(v[0],v[1],v[2]);
}

void vtkImageSinusoidSource::SetDirection(double v0, double v1, double v2)
{
  double sum;

  sum = v0*v0 + v1*v1 + v2*v2;

  if (sum == 0.0)
    {
    vtkErrorMacro("Zero direction vector");
    return;
    }

  // normalize
  sum = 1.0 / sqrt(sum);
  v0 *= sum;
  v1 *= sum;
  v2 *= sum;

  if (this->Direction[0] == v0 && this->Direction[1] == v1
      && this->Direction[2] == v2)
    {
    return;
    }

  this->Direction[0] = v0;
  this->Direction[1] = v1;
  this->Direction[2] = v2;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageSinusoidSource::SetWholeExtent(int xMin, int xMax,
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

//----------------------------------------------------------------------------
int vtkImageSinusoidSource::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkDataObject::SPACING(), 1.0, 1.0, 1.0);
  outInfo->Set(vtkDataObject::ORIGIN(),  0.0, 0.0, 0.0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->WholeExtent,6);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageSinusoidSource::ExecuteDataWithInformation(vtkDataObject *output,
                                                        vtkInformation* outInfo)
{
  vtkImageData *data = this->AllocateOutputData(output, outInfo);
  double *outPtr;
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int *outExt;
  double sum;
  double yContrib, zContrib, xContrib;
  unsigned long count = 0;
  unsigned long target;

  if (data->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro("Execute: This source only outputs doubles");
    }

  outExt = data->GetExtent();

  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];

  // Get increments to march through data
  data->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outPtr = static_cast<double *>(
    data->GetScalarPointer(outExt[0],outExt[2],outExt[4]));

  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    zContrib = this->Direction[2] * (idxZ + outExt[4]);
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!(count%target))
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;
      yContrib = this->Direction[1] * (idxY + outExt[2]);
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        xContrib = this->Direction[0] * static_cast<double>(idxX + outExt[0]);
        // find dot product
        sum = zContrib + yContrib + xContrib;

        *outPtr = this->Amplitude *
          cos((2.0 * vtkMath::Pi() * sum / this->Period) - this->Phase);
        outPtr++;
        }
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
}

//----------------------------------------------------------------------------
void vtkImageSinusoidSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Period: " << this->Period << "\n";
  os << indent << "Phase: " << this->Phase << "\n";
  os << indent << "Amplitude: " << this->Amplitude << "\n";
  os << indent << "Direction: ( "
     << this->Direction[0] << ", "
     << this->Direction[1] << ", "
     << this->Direction[2] << " )\n";

}

