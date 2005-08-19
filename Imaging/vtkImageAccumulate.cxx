/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAccumulate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageAccumulate.h"

#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageAccumulate, "1.63");
vtkStandardNewMacro(vtkImageAccumulate);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageAccumulate::vtkImageAccumulate()
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->ComponentSpacing[idx] = 1.0;
    this->ComponentOrigin[idx] = 0.0;
    this->ComponentExtent[idx*2] = 0;
    this->ComponentExtent[idx*2+1] = 0;
    }
  this->ComponentExtent[1] = 255;
  
  this->ReverseStencil = 0;

  this->Min[0] = this->Min[1] = this->Min[2] = 0.0;
  this->Max[0] = this->Max[1] = this->Max[2] = 0.0;
  this->Mean[0] = this->Mean[1] = this->Mean[2] = 0.0;
  this->StandardDeviation[0] = this->StandardDeviation[1] = this->StandardDeviation[2] = 0.0;  
  this->VoxelCount = 0;

  // we have the image input and the optional stencil input
  this->SetNumberOfInputPorts(2);
}


//----------------------------------------------------------------------------
vtkImageAccumulate::~vtkImageAccumulate()
{
}

//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->ComponentExtent[idx] != extent[idx])
      {
      this->ComponentExtent[idx] = extent[idx];
      this->Modified();
      }
    }
  if (modified)
    {
    this->Modified();
    }
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentExtent(int minX, int maxX, 
                                            int minY, int maxY,
                                            int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetComponentExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::GetComponentExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->ComponentExtent[idx];
    }
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::SetStencil(vtkImageStencilData *stencil)
{
  this->SetInput(1, stencil); 
}


//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageAccumulate::GetStencil()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return 0;
    }
  return vtkImageStencilData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageAccumulateExecute(vtkImageAccumulate *self,
                               vtkImageData *inData, T *inPtr,
                               vtkImageData *outData, int *outPtr,
                               double Min[3], double Max[3],
                               double Mean[3],
                               double StandardDeviation[3],
                               long int *VoxelCount,
                               int* updateExtent)
{
  int idX, idY, idZ, idxC;
  int iter, pmin0, pmax0, min0, max0, min1, max1, min2, max2;
  vtkIdType inInc0, inInc1, inInc2;
  T *tempPtr;
  int *outPtrC;
  int numC, outIdx, *outExtent;
  vtkIdType *outIncs;
  double *origin, *spacing;
  unsigned long count = 0;
  unsigned long target;
  double sumSqr[3], variance;

  // variables used to compute statistics (filter handles max 3 components)
  double sum[3];
  sum[0] = sum[1] = sum[2] = 0.0;
  Min[0] = Min[1] = Min[2] = VTK_DOUBLE_MAX;
  Max[0] = Max[1] = Max[2] = VTK_DOUBLE_MIN;
  sumSqr[0] = sumSqr[1] = sumSqr[2] = 0.0;
  StandardDeviation[0] = StandardDeviation[1] = StandardDeviation[2] = 0.0;
  *VoxelCount = 0;
  
  vtkImageStencilData *stencil = self->GetStencil();

  // Zero count in every bin
  outData->GetExtent(min0, max0, min1, max1, min2, max2);
  memset((void *)outPtr, 0, 
         (max0-min0+1)*(max1-min1+1)*(max2-min2+1)*sizeof(int));
    
  // Get information to march through data 
  numC = inData->GetNumberOfScalarComponents();
  min0 = updateExtent[0];
  max0 = updateExtent[1];
  min1 = updateExtent[2];
  max1 = updateExtent[3];
  min2 = updateExtent[4];
  max2 = updateExtent[5];
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outExtent = outData->GetExtent();
  outIncs = outData->GetIncrements();
  origin = outData->GetOrigin();
  spacing = outData->GetSpacing();

  target = (unsigned long)((max2 - min2 + 1)*(max1 - min1 +1)/50.0);
  target++;

  int reverse = self->GetReverseStencil();
  
  // Loop through input pixels
  for (idZ = min2; idZ <= max2; idZ++)
    {
    for (idY = min1; idY <= max1; idY++)
      {
      if (!(count%target))
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;

      // loop over stencil sub-extents, -1 flags
      // that we want the complementary extents
      iter = reverse ? -1 : 0;

      pmin0 = min0;
      pmax0 = max0;
      while ((stencil != 0 && 
              stencil->GetNextExtent(pmin0,pmax0,min0,max0,idY,idZ,iter)) ||
             (stencil == 0 && iter++ == 0))
        {
        // set up pointer for sub extent
        tempPtr = inPtr + (inInc2*(idZ - min2) +
                           inInc1*(idY - min1) +
                           numC*(pmin0 - min0));

        // accumulate over the sub extent
        for (idX = pmin0; idX <= pmax0; idX++)
          {
          // find the bin for this pixel.
          outPtrC = outPtr;
          for (idxC = 0; idxC < numC; ++idxC)
            {
            // Gather statistics
            sum[idxC]+= *tempPtr;
            sumSqr[idxC]+= (*tempPtr * *tempPtr);
            if (*tempPtr > Max[idxC])
              {
              Max[idxC] = *tempPtr;
              }
            else if (*tempPtr < Min[idxC])
              {
              Min[idxC] = *tempPtr;
              }
            (*VoxelCount)++;
            // compute the index
            outIdx = (int) floor((((double)*tempPtr++ - origin[idxC]) 
                                  / spacing[idxC]));
            if (outIdx < outExtent[idxC*2] || outIdx > outExtent[idxC*2+1])
              {
              // Out of bin range
              outPtrC = NULL;
              break;
              }
            outPtrC += (outIdx - outExtent[idxC*2]) * outIncs[idxC];
            }
          if (outPtrC)
            {
            ++(*outPtrC);
            }
          }
        }
      }
    }
  
  if (*VoxelCount) // avoid the div0
    {
    Mean[0] = sum[0] / (double)*VoxelCount;    
    Mean[1] = sum[1] / (double)*VoxelCount;    
    Mean[2] = sum[2] / (double)*VoxelCount;    

    variance = sumSqr[0] / (double)(*VoxelCount-1) - ((double) *VoxelCount * Mean[0] * Mean[0] / (double) (*VoxelCount - 1));
    StandardDeviation[0] = sqrt(variance);
    variance = sumSqr[1] / (double)(*VoxelCount-1) - ((double) *VoxelCount * Mean[1] * Mean[1] / (double) (*VoxelCount - 1));
    StandardDeviation[1] = sqrt(variance);
    variance = sumSqr[2] / (double)(*VoxelCount-1) - ((double) *VoxelCount * Mean[2] * Mean[2] / (double) (*VoxelCount - 1));
    StandardDeviation[2] = sqrt(variance);
    }
  else
    {
    Mean[0] = Mean[1] = Mean[2] = 0.0;
    StandardDeviation[0] = StandardDeviation[1] = StandardDeviation[2] = 0.0;
    }
  
}

        

