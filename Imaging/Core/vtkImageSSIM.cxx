// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageSSIM.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkImageSSIM);

namespace
{
//==============================================================================
struct SSIMWorker
{
  template <class Array1T, class Array2T>
  void operator()(Array1T* array1, Array2T* array2, vtkImageData* im1, vtkImageData* im2,
    vtkImageData* imOut, vtkDoubleArray* out, const int* outExt, double radius,
    std::vector<std::array<double, 2>>& C, bool clamp) const
  {
    auto data1 = vtk::DataArrayTupleRange(array1);
    auto data2 = vtk::DataArrayTupleRange(array2);

    using Ref1Type = typename decltype(data1)::ComponentReferenceType;
    using Ref2Type = typename decltype(data2)::ComponentReferenceType;

    int ijk[3] = {};
    int nComp = array1->GetNumberOfComponents();

    const int* e1 = im1->GetExtent();
    const int* e2 = im2->GetExtent();

    auto inBounds = [&e1, &e2](int i, int j, int k) {
      return i >= e1[0] && j >= e1[2] && k >= e1[4] && i >= e2[0] && j >= e2[2] && k >= e2[4] &&
        i <= e1[1] && j <= e1[3] && k <= e1[5] && i <= e2[1] && j <= e2[3] && k <= e2[5];
    };

    double sigma2 = radius / 3.0;
    sigma2 *= sigma2;

    int ithick = outExt[1] - outExt[0];
    int jthick = outExt[3] - outExt[2];
    int kthick = outExt[5] - outExt[4];

    int d[3], dijk[3];
    double squaredRadius = radius * radius;

    auto inPatch = [&ijk, &dijk, &d, &squaredRadius] {
      vtkMath::Subtract(ijk, dijk, d);
      return vtkMath::SquaredNorm(d) <= squaredRadius;
    };

    for (int dim = 0; dim < nComp; ++dim)
    {
      for (ijk[2] = outExt[4]; ijk[2] <= outExt[5]; ++ijk[2])
      {
        for (ijk[1] = outExt[2]; ijk[1] <= outExt[3]; ++ijk[1])
        {
          for (ijk[0] = outExt[0]; ijk[0] <= outExt[1]; ++ijk[0])
          {
            int imin = ithick ? ijk[0] - radius : outExt[0];
            int imax = ithick ? ijk[0] + radius : outExt[0];
            int jmin = jthick ? ijk[1] - radius : outExt[2];
            int jmax = jthick ? ijk[1] + radius : outExt[2];
            int kmin = kthick ? ijk[2] - radius : outExt[4];
            int kmax = kthick ? ijk[2] + radius : outExt[4];

            double mean1 = 0, mean2 = 0, var1 = 0, var2 = 0, covar = 0;

            double totalWeights = 0.0;
            auto smooth = [&sigma2](double x2) { return std::exp(-x2 / (2 * sigma2)); };
            auto coordToNorm2 = [&](const int v[3]) {
              int x = v[0] - (imax + imin) * 0.5;
              int y = v[1] - (jmax + jmin) * 0.5;
              int z = v[2] - (kmax + kmin) * 0.5;
              return x * x + y * y + z * z;
            };
            // Compute the means
            for (dijk[2] = kmin; dijk[2] <= kmax; ++dijk[2])
            {
              for (dijk[1] = jmin; dijk[1] <= jmax; ++dijk[1])
              {
                for (dijk[0] = imin; dijk[0] <= imax; ++dijk[0])
                {
                  if (inBounds(dijk[0], dijk[1], dijk[2]) && inPatch())
                  {
                    double w = smooth(coordToNorm2(dijk));
                    w = 1.0;
                    vtkIdType id1 = vtkStructuredData::ComputePointIdForExtent(e1, dijk);
                    vtkIdType id2 = vtkStructuredData::ComputePointIdForExtent(e2, dijk);
                    mean1 += w * data1[id1][dim];
                    mean2 += w * data2[id2][dim];
                    totalWeights += w;
                  }
                }
              }
            }
            mean1 /= totalWeights;
            mean2 /= totalWeights;

            // Compute the variances and covariance
            for (dijk[2] = kmin; dijk[2] <= kmax; ++dijk[2])
            {
              for (dijk[1] = jmin; dijk[1] <= jmax; ++dijk[1])
              {
                for (dijk[0] = imin; dijk[0] <= imax; ++dijk[0])
                {
                  if (inBounds(dijk[0], dijk[1], dijk[2]) && inPatch())
                  {
                    double w = smooth(coordToNorm2(dijk));
                    w = 1.0;
                    vtkIdType id1 = vtkStructuredData::ComputePointIdForExtent(e1, dijk);
                    vtkIdType id2 = vtkStructuredData::ComputePointIdForExtent(e2, dijk);
                    Ref1Type v1 = data1[id1][dim];
                    Ref2Type v2 = data2[id2][dim];
                    var1 += w * (v1 - mean1) * (v1 - mean1);
                    var2 += w * (v2 - mean2) * (v2 - mean2);
                    covar += w * (v1 - mean1) * (v2 - mean2);
                  }
                }
              }
            }
            var1 /= totalWeights;
            var2 /= totalWeights;
            covar /= totalWeights;

            double c1 = C[dim][0];
            double c2 = C[dim][1];

            // Computing SSIM
            // The order of computation matters for 2 * (mean1 * mean2) in order to reduce rounding
            // error
            double ssim = (2 * (mean1 * mean2) + c1) * (2 * covar + c2) /
              ((mean1 * mean1 + mean2 * mean2 + c1) * (var1 + var2 + c2));

            // Clamping negative values if requested
            ssim = clamp ? std::max(0.0, ssim) : ssim;

            vtkIdType id = vtkStructuredData::ComputePointIdForExtent(imOut->GetExtent(), ijk);
            out->SetTypedComponent(id, dim, ssim);
          }
        }
      }
    }
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
// Construct object to extract all of the input data.
vtkImageSSIM::vtkImageSSIM()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
void vtkImageSSIM::GrowExtent(int* uExt, int* wholeExtent)
{
  // grow input whole extent.
  for (int idx = 0; idx < 2; ++idx)
  {
    uExt[idx * 2] -= 2;
    uExt[idx * 2 + 1] += 2;

    // we must clip extent with whole extent is we handle boundaries.
    if (uExt[idx * 2] < wholeExtent[idx * 2])
    {
      uExt[idx * 2] = wholeExtent[idx * 2];
    }
    if (uExt[idx * 2 + 1] > wholeExtent[idx * 2 + 1])
    {
      uExt[idx * 2 + 1] = wholeExtent[idx * 2 + 1];
    }
  }
}

//------------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
int vtkImageSSIM::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int uExt[6];

  // Recover and grow extent into first input extent
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  int* wholeExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt);
  this->GrowExtent(uExt, wholeExtent);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt, 6);

