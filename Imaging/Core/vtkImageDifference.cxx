/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDifference.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageDifference);

// Thread-local data needed for each thread.
class vtkImageDifferenceThreadData
{
public:
  vtkImageDifferenceThreadData()
    : ErrorMessage(nullptr)
    , Error(0.0)
    , ThresholdedError(0.0)
  {
  }

  const char* ErrorMessage;
  double Error;
  double ThresholdedError;
};

// Holds thread-local data for all threads.
class vtkImageDifferenceSMPThreadLocal : public vtkSMPThreadLocal<vtkImageDifferenceThreadData>
{
public:
  typedef vtkSMPThreadLocal<vtkImageDifferenceThreadData>::iterator iterator;
};

// Construct object to extract all of the input data.
vtkImageDifference::vtkImageDifference()
{
  this->Threshold = 105;
  this->AllowShift = 1;
  this->Averaging = 1;
  this->AverageThresholdFactor = 0.65;
  // ideally threshold*averageThresholdFactor should be < 255/9
  // to capture one pixel errors or 510/9 to capture 2 pixel errors

  this->ErrorMessage = nullptr;
  this->Error = 0.0;
  this->ThresholdedError = 0.0;

  this->ThreadData = nullptr;
  this->SMPThreadData = nullptr;

  this->SetNumberOfInputPorts(2);
}

