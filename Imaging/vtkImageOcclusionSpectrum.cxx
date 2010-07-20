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
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <iterator>
#include <numeric>

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
   vtkImageData* const IData, T const* IPtr,
   vtkImageData* const OData, double * OPtr,
   int OExt [6], int const tid)
  {
    // Build inclusive prefix sum volume that speed up later computation by N.
    vtkSmartPointer<vtkImageData> SData = vtkSmartPointer<vtkImageData>::New();

    // Make sure the prefix sum volume has EXACTLY the same shape
    // as the input update extent of the input volume.
    SData->SetExtent(IData->GetUpdateExtent());
    SData->SetNumberOfScalarComponents(IData->GetNumberOfScalarComponents());
    SData->SetScalarType(IData->GetScalarType());

    // Generate prefix sum volume.
    // Get the pointers to the first element that we should work on.
    int const* const IExt = IData->GetExtent();
    int const* const SExt = SData->GetExtent();

    IPtr += (SExt[0] - IExt[0]) * IData->GetIncrements()[0]
          + (SExt[2] - IExt[2]) * IData->GetIncrements()[1]
          + (SExt[4] - IExt[4]) * IData->GetIncrements()[2];
    T* SPtr = static_cast<T*>
              (SData->GetPointData()->GetScalars()->GetVoidPointer(0));

    vtkIdType IIncs [3] = {0};
    IData->GetContinuousIncrements
      (const_cast<int*>(SExt), IIncs[0], IIncs[1], IIncs[2]);

    // Loop through each ROW in the input extent of the volume.
    for (int z = IExt[4]; z <= IExt[5]; ++z, IPtr += IIncs[2])
    for (int y = IExt[2]; y <= IExt[3]; ++y, IPtr += IIncs[1])
      {
      // Generate prefix sum array by accumulating along x direction.
      SPtr = vtkstd::partial_sum(IPtr + IExt[0], IPtr + IExt[1] + 1, SPtr);
      }

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

    int const __r2 = self->Radius * self->Radius;

    vtkIdType OIncs [3] = {0};
    OData->GetContinuousIncrements(OExt, OIncs[0], OIncs[1], OIncs[2]);

    // Loop through all points in the output extent of the output volume.
    for (int z = OExt[4]; z <= OExt[5]; ++z, OPtr += OIncs[2])
    for (int y = OExt[2]; y <= OExt[3]; ++y, OPtr += OIncs[1])
    for (int x = OExt[0]; x <= OExt[1]; ++x, ++OPtr)
      {
      // Get the extent of the neighbor box of the current voxel.
      int NExt [6] =
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

        if (NExt[l] < IExt[l])
          {
          NExt[l] = IExt[l];
          }
        if (NExt[u] > IExt[u])
          {
          NExt[u] = IExt[u];
          }
        }

      // if (0 == tid)
      // {
      // cout << "voxel : " << x << " " << y << " " << z << "\n";
      // cout << "NExt  : " << NExt[0] << " " << NExt[1] << " " << NExt[2] << " "
      //                    << NExt[3] << " " << NExt[4] << " " << NExt[5] << "\n";
      // cout << "IExt  : " << I.ext[0] << " " << I.ext[1] << " " << I.ext[2] << " "
      //                    << I.ext[3] << " " << I.ext[4] << " " << I.ext[5] << "\n";
      // cout << endl;
      // }

      // Advance input data pointer to the start location of neighbor extent.
      // T const* __IPtr = IPtr
      //                 + (NExt[0] - I.ext[0]) * I.inc[0]
      //                 + (NExt[2] - I.ext[2]) * I.inc[1]
      //                 + (NExt[4] - I.ext[4]) * I.inc[2];
      // vtkIdType __I [3] = {0}; // Jumping in the neighbor extent
      // IData->GetContinuousIncrements(NExt,__I[0],__I[1],__I[2]);

      // Since we have the prefix sum volume so that we can speed up
      // the following process by a factor of N.
      // For each point in the output extent of the output volume.
      // [0] Generate its neighbor sphere.
      // [1] Project its neighbor sphere onto YOZ plane,
      //     i.e., compress in X direction.
      // [2] For each point that lays in or on the projected disk.
      // [2.1] Compute the span in X direction of the intersection of
      //       the neighbor sphere and the prefix sum volume.
      // [2.2] The difference between the two intersection points are the sum of
      //       all points that lay on the intersected row. Make sure to use the
      //       point of larger X index to minus the one of smaller X index.

      T   sum = 0; // T used to avoid unnecessary cast on every accumulation.
      int num = 0;

      int const* SIncs = SData->GetIncrements();
      T* SPtr = static_cast<T*>
                (SData->GetPointData()->GetScalars()->GetVoidPointer(0));

      // Loop through each grid point in the projected disk
      // of the neighbor sphere.
      for (int k = NExt[4]; k <= NExt[5]; ++k)
      for (int j = NExt[2]; j <= NExt[3]; ++j)
        {
        int const t = k * SIncs[2] + j * SIncs[1];
        if (int const radius = x + sqrt(__r2 - (y-j)*(y-j) - (z-k)*(z-k)))
          {
          int l = x - radius;
          int u = x + radius;

          // Make sure we do not fall off the prefix sum volume whole extent.
          if (l < SExt[0])
            {
            l = SExt[0];
            }
          if (u > SExt[1])
            {
            u = SExt[1];
            }

          sum += SPtr[t+u] - SPtr[t+l];
          num += 2*radius + 1;
          }
        else
          {
          if (0 == x)
            {
            sum += SPtr[t];
            }
          else
            {
            sum += SPtr[t+x] - SPtr[t+x-1];
            }
          ++num;
          }
        }

      // Generate the corresponding output occlusion spectrum.
      // Guard against divide by zero and cast only once here.
      *OPtr = num ? (double)sum / num : 0;
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