  // Recover and grow extent into second input extent
  inInfo = inputVector[1]->GetInformationObject(0);
  wholeExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt);
  this->GrowExtent(uExt, wholeExtent);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt, 6);

  return 1;
}

//------------------------------------------------------------------------------
void vtkImageSSIM::SetInputToLab()
{
  if (this->Mode != MODE_LAB)
  {
    this->C.resize(3);
    this->C[0][0] = 100.0;
    this->C[1][0] = 650.25;
    this->C[2][0] = 650.25;
    this->C[0][1] = 900.0;
    this->C[1][1] = 5852.25;
    this->C[2][1] = 5852.25;

    this->Mode = MODE_LAB;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImageSSIM::SetInputToRGB()
{
  if (this->Mode != MODE_RGB)
  {
    this->C.resize(3);
    for (int i = 0; i < 3; ++i)
    {
      this->C[i][0] = 6.5025;
      this->C[i][1] = 58.5225;
    }

    this->Mode = MODE_RGB;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImageSSIM::SetInputToGrayscale()
{
  if (this->Mode != MODE_GRAYSCALE)
  {
    this->C.resize(1);
    this->C[0][0] = 6.5025;
    this->C[0][1] = 58.5225;

    this->Mode = MODE_GRAYSCALE;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImageSSIM::SetInputRange(std::vector<int>& range)
{
  if (this->Mode != MODE_NONE)
  {
    this->C.resize(range.size());
    for (std::size_t i = 0; i < range.size(); ++i)
    {
      this->C[i][0] = 0.0001 * range[i] * range[i];
      this->C[i][1] = 0.0009 * range[i] * range[i];
    }

    this->Modified();
    this->Mode = MODE_NONE;
  }
}

//------------------------------------------------------------------------------
int vtkImageSSIM::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto scalar1 = vtkDataSet::GetData(inputVector[0], 0)->GetPointData()->GetScalars();
  auto scalar2 = vtkDataSet::GetData(inputVector[1], 0)->GetPointData()->GetScalars();

  if (!scalar1 || !scalar2)
  {
    vtkErrorMacro("No input scalars. Aborting.");
    return 0;
  }

  if (scalar1->GetNumberOfComponents() != scalar2->GetNumberOfComponents())
  {
    vtkErrorMacro("Input arrays don't have the same number of components");
    return 0;
  }

  const int nComp = scalar1->GetNumberOfComponents();

  if (nComp != scalar2->GetNumberOfComponents())
  {
    vtkLog(ERROR, "Inputs do not have matching number of components, aborting.");
    return 0;
  }

  // The user hasn't put the right input range
  if (C.size() != static_cast<std::size_t>(nComp))
  {
    C.resize(nComp);
    double r[2];
    for (int i = 0; i < nComp; ++i)
    {
      scalar1->GetRange(C[i].data(), i);
      scalar2->GetRange(r, i);
      C[i][0] = std::max(C[i][1] - C[i][0], r[1] - r[0]);
      C[i][0] *= 0.0001 * C[i][0];
      C[i][1] = 9 * C[i][1];
    }
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
void vtkImageSSIM::AllocateOutputData(vtkImageData* output, vtkInformation* outInfo, int* uExtent)
{
  // set the extent to be the update extent
  output->SetExtent(uExtent);
  int numComponents = vtkImageData::GetNumberOfScalarComponents(outInfo);
  output->AllocateScalars(VTK_DOUBLE, numComponents);
}

//------------------------------------------------------------------------------
void vtkImageSSIM::ThreadedRequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector**,
  vtkInformationVector* vtkNotUsed(outputVector), vtkImageData*** inData, vtkImageData** outData,
  int outExt[6], int)
{
  vtkImageData* im1 = inData[0][0];
  vtkImageData* im2 = inData[1][0];

  vtkDataArray* scalar1 = im1->GetPointData()->GetScalars();
  vtkDataArray* scalar2 = im2->GetPointData()->GetScalars();

  using Dispatcher = vtkArrayDispatch::Dispatch2SameValueType;
  SSIMWorker worker;

  vtkImageData* outIm = outData[0];
  vtkDoubleArray* out = vtkArrayDownCast<vtkDoubleArray>(outIm->GetPointData()->GetScalars());

  if (!Dispatcher::Execute(scalar1, scalar2, worker, im1, im2, outIm, out, outExt,
        this->PatchRadius, this->C, this->ClampNegativeValues))
  {
    worker(scalar1, scalar2, im1, im2, outIm, out, outExt, this->PatchRadius, this->C,
      this->ClampNegativeValues);
  }
}

//------------------------------------------------------------------------------
// Make the output the intersection of the inputs, of course the inputs better
// be the same size
int vtkImageSSIM::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo1 = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  int* in1Ext = inInfo1->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int* in2Ext = inInfo2->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  if (in1Ext[0] != in2Ext[0] || in1Ext[1] != in2Ext[1] || in1Ext[2] != in2Ext[2] ||
    in1Ext[3] != in2Ext[3] || in1Ext[4] != in2Ext[4] || in1Ext[5] != in2Ext[5])
  {
    vtkErrorMacro("ExecuteInformation: Input are not the same size.\n"
      << " Input1 is: " << in1Ext[0] << "," << in1Ext[1] << "," << in1Ext[2] << "," << in1Ext[3]
      << "," << in1Ext[4] << "," << in1Ext[5] << "\n"
      << " Input2 is: " << in2Ext[0] << "," << in2Ext[1] << "," << in2Ext[2] << "," << in2Ext[3]
      << "," << in2Ext[4] << "," << in2Ext[5]);
  }

  // We still need to set the whole extent to be the intersection.
  // Otherwise the execute may crash.
  int ext[6];
  for (int i = 0; i < 3; ++i)
  {
    ext[i * 2] = in1Ext[i * 2];
    if (ext[i * 2] < in2Ext[i * 2])
    {
      ext[i * 2] = in2Ext[i * 2];
    }
    ext[i * 2 + 1] = in1Ext[i * 2 + 1];
    if (ext[i * 2 + 1] > in2Ext[i * 2 + 1])
    {
      ext[i * 2 + 1] = in2Ext[i * 2 + 1];
    }
  }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);

  return 1;
}

//------------------------------------------------------------------------------
void vtkImageSSIM::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