// not so simple macro for calculating error
#define vtkImageDifferenceComputeError(c1, c2)                                                     \
  /* if averaging is on and we have neighbor info then compute */                                  \
  /* avg(input1) to avg(input2) */                                                                 \
  /* compute the pixel to pixel difference first */                                                \
  if (averaging == 0 && (idx0 + xneigh >= inMinX) && (idx0 + xneigh <= inMaxX) &&                  \
    (idx1 + yneigh >= inMinY) && (idx1 + yneigh <= inMaxY))                                        \
  {                                                                                                \
    r1 = abs((static_cast<int>((c1)[0]) - static_cast<int>((c2)[0])));                             \
    g1 = abs((static_cast<int>((c1)[1]) - static_cast<int>((c2)[1])));                             \
    b1 = abs((static_cast<int>((c1)[2]) - static_cast<int>((c2)[2])));                             \
    if ((r1 + g1 + b1) < (tr + tg + tb))                                                           \
    {                                                                                              \
      tr = r1;                                                                                     \
      tg = g1;                                                                                     \
      tb = b1;                                                                                     \
    }                                                                                              \
    haveValues = true;                                                                             \
  }                                                                                                \
  if (averaging == 1 && (idx0 + xneigh > inMinX) && (idx0 + xneigh < inMaxX) &&                    \
    (idx1 + yneigh > inMinY) && (idx1 + yneigh < inMaxY))                                          \
  {                                                                                                \
    ar1 = static_cast<int>((c1)[0]) + static_cast<int>((c1 - in1Inc0)[0]) +                        \
      static_cast<int>((c1 + in1Inc0)[0]) + static_cast<int>((c1 - in1Inc1)[0]) +                  \
      static_cast<int>((c1 - in1Inc1 - in1Inc0)[0]) +                                              \
      static_cast<int>((c1 - in1Inc1 + in1Inc0)[0]) + static_cast<int>((c1 + in1Inc1)[0]) +        \
      static_cast<int>((c1 + in1Inc1 - in1Inc0)[0]) +                                              \
      static_cast<int>((c1 + in1Inc1 + in1Inc0)[0]);                                               \
    ag1 = static_cast<int>((c1)[1]) + static_cast<int>((c1 - in1Inc0)[1]) +                        \
      static_cast<int>((c1 + in1Inc0)[1]) + static_cast<int>((c1 - in1Inc1)[1]) +                  \
      static_cast<int>((c1 - in1Inc1 - in1Inc0)[1]) +                                              \
      static_cast<int>((c1 - in1Inc1 + in1Inc0)[1]) + static_cast<int>((c1 + in1Inc1)[1]) +        \
      static_cast<int>((c1 + in1Inc1 - in1Inc0)[1]) +                                              \
      static_cast<int>((c1 + in1Inc1 + in1Inc0)[1]);                                               \
    ab1 = static_cast<int>((c1)[2]) + static_cast<int>((c1 - in1Inc0)[2]) +                        \
      static_cast<int>((c1 + in1Inc0)[2]) + static_cast<int>((c1 - in1Inc1)[2]) +                  \
      static_cast<int>((c1 - in1Inc1 - in1Inc0)[2]) +                                              \
      static_cast<int>((c1 - in1Inc1 + in1Inc0)[2]) + static_cast<int>((c1 + in1Inc1)[2]) +        \
      static_cast<int>((c1 + in1Inc1 - in1Inc0)[2]) +                                              \
      static_cast<int>((c1 + in1Inc1 + in1Inc0)[2]);                                               \
    ar2 = static_cast<int>((c2)[0]) + static_cast<int>((c2 - in2Inc0)[0]) +                        \
      static_cast<int>((c2 + in2Inc0)[0]) + static_cast<int>((c2 - in2Inc1)[0]) +                  \
      static_cast<int>((c2 - in2Inc1 - in2Inc0)[0]) +                                              \
      static_cast<int>((c2 - in2Inc1 + in2Inc0)[0]) + static_cast<int>((c2 + in2Inc1)[0]) +        \
      static_cast<int>((c2 + in2Inc1 - in2Inc0)[0]) +                                              \
      static_cast<int>((c2 + in2Inc1 + in2Inc0)[0]);                                               \
    ag2 = static_cast<int>((c2)[1]) + static_cast<int>((c2 - in2Inc0)[1]) +                        \
      static_cast<int>((c2 + in2Inc0)[1]) + static_cast<int>((c2 - in2Inc1)[1]) +                  \
      static_cast<int>((c2 - in2Inc1 - in2Inc0)[1]) +                                              \
      static_cast<int>((c2 - in2Inc1 + in2Inc0)[1]) + static_cast<int>((c2 + in2Inc1)[1]) +        \
      static_cast<int>((c2 + in2Inc1 - in2Inc0)[1]) +                                              \
      static_cast<int>((c2 + in2Inc1 + in2Inc0)[1]);                                               \
    ab2 = static_cast<int>((c2)[2]) + static_cast<int>((c2 - in2Inc0)[2]) +                        \
      static_cast<int>((c2 + in2Inc0)[2]) + static_cast<int>((c2 - in2Inc1)[2]) +                  \
      static_cast<int>((c2 - in2Inc1 - in2Inc0)[2]) +                                              \
      static_cast<int>((c2 - in2Inc1 + in2Inc0)[2]) + static_cast<int>((c2 + in2Inc1)[2]) +        \
      static_cast<int>((c2 + in2Inc1 - in2Inc0)[2]) +                                              \
      static_cast<int>((c2 + in2Inc1 + in2Inc0)[2]);                                               \
    r1 = abs(ar1 - ar2) / (9 * this->AverageThresholdFactor);                                      \
    g1 = abs(ag1 - ag2) / (9 * this->AverageThresholdFactor);                                      \
    b1 = abs(ab1 - ab2) / (9 * this->AverageThresholdFactor);                                      \
    haveValues = true;                                                                             \
    if ((r1 + g1 + b1) < (tr + tg + tb))                                                           \
    {                                                                                              \
      tr = r1;                                                                                     \
      tg = g1;                                                                                     \
      tb = b1;                                                                                     \
    }                                                                                              \
  }

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkImageDifferenceSMPFunctor::operator()(vtkIdType begin, vtkIdType end)
{
  this->Algorithm->SMPRequestData(nullptr, nullptr, nullptr, this->Inputs, this->Outputs, begin,
    end, this->NumberOfPieces, this->Extent);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
int vtkImageDifference::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int idx;

  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int* wholeExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  int uExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt);

  // grow input whole extent.
  for (idx = 0; idx < 2; ++idx)
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
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt, 6);

  // now do the second input
  inInfo = inputVector[1]->GetInformationObject(0);
  wholeExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt);

  // grow input whole extent.
  for (idx = 0; idx < 2; ++idx)
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
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageDifference::ThreadedRequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector),
  vtkImageData*** inData, vtkImageData** outData, int outExt[6], int id)
{
  unsigned char *in1Ptr0, *in1Ptr2;
  unsigned char *in2Ptr0, *in2Ptr2;
  unsigned char *outPtr0, *outPtr1, *outPtr2;
  int min0, max0, min1, max1, min2, max2;
  int idx0, idx1, idx2;
  vtkIdType in1Inc0, in1Inc1, in1Inc2;
  vtkIdType in2Inc0, in2Inc1, in2Inc2;
  vtkIdType outInc0, outInc1, outInc2;
  int tr, tg, tb, r1, g1, b1;
  int ar1, ag1, ab1, ar2, ag2, ab2;
  int inMinX, inMaxX, inMinY, inMaxY;
  int* inExt;
  unsigned long count = 0;
  unsigned long target;
  double error = 0.0;
  double thresholdedError = 0.0;
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

  if (inData[0][0]->GetNumberOfScalarComponents() != 3 ||
    inData[1][0]->GetNumberOfScalarComponents() != 3 ||
    outData[0]->GetNumberOfScalarComponents() != 3)
  {
    threadData->ErrorMessage = "Expecting 3 components (RGB)";
    return;
  }

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != VTK_UNSIGNED_CHAR ||
    inData[1][0]->GetScalarType() != VTK_UNSIGNED_CHAR ||
    outData[0]->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    threadData->ErrorMessage = "All ScalarTypes must be unsigned char";
    return;
  }

  outPtr2 = static_cast<unsigned char*>(outData[0]->GetScalarPointerForExtent(outExt));
  outData[0]->GetIncrements(outInc0, outInc1, outInc2);

  min0 = outExt[0];
  max0 = outExt[1];
  min1 = outExt[2];
  max1 = outExt[3];
  min2 = outExt[4];
  max2 = outExt[5];

  // copy both input data arrays into new arrays for gamma correction
  inData[0][0]->GetIncrements(in1Inc0, in1Inc1, in1Inc2);
  inData[1][0]->GetIncrements(in2Inc0, in2Inc1, in2Inc2);

  inExt = inData[0][0]->GetExtent();
  int cmax0 = inExt[1] > max0 + 3 ? max0 + 3 : inExt[1];
  int cmin0 = inExt[0] < min0 - 3 ? min0 - 3 : inExt[0];
  int cmax1 = inExt[3] > max1 + 3 ? max1 + 3 : inExt[3];
  int cmin1 = inExt[2] < min1 - 3 ? min1 - 3 : inExt[2];

  in1Ptr2 = static_cast<unsigned char*>(inData[0][0]->GetScalarPointer(cmin0, cmin1, min2));
  in2Ptr2 = static_cast<unsigned char*>(inData[1][0]->GetScalarPointer(cmin0, cmin1, min2));

  // reset increments for the new arrays
  in1Inc1 = in1Inc0 * (cmax0 - cmin0 + 1);
  in1Inc2 = in1Inc1 * (cmax1 - cmin1 + 1);
  in2Inc1 = in2Inc0 * (cmax0 - cmin0 + 1);
  in2Inc2 = in2Inc1 * (cmax1 - cmin1 + 1);

  // we set min and Max to be one pixel in from actual values to support
  // the 3x3 averaging we do
  inMinX = cmin0;
  inMaxX = cmax0;
  inMinY = cmin1;
  inMaxY = cmax1;

  target = static_cast<unsigned long>((max2 - min2 + 1) * (max1 - min1 + 1) / 50.0);
  target++;

  int contInIncr1 = (cmax0 - cmin0 - max0 + min0) * in1Inc0;
  int contInIncr2 = (cmax1 - cmin1 - max1 + min1) * in1Inc1;
  in1Ptr0 = in1Ptr2 + (min1 - cmin1) * in1Inc1 + (min0 - cmin0) * in1Inc0;
  in2Ptr0 = in2Ptr2 + (min1 - cmin1) * in1Inc1 + (min0 - cmin0) * in1Inc0;

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
        int rmax = 0;
        int gmax = 0;
        int bmax = 0;

        // ignore the boundary within two pixels as we cannot
        // do a good average calc on the boundary
        if (idx0 >= inExt[0] + 2 && idx0 <= inExt[1] - 2 && idx1 >= inExt[2] + 2 &&
          idx1 <= inExt[3] - 2)
        {
          for (int direction = 0; direction <= 1; ++direction)
          {
            unsigned char* dir1Ptr0 = direction == 0 ? in1Ptr0 : in2Ptr0;
            unsigned char* dir2Ptr0 = direction == 0 ? in2Ptr0 : in1Ptr0;
            tr = 1000;
            tg = 1000;
            tb = 1000;
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
                  vtkImageDifferenceComputeError(
                    dir1Ptr0 + yneigh * in1Inc1 + xneigh * in1Inc0, dir2Ptr0);
                  // once we have a good enough match stop to save time
                  if (tr < this->Threshold && tg < this->Threshold && tb < this->Threshold)
                  {
                    done = true;
                  }
                }
              }
            }
            if (haveValues)
            {
              rmax = tr > rmax ? tr : rmax;
              gmax = tg > gmax ? tg : gmax;
              bmax = tb > bmax ? tb : bmax;
            }
          }
        }

        tr = rmax;
        tg = gmax;
        tb = bmax;

        error += (tr + tg + tb) / (3.0 * 255);
        tr -= this->Threshold;
        if (tr < 0)
        {
          tr = 0;
        }
        tg -= this->Threshold;
        if (tg < 0)
        {
          tg = 0;
        }
        tb -= this->Threshold;
        if (tb < 0)
        {
          tb = 0;
        }
        *outPtr0++ = static_cast<unsigned char>(tr);
        *outPtr0++ = static_cast<unsigned char>(tg);
        *outPtr0++ = static_cast<unsigned char>(tb);
        thresholdedError += (tr + tg + tb) / (3.0 * 255.0);

        in1Ptr0 += 3;
        in2Ptr0 += 3;
      }
      in1Ptr0 += contInIncr1;
      in2Ptr0 += contInIncr1;
      outPtr1 += outInc1;
    }
    in1Ptr0 += contInIncr2;
    in2Ptr0 += contInIncr2;
    outPtr2 += outInc2;
  }

  // Add the results to the thread-local total.
  threadData->Error += error;
  threadData->ThresholdedError += thresholdedError;
}

