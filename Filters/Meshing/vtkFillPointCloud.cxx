// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFillPointCloud.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkJogglePoints.h"
#include "vtkLabeledImagePointSampler.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVoronoiCore.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFillPointCloud);

namespace // anonymous
{
// Dispatch some methods: use a fast path on real (float,double) data.
using vtkArrayDispatch::Reals;

// Metadata to keep track of number of new points per locator k slice.
using SliceMetaData = std::vector<vtkIdType>;

// Count the number of new points to be added.
struct CountPoints
{
  vtkStaticPointLocator* Locator;
  int Dims[3];
  SliceMetaData& SMD;
  vtkIdType NumberNewPoints;

  CountPoints(vtkStaticPointLocator* locator, int dims[3], SliceMetaData& sMD)
    : Locator(locator)
    , Dims{ dims[0], dims[1], dims[2] }
    , SMD(sMD)
    , NumberNewPoints(0)
  {
  }

  void Initialize() {}

  void operator()(vtkIdType slice, vtkIdType sliceEnd)
  {
    // Loop over all k slices
    for (; slice < sliceEnd; ++slice)
    {
      vtkIdType numNewPts = 0;
      vtkIdType kOffset = slice * this->Dims[0] * this->Dims[1];
      for (int j = 0; j < this->Dims[1]; ++j)
      {
        vtkIdType jOffset = j * this->Dims[0];
        for (int i = 0; i < this->Dims[0]; ++i)
        {
          vtkIdType binIdx = i + jOffset + kOffset;
          if (this->Locator->GetNumberOfPointsInBucket(binIdx) <= 0)
          {
            numNewPts++;
          }
        } // over i
      }   // over j
      this->SMD[slice] = numNewPts;
    } // over k slices
  }   // operator()

