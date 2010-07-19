/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOcclusionSpectrum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageOcclusionSpectrum.h"

vtkStandardNewMacro(vtkImageOcclusionSpectrum);

//----------------------------------------------------------------------------
vtkImageOcclusionSpectrum::vtkImageOcclusionSpectrum()
{
  this->Radii[0] = this->Radii[1] = this->Radii[2] = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
void vtkImageOcclusionSpectrum::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Radii : "
     << this->Radii[0] << " "
     << this->Radii[1] << " "
     << this->Radii[2] << "\n";
}

int vtkImageOcclusionSpectrum::RequestInformation
(vtkInformation*,
 vtkInformationVector** inputVector,
 vtkInformationVector* outputVector)
{
  // Get input and output pipeline information.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo  = inputVector[0]->GetInformationObject(0);

  // Set the number of point data componets to 1
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, 1);

  return 1;
}

int vtkImageOcclusionSpectrum::RequestUpdateExtent
(vtkInformation*,
 vtkInformationVector** inputVector,
 vtkInformationVector* outputVector)
{
  // Get input and output pipeline information.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo  = inputVector[0]->GetInformationObject(0);

  // Get the input whole extent.
  int wholeExtent [6] = {0};
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  // Get the requested update extent from the output.
  int updateExtent [6] = {0};
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);

  // Radii of the neighboring sphere
  this->Radii[0] = .3 * (wholeExtent[1]-wholeExtent[0]);
  this->Radii[1] = .3 * (wholeExtent[3]-wholeExtent[2]);
  this->Radii[2] = .3 * (wholeExtent[5]-wholeExtent[4]);

  for (int i = 0, I = 3; i < I; ++i)
    {
    int const l = i*2;
    int const u = l+1;

    updateExtent[l] -= this->Radii[i];
    updateExtent[u] += this->Radii[i];

    if (updateExtent[l] < 0)
      {
       updateExtent[l] = 0;
      }
    if (updateExtent[u] > wholeExtent[u])
      {
      updateExtent[u] = wholeExtent[u];
      }
    }

  // Store the update extent needed from the intput.
  inInfo->Set
    (vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent, 6);

  return 1;
}

void vtkImageOcclusionSpectrum::ThreadedRequestData
(vtkInformation*, vtkInformationVector**, vtkInformationVector*,
 vtkImageData*** inData, vtkImageData** outData, int outExt [6],
 int threadId)
{
  if (3 != this->GetDimensionality)
    {
    vtkErrorMacro("vtkImageOcclusionSpectrum only works with 3D volume data.");
    return;
    }

  // Get the input and output data objects.
  vtkImageData* input  = **inData;
  vtkImageData* output = *outData;

  // The ouptut scalar type must be double to store proper gradients.
  if (output->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro("Execute: output ScalarType is "
                  << output->GetScalarType() << " but must be double.");
    return;
    }

  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (!inputArray)
    {
    vtkErrorMacro("No input array was found. Cannot execute");
    return;
    }

  if (inputArray->GetNumberOfComponents() != 1)
    {
    vtkErrorMacro(
      "Execute: input has more than one component. "
      "The input to occlusion spectrum should be a single component image.");
    return;
    }

  switch (inputArray->GetDataType())
    {
    vtkTemplateMacro(Execute(this,
      input ,static_cast<VTK_TT*>(inputArray->GetVoidPointer(0)),
      output,static_cast<double*>(output->GetScalarPointerForExtent(outExt)),
      outExt,threadId));
    default :
      vtkErrorMacro("Execute: Unknown ScalarType " << input->GetScalarType());
      return;
    }
}

namespace
{
  template <typename T>
  void Execute
  (vtkImageOcclusionSpectrum* const self,
   vtkImageData* const inData , T     * const inPtr,
   vtkImageData* const outData, double* const outPtr,
   int const outExt [6], int const tid)
  {
    // Input extent and increments for each dimension.
    int       const* const inExtent = inData->GetExtent();
    vtkIdType const* const inIncs   = inData->GetIncrements();

    int       const* const outExtent= outData->GetExtent();

    // Loop through all voxels in the rectangular output extent.
    for (int z = outExt[4]; z <= outExt[5]; ++z)
    for (int y = outExt[3]; y <= outExt[2]; ++y)
    for (int x = outExt[1]; x <= outExt[0]; ++x)
      {
        // Get the extent of the neighbor box.
        int extent [6] =
        {
          x - this->Radii[0], x + this->Radii[0],
          y - this->Radii[1], y + this->Radii[1],
          z - this->Radii[2], z + this->Radii[2],
        };

        // Adjust the extent to make sure it falls into the input extent.
        for (int i = 0; i < 3; ++i)
          {
           if (extent[2*i] < inExtent[2*i])
             {
             extent[2*i] = inExtent[2*i];
             }
           if (extent[2*i+1] > inExtent[2*i+1])
             {
             extent[2*i+1] = inExtent[2*i+1];
             }
          }

        // Sum all data in the extent.
        double sum = 0;
        int    num = 0;
        for (int k = extent[4]; k <= extent[5]; ++k)
        for (int j = extent[2]; j <= extent[3]; ++j)
        for (int i = extent[0]; i <= extent[1]; ++i)
          {
          sum += inPtr[k*inIncs[2]+j*inIncs[1]+i*inIncs[0]];
          ++num;
          }

        // Average to get the output for the current voxel.
        if (num)
          {
          sum /= num;
          }
        else
          {
           sum = 0;
          }
        outPtr[z*outIncs[2]+y*outIncs[1]+x*outIncs[0]] = sum;
      }
  }
}
