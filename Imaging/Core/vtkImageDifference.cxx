// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageDifference.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <iostream>
#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
constexpr int MAX_NCOMPS = 4;
constexpr double DEFAULT_ERROR = 1000.;
using max_ncomps_array_t = std::array<int, MAX_NCOMPS>;

vtkStandardNewMacro(vtkImageDifference);

// Thread-local data needed for each thread.
class vtkImageDifferenceThreadData
{
public:
  vtkImageDifferenceThreadData() = default;

  const char* ErrorMessage = nullptr;
  double Error = 0.0;
  double ThresholdedError = 0.0;
};

// Holds thread-local data for all threads.
class vtkImageDifferenceSMPThreadLocal : public vtkSMPThreadLocal<vtkImageDifferenceThreadData>
{
public:
  typedef vtkSMPThreadLocal<vtkImageDifferenceThreadData>::iterator iterator;
};

//------------------------------------------------------------------------------
// This functor is used with vtkSMPTools to execute the algorithm in pieces
// split over the extent of the data.
class vtkImageDifferenceSMPFunctor
{
public:
  // Create the functor by providing all of the information that will be
  // needed by the ThreadedRequestData method that the functor will call.
  vtkImageDifferenceSMPFunctor(vtkImageDifference* algo, vtkImageData*** inputs,
    vtkImageData** outputs, int* extent, vtkIdType pieces)
    : Algorithm(algo)
    , Inputs(inputs)
    , Outputs(outputs)
    , Extent(extent)
    , NumberOfPieces(pieces)
  {
  }

  void Initialize() {}
  void operator()(vtkIdType begin, vtkIdType end);
  void Reduce();

private:
  vtkImageDifferenceSMPFunctor() = delete;

  vtkImageDifference* Algorithm;
  vtkImageData*** Inputs;
  vtkImageData** Outputs;
  int* Extent;
  vtkIdType NumberOfPieces;
};

//------------------------------------------------------------------------------
void vtkImageDifferenceSMPFunctor::operator()(vtkIdType begin, vtkIdType end)
{
  this->Algorithm->SMPRequestData(nullptr, nullptr, nullptr, this->Inputs, this->Outputs, begin,
    end, this->NumberOfPieces, this->Extent);
}

//------------------------------------------------------------------------------
// Used with vtkSMPTools to compute the error
void vtkImageDifferenceSMPFunctor::Reduce()
{
  const char* errorMessage = nullptr;
  double error = 0.0;
  double thresholdedError = 0.0;

  for (vtkImageDifferenceSMPThreadLocal::iterator iter = this->Algorithm->SMPThreadData->begin();
       iter != this->Algorithm->SMPThreadData->end(); ++iter)
  {
    errorMessage = iter->ErrorMessage;
    if (errorMessage)
    {
      break;
    }
    error += iter->Error;
    thresholdedError += iter->ThresholdedError;
  }

  this->Algorithm->ErrorMessage = errorMessage;
  this->Algorithm->Error = error;
  this->Algorithm->ThresholdedError = thresholdedError;
}

//------------------------------------------------------------------------------
// Construct object to extract all of the input data.
vtkImageDifference::vtkImageDifference()
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
void vtkImageDifference::GrowExtent(int* uExt, int* wholeExtent)
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
int vtkImageDifference::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
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
int vtkImageDifference::ComputeSumedValue(unsigned char* values, vtkIdType* indices, int comp)
{
  return static_cast<int>((values)[comp]) + static_cast<int>((values - indices[0])[comp]) +
    static_cast<int>((values + indices[0])[comp]) + static_cast<int>((values - indices[1])[comp]) +
    static_cast<int>((values - indices[1] - indices[0])[comp]) +
    static_cast<int>((values - indices[1] + indices[0])[comp]) +
    static_cast<int>((values + indices[1])[comp]) +
    static_cast<int>((values + indices[1] - indices[0])[comp]) +
    static_cast<int>((values + indices[1] + indices[0])[comp]);
}