  // Roll up the total points with a prefix sum.
  void Reduce()
  {
    this->NumberNewPoints = 0;
    vtkIdType totalPts = 0;
    for (int slice = 0; slice < this->Dims[2]; ++slice)
    {
      vtkIdType numPts = this->SMD[slice];
      this->SMD[slice] = totalPts;
      totalPts += numPts;
    }
    this->NumberNewPoints = totalPts;
    this->SMD[this->Dims[2]] = totalPts;
  }

}; // CountPoints

// Copy points from the input points to the new output points.
struct CopyWorker
{
  template <typename InPT, typename OutPT>
  void operator()(InPT* inPts, OutPT* outPts, vtkIdType numInPts, vtkFillPointCloud* self)
  {
    const auto ipts = vtk::DataArrayTupleRange<3>(inPts);
    auto opts = vtk::DataArrayTupleRange<3>(outPts);

    // We use a threshold to test if the data size is small enough
    // to execute the functor serially.
    vtkSMPTools::For(0, numInPts, 10000,
      [self, &ipts, &opts](vtkIdType ptId, vtkIdType endPtId)
      {
        bool isFirst = vtkSMPTools::GetSingleThread();
        for (; ptId < endPtId; ++ptId)
        {
          if (isFirst)
          {
            self->CheckAbort();
          }
          if (self->GetAbortOutput())
          {
            break;
          }

          const auto xi = ipts[ptId];
          auto xo = opts[ptId];

          xo[0] = xi[0];
          xo[1] = xi[1];
          xo[2] = xi[2];
        } // over all input points
      }); // lambda
  }
}; // CopyWorker

// Appends / copies the output points of the point sampler onto
// the new output points.
struct AppendWorker
{
  template <typename InPT, typename OutPT>
  void operator()(
    InPT* inPts, OutPT* outPts, vtkIdType numInPts, vtkIdType numNewPts, vtkFillPointCloud* self)
  {
    const auto ipts = vtk::DataArrayTupleRange<3>(inPts);
    auto opts = vtk::DataArrayTupleRange<3>(outPts);

    // We use a threshold to test if the data size is small enough
    // to execute the functor serially.
    vtkSMPTools::For(0, numNewPts, 10000,
      [numInPts, self, &ipts, &opts](vtkIdType ptId, vtkIdType endPtId)
      {
        bool isFirst = vtkSMPTools::GetSingleThread();
        for (; ptId < endPtId; ++ptId)
        {
          if (isFirst)
          {
            self->CheckAbort();
          }
          if (self->GetAbortOutput())
          {
            break;
          }

          const auto xi = ipts[ptId];
          auto xo = opts[ptId + numInPts];

          xo[0] = xi[0];
          xo[1] = xi[1];
          xo[2] = xi[2];
        } // over all input points
      }); // lambda
  }
}; // AppendWorker

// Perform the actual point filling / generation for the uniform strategy.
struct FillWorker
{
  template <typename OutPT>
  void operator()(OutPT* outPts, vtkIdType numInPts, int dims[3], double radius, SliceMetaData& sMD,
    vtkFillPointCloud* self)
  {
    auto opts = vtk::DataArrayTupleRange<3>(outPts);
    vtkIdType numSlices = sMD.size() - 1;
    vtkStaticPointLocator* locator = self->GetLocator();
    bool joggle = self->GetJoggle();

    // We use a threshold to test if the data size is small enough
    // to execute the functor serially.
    vtkSMPThreadLocal<vtkVoronoiRandom01Range> LocalGenerator;
    vtkSMPTools::For(0, numSlices, 25,
      [numInPts, dims, locator, sMD, self, joggle, radius, &opts, &LocalGenerator](
        vtkIdType slice, vtkIdType endSlice)
      {
        auto& localGen = LocalGenerator.Local();
        bool isFirst = vtkSMPTools::GetSingleThread();
        for (; slice < endSlice; ++slice)
        {
          if (isFirst)
          {
            self->CheckAbort();
          }
          if (self->GetAbortOutput())
          {
            break;
          }

          vtkIdType ptOffset = numInPts + sMD[slice];

          vtkIdType kOffset = slice * dims[0] * dims[1];
          for (int j = 0; j < dims[1]; ++j)
          {
            vtkIdType jOffset = j * dims[0];
            for (int i = 0; i < dims[0]; ++i)
            {
              vtkIdType binIdx = i + jOffset + kOffset;
              if (locator->GetNumberOfPointsInBucket(binIdx) <= 0)
              {
                auto xo = opts[ptOffset];

                double x[3];
                locator->GetBucketCenter(i, j, slice, x);

                if (joggle)
                {
                  localGen.Seed(ptOffset); // produce invariant output
                  vtkVoronoiJoggle::JoggleXYZ(x, x, radius, localGen);
                }

                xo[0] = x[0];
                xo[1] = x[1];
                xo[2] = x[2];

                ptOffset++;
              } // if need to add point
            }   // over i bins
          }     // over j bins
        }       // over all slices
      });       // lambda
  }
}; // FillWorker

// Coordinate with a vtkLabeledImagePointSampler to implement the adaptive
// strategy.
vtkIdType ExecuteAdaptiveStrategy(vtkStaticPointLocator* locator, int dims[3], double origin[3],
  double spacing[3], vtkLabeledImagePointSampler* pointSampler, double radius,
  vtkFillPointCloud* filter)
{
  // Create a volume based on the binning locator. Use an integer
  // segmentation map.
  vtkNew<vtkImageData> volume;
  volume->SetDimensions(dims);
  volume->SetOrigin(origin);
  volume->SetSpacing(spacing);

  vtkNew<vtkIntArray> volScalars;
  volScalars->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
  int* volPtr = volScalars->GetPointer(0);
  volume->GetPointData()->SetScalars(volScalars);

  // Now process the volume slice-by-slice to assign an occupancy label
  // to the volume (1=occupied, 0=unoccupied).
  int inLabel = filter->GetInLabel();
  int backgroundLabel = filter->GetBackgroundLabel();
  vtkSMPTools::For(0, dims[2], 50,
    [dims, locator, inLabel, backgroundLabel, volPtr](vtkIdType slice, vtkIdType sliceEnd)
    {
      // Loop over all k slices
      for (; slice < sliceEnd; ++slice)
      {
        vtkIdType kOffset = slice * dims[0] * dims[1];
        for (int j = 0; j < dims[1]; ++j)
        {
          vtkIdType jOffset = j * dims[0];
          for (int i = 0; i < dims[0]; ++i)
          {
            vtkIdType binIdx = i + jOffset + kOffset;
            if (locator->GetNumberOfPointsInBucket(binIdx) > 0)
            {
              volPtr[binIdx] = inLabel; // bin currently occupied
            }
            else
            {
              volPtr[binIdx] = backgroundLabel; // bin unoccupied
            }
          } // over i
        }   // over j
      }     // over k slices
    });     // lambda

  // Configure the point sampler.
  pointSampler->SetInputData(volume);
  pointSampler->SetLabel(0, inLabel);
  pointSampler->SetJoggle(filter->GetJoggle());
  pointSampler->SetJoggleRadiusIsAbsolute(filter->GetJoggleRadiusIsAbsolute());
  pointSampler->SetJoggleRadius(radius);
  pointSampler->Update();

  return pointSampler->GetOutput()->GetNumberOfPoints();
}

} // anonymous namespace

