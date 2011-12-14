/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageHistogram.h"

#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiThreader.h"
#include "vtkTemplateAliasMacro.h"

#include <math.h>

// turn off 64-bit ints when templating over all types
# undef VTK_USE_INT64
# define VTK_USE_INT64 0
# undef VTK_USE_UINT64
# define VTK_USE_UINT64 0

vtkStandardNewMacro(vtkImageHistogram);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageHistogram::vtkImageHistogram()
{
  this->ActiveComponent = -1;
  this->AutomaticBinning = false;
  this->MaximumNumberOfBins = 65536;
  this->NumberOfBins = 256;
  this->BinOrigin = 0.0;
  this->BinSpacing = 1.0;

  this->GenerateHistogramImage = true;
  this->HistogramImageSize[0] = 256;
  this->HistogramImageSize[1] = 256;
  this->HistogramImageScale = vtkImageHistogram::Linear;

  this->Histogram = vtkIdTypeArray::New();
  this->Total = 0;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkImageHistogram::~vtkImageHistogram()
{
  if (this->Histogram)
    {
    this->Histogram->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Stencil: " << this->GetStencil() << "\n";

  os << indent << "ActiveComponent: " << this->ActiveComponent << "\n";

  os << indent << "AutomaticBinning: "
     << (this->AutomaticBinning ? "On\n" : "Off\n") << "\n";
  os << indent << "MaximumNumberOfBins: " << this->MaximumNumberOfBins << "\n";
  os << indent << "NumberOfBins: " << this->NumberOfBins << "\n";
  os << indent << "BinOrigin: " << this->BinOrigin << "\n";
  os << indent << "BinSpacing: " << this->BinSpacing << "\n";

  os << indent << "GenerateHistogramImage: "
     << (this->GenerateHistogramImage ? "On\n" : "Off\n") << "\n";
  os << indent << "HistogramImageSize: " << this->HistogramImageSize[0] << " "
     << this->HistogramImageSize[1] << "\n";
  os << indent << "HistogramImageScale: "
     << this->GetHistogramImageScaleAsString() << "\n";

  os << indent << "Total: " << this->Total << "\n";
  os << indent << "Histogram: " << this->Histogram << "\n";
}

//----------------------------------------------------------------------------
const char *vtkImageHistogram::GetHistogramImageScaleAsString()
{
  const char *s = "Unknown";

  switch (this->HistogramImageScale)
    {
    case vtkImageHistogram::Log:
      s = "Log";
      break;
    case vtkImageHistogram::Sqrt:
      s = "Sqrt";
      break;
    case vtkImageHistogram::Linear:
      s = "Linear";
      break;
    }

  return s;
}

//----------------------------------------------------------------------------
vtkIdTypeArray *vtkImageHistogram::GetHistogram()
{
  return this->Histogram;
}

//----------------------------------------------------------------------------
void vtkImageHistogram::SetStencil(vtkImageStencilData *stencil)
{
  this->SetInput(1, stencil);
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageHistogram::GetStencil()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  return vtkImageStencilData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int vtkImageHistogram::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageHistogram::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageHistogram::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int outWholeExt[6];
  double outOrigin[3];
  double outSpacing[3];

  outWholeExt[0] = 0;
  outWholeExt[1] = this->HistogramImageSize[0] - 1;
  outWholeExt[2] = 0;
  outWholeExt[3] = this->HistogramImageSize[1] - 1;
  outWholeExt[4] = 0;
  outWholeExt[5] = 0;

  outOrigin[0] = 0.0;
  outOrigin[1] = 0.0;
  outOrigin[2] = 0.0;

  outSpacing[0] = 1.0;
  outSpacing[1] = 1.0;
  outSpacing[2] = 1.0;

  if (!this->GenerateHistogramImage)
    {
    outWholeExt[1] = -1;
    outWholeExt[3] = -1;
    outWholeExt[5] = -1;
    }

  if (this->GetNumberOfOutputPorts() > 0)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 outWholeExt, 6);

    outInfo->Set(vtkDataObject::ORIGIN(), outOrigin, 3);
    outInfo->Set(vtkDataObject::SPACING(), outSpacing, 3);

    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, VTK_UNSIGNED_CHAR, 1);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageHistogram::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  int inExt[6];
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the input extent
  if (this->GetNumberOfInputConnections(1) > 0)
    {
    vtkInformation *stencilInfo = inputVector[1]->GetInformationObject(0);
    stencilInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                     inExt, 6);
    }

  return 1;
}