//----------------------------------------------------------------------------
// Create thread-local objects before initiating the multithreading
int vtkImageDifference::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int r = 1;

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
    bool debug = this->Debug;
    this->Debug = false;
    vtkSMPTools::For(0, pieces, functor);
    this->Debug = debug;
    this->SMPThreadData = nullptr;
  }
  else
  {
    // For vtkMultiThreader implementation.
    int n = this->NumberOfThreads;
    this->ThreadData = new vtkImageDifferenceThreadData[n];

    // The superclass will call ThreadedRequestData
    r = this->Superclass::RequestData(request, inputVector, outputVector);

    // Compute error sums here.
    this->Error = 0.0;
    this->ThresholdedError = 0.0;
    for (int i = 0; i < n; i++)
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
    this->Error = 1000.0;
    this->ThresholdedError = 1000.0;
    r = 0;
  }

  return r;
}

//----------------------------------------------------------------------------
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
    this->Error = 1000.0;
    this->ThresholdedError = 1000.0;

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

//----------------------------------------------------------------------------
vtkImageData* vtkImageDifference::GetImage()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
void vtkImageDifference::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Error: " << this->Error << "\n";
  os << indent << "ThresholdedError: " << this->ThresholdedError << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "AllowShift: " << this->AllowShift << "\n";
  os << indent << "Averaging: " << this->Averaging << "\n";
}
