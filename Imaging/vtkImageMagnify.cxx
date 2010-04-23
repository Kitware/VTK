/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMagnify.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageMagnify);

//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
vtkImageMagnify::vtkImageMagnify()
{
  this->Interpolate = 0;

  this->MagnificationFactors[0] = 
  this->MagnificationFactors[1] = 
  this->MagnificationFactors[2] = 1;
}

//----------------------------------------------------------------------------
// Computes any global image information associated with regions.
int vtkImageMagnify::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  double spacing[3];
  int idx;
  double outSpacing[3];
  int inExt[6], outExt[6];
  
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Get(vtkDataObject::SPACING(), spacing);
  
  for (idx = 0; idx < 3; idx++)
    {
    // Scale the output extent
    outExt[idx*2] = inExt[idx*2] * this->MagnificationFactors[idx];
    outExt[idx*2+1] = outExt[idx*2] + 
      (inExt[idx*2+1] - inExt[idx*2] + 1)*this->MagnificationFactors[idx] - 1;
    
    // Change the data spacing
    outSpacing[idx] = spacing[idx] /
      static_cast<double>(this->MagnificationFactors[idx]);
    }
  
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),outExt,6);
  outInfo->Set(vtkDataObject::SPACING(),outSpacing,3);

  return 1;
}

//----------------------------------------------------------------------------
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
int vtkImageMagnify::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int outExt[6], inExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);

  this->InternalRequestUpdateExtent(inExt, outExt);
  
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

void vtkImageMagnify::InternalRequestUpdateExtent(int *inExt, int *outExt)
{
  int idx;

  for (idx = 0; idx < 3; idx++)
    {
    // For Min. Round Down
    inExt[idx*2] = static_cast<int>(
      floor(static_cast<double>(outExt[idx*2]) / 
            static_cast<double>(this->MagnificationFactors[idx])));
    inExt[idx*2+1] = static_cast<int>(
      floor(static_cast<double>(outExt[idx*2+1]) / 
            static_cast<double>(this->MagnificationFactors[idx])));
    }
}