//----------------------------------------------------------------------------
// anonymous namespace for internal classes and functions
namespace {

struct vtkImageHistogramThreadStruct
{
  vtkImageHistogram *Algorithm;
  vtkInformation *Request;
  vtkInformationVector **InputsInfo;
  vtkInformationVector *OutputsInfo;
};

//----------------------------------------------------------------------------
// override from vtkThreadedImageAlgorithm to split input extent, instead
// of splitting the output extent
VTK_THREAD_RETURN_TYPE vtkImageHistogramThreadedExecute(void *arg)
{
  vtkMultiThreader::ThreadInfo *ti =
    static_cast<vtkMultiThreader::ThreadInfo *>(arg);
  vtkImageHistogramThreadStruct *ts =
    static_cast<vtkImageHistogramThreadStruct *>(ti->UserData);

  int extent[6] = { 0, -1, 0, -1, 0, -1 };

  bool foundConnection = false;
  int numPorts = ts->Algorithm->GetNumberOfInputPorts();
  for (int inPort = 0; inPort < numPorts; ++inPort)
    {
    int numConnections = ts->Algorithm->GetNumberOfInputConnections(inPort);
    if (numConnections)
      {
      vtkInformation *inInfo =
        ts->InputsInfo[inPort]->GetInformationObject(0);
      vtkImageData *inData = vtkImageData::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
      if (inData)
        {
        inData->GetExtent(extent);
        foundConnection = true;
        break;
        }
      }
    }

  if (foundConnection)
    {
    // execute the actual method with appropriate extent
    // first find out how many pieces extent can be split into.
    int splitExt[6];
    int total = ts->Algorithm->SplitExtent(
      splitExt, extent, ti->ThreadID, ti->NumberOfThreads);

    if (ti->ThreadID < total &&
        splitExt[1] >= splitExt[0] &&
        splitExt[3] >= splitExt[2] &&
        splitExt[5] >= splitExt[4])
      {
      ts->Algorithm->ThreadedRequestData(
        ts->Request, ts->InputsInfo, ts->OutputsInfo, NULL, NULL,
        splitExt, ti->ThreadID);
      }
    }

  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
template<class T>
void vtkImageHistogramExecuteRange(
  vtkImageData *inData, vtkImageStencilData *stencil,
  T *inPtr, int extent[6], double range[2], int component)
{
  vtkImageStencilIterator<T> inIter(inData, stencil, extent, NULL);

  T xmin = vtkTypeTraits<T>::Max();
  T xmax = vtkTypeTraits<T>::Min();

  // set up components
  int nc = inData->GetNumberOfScalarComponents();
  int c = component;
  if (c < 0)
    {
    nc = 1;
    c = 0;
    }

  // iterate over all spans in the stencil
  while (!inIter.IsAtEnd())
    {
    if (inIter.IsInStencil())
      {
      inPtr = inIter.BeginSpan();
      T *inPtrEnd = inIter.EndSpan();
      if (inPtr != inPtrEnd)
        {
        int n = static_cast<int>((inPtrEnd - inPtr)/nc);
        inPtr += c;
        do
          {
          T x = *inPtr;

          xmin = (xmin < x ? xmin : x);
          xmax = (xmax > x ? xmax : x);

          inPtr += nc;
          }
        while (--n);
        }
      }
    inIter.NextSpan();
    }

  range[0] = xmin;
  range[1] = xmax;
}

//----------------------------------------------------------------------------
template<class T>
void vtkImageHistogramExecute(
  vtkImageHistogram *self,
  vtkImageData *inData, vtkImageStencilData *stencil,
  T *inPtr, int extent[6], vtkIdType *outPtr, int binRange[2],
  double o, double s, int component, int threadId)
{
  vtkImageStencilIterator<T>
    inIter(inData, stencil, extent, ((threadId == 0) ? self : NULL));

  // set up components
  int nc = inData->GetNumberOfScalarComponents();
  int c = component;
  if (c < 0)
    {
    nc = 1;
    c = 0;
    }

  // compute shift/scale values for fast bin computation
  double xmin = binRange[0];
  double xmax = binRange[1];
  double xshift = -o;
  double xscale = 1.0/s;

  // iterate over all spans in the stencil
  while (!inIter.IsAtEnd())
    {
    if (inIter.IsInStencil())
      {
      inPtr = inIter.BeginSpan();
      T *inPtrEnd = inIter.EndSpan();

      // iterate over all voxels in the span
      if (inPtr != inPtrEnd)
        {
        int n = static_cast<int>((inPtrEnd - inPtr)/nc);
        inPtr += c;
        do
          {
          double x = *inPtr;

          x += xshift;
          x *= xscale;

          x = (x > xmin ? x : xmin);
          x = (x < xmax ? x : xmax);

          int xi = static_cast<int>(x + 0.5);

          outPtr[xi]++;

          inPtr += nc;
          }
        while (--n);
        }
      }
    inIter.NextSpan();
    }
}

//----------------------------------------------------------------------------
template<class T>
void vtkImageHistogramExecuteInt(
  vtkImageHistogram *self,
  vtkImageData *inData, vtkImageStencilData *stencil,
  T *inPtr, int extent[6], vtkIdType *outPtr, int component, int threadId)
{
  vtkImageStencilIterator<T>
    inIter(inData, stencil, extent, ((threadId == 0) ? self : NULL));

  // set up components
  int nc = inData->GetNumberOfScalarComponents();
  int c = component;
  if (c < 0)
    {
    nc = 1;
    c = 0;
    }

  // iterate over all spans in the stencil
  while (!inIter.IsAtEnd())
    {
    if (inIter.IsInStencil())
      {
      inPtr = inIter.BeginSpan();
      T *inPtrEnd = inIter.EndSpan();

      // iterate over all voxels in the span
      if (inPtr != inPtrEnd)
        {
        int n = static_cast<int>((inPtrEnd - inPtr)/nc);
        inPtr += c;
        do
          {
          outPtr[*inPtr]++;
          inPtr += nc;
          }
        while (--n);
        }
      }
    inIter.NextSpan();
    }
}

// no-op version for float
void vtkImageHistogramExecuteInt(
  vtkImageHistogram *, vtkImageData *, vtkImageStencilData *,
  float *, int [6], vtkIdType *, int, int)
{
}

// no-op version for double
void vtkImageHistogramExecuteInt(
  vtkImageHistogram *, vtkImageData *, vtkImageStencilData *,
  double *, int [6], vtkIdType *, int, int)
{
}

//----------------------------------------------------------------------------
void vtkImageHistogramGenerateImage(
  vtkIdType *histogram, int nx,
  unsigned char *outPtr, int scale, int size[2], int extent[6])
{
  vtkIdType incX = 1;
  vtkIdType incY = (extent[1] - extent[0] + 1);

  // find tallest peak in histogram
  vtkIdType peak = 0;
  int ix;
  for (ix = 0; ix < nx; ++ix)
    {
    vtkIdType c = histogram[ix];
    peak = (peak >= c ? peak : c);
    }

  // compute vertical scale factor
  double b = 0.0;
  if (peak > 0)
    {
    double sum = peak;
    switch (scale)
      {
      case vtkImageHistogram::Log:
        sum = log(sum) + 1.0;
        break;
      case vtkImageHistogram::Sqrt:
        sum = sqrt(sum);
        break;
      case vtkImageHistogram::Linear:
        break;
      }
    b = (size[1] - 1)/sum;
    }

  // compute horizontal scale factor
  double a = 0.0;
  if (size[0] > 0)
    {
    a = nx*1.0/size[0];
    }

  double x = extent[0]*a;
  ix = static_cast<int>(x);
  for (int i = extent[0]; i <= extent[1]; i++)
    {
    // use max of the original bins to compute new bin height
    double sum = histogram[ix];
    x = (i + 1)*a;
    int ix1 = static_cast<int>(x);
    for (; ix < ix1; ix++)
      {
      double v = histogram[ix];
      sum = (sum > v ? sum : v);
      }
    // scale the bin height
    if (sum > 0)
      {
      switch (scale)
        {
        case vtkImageHistogram::Log:
          sum = log(sum) + 1;
          break;
        case vtkImageHistogram::Sqrt:
          sum = sqrt(sum);
          break;
        case vtkImageHistogram::Linear:
          break;
        }
      }
    int height = static_cast<int>(sum*b);
    height = (height < extent[3] ? height : extent[3]);
    // draw the bin
    unsigned char *outPtr1 = outPtr;
    int j = extent[2];
    for (; j <= height; j++)
      {
      *outPtr1 = 255;
      outPtr1 += incY;
      }
    for (; j <= extent[3]; j++)
      {
      *outPtr1 = 0;
      outPtr1 += incY;
      }
    outPtr += incX;
    }
}

} // end anonymous namespace

//----------------------------------------------------------------------------
// override from vtkThreadedImageAlgorithm to customize the multithreading
int vtkImageHistogram::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // clear the thread output pointers
  int n = this->GetNumberOfThreads();
  for (int k = 0; k < n; k++)
    {
    this->ThreadOutput[k] = 0;
    }

  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  vtkImageData *image = vtkImageData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  int scalarType = image->GetScalarType();
  double scalarRange[2];