//------------------------------------------------------------------------------
vtkFillPointCloud::vtkFillPointCloud()
{
  this->FillStrategy = vtkFillPointCloud::UNIFORM;
  this->InLabel = 0;
  this->BackgroundLabel = -100;
  this->MaximumNumberOfPoints = 1000;
  this->Joggle = true;
  this->JoggleRadius = 0.1;
  this->JoggleRadiusIsAbsolute = false;

  this->ManualLocatorControl = false;
  this->Locator = vtkSmartPointer<vtkStaticPointLocator>::New();

  this->PointSampler = vtkSmartPointer<vtkLabeledImagePointSampler>::New();
  this->PointSampler->SetDensityDistributionToExponential();
  this->PointSampler->SetN(2);
  this->PointSampler->RandomizeOn();
  this->PointSampler->SetOutputTypeToBackgroundPoints();
  this->PointSampler->JoggleOn();
  this->PointSampler->JoggleRadiusIsAbsoluteOn(); // let this filter compute the radius

  this->NumberOfAddedPoints = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkFillPointCloud::~vtkFillPointCloud() = default;

//------------------------------------------------------------------------------
// Needed to synchronize joggle data members
void vtkFillPointCloud::UpdateJoggleInfo()
{
  this->Joggle = this->PointSampler->GetJoggle();
  this->JoggleRadius = this->PointSampler->GetJoggleRadius();
  this->JoggleRadiusIsAbsolute = this->PointSampler->GetJoggleRadiusIsAbsolute();
}

//------------------------------------------------------------------------------
// Create the output points and output region ids. This is filled in
// as much as possible, leaving the newly created fill points to be
// copied on to the end of the output points.
vtkPoints* vtkFillPointCloud::CreatePointsAndRegions(
  vtkPointSet* input, vtkInformationVector** inputVector, vtkIdType numNewPts, vtkPolyData* output)
{
  // Allocate new points. Currently the output type is always double;
  // this could be changed to user specified precision at some point.
  vtkPoints* inPts = input->GetPoints();
  vtkIdType numInPts = inPts->GetNumberOfPoints();
  vtkNew<vtkPoints> newPts;
  newPts->SetDataTypeToDouble();
  vtkIdType totalPts = numInPts + numNewPts;
  newPts->SetNumberOfPoints(totalPts);
  output->SetPoints(newPts);

  // Copy the input points to the output points.
  using CopyDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
  CopyWorker copyWorker;

  if (!CopyDispatch::Execute(inPts->GetData(), newPts->GetData(), copyWorker, numInPts, this))
  { // fallback to slowpath
    copyWorker(inPts->GetData(), newPts->GetData(), numInPts, this);
  }

  // See if there are input region labels. Create an output region label
  // array.
  vtkSmartPointer<vtkIntArray> regionIds;
  vtkDataArray* rIds = this->GetInputArrayToProcess(0, inputVector);
  if (rIds)
  {
    if (!(regionIds = vtkIntArray::FastDownCast(rIds)) || regionIds->GetNumberOfComponents() > 1)
    {
      regionIds = nullptr;
    }
  }

  vtkNew<vtkIntArray> outRegions;
  outRegions->SetNumberOfTuples(totalPts);
  outRegions->SetName("RegionIds");
  output->GetPointData()->SetScalars(outRegions);

  // Input points region ids are captured.
  if (regionIds)
  { // copy
    std::copy(
      regionIds->GetPointer(0), regionIds->GetPointer(0) + numInPts, outRegions->GetPointer(0));
  }
  else
  { // fill
    vtkSMPTools::Fill(
      outRegions->GetPointer(0), outRegions->GetPointer(0) + numInPts, this->InLabel);
  }

  // Fill in the newly added points region ids.
  vtkSMPTools::Fill(outRegions->GetPointer(0) + numInPts,
    outRegions->GetPointer(0) + numInPts + numNewPts, this->BackgroundLabel);

  return newPts.Get();
} // CreatePointsAndRegions

//------------------------------------------------------------------------------
int vtkFillPointCloud::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Synchronize the joggle-related data members with the internal instance of
  // vtkLabeledImagePointSampler.
  this->UpdateJoggleInfo();

  // A locator is used to locate closest points. Here we are using it as an occupancy
  // measure: locating bins not containing any points.
  if (!this->Locator)
  {
    vtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  if (!this->ManualLocatorControl)
  {
    this->Locator->SetMaxNumberOfBuckets(this->MaximumNumberOfPoints);
  }
  this->Locator->BuildLocator();

  // Grab information from the locator.
  int dims[3];
  this->Locator->GetDivisions(dims);
  double bounds[6], origin[3];
  this->Locator->GetBounds(bounds);
  origin[0] = bounds[0];
  origin[1] = bounds[2];
  origin[2] = bounds[4];
  double spacing[3];
  this->Locator->GetSpacing(spacing);

  // Set the joggle radius.
  double radius = this->JoggleRadius;
  if (!this->JoggleRadiusIsAbsolute)
  {
    double minLen = std::min(spacing[0], spacing[1]);
    minLen = std::min(minLen, spacing[2]);
    radius *= minLen;
  }

  // Shallow copy the input to the output. We'll allocate and replace the
  // points and verts later.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Fork execution depending on fill strategy. At this point, we
  // need a count of newly executed points.
  vtkIdType numInPts = input->GetNumberOfPoints();
  vtkPoints* newPts;
  vtkIdType numNewPts;
  if (this->FillStrategy == vtkFillPointCloud::UNIFORM)
  {
    // The uniform processing of data will be slice by slice. We create slice
    // metadata (sMD) to keep track of information (i.e., the number of new
    // points added) on each slice. This original point count is then
    // transformed into offets for later threaded processing.
    SliceMetaData sMD(dims[2] + 1);

    // Count the number of empty locator bins -> this determines the
    // number of new points added. Process over k slices.
    CountPoints countPoints(this->Locator, dims, sMD);
    vtkSMPTools::For(0, dims[2], 100, countPoints);
    numNewPts = countPoints.NumberNewPoints;

    // If no new points are to be added, just return-we are done.
    if (numNewPts <= 0)
    {
      return 1;
    }

    // Create the new points, and manage the regions ids.
    newPts = this->CreatePointsAndRegions(input, inputVector, numNewPts, output);

    // Dispatch the fill process. Fastpath for real types, fallback to slower
    // path for non-real types.
    using FillDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    FillWorker fillWorker;

    // Finally execute the filling operation.
    if (!FillDispatch::Execute(newPts->GetData(), fillWorker, numInPts, dims, radius, sMD, this))
    { // fallback to slowpath
      fillWorker(newPts->GetData(), numInPts, dims, radius, sMD, this);
    }
  }    // UNIFORM strategy
  else // if ( this->FillStrategy == vtkFillPointCloud::ADAPTIVE )
  {
    numNewPts = ExecuteAdaptiveStrategy(
      this->Locator, dims, origin, spacing, this->PointSampler, radius, this);

    // If no new points are to be added, just return-we are done.
    if (numNewPts <= 0)
    {
      return 1;
    }

    // Create the new points, and manage the regions ids.
    newPts = this->CreatePointsAndRegions(input, inputVector, numNewPts, output);

    // Now copy the points from the point sampler into the tail end of the
    // new points array.
    using AppendDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
    AppendWorker appendWorker;

    // Finally execute the appending operation.
    vtkPoints* samplerPts = this->PointSampler->GetOutput()->GetPoints();
    if (!AppendDispatch::Execute(
          samplerPts->GetData(), newPts->GetData(), appendWorker, numInPts, numNewPts, this))
    { // fallback to slowpath
      appendWorker(samplerPts->GetData(), newPts->GetData(), numInPts, numNewPts, this);
    }
  } // ADAPTIVE strategy

  this->NumberOfAddedPoints = numNewPts;
  vtkIdType totalPts = numInPts + numNewPts;

  // Produce vertices for rendering. The vertices are represented by a single
  // polyvertex cell.
  vtkNew<vtkCellArray> verts;

  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfTuples(2);
  offsets->SetValue(0, 0);
  offsets->SetValue(1, totalPts);

  vtkNew<vtkIdTypeArray> conn;
  conn->SetNumberOfTuples(totalPts);
  vtkIdType* c = conn->GetPointer(0);
  vtkIdType ptId = 0;
  std::generate(c, c + totalPts, [&] { return ptId++; });

  verts->SetData(offsets, conn);
  output->SetVerts(verts);

  return 1;
}

//------------------------------------------------------------------------------
// Since users have access to the locator and point sampler we need to take
// into account their modified time.
vtkMTimeType vtkFillPointCloud::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  mTime = std::max(mTime, this->Locator->GetMTime());
  return std::max(mTime, this->PointSampler->GetMTime());
}

//------------------------------------------------------------------------------
int vtkFillPointCloud::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkFillPointCloud::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Fill Strategy: " << this->FillStrategy << "\n";

  os << indent << "In Label: " << this->InLabel << "\n";
  os << indent << "Background Label: " << this->BackgroundLabel << "\n";
  os << indent << "Maximum Number Of Points: " << this->MaximumNumberOfPoints << "\n";

  os << indent << "Joggle: " << (this->Joggle ? "On\n" : "Off\n");
  os << indent << "Joggle Radius: " << this->JoggleRadius << "\n";
  os << indent
     << "Joggle Radius Is Absolute: " << (this->JoggleRadiusIsAbsolute ? "On\n" : "Off\n");

  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Manual Locator Control: " << (this->ManualLocatorControl ? "On\n" : "Off\n");

  os << indent << "Point Sampler: " << this->PointSampler << "\n";
}
VTK_ABI_NAMESPACE_END