//------------------------------------------------------------------------------
void vtkImageDifference::ThreadedRequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector),
  vtkImageData*** inData, vtkImageData** outData, int outExt[6], int id)
{
  vtkImageDifferenceThreadData* threadData = nullptr;

  if (this->EnableSMP)
  {
    threadData = &this->SMPThreadData->Local();
  }
  else
  {
    threadData = &this->ThreadData[id];
  }

  // If an error has occurred, then do not continue.
  if (threadData->ErrorMessage)
  {
    return;
  }

  if (inData[0] == nullptr || inData[1] == nullptr || outData == nullptr)
  {
    threadData->ErrorMessage = "Missing data";
    return;
  }

  // This code supports up to MAX_NCOMPS components, storing intermediate
  // results in max_ncomps_array_t.
  // nComp is not taken into account with std::copy and std::accumulate
  // as it is simplifying the code and because non-considered-component
  // threshold value is always zero.
  int nComp = inData[0][0]->GetNumberOfScalarComponents();
  int input1NComp = inData[1][0]->GetNumberOfScalarComponents();
  int outputNComp = outData[0]->GetNumberOfScalarComponents();
  if (nComp != input1NComp)
  {
    threadData->ErrorMessage = "Inputs number of components are differents";
    return;
  }
  if (outputNComp != input1NComp)
  {
    threadData->ErrorMessage = "Input and output number of components are differents";
    return;
  }
  if (nComp > MAX_NCOMPS || nComp <= 0)
  {
    threadData->ErrorMessage = "Expecting between 1 and MAX_NCOMPS components";
  }

  // this filter expects that both inputs and output are of the same type.
  if (inData[0][0]->GetScalarType() != VTK_UNSIGNED_CHAR ||
    inData[1][0]->GetScalarType() != VTK_UNSIGNED_CHAR ||
    outData[0]->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    threadData->ErrorMessage = "All ScalarTypes must be unsigned char";
    return;
  }

  unsigned char *outPtr0, *outPtr1, *outPtr2;
  outPtr2 = static_cast<unsigned char*>(outData[0]->GetScalarPointerForExtent(outExt));

  std::array<vtkIdType, 3> outInc;
  outData[0]->GetIncrements(outInc.data());

  int min0, max0, min1, max1, min2, max2;
  min0 = outExt[0];
  max0 = outExt[1];
  min1 = outExt[2];
  max1 = outExt[3];
  min2 = outExt[4];
  max2 = outExt[5];

  // copy both input data arrays into new arrays for gamma correction
  std::array<vtkIdType, 3> in1Inc;
  inData[0][0]->GetIncrements(in1Inc.data());
  std::array<vtkIdType, 3> in2Inc;
  inData[1][0]->GetIncrements(in2Inc.data());

  int* inExt;
  inExt = inData[0][0]->GetExtent();
  int cmax0 = inExt[1] > max0 + 3 ? max0 + 3 : inExt[1];
  int cmin0 = inExt[0] < min0 - 3 ? min0 - 3 : inExt[0];
  int cmax1 = inExt[3] > max1 + 3 ? max1 + 3 : inExt[3];
  int cmin1 = inExt[2] < min1 - 3 ? min1 - 3 : inExt[2];

  unsigned char *in1Ptr2, *in2Ptr2;
  in1Ptr2 = static_cast<unsigned char*>(inData[0][0]->GetScalarPointer(cmin0, cmin1, min2));
  in2Ptr2 = static_cast<unsigned char*>(inData[1][0]->GetScalarPointer(cmin0, cmin1, min2));

  // reset increments for the new arrays
  in1Inc[1] = in1Inc[0] * (cmax0 - cmin0 + 1);
  in1Inc[2] = in1Inc[1] * (cmax1 - cmin1 + 1);
  in2Inc[1] = in2Inc[0] * (cmax0 - cmin0 + 1);
  in2Inc[2] = in2Inc[1] * (cmax1 - cmin1 + 1);

  // we set min and max to be one pixel in from actual values to support
  // the 3x3 averaging we do
  int inMinX, inMaxX, inMinY, inMaxY;
  inMinX = cmin0;
  inMaxX = cmax0;
  inMinY = cmin1;
  inMaxY = cmax1;

  unsigned long target;
  target = static_cast<unsigned long>((max2 - min2 + 1) * (max1 - min1 + 1) / 50.0);
  target++;

  int contInIncr1 = (cmax0 - cmin0 - max0 + min0) * in1Inc[0];
  int contInIncr2 = (cmax1 - cmin1 - max1 + min1) * in1Inc[1];

  unsigned char *in1Ptr0, *in2Ptr0;
  in1Ptr0 = in1Ptr2 + (min1 - cmin1) * in1Inc[1] + (min0 - cmin0) * in1Inc[0];
  in2Ptr0 = in2Ptr2 + (min1 - cmin1) * in1Inc[1] + (min0 - cmin0) * in1Inc[0];

  double error = 0.0;
  double thresholdedError = 0.0;
  unsigned long count = 0;
  int idx0, idx1, idx2;
  for (idx2 = min2; idx2 <= max2; ++idx2)
  {
    outPtr1 = outPtr2;
    for (idx1 = min1; !this->AbortExecute && idx1 <= max1; ++idx1)
    {
      if (!id)
      {
        if (!(count % target))
        {
          this->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
      {
        max_ncomps_array_t rgbaMax;
        rgbaMax.fill(0);
        max_ncomps_array_t rgbaTresh;

        // ignore the boundary within two pixels as we cannot
        // do a good average calc on the boundary
        if (idx0 >= inExt[0] + 2 && idx0 <= inExt[1] - 2 && idx1 >= inExt[2] + 2 &&
          idx1 <= inExt[3] - 2)
        {
          for (int direction = 0; direction <= 1; ++direction)
          {
            rgbaTresh.fill(DEFAULT_ERROR);
            unsigned char* dir1Ptr0 = direction == 0 ? in1Ptr0 : in2Ptr0;
            unsigned char* dir2Ptr0 = direction == 0 ? in2Ptr0 : in1Ptr0;
            bool haveValues = false;
            bool done = false;

            for (int averaging = 0; averaging <= (this->Averaging ? 1 : 0); ++averaging)
            {
              for (int yneigh = this->AllowShift ? -2 : 0;
                   yneigh <= (this->AllowShift ? 2 : 0) && !done; ++yneigh)
              {
                for (int xneigh = this->AllowShift ? -2 : 0;
                     xneigh <= (this->AllowShift ? 2 : 0) && !done; ++xneigh)
                {
                  unsigned char* c1 = dir1Ptr0 + yneigh * in1Inc[1] + xneigh * in1Inc[0];
                  unsigned char* c2 = dir2Ptr0;
                  if ((idx0 + xneigh - averaging >= inMinX) &&
                    (idx0 + xneigh + averaging <= inMaxX) &&
                    (idx1 + yneigh - averaging >= inMinY) && (idx1 + yneigh + averaging <= inMaxY))
                  {
                    max_ncomps_array_t rgba1;
                    rgba1.fill(0);
                    if (averaging == 1)
                    {
                      max_ncomps_array_t rgbaA1;
                      rgbaA1.fill(0);
                      max_ncomps_array_t rgbaA2;
                      rgbaA2.fill(0);

                      for (int i = 0; i < nComp; i++)
                      {
                        rgbaA1[i] = ComputeSumedValue(c1, in1Inc.data(), i);
                        rgbaA2[i] = ComputeSumedValue(c2, in2Inc.data(), i);
                        rgba1[i] = abs(rgbaA1[i] - rgbaA2[i]) / (9 * this->AverageThresholdFactor);
                      }
                    }
                    else
                    {
                      for (int i = 0; i < nComp; i++)
                      {
                        rgba1[i] = abs((static_cast<int>((c1)[i]) - static_cast<int>((c2)[i])));
                      }
                    }

                    if (std::accumulate(rgba1.begin(), rgba1.end(), 0) <
                      std::accumulate(rgbaTresh.begin(), rgbaTresh.end(), 0))
                    {
                      std::copy(rgba1.begin(), rgba1.end(), rgbaTresh.begin());
                    }
                    haveValues = true;
                  }

                  done = true;
                  for (int i = 0; i < nComp; i++)
                  {
                    // once we have a good enough match stop to save time
                    if (rgbaTresh[i] >= this->Threshold)
                    {
                      done = false;
                    }
                  }
                }
              }
            }
            if (haveValues)
            {
              for (int i = 0; i < nComp; i++)
              {
                rgbaMax[i] = std::max(rgbaTresh[i], rgbaMax[i]);
              }
            }
          }
        }

        std::copy(rgbaMax.begin(), rgbaMax.end(), rgbaTresh.begin());

        error += std::accumulate(rgbaTresh.begin(), rgbaTresh.end(), 0.) / (nComp * 255.);

        for (int i = 0; i < nComp; i++)
        {
          rgbaTresh[i] -= this->Threshold;
          rgbaTresh[i] = std::max(0, rgbaTresh[i]);
          *outPtr0++ = static_cast<unsigned char>(rgbaTresh[i]);
        }
        thresholdedError +=
          std::accumulate(rgbaTresh.begin(), rgbaTresh.end(), 0.) / (nComp * 255.);

        in1Ptr0 += nComp;
        in2Ptr0 += nComp;
      }
      in1Ptr0 += contInIncr1;
      in2Ptr0 += contInIncr1;
      outPtr1 += outInc[1];
    }
    in1Ptr0 += contInIncr2;
    in2Ptr0 += contInIncr2;
    outPtr2 += outInc[2];
  }

  // Add the results to the thread-local total.
  threadData->Error += error;
  threadData->ThresholdedError += thresholdedError;
}

//------------------------------------------------------------------------------
// Create thread-local objects before initiating the multithreading
int vtkImageDifference::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int ret = 1;

  if (this->EnableSMP) // For vtkSMPTools implementation.
  {
    // Get the input and output data objects
    vtkImageData* inDataPointer[2];
    vtkImageData** inData[2] = { &inDataPointer[0], &inDataPointer[1] };
    vtkImageData* outData[1];
    this->PrepareImageData(inputVector, outputVector, inData, outData);

    // Get the extent
    int extent[6];
    outData[0]->GetExtent(extent);

    // Do a dummy execution of SplitExtent to compute the number of pieces
    vtkIdType pieces = this->SplitExtent(nullptr, extent, 0, this->NumberOfThreads);

    // Use vtkSMPTools to multithread the functor
    vtkImageDifferenceSMPThreadLocal threadData;
    vtkImageDifferenceSMPFunctor functor(this, inData, outData, extent, pieces);
    this->SMPThreadData = &threadData;
    vtkSMPTools::For(0, pieces, functor);
    this->SMPThreadData = nullptr;
  }
  else
  {
    // For vtkMultiThreader implementation.
    this->ThreadData = new vtkImageDifferenceThreadData[this->NumberOfThreads];

    // The superclass will call ThreadedRequestData
    ret = this->Superclass::RequestData(request, inputVector, outputVector);

    // Compute error sums here.
    this->Error = 0.0;
    this->ThresholdedError = 0.0;
    for (int i = 0; i < this->NumberOfThreads; i++)
    {
      this->Error += this->ThreadData[i].Error;
      this->ThresholdedError += this->ThreadData[i].ThresholdedError;
      this->ErrorMessage = this->ThreadData[i].ErrorMessage;
      if (this->ErrorMessage)
      {
        break;
      }
    }

    delete[] this->ThreadData;
    this->ThreadData = nullptr;
  }

  if (this->ErrorMessage)
  {
    // Report errors here, do not report errors while multithreading!
    vtkErrorMacro("RequestData: " << this->ErrorMessage);
    this->ErrorMessage = nullptr;
    this->Error = DEFAULT_ERROR;
    this->ThresholdedError = DEFAULT_ERROR;
    ret = 0;
  }

  return ret;
}

//------------------------------------------------------------------------------
// Make the output the intersection of the inputs, of course the inputs better
// be the same size
int vtkImageDifference::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo1 = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  int* in1Ext = inInfo1->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int* in2Ext = inInfo2->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  int i;
  if (in1Ext[0] != in2Ext[0] || in1Ext[1] != in2Ext[1] || in1Ext[2] != in2Ext[2] ||
    in1Ext[3] != in2Ext[3] || in1Ext[4] != in2Ext[4] || in1Ext[5] != in2Ext[5])
  {
    this->Error = DEFAULT_ERROR;
    this->ThresholdedError = DEFAULT_ERROR;

    vtkErrorMacro("ExecuteInformation: Input are not the same size.\n"
      << " Input1 is: " << in1Ext[0] << "," << in1Ext[1] << "," << in1Ext[2] << "," << in1Ext[3]
      << "," << in1Ext[4] << "," << in1Ext[5] << "\n"
      << " Input2 is: " << in2Ext[0] << "," << in2Ext[1] << "," << in2Ext[2] << "," << in2Ext[3]
      << "," << in2Ext[4] << "," << in2Ext[5]);
  }

  // We still need to set the whole extent to be the intersection.
  // Otherwise the execute may crash.
  int ext[6];
  for (i = 0; i < 3; ++i)
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
vtkImageData* vtkImageDifference::GetImage()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
void vtkImageDifference::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Error: " << this->Error << "\n";
  os << indent << "ThresholdedError: " << this->ThresholdedError << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "AllowShift: " << this->AllowShift << "\n";
  os << indent << "Averaging: " << this->Averaging << "\n";
}
VTK_ABI_NAMESPACE_END