  // handle automatic binning
  if (this->AutomaticBinning)
    {
    switch (scalarType)
      {
      case VTK_CHAR:
      case VTK_UNSIGNED_CHAR:
      case VTK_SIGNED_CHAR:
        {
        vtkDataArray::GetDataTypeRange(scalarType, scalarRange);
        this->NumberOfBins = 256;
        this->BinSpacing = 1.0;
        this->BinOrigin = scalarRange[0];
        }
        break;
      case VTK_SHORT:
      case VTK_UNSIGNED_SHORT:
      case VTK_INT:
      case VTK_UNSIGNED_INT:
      case VTK_LONG:
      case VTK_UNSIGNED_LONG:
        {
        this->ComputeImageScalarRange(image, scalarRange);
        if (scalarRange[0] > 0) { scalarRange[0] = 0; }
        if (scalarRange[1] < 0) { scalarRange[1] = 0; }
        unsigned long binMaxId =
          static_cast<unsigned long>(scalarRange[1] - scalarRange[0]);
        this->BinOrigin = scalarRange[0];
        this->BinSpacing = 1.0;
        if (binMaxId < 255)
          {
          binMaxId = 255;
          }
        if (binMaxId > static_cast<unsigned long>(this->MaximumNumberOfBins-1))
          {
          binMaxId = static_cast<unsigned long>(this->MaximumNumberOfBins-1);
          if (binMaxId > 0)
            {
            this->BinSpacing = (scalarRange[1] - scalarRange[0])/binMaxId;
            }
          }
        this->NumberOfBins = static_cast<int>(binMaxId + 1);
        }
        break;
      default:
        {
        this->NumberOfBins = this->MaximumNumberOfBins;
        this->ComputeImageScalarRange(image, scalarRange);
        if (scalarRange[0] > 0) { scalarRange[0] = 0; }
        if (scalarRange[1] < 0) { scalarRange[1] = 0; }
        this->BinOrigin = scalarRange[0];
        this->BinSpacing = 1.0;
        if (scalarRange[1] > scalarRange[0])
          {
          if (this->NumberOfBins > 1)
            {
            this->BinSpacing =
              (scalarRange[1] - scalarRange[0])/(this->NumberOfBins - 1);
            }
          }
        }
        break;
      }
    }

