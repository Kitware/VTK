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
#include "vtkDoubleArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
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

  this->SetNumberOfThreads(1);
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

//   template <typename T>
//   void print (int const* ext, T const* data)
//   {
// #define for_(i, x, y) for (int i = x, I = y; i < I; ++i)
//     for_(z,ext[4],ext[5])
//       {
//       for_(y,ext[2],ext[3])
//         {
//         data = printv(data, ext[0], ext[1]);
//         cout << endl;
//         }
//       cout << endl;
//       }
// #undef for_
//   }
//
//   template <typename T>
//   T const* printv (T const* data, int b = 0, int e = 2)
//   {
//     for (; b <= e; ++b, ++data)
//       {
//       cout << *data;
//       }
//     return data;
//   }

  template <typename T>
  vtkSmartPointer<vtkImageData> prefix_sum
  (vtkImageData* const iimg, T const* idat)
  {
    // Build inclusive prefix sum volume which can speed up computation by N.
    vtkSmartPointer<vtkImageData> simg = vtkSmartPointer<vtkImageData>::New();

    // Make sure the prefix sum volume has EXACTLY the same shape and type
    // as the input update extent.
    simg->SetExtent(iimg->GetUpdateExtent());
    simg->SetNumberOfScalarComponents(iimg->GetNumberOfScalarComponents());
    simg->SetScalarType(iimg->GetScalarType());
    simg->AllocateScalars();

    int const* const iext = iimg->GetExtent();
    int const* const sext = simg->GetExtent();

    // Beginning of input and output.
    idat += (sext[0] - iext[0]) * iimg->GetIncrements()[0]
          + (sext[2] - iext[2]) * iimg->GetIncrements()[1]
          + (sext[4] - iext[4]) * iimg->GetIncrements()[2];
    T* sdat = static_cast<T*>(simg->GetScalarPointer());

    // Loop through each ROW in the input extent of the volume.
    vtkIdType iinc [3] = {0};
    iimg->GetContinuousIncrements
      (const_cast<int*>(sext), iinc[0], iinc[1], iinc[2]);
    for (int z = iext[4]; z <= iext[5]; ++z, idat += iinc[2])
    for (int y = iext[2]; y <= iext[3]; ++y, idat += iinc[1])
      {
      // Generate prefix sum array and advance output pointer.
      sdat = vtkstd::partial_sum(idat+iext[0], idat+iext[1]+1, sdat);
      idat+= iext[1]-iext[0]+1; // advance input pointer.
      }
    return simg;
  }

  template <typename T, typename Functor>
  void __execute
  (Functor const& M, vtkImageOcclusionSpectrum* const self,
   vtkImageData* const iimg, T const* idat,
   vtkImageData* const oimg, double * odat,
   int const oext [6], int const tid)
  {
    // Prefix sum volume
    vtkSmartPointer<vtkImageData> simg = prefix_sum(iimg, idat);

    int const r2 = self->Radius * self->Radius;
    int const* iext = iimg->GetExtent();

    // Loop through all points in the output extent of the output volume.
    vtkIdType oinc [3] = {0};
    oimg->GetContinuousIncrements
      (const_cast<int*>(oext), oinc[0], oinc[1], oinc[2]);
    for (int z = oext[4]; z <= oext[5]; ++z, odat += oinc[2])
    for (int y = oext[2]; y <= oext[3]; ++y, odat += oinc[1])
    for (int x = oext[0]; x <= oext[1]; ++x, ++odat)
      {
      // Adjust it if it falls out of the input extent in any dimension.
      int const next [6] = // Short for nExtent, not "next"
        {
        vtkstd::max(x-self->Radius,iext[0]),vtkstd::min(x+self->Radius,iext[1]),
        vtkstd::max(y-self->Radius,iext[2]),vtkstd::min(y+self->Radius,iext[3]),
        vtkstd::max(z-self->Radius,iext[4]),vtkstd::min(z+self->Radius,iext[5]),
        };

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

      int const* const sinc = simg->GetIncrements();
      int const* const sext = simg->GetExtent();
      T   const* const sdat = static_cast<T*>(simg->GetScalarPointer());

      // Loop through each grid point in the projected disk
      // of the neighbor sphere.
      for (int k = next[4]; k <= next[5]; ++k)
      for (int j = next[2]; j <= next[3]; ++j)
        {
        int i = r2 - (y-j)*(y-j) - (z-k)*(z-k);
        if (i < 0) // In box neighbor, but not in spherical neighbor.
          {
          continue;
          }

        int const t = k * sinc[2] + j * sinc[1];
        i = sqrt(i);

        // Make sure we do not fall off the prefix sum volume whole extent.
        int const i0 = vtkstd::max(x-i, sext[0]); // lower bound
        int const i1 = vtkstd::min(x+i, sext[1]); // upper bound

        // sum of every thing in [i0,i1]
        sum += sdat[t+i1] - (0==i0 ? 0:sdat[t+i0-1]);
        num += i1-i0+1;
        }

      // Guard against divide by zero and cast only once here.
      *odat = num ? (double)sum / num : 0;
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