//----------------------------------------------------------------------------
// This method is passed a input and output Data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the Datas data types.
int vtkImageAccumulate::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  void *inPtr;
  void *outPtr;
  
  // get the input
  vtkInformation* in1Info = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    in1Info->Get(vtkDataObject::DATA_OBJECT()));
  int *uExt = in1Info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  
  // get the output
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkDebugMacro(<<"Executing image accumulate");
  
  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  outData->SetExtent(outData->GetWholeExtent());
  outData->AllocateScalars();
  
  vtkDataArray *inArray = this->GetInputArrayToProcess(0,inputVector);
  inPtr = inData->GetArrayPointerForExtent(inArray, uExt);
  outPtr = outData->GetScalarPointer();
  
  // Components turned into x, y and z
  if (inData->GetNumberOfScalarComponents() > 3)
    {
    vtkErrorMacro("This filter can handle upto 3 components");
    return 1;
    }
  
  // this filter expects that output is type int.
  if (outData->GetScalarType() != VTK_INT)
    {
    vtkErrorMacro(<< "Execute: out ScalarType " << outData->GetScalarType()
                  << " must be int\n");
    return 1;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageAccumulateExecute( this, 
                                                inData, (VTK_TT *)(inPtr), 
                                                outData, (int *)(outPtr),
                                                this->Min, this->Max,
                                                this->Mean,
                                                this->StandardDeviation, 
                                                &this->VoxelCount,
                                                uExt ));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 1;
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkImageAccumulate::RequestInformation (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->ComponentExtent,6);
  outInfo->Set(vtkDataObject::ORIGIN(),this->ComponentOrigin,3);
  outInfo->Set(vtkDataObject::SPACING(),this->ComponentSpacing,3);

  // need to set the spacing and origin of the stencil to match the output
  if (inInfo2)
    {
    inInfo2->Set(vtkDataObject::SPACING(),
                 inInfo->Get(vtkDataObject::SPACING()),3);
    inInfo2->Set(vtkDataObject::ORIGIN(),
                 inInfo->Get(vtkDataObject::ORIGIN()),3);
    }

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_INT, 1);
  return 1;
}

//----------------------------------------------------------------------------
// Get ALL of the input.
int vtkImageAccumulate::RequestUpdateExtent (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
              inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),6);

  return 1;
}

int vtkImageAccumulate::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    // the stencil input is optional
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    }
  return 1;
}

void vtkImageAccumulate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Mean: ("
     << this->Mean[0] << ", "
     << this->Mean[1] << ", "
     << this->Mean[2] << ")\n";
  os << indent << "Min: ("
     << this->Min[0] << ", "
     << this->Min[1] << ", "
     << this->Min[2] << ")\n";
  os << indent << "Max: ("
     << this->Max[0] << ", "
     << this->Max[1] << ", "
     << this->Max[2] << ")\n";
  os << indent << "StandardDeviation: ("
     << this->StandardDeviation[0] << ", "
     << this->StandardDeviation[1] << ", "
     << this->StandardDeviation[2] << ")\n";
  os << indent << "VoxelCount: " << this->VoxelCount << "\n";
  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "ReverseStencil: " << (this->ReverseStencil ?
                                         "On\n" : "Off\n");

  os << indent << "ComponentOrigin: ( "
     << this->ComponentOrigin[0] << ", "
     << this->ComponentOrigin[1] << ", "
     << this->ComponentOrigin[2] << " )\n";

  os << indent << "ComponentSpacing: ( "
     << this->ComponentSpacing[0] << ", "
     << this->ComponentSpacing[1] << ", "
     << this->ComponentSpacing[2] << " )\n";

  os << indent << "ComponentExtent: ( "
     << this->ComponentExtent[0] << "," << this->ComponentExtent[1] << " "
     << this->ComponentExtent[2] << "," << this->ComponentExtent[3] << " "
     << this->ComponentExtent[4] << "," << this->ComponentExtent[5] << " }\n";
}