//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// 2d even though operation is 1d.
// Note: Slight misalignment (pixel replication is not nearest neighbor).
template <class T>
void vtkImageMagnifyExecute(vtkImageMagnify *self,
                            vtkImageData *inData, T *inPtr, int inExt[6],
                            vtkImageData *outData, T *outPtr,
                            int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int inIdxX, inIdxY, inIdxZ;
  int inMaxX, inMaxY, inMaxZ;
  int maxC, maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int interpolate;
  int magXIdx, magX;
  int magYIdx, magY;
  int magZIdx, magZ;
  T *inPtrZ, *inPtrY, *inPtrX, *outPtrC;
  double iMag, iMagP = 0.0, iMagPY = 0.0, iMagPZ = 0.0, iMagPYZ = 0.0;
  T dataP = 0, dataPX = 0, dataPY = 0, dataPZ = 0;
  T dataPXY = 0, dataPXZ = 0, dataPYZ = 0, dataPXYZ = 0;
  int interpSetup;
  
  interpolate = self->GetInterpolate();
  magX = self->GetMagnificationFactors()[0];
  magY = self->GetMagnificationFactors()[1];
  magZ = self->GetMagnificationFactors()[2];
  iMag = 1.0/(magX*magY*magZ);
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>(maxC*(maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Now I am putting in my own boundary check because of ABRs and FMRs
  // And I do not understand (nor do I care to figure out) what
  // Ken is doing with his checks. (Charles)
  inMaxX = inExt[1];
  inMaxY = inExt[3];
  inMaxZ = inExt[5];
  inData->GetExtent(idxC, inMaxX, idxC, inMaxY, idxC, inMaxZ);
  
  // Loop through ouput pixels
  for (idxC = 0; idxC < maxC; idxC++)
    {
    inPtrZ = inPtr + idxC;
    inIdxZ = inExt[4];
    outPtrC = outPtr + idxC;
    magZIdx = magZ - outExt[4]%magZ - 1;
    for (idxZ = 0; idxZ <= maxZ; idxZ++, magZIdx--)
      {
      inPtrY = inPtrZ;
      inIdxY = inExt[2];
      magYIdx = magY - outExt[2]%magY - 1;
      for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++, magYIdx--)
        {
        if (!id) 
          {
          if (!(count%target))
            {
            self->UpdateProgress(count/(50.0*target));
            }
          count++;
          }
        
        if (interpolate)
          {
          // precompute some values for interpolation
          iMagP = (magYIdx + 1)*(magZIdx + 1)*iMag;
          iMagPY = (magY - magYIdx - 1)*(magZIdx + 1)*iMag;
          iMagPZ = (magYIdx + 1)*(magZ - magZIdx - 1)*iMag;
          iMagPYZ = (magY - magYIdx - 1)*(magZ - magZIdx - 1)*iMag;
          }
        
        magXIdx = magX - outExt[0]%magX - 1;
        inPtrX = inPtrY;
        inIdxX = inExt[0];
        interpSetup = 0;
        for (idxX = 0; idxX <= maxX; idxX++, magXIdx--)
          {
          // Pixel operation
          if (!interpolate)
            {
            *outPtrC = *inPtrX;
            }
          else
            {
            // setup data values for interp, overload dataP as an 
            // indicator of if this has been done yet
            if (!interpSetup) 
              {
              int tiX, tiY, tiZ;
              
              dataP = *inPtrX;

              // Now I am putting in my own boundary check because of 
              // ABRs and FMRs
              // And I do not understand (nor do I care to figure out) what
              // Ken was doing with his checks. (Charles)
              if (inIdxX < inMaxX) 
                {
                tiX = inIncX;
                }
              else
                {
                tiX = 0;
                }
              if (inIdxY < inMaxY) 
                {
                tiY = inIncY;
                }
              else
                {
                tiY = 0;
                }
              if (inIdxZ < inMaxZ)
                {
                tiZ = inIncZ;
                }
              else
                {
                tiZ = 0;
                }
              dataPX = *(inPtrX + tiX); 
              dataPY = *(inPtrX + tiY); 
              dataPZ = *(inPtrX + tiZ);
              dataPXY = *(inPtrX + tiX + tiY); 
              dataPXZ = *(inPtrX + tiX + tiZ); 
              dataPYZ = *(inPtrX + tiY + tiZ); 
              dataPXYZ = *(inPtrX + tiX + tiY + tiZ); 
              interpSetup = 1;
              }
            *outPtrC = static_cast<T>
              (static_cast<double>(dataP)*(magXIdx + 1)*iMagP + 
               static_cast<double>(dataPX)*(magX - magXIdx - 1)*iMagP +
               static_cast<double>(dataPY)*(magXIdx + 1)*iMagPY + 
               static_cast<double>(dataPXY)*(magX - magXIdx - 1)*iMagPY +
               static_cast<double>(dataPZ)*(magXIdx + 1)*iMagPZ + 
               static_cast<double>(dataPXZ)*(magX - magXIdx - 1)*iMagPZ +
               static_cast<double>(dataPYZ)*(magXIdx + 1)*iMagPYZ + 
               static_cast<double>(dataPXYZ)*(magX - magXIdx - 1)*iMagPYZ);
            }
          outPtrC += maxC;
          if (!magXIdx) 
            {
            inPtrX += inIncX;
            ++inIdxX;
            magXIdx = magX;
            interpSetup = 0;
            }
          }
        outPtrC += outIncY;
        if (!magYIdx) 
          {
          inPtrY += inIncY;
          ++inIdxY;
          magYIdx = magY;
          }
        }
      outPtrC += outIncZ;
      if (!magZIdx) 
        {
        inPtrZ += inIncZ;
        ++inIdxZ;
        magZIdx = magZ;
        }
      }
    }
}

void vtkImageMagnify::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  int inExt[6];
  this->InternalRequestUpdateExtent(inExt, outExt);

  void *inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro("Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }
  
  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageMagnifyExecute(this, inData[0][0],
                             static_cast<VTK_TT *>(inPtr), inExt, outData[0], 
                             static_cast<VTK_TT *>(outPtr),
                             outExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageMagnify::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MagnificationFactors: ( "
     << this->MagnificationFactors[0] << ", "
     << this->MagnificationFactors[1] << ", "
     << this->MagnificationFactors[2] << " )\n";

  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");
}