  // start of code copied from vtkThreadedImageAlgorithm

  // setup the threads structure
  vtkImageHistogramThreadStruct ts;
  ts.Algorithm = this;
  ts.Request = request;
  ts.InputsInfo = inputVector;
  ts.OutputsInfo = outputVector;

  // allocate the output data
  int numberOfOutputs = this->GetNumberOfOutputPorts();
  if (numberOfOutputs > 0)
    {
    for (int i = 0; i < numberOfOutputs; ++i)
      {
      vtkInformation* outInfo = outputVector->GetInformationObject(i);
      vtkImageData *outData = vtkImageData::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));
      if (outData)
        {
        int updateExtent[6];
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                     updateExtent);
        this->AllocateOutputData(outData, updateExtent);
        }
      }
    }

  // copy arrays from first input to output
  int numberOfInputs = this->GetNumberOfInputPorts();
  if (numberOfInputs > 0)
    {
    vtkInformationVector* portInfo = inputVector[0];
    int numberOfConnections = portInfo->GetNumberOfInformationObjects();
    if (numberOfConnections && numberOfOutputs)
      {
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
      vtkImageData *inData = vtkImageData::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
      vtkInformation* outInfo = outputVector->GetInformationObject(0);
      vtkImageData *outData = vtkImageData::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));
      this->CopyAttributeData(inData, outData, inputVector);
      }
    }

  this->Threader->SetNumberOfThreads(this->NumberOfThreads);
  this->Threader->SetSingleMethod(vtkImageHistogramThreadedExecute, &ts);

  // always shut off debugging to avoid threading problems with GetMacros
  int debug = this->Debug;
  this->Debug = 0;
  this->Threader->SingleMethodExecute();
  this->Debug = debug;

  // end of code copied from vtkThreadedImageAlgorithm

  // create the histogram array
  this->Histogram->SetNumberOfComponents(1);
  this->Histogram->SetNumberOfTuples(this->NumberOfBins);
  vtkIdType *histogram = this->Histogram->GetPointer(0);

  // clear histogram to zero
  int nx = this->NumberOfBins;
  int ix;
  for (ix = 0; ix < nx; ++ix)
    {
    histogram[ix] = 0;
    }

  // piece together the histogram results from each thread
  vtkIdType total = 0;
  for (int j = 0; j < n; j++)
    {
    vtkIdType *outPtr2 = this->ThreadOutput[j];
    if (outPtr2)
      {
      int xmin = this->ThreadBinRange[j][0];
      int xmax = this->ThreadBinRange[j][1];
      for (ix = xmin; ix <= xmax; ++ix)
        {
        vtkIdType c = *outPtr2++;
        histogram[ix] += c;
        total += c;
        }
      }
    }

  // set the total
  this->Total = total;

  // delete the temporary memory
  for (int j = 0; j < n; j++)
    {
    if (this->ThreadOutput[j])
      {
      delete [] this->ThreadOutput[j];
      }
    }

  // generate the output image
  if (this->GetNumberOfOutputPorts() > 0 &&
      this->GenerateHistogramImage)
    {
    info = outputVector->GetInformationObject(0);
    image = vtkImageData::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));
    int *outExt = image->GetExtent();
    vtkImageHistogramGenerateImage(
      this->Histogram->GetPointer(0), this->NumberOfBins,
      static_cast<unsigned char *>(image->GetScalarPointerForExtent(outExt)),
      this->HistogramImageScale, this->HistogramImageSize, outExt);
    }

  return 1;
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageHistogram::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***vtkNotUsed(inData),
  vtkImageData **vtkNotUsed(outData),
  int extent[6], int threadId)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  void *inPtr = inData->GetScalarPointerForExtent(extent);

  vtkImageStencilData *stencil = this->GetStencil();

  double binOrigin = this->BinOrigin;
  double binSpacing = this->BinSpacing;
  int scalarType = inData->GetScalarType();
  int component = this->ActiveComponent;

  // can use faster binning method for int data
  bool useFastExecute = (binSpacing == 1.0 &&
    scalarType != VTK_FLOAT && scalarType != VTK_DOUBLE);

  double scalarRange[2];
  int *binRange = this->ThreadBinRange[threadId];

  // compute the scalar range of the data unless it is byte data,
  // this allows us to allocate less memory for the histogram
  if (scalarType == VTK_CHAR || scalarType == VTK_UNSIGNED_CHAR ||
      scalarType == VTK_SIGNED_CHAR)
    {
    vtkDataArray::GetDataTypeRange(scalarType, scalarRange);
    }
  else
    {
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        vtkImageHistogramExecuteRange(
          inData, stencil, static_cast<VTK_TT *>(inPtr),
          extent, scalarRange, component));
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
      }

    // if no voxels (e.g. due to stencil) then return
    if (scalarRange[0] > scalarRange[1])
      {
      return;
      }
    }

  // convert to bin numbers
  int maxBin = this->NumberOfBins - 1;
  double scale = 1.0/binSpacing;
  double minBinRange = (scalarRange[0] - binOrigin)*scale;
  double maxBinRange = (scalarRange[1] - binOrigin)*scale;
  if (minBinRange < 0)
    {
    minBinRange = 0;
    useFastExecute = false;
    }
  if (maxBinRange > maxBin)
    {
    maxBinRange = maxBin;
    useFastExecute = false;
    }
  binRange[0] = vtkMath::Floor(minBinRange + 0.5);
  binRange[1] = vtkMath::Floor(maxBinRange + 0.5);

  // allocate the histogram
  int n = binRange[1] - binRange[0] + 1;
  vtkIdType *histogram = new vtkIdType[n];
  this->ThreadOutput[threadId] = histogram;
  vtkIdType *tmpPtr = histogram;
  do { *tmpPtr++ = 0; } while (--n);

  // generate the histogram
  if (useFastExecute)
    {
    // adjust the pointer to allow direct indexing
    histogram -= binRange[0] + vtkMath::Floor(binOrigin + 0.5);

    // fast path for integer data
    switch(scalarType)
      {
      vtkTemplateAliasMacro(
        vtkImageHistogramExecuteInt(
          this, inData, stencil, static_cast<VTK_TT *>(inPtr),
          extent, histogram, component, threadId));
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
      }
    }
  else
    {
    // adjust the pointer to allow direct indexing
    histogram -= binRange[0];

    // bin via floating point shift/scale
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        vtkImageHistogramExecute(
          this, inData, stencil, static_cast<VTK_TT *>(inPtr),
          extent, histogram, binRange, binOrigin, binSpacing,
          component, threadId));
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageHistogram::ComputeImageScalarRange(
  vtkImageData *data, double range[2])
{
  if (data->GetNumberOfScalarComponents() == 1)
    {
    data->GetScalarRange(range);
    return;
    }

  int *extent = data->GetExtent();
  void *inPtr = data->GetScalarPointerForExtent(extent);
  int component = this->ActiveComponent;

  switch (data->GetScalarType())
    {
    vtkTemplateAliasMacro(
      vtkImageHistogramExecuteRange(
        data, 0, static_cast<VTK_TT *>(inPtr),
        extent, range, component));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
    }
}
