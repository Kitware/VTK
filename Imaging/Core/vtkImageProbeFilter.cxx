// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageProbeFilter.h"

#include "vtkImageData.h"
#include "vtkImageInterpolator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkThreadedImageAlgorithm.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageProbeFilter);

vtkCxxSetObjectMacro(vtkImageProbeFilter, Interpolator, vtkAbstractImageInterpolator);

//------------------------------------------------------------------------------
vtkImageProbeFilter::vtkImageProbeFilter()
{
  // Default is no interpolation
  this->Interpolator = nullptr;

  // Inputs are "Input" and "Source"
  this->SetNumberOfInputPorts(2);

  // The mask for Input points that intersect the Source image.
  this->MaskScalars = nullptr;

  // Process active point scalars on "Source" input
  this->SetInputArrayToProcess(
    0, 1, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkImageProbeFilter::~vtkImageProbeFilter()
{
  if (this->Interpolator)
  {
    this->Interpolator->Delete();
  }
}

//------------------------------------------------------------------------------
int vtkImageProbeFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  }
  else
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkImageProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
void vtkImageProbeFilter::SetSourceData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkImageProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//------------------------------------------------------------------------------
int vtkImageProbeFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* source = vtkImageData::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // copy the geometry of the Input to the Output
  output->CopyStructure(input);

  // probe the Source to generate the Output attributes
  if (source)
  {
    this->Probe(input, source, output);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkImageProbeFilter::InitializeForProbing(
  vtkDataSet* input, vtkImageData* source, vtkDataSet* output)
{
  // Get information about the input
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkDataArray* inScalars = source->GetPointData()->GetScalars();
  int scalarType = inScalars->GetDataType();
  int numComponents = inScalars->GetNumberOfComponents();

  // Prepare the interpolator
  if (this->Interpolator == nullptr)
  {
    vtkImageInterpolator* interp = vtkImageInterpolator::New();
    interp->SetInterpolationModeToNearest();
    interp->SetTolerance(0.5);
    this->Interpolator = interp;
  }
  this->Interpolator->Initialize(source);
  numComponents = this->Interpolator->ComputeNumberOfComponents(numComponents);

  // Create the output scalar array (same type as "Source")
  vtkDataArray* scalars = vtkDataArray::CreateDataArray(scalarType);
  scalars->SetNumberOfComponents(numComponents);
  scalars->SetNumberOfTuples(numPts);
  scalars->SetName("ImageScalars");

  // Create a scalar array for mask
  vtkUnsignedCharArray* mask = vtkUnsignedCharArray::New();
  mask->SetNumberOfComponents(1);
  mask->SetNumberOfTuples(numPts);
  mask->SetName("MaskScalars");
  this->MaskScalars = mask;

  vtkPointData* outPD = output->GetPointData();
  int idx = outPD->AddArray(scalars);
  scalars->Delete();
  outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  outPD->AddArray(mask);
  mask->Delete();
}

//------------------------------------------------------------------------------
// Thread local storage for the execution of the filter.
struct vtkImageProbeFilter::ProbePointsThreadLocal
{
  ProbePointsThreadLocal()
    : BaseThread(false)
  {
  }

  // Per-thread storage space.
  std::vector<double> ThreadStore;
  // BaseThread will be set 'true' for thread that gets first piece.
  bool BaseThread;
};

//------------------------------------------------------------------------------
// This functor is used by vtkSMPTools, it is called by the threads and,
// in turn, it calls ProbePoints() over a range of points.
class vtkImageProbeFilter::ProbePointsWorklet
{
public:
  ProbePointsWorklet(
    vtkImageProbeFilter* probeFilter, vtkDataSet* input, vtkImageData* source, vtkPointData* outPD)
    : ProbeFilter(probeFilter)
    , Input(input)
    , Source(source)
    , OutPointData(outPD)
  {
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    if (startId == 0)
    {
      this->Thread.Local().BaseThread = true;
    }
    this->ProbeFilter->ProbePoints(
      this->Input, this->Source, this->OutPointData, startId, endId, &this->Thread.Local());
  }

private:
  vtkImageProbeFilter* ProbeFilter;
  vtkDataSet* Input;
  vtkImageData* Source;
  vtkPointData* OutPointData;
  vtkSMPThreadLocal<vtkImageProbeFilter::ProbePointsThreadLocal> Thread;
};

//------------------------------------------------------------------------------
// This is for vtkMultiThreader (not for vtkSMPTools)
struct vtkImageProbeFilter::ProbePointsThreadStruct
{
  ProbePointsThreadStruct(vtkImageProbeFilter* probeFilter, vtkDataSet* input, vtkImageData* source,
    vtkPointData* outPD, vtkIdType numberOfPoints, int numberOfThreads)
    : ProbeFilter(probeFilter)
    , Input(input)
    , Source(source)
    , OutPointData(outPD)
    , NumberOfPoints(numberOfPoints)
    , Thread(numberOfThreads)
  {
    if (numberOfThreads > 0)
    {
      Thread[0].BaseThread = true;
    }
  }

  static VTK_THREAD_RETURN_TYPE Execute(void* arg)
  {
    vtkMultiThreader::ThreadInfo* ti = static_cast<vtkMultiThreader::ThreadInfo*>(arg);
    ProbePointsThreadStruct* ts = static_cast<ProbePointsThreadStruct*>(ti->UserData);

    vtkTypeInt64 numPts = ts->NumberOfPoints;
    vtkIdType startId = static_cast<vtkIdType>((numPts * ti->ThreadID) / ti->NumberOfThreads);
    vtkIdType endId = static_cast<vtkIdType>((numPts * (ti->ThreadID + 1)) / ti->NumberOfThreads);

    ts->ProbeFilter->ProbePoints(
      ts->Input, ts->Source, ts->OutPointData, startId, endId, &ts->Thread[ti->ThreadID]);

    return VTK_THREAD_RETURN_VALUE;
  }

private:
  vtkImageProbeFilter* ProbeFilter;
  vtkDataSet* Input;
  vtkImageData* Source;
  vtkPointData* OutPointData;
  vtkIdType NumberOfPoints;
  std::vector<vtkImageProbeFilter::ProbePointsThreadLocal> Thread;
};

//------------------------------------------------------------------------------
void vtkImageProbeFilter::DoProbing(vtkDataSet* input, vtkImageData* source, vtkDataSet* output)
{
  vtkDebugMacro(<< "Probing data");

  vtkPointData* outPD = output->GetPointData();

  if (!vtkThreadedImageAlgorithm::GetGlobalDefaultEnableSMP())
  {
    // Use vtkMultiThreader, use fewer threads for small data
    vtkIdType numPts = input->GetNumberOfPoints();
    int numThreads = vtkMultiThreader::GetGlobalDefaultNumberOfThreads();
    numThreads = (numPts >= numThreads * 100 ? numThreads : static_cast<int>(1 + numPts / 100));
    if (numThreads > 1)
    {
      // Use multi-threader to execute in parallel
      ProbePointsThreadStruct ts(this, input, source, outPD, numPts, numThreads);
      vtkNew<vtkMultiThreader> threader;
      threader->SetNumberOfThreads(numThreads);
      threader->SetSingleMethod(ProbePointsThreadStruct::Execute, &ts);
      threader->SingleMethodExecute();
    }
    else
    {
      // Execute in main thread
      vtkImageProbeFilter::ProbePointsThreadLocal tl;
      this->ProbePoints(input, source, outPD, 0, numPts, &tl);
    }
    return;
  }

  // Estimate the granularity for multithreading
  int threads = vtkSMPTools::GetEstimatedNumberOfThreads();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType grain = numPts / threads;
  vtkIdType minGrain = 100;
  vtkIdType maxGrain = 1000;
  grain = vtkMath::ClampValue(grain, minGrain, maxGrain);

  // Multithread the execution
  ProbePointsWorklet worklet(this, input, source, outPD);
  vtkSMPTools::For(0, numPts, grain, worklet);
}

//------------------------------------------------------------------------------
void vtkImageProbeFilter::Probe(vtkDataSet* input, vtkImageData* source, vtkDataSet* output)
{
  // second stage of attribute initialization: initialize from Input
  this->InitializeForProbing(input, source, output);

  // probe the Source at each Input point
  this->DoProbing(input, source, output);
}

//------------------------------------------------------------------------------
void vtkImageProbeFilter::ProbePoints(vtkDataSet* input, vtkImageData* source, vtkPointData* outPD,
  vtkIdType startId, vtkIdType endId, ProbePointsThreadLocal* threadLocal)
{
  // Get information about the pixels type
  vtkDataArray* scalars = outPD->GetScalars();
  double minVal = scalars->GetDataTypeMin();
  double maxVal = scalars->GetDataTypeMax();
  int scalarType = scalars->GetDataType();
  int numComp = scalars->GetNumberOfComponents();

  // Information about the interpolator
  vtkAbstractImageInterpolator* interpolator = this->Interpolator;
  int numToClamp = numComp;
  int numToRound = numComp;
  if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
  {
    // Only integer data needs clamping or rounding
    numToClamp = 0;
    numToRound = 0;
  }
  else
  {
    vtkImageInterpolator* ii = vtkImageInterpolator::SafeDownCast(interpolator);
    if (ii && ii->GetInterpolationMode() == VTK_NEAREST_INTERPOLATION)
    {
      // Neither clamping nor rounding is used for nearest-neighbor interpolation
      numToClamp = 0;
      numToRound = 0;
    }
    else if (ii && ii->GetInterpolationMode() == VTK_LINEAR_INTERPOLATION)
    {
      // Clamping is only needed for high-order (e.g. cubic, sinc) interpolation.
      numToClamp = 0;
    }
  }

  // We need workspace for pixel computations, use the stack if possible
  // or use thread-local storage vectors if pixels have >4 components.
  double storage[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  double* value = storage;
  double* zeros = storage + 4;
  if (numComp > 4)
  {
    std::vector<double>& storageVec = threadLocal->ThreadStore;
    storageVec.resize(numComp * 2, 0);
    value = storageVec.data();
    zeros = value + numComp;
  }

  // The mask array, says which points were within the image.
  unsigned char* mask = this->MaskScalars->GetPointer(0);

  // Loop over all input points, interpolating source data
  vtkIdType progressInterval = endId / 20 + 1;
  for (vtkIdType ptId = startId; ptId < endId && !GetAbortExecute(); ptId++)
  {
    if (threadLocal->BaseThread && !(ptId % progressInterval))
    {
      // This is not ideal, because if the base thread executes more than one piece,
      // then the progress will repeat its 0.0 to 1.0 progression for each piece.
      this->UpdateProgress(static_cast<double>(ptId) / endId);
    }

    // Get the xyz coordinate of the point in the input dataset
    double x[3];
    input->GetPoint(ptId, x);

    // Convert to structured coordinates
    double ijk[3];
    source->TransformPhysicalPointToContinuousIndex(x, ijk);

    if (interpolator->CheckBoundsIJK(ijk))
    {
      // Do the interpolation
      interpolator->InterpolateIJK(ijk, value);
      for (int c = 0; c < numToClamp; c++)
      {
        // Clamping is needed to avoid overflow when output is integer
        value[c] = vtkMath::ClampValue(value[c], minVal, maxVal);
      }
      for (int c = 0; c < numToRound; c++)
      {
        // This bias results in rounding when SetTuple casts to integer,
        // e.g. see implementation of vtkMath::Round()
        value[c] += (value[c] > 0.0 ? 0.5 : -0.5);
      }
      scalars->SetTuple(ptId, value);
      mask[ptId] = 1;
    }
    else
    {
      // If outside of the image, set to zero
      scalars->SetTuple(ptId, zeros);
      mask[ptId] = 0;
    }
  }
}

//------------------------------------------------------------------------------
int vtkImageProbeFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // copy extent info from Input to Output
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // copy scalar info from Source to Output
  int scalarType = vtkImageData::GetScalarType(sourceInfo);
  int numComponents = vtkImageData::GetNumberOfScalarComponents(sourceInfo);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, numComponents);

  return 1;
}

//------------------------------------------------------------------------------
int vtkImageProbeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // copy update extent from Output to Input
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);

  // set update extent of Source to its whole extent
  sourceInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkImageProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject* source = this->GetSource();
  vtkAbstractImageInterpolator* interpolator = this->GetInterpolator();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "Interpolator: " << interpolator << "\n";
}
VTK_ABI_NAMESPACE_END
