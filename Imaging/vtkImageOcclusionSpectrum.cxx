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

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageOcclusionSpectrum);

//----------------------------------------------------------------------------
vtkImageOcclusionSpectrum::vtkImageOcclusionSpectrum()
{
  this->Radius = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);

  this->SetNumberOfThreads(8);
}

//----------------------------------------------------------------------------
void vtkImageOcclusionSpectrum::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Radius : " << this->Radius << "\n";
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

#include <algorithm>
#include <iterator>

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

  cout << "Input whole extent ";
  for (int i = 0; i < 6; ++i) { cout << wholeExtent[i] << " "; } cout << endl;

  // Get the requested update extent from the output.
  int updateExtent [6] = {0};
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);

  cout << "Output initial update extent ";
  for (int i = 0; i < 6; ++i) { cout << updateExtent[i] << " "; } cout << endl;

  // Radius of the neighboring sphere
  this->Radius = wholeExtent[1] - wholeExtent[0] + 1
               + wholeExtent[3] - wholeExtent[2] + 1
               + wholeExtent[5] - wholeExtent[4] + 1;
  this->Radius/= 3;
  this->Radius*=.3;

  for (int i = 0, I = 3; i < I; ++i)
    {
    int const l = i*2;
    int const u = l+1;

    updateExtent[l] -= this->Radius;
    updateExtent[u] += this->Radius;

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

namespace
{
  // A list of possible choices of M
  struct LinearRamp
  {
    template <typename T>
    T operator () (T x) const
    {
      return x;
    }
  };
  struct TruncatedLinearRamp
  {
    template <typename T>
    T operator () (T x) const
    {
      static T const l = 0, u = 1; // This choice of values are arbitrary.
      return l < x && x < u ? x : 0;
    }
  };
  // struct DistanceWeighted
  // {
  //   template <typename T>
  //   T operator () (T x) const
  //   {
  //     return x * exp();
  //   }
  // };

  template <typename T, typename Functor>
  void __execute
  (Functor const& M, vtkImageOcclusionSpectrum* const self,
   vtkImageData* const IData, T const* const IPointer,
   vtkImageData* const OData, double * OPointer,
   int OExtent [6], int const tid)
  {
    // Local data structure to hold related information together for ease of
    // manipulation.
    struct
      {
      int      * ext; // extent of the data
      vtkIdType* inc; // increment to get to the memory location of
                      // the next logical element if the data memory extent
                      // and dimension extent are the same.
                      // X == 0
                      // Y == ext[0]
                      // Z == ext[0] * ext[1]
      vtkIdType jmp [3]; // increment to get to the memory location of
                         // the next logical element if the data memory
                         // extent and dimension extent are different
                         // which means there are "gaps" after each line
                         // and each slice and we need to jump.
      }
    I = { IData->GetExtent(), IData->GetIncrements() },
    O = { OExtent           , OData->GetIncrements() };
    IData->GetContinuousIncrements(OExtent, I.jmp[0], I.jmp[1], I.jmp[2]);
    OData->GetContinuousIncrements(OExtent, O.jmp[0], O.jmp[1], O.jmp[2]);

    // if (0 == tid)
    // {
    // cout << "thread id : " << tid << "\n";
    // cout << " input : \n"
    //      << "  ext : " << I.ext[0] << " " << I.ext[1] << " "
    //                    << I.ext[2] << " " << I.ext[3] << " "
    //                    << I.ext[4] << " " << I.ext[5] << "\n"
    //      << "  inc : " << I.inc[0] << " " << I.inc[1] << " " << I.inc[2] << "\n"
    //      << "  jmp : " << I.jmp[0] << " " << I.jmp[1] << " " << I.jmp[2] << "\n";
    // cout << " output: \n"
    //      << "  ext : " << O.ext[0] << " " << O.ext[1] << " "
    //                    << O.ext[2] << " " << O.ext[3] << " "
    //                    << O.ext[4] << " " << O.ext[5] << "\n"
    //      << "  inc : " << O.inc[0] << " " << O.inc[1] << " " << O.inc[2] << "\n"
    //      << "  jmp : " << O.jmp[0] << " " << O.jmp[1] << " " << O.jmp[2] << "\n";
    // cout << endl;
    // }

    // Loop through all voxels in the rectangular output extent.
    for (int z = OExtent[4]; z <= OExtent[5]; ++z, OPointer+=O.jmp[2])
    for (int y = OExtent[2]; y <= OExtent[3]; ++y, OPointer+=O.jmp[1])
    for (int x = OExtent[0]; x <= OExtent[1]; ++x, ++OPointer)
      {
      // Get the extent of the neighbor box of the current voxel.
      int NExtent [6] =
        {
        x - self->Radius, x + self->Radius,
        y - self->Radius, y + self->Radius,
        z - self->Radius, z + self->Radius,
        };

      // Adjust neighbor extent to make sure
      // it falls into the input extent in all dimension.
      for (int i = 0; i < 3; ++i)
        {
        int const l = i*2;
        int const u = l+1;

        if (NExtent[l] < I.ext[l])
          {
          NExtent[l] = I.ext[l];
          }
        if (NExtent[u] > I.ext[u])
          {
          NExtent[u] = I.ext[u];
          }
        }

      // if (0 == tid)
      // {
      // cout << "voxel : " << x << " " << y << " " << z << "\n";
      // cout << "NExt  : " << NExtent[0] << " " << NExtent[1] << " " << NExtent[2] << " "
      //                    << NExtent[3] << " " << NExtent[4] << " " << NExtent[5] << "\n";
      // cout << "IExt  : " << I.ext[0] << " " << I.ext[1] << " " << I.ext[2] << " "
      //                    << I.ext[3] << " " << I.ext[4] << " " << I.ext[5] << "\n";
      // cout << endl;
      // }

      // Advance input data pointer to the start location of neighbor extent.
      T const* __IPointer = IPointer
                          + (NExtent[0] - I.ext[0]) * I.inc[0]
                          + (NExtent[2] - I.ext[2]) * I.inc[1]
                          + (NExtent[4] - I.ext[4]) * I.inc[2];
      vtkIdType __jmp [3] = {0}; // Jumping in the neighbor extent
      IData->GetContinuousIncrements(NExtent, __jmp[0], __jmp[1], __jmp[2]);

      // Loop through each grid point in the neighbor extent
      T   sum = 0; // T used to avoid unnecessary cast on every accumulation.
      int num = 0;
      for (int k = NExtent[4]; k <= NExtent[5]; ++k, __IPointer+=__jmp[2])
      for (int j = NExtent[2]; j <= NExtent[3]; ++j, __IPointer+=__jmp[1])
      for (int i = NExtent[0]; i <= NExtent[1]; ++i, ++__IPointer)
        {
        // MUST be within a spherical neighborhood.
        if (((x-i)*(x-i)+(y-j)*(y-j)+(z-k)*(z-k)) <= self->Radius*self->Radius)
          {
          sum += M(*__IPointer);
          ++num;
          }
        }

      // Generate the corresponding output data.
      *OPointer = num ? (double)sum / num : 0; // Cast only once here.
      }
  }
}

void vtkImageOcclusionSpectrum::ThreadedRequestData
(vtkInformation*,
 vtkInformationVector** inputVector, vtkInformationVector*,
 vtkImageData*** inData, vtkImageData** outData,
 int outExt [6], int threadId)
{
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
    vtkTemplateMacro(__execute(LinearRamp(), this,
      input , static_cast<VTK_TT*>(inputArray->GetVoidPointer(0)),
      output, static_cast<double*>(output->GetScalarPointerForExtent(outExt)),
      outExt, threadId));
    default :
      vtkErrorMacro("Execute: Unknown ScalarType " << input->GetScalarType());
      return;
    }
}
