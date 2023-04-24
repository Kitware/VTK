// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) CSCS - Swiss National Supercomputing Centre
// SPDX-License-Identifier: BSD-3-Clause
// Hide VTK_DEPRECATED_IN_9_3_0() warnings
#define VTK_DEPRECATION_LEVEL 0

#include "vtkTemporalPathLineFilter.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedIntArray.h"

//
#include <cmath>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTemporalPathLineFilter);
//------------------------------------------------------------------------------
//
struct Position_t
{
  double x[3];
};
using Position = struct Position_t;

typedef std::vector<Position> CoordList;
typedef std::vector<vtkIdType> IdList;
typedef std::vector<vtkSmartPointer<vtkAbstractArray>> FieldList;

class ParticleTrail : public vtkObject
{
public:
  static ParticleTrail* New();
  vtkTypeMacro(ParticleTrail, vtkObject);
  //
  unsigned int firstpoint;
  unsigned int lastpoint;
  unsigned int length;
  long int GlobalId;
  vtkIdType TrailId;
  vtkIdType FrontPointId;
  bool alive;
  bool updated;
  CoordList Coords;
  FieldList Fields;
  //
  ParticleTrail()
  {
    this->TrailId = 0;
    this->FrontPointId = 0;
    this->GlobalId = ParticleTrail::UniqueId++;
  }

  static long int UniqueId;
};
vtkStandardNewMacro(ParticleTrail);

long int ParticleTrail::UniqueId = 0;

typedef vtkSmartPointer<ParticleTrail> TrailPointer;
typedef std::pair<vtkIdType, TrailPointer> TrailMapType;

class vtkTemporalPathLineFilterInternals : public vtkObject
{
public:
  static vtkTemporalPathLineFilterInternals* New();
  vtkTypeMacro(vtkTemporalPathLineFilterInternals, vtkObject);
  //
  typedef std::map<vtkIdType, TrailPointer>::iterator TrailIterator;
  std::map<vtkIdType, TrailPointer> Trails;
  //
  std::string LastIdArrayName;
  std::map<int, double> TimeStepSequence;
  //
  // This specifies the order of the arrays in the trails fields.  These are
  // valid in between calls to RequestData.
  std::vector<std::string> TrailFieldNames;
  // Input arrays corresponding to the entries in TrailFieldNames.  nullptr arrays
  // indicate missing arrays.  This field is only valid during a call to
  // RequestData.
  std::vector<vtkSmartPointer<vtkAbstractArray>> InputFieldArrays;

  std::vector<double> TimeSteps;
};
vtkStandardNewMacro(vtkTemporalPathLineFilterInternals);

typedef std::map<int, double>::iterator TimeStepIterator;
static constexpr double LATEST_TIME_MAX = VTK_DOUBLE_MAX;
//------------------------------------------------------------------------------
vtkTemporalPathLineFilter::vtkTemporalPathLineFilter()
{
  this->LatestTime = LATEST_TIME_MAX;

  this->PolyLines = vtkSmartPointer<vtkCellArray>::New();
  this->Vertices = vtkSmartPointer<vtkCellArray>::New();
  this->LineCoordinates = vtkSmartPointer<vtkPoints>::New();
  this->VertexCoordinates = vtkSmartPointer<vtkPoints>::New();
  this->TrailId = vtkSmartPointer<vtkFloatArray>::New();
  this->Internals = vtkSmartPointer<vtkTemporalPathLineFilterInternals>::New();

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2); // Lines and points
}
//------------------------------------------------------------------------------
vtkTemporalPathLineFilter::~vtkTemporalPathLineFilter()
{
  delete[] this->IdChannelArray;
  this->IdChannelArray = nullptr;
}
//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}
//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  // Lines on 0, First point as Vertex Cell on 1
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::SetBackwardTime(bool backward)
{
  if (this->BackwardTime != backward)
  {
    this->LatestTime = backward ? 0 : LATEST_TIME_MAX;
    this->BackwardTime = backward;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::SetSelectionConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::SetSelectionData(vtkDataSet* input)
{
  this->SetInputData(1, input);
}
//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Get(vtkStreamingDemandDrivenPipeline::INCOMPLETE_TIME_STEPS()))
  {
    return 1;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // The output data of this filter has no time associated with it.  It is the
  // result of computations that happen over all time.
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//------------------------------------------------------------------------------
TrailPointer vtkTemporalPathLineFilter::GetTrail(vtkIdType i)
{
  TrailPointer trail;
  vtkTemporalPathLineFilterInternals::TrailIterator t = this->Internals->Trails.find(i);
  if (t == this->Internals->Trails.end())
  {
    trail = vtkSmartPointer<ParticleTrail>::New();
    std::pair<vtkTemporalPathLineFilterInternals::TrailIterator, bool> result =
      this->Internals->Trails.insert(TrailMapType(i, trail));
    if (!result.second)
    {
      throw std::runtime_error("Unexpected map error");
    }
    // new trail created, reserve memory now for efficiency
    trail = result.first->second;
    trail->Coords.assign(this->MaxTrackLength, Position());
    trail->lastpoint = 0;
    trail->firstpoint = 0;
    trail->length = 0;
    trail->alive = true;
    trail->updated = false;
    trail->TrailId = i;

    trail->Fields.assign(this->Internals->InputFieldArrays.size(), nullptr);
    for (size_t j = 0; j < this->Internals->InputFieldArrays.size(); j++)
    {
      vtkAbstractArray* inputArray = this->Internals->InputFieldArrays[j];
      if (!inputArray)
        continue;
      trail->Fields[j].TakeReference(inputArray->NewInstance());
      trail->Fields[j]->SetName(inputArray->GetName());
      trail->Fields[j]->SetNumberOfComponents(inputArray->GetNumberOfComponents());
      trail->Fields[j]->SetNumberOfTuples(this->MaxTrackLength);
    }
  }
  else
  {
    trail = t->second;
  }
  return trail;
}
//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::IncrementTrail(TrailPointer trail, vtkDataSet* input, vtkIdType id)
{
  //
  // After a clip operation, some points might not exist anymore
  // if the Id is out of bounds, kill the trail
  //
  if (id >= input->GetNumberOfPoints())
  {
    trail->alive = false;
    trail->updated = true;
    return;
  }
  // if for some reason, two particles have the same ID, only update once
  // and use the point that is closest to the last point on the trail
  if (trail->updated && trail->length > 0)
  {
    unsigned int lastindex = (trail->lastpoint - 2) % this->MaxTrackLength;
    unsigned int thisindex = (trail->lastpoint - 1) % this->MaxTrackLength;
    double* coord0 = trail->Coords[lastindex].x;
    double* coord1a = trail->Coords[thisindex].x;
    double* coord1b = input->GetPoint(id);
    if (vtkMath::Distance2BetweenPoints(coord0, coord1b) <
      vtkMath::Distance2BetweenPoints(coord0, coord1a))
    {
      // new point is closer to previous than the one already present.
      // replace with this one.
      input->GetPoint(id, coord1a);
      for (size_t fieldId = 0; fieldId < trail->Fields.size(); fieldId++)
      {
        trail->Fields[fieldId]->InsertTuple(
          trail->lastpoint, id, this->Internals->InputFieldArrays[fieldId]);
      }
    }
    // all indices have been updated already, so just exit
    return;
  }
  //
  // Copy coord and scalar into trail
  //
  double* coord = trail->Coords[trail->lastpoint].x;
  input->GetPoint(id, coord);
  for (size_t fieldId = 0; fieldId < trail->Fields.size(); fieldId++)
  {
    trail->Fields[fieldId]->InsertTuple(
      trail->lastpoint, id, this->Internals->InputFieldArrays[fieldId]);
  }
  // make sure the increment is within our allowed range
  // and disallow zero distances
  double dist = 1.0;
  if (trail->length > 0)
  {
    unsigned int lastindex = (this->MaxTrackLength + trail->lastpoint - 1) % this->MaxTrackLength;
    double* lastcoord = trail->Coords[lastindex].x;
    //
    double distx = fabs(lastcoord[0] - coord[0]);
    double disty = fabs(lastcoord[1] - coord[1]);
    double distz = fabs(lastcoord[2] - coord[2]);
    dist = sqrt(distx * distx + disty * disty + distz * distz);
    //
    if (distx > this->MaxStepDistance[0] || disty > this->MaxStepDistance[1] ||
      distz > this->MaxStepDistance[2])
    {
      trail->alive = false;
      trail->updated = true;
      return;
    }
  }
  //
  // Extend the trail and wrap accordingly around maxlength
  //
  if (dist > 1E-9)
  {
    trail->lastpoint++;
    trail->length++;
    if (trail->length >= this->MaxTrackLength)
    {
      trail->lastpoint = trail->lastpoint % this->MaxTrackLength;
      trail->firstpoint = trail->lastpoint;
      trail->length = this->MaxTrackLength;
    }
    trail->updated = true;
  }
  trail->FrontPointId = id;
  trail->alive = true;
}

//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Get(vtkStreamingDemandDrivenPipeline::INCOMPLETE_TIME_STEPS()))
  {
    return 1;
  }

  // The RequestData method will tell the pipeline executive to iterate the
  // upstream pipeline to get each time step in order.  The executive in turn
  // will call this method to get the extent request for each iteration (in this
  // case the time step).
  double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestUpdateTime(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Get(vtkStreamingDemandDrivenPipeline::INCOMPLETE_TIME_STEPS()))
  {
    return 1;
  }

  // The RequestData method will tell the pipeline executive to iterate the
  // upstream pipeline to get each time step in order.  The executive in turn
  // will call this method to get the extent request for each iteration (in this
  // case the time step).
  double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->MaskPoints = std::max(this->MaskPoints, 1);

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo->Has(vtkStreamingDemandDrivenPipeline::INCOMPLETE_TIME_STEPS()))
  {
    return this->ExecuteStreaming(request, inputVector, outputVector);
  }
  return this->Execute(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::InitializeExecute(vtkDataSet* input, vtkPolyData* pathLines)
{
  vtkPointData* outPD = pathLines->GetPointData();
  outPD->CopyAllocate(
    input->GetPointData(), input->GetNumberOfPoints() * this->MaxTrackLength / this->MaskPoints);
  this->Internals->TrailFieldNames.resize(outPD->GetNumberOfArrays());
  for (int i = 0; i < outPD->GetNumberOfArrays(); i++)
  {
    this->Internals->TrailFieldNames[i] = outPD->GetArrayName(i);
  }

  this->Internals->InputFieldArrays.resize(this->Internals->TrailFieldNames.size());
}

//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::AccumulateTrails(vtkDataSet* input, vtkDataSet* selection)
{
  vtkPointData* inputPD = input->GetPointData();
  vtkIdTypeArray* ids = vtkArrayDownCast<vtkIdTypeArray>(inputPD->GetArray(this->IdChannelArray));
  vtkIdTypeArray* selectionIds = vtkArrayDownCast<vtkIdTypeArray>([&selection, &ids, this] {
    if (!selection)
    {
      return (vtkDataArray*)(nullptr);
    }
    vtkPointData* selectionPD = selection->GetPointData();
    return ids ? selectionPD->GetArray(this->IdChannelArray) : selectionPD->GetGlobalIds();
  }());
  ids = ids ? ids : vtkArrayDownCast<vtkIdTypeArray>(inputPD->GetGlobalIds());
  selectionIds = ids ? selectionIds : nullptr;

  std::unordered_set<vtkIdType> selectionIdsSet;

  if (selectionIds)
  {
    for (vtkIdType i = 0; i < selectionIds->GetNumberOfTuples(); ++i)
    {
      selectionIdsSet.emplace(selectionIds->GetValue(i));
    }
  }

  for (std::size_t i = 0; i < this->Internals->InputFieldArrays.size(); ++i)
  {
    this->Internals->InputFieldArrays[i] =
      inputPD->GetAbstractArray(this->Internals->TrailFieldNames[i].c_str());
  }

  //
  // Clear all trails' 'alive' flag so that
  // 'dead' ones can be removed at the end
  // Increment Trail marks the trail as alive
  //
  for (vtkTemporalPathLineFilterInternals::TrailIterator t = this->Internals->Trails.begin();
       t != this->Internals->Trails.end(); ++t)
  {
    t->second->alive = false;
    t->second->updated = false;
  }

  //
  // If the user provided a valid selection, we will use the IDs from it
  // to choose particles for building trails
  //
  if (selectionIds)
  {
    for (vtkIdType pointId = 0; pointId < input->GetNumberOfPoints(); pointId++)
    {
      vtkIdType pointGlobalId = ids->GetValue(pointId);
      if (selectionIdsSet.count(ids->GetValue(pointGlobalId)))
      {
        TrailPointer trail = this->GetTrail(pointGlobalId);
        IncrementTrail(trail, input, pointId);
      }
    }
  }
  else
  {
    for (vtkIdType pointId = 0; pointId < input->GetNumberOfPoints(); pointId += this->MaskPoints)
    {
      TrailPointer trail = this->GetTrail(pointId);
      IncrementTrail(trail, input, pointId);
    }
  }

  //
  // check the 'alive' flag and remove any that are dead
  //
  if (!this->KeepDeadTrails)
  {
    std::vector<vtkIdType> deadIds;
    deadIds.reserve(this->Internals->Trails.size());
    for (vtkTemporalPathLineFilterInternals::TrailIterator t = this->Internals->Trails.begin();
         t != this->Internals->Trails.end(); ++t)
    {
      if (!t->second->alive)
        deadIds.push_back(t->first);
    }
    for (std::vector<vtkIdType>::iterator it = deadIds.begin(); it != deadIds.end(); ++it)
    {
      this->Internals->Trails.erase(*it);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::PostExecute(
  vtkDataSet* input, vtkPolyData* pathLines, vtkPolyData* particles)
{
  //
  // Create the polydata outputs
  //
  this->LineCoordinates = vtkSmartPointer<vtkPoints>::New();
  this->VertexCoordinates = vtkSmartPointer<vtkPoints>::New();
  this->Vertices = vtkSmartPointer<vtkCellArray>::New();
  this->PolyLines = vtkSmartPointer<vtkCellArray>::New();
  this->TrailId = vtkSmartPointer<vtkFloatArray>::New();
  //
  size_t size = this->Internals->Trails.size();
  this->LineCoordinates->Allocate(static_cast<vtkIdType>(size * this->MaxTrackLength));
  this->Vertices->AllocateEstimate(static_cast<vtkIdType>(size), 1);
  this->VertexCoordinates->Allocate(static_cast<vtkIdType>(size));
  this->PolyLines->AllocateEstimate(static_cast<vtkIdType>(2 * size * this->MaxTrackLength), 1);
  this->TrailId->Allocate(static_cast<vtkIdType>(size * this->MaxTrackLength));
  this->TrailId->SetName("TrailId");

  vtkNew<vtkUnsignedIntArray> trackLength;
  trackLength->Allocate(static_cast<vtkIdType>(size * this->MaxTrackLength));
  trackLength->SetName("TrackLength");
  //
  std::vector<vtkIdType> TempIds(this->MaxTrackLength);
  vtkIdType VertexId = 0;

  vtkPointData* outPD = pathLines->GetPointData();
  std::vector<vtkAbstractArray*> outputFieldArrays;
  outputFieldArrays.resize(this->Internals->TrailFieldNames.size());
  for (size_t i = 0; i < this->Internals->TrailFieldNames.size(); i++)
  {
    outputFieldArrays[i] = outPD->GetAbstractArray(this->Internals->TrailFieldNames[i].c_str());
  }

  vtkPointData* particlesPD = particles->GetPointData();
  vtkPointData* inputPD = input->GetPointData();

  //
  for (vtkTemporalPathLineFilterInternals::TrailIterator t = this->Internals->Trails.begin();
       t != this->Internals->Trails.end(); ++t)
  {
    if (this->CheckAbort())
    {
      break;
    }
    TrailPointer tp = t->second;
    if (tp->length > 0)
    {
      for (unsigned int p = 0; p < tp->length; p++)
      {
        // build list of Ids that make line
        unsigned int index = (tp->firstpoint + p) % this->MaxTrackLength;
        double* coord = tp->Coords[index].x;
        TempIds[p] = this->LineCoordinates->InsertNextPoint(coord);
        for (size_t fieldId = 0; fieldId < outputFieldArrays.size(); fieldId++)
        {
          outputFieldArrays[fieldId]->InsertNextTuple(index, tp->Fields[fieldId]);
        }
        this->TrailId->InsertNextTuple1(static_cast<double>(tp->TrailId));
        trackLength->InsertNextValue(tp->length - p);

        // export the front end of the line as a vertex on Output1
        if (p == (tp->length - 1))
        {
          VertexId = this->VertexCoordinates->InsertNextPoint(coord);
          // copy all point scalars from input to new point data
          particlesPD->CopyData(inputPD, tp->FrontPointId, VertexId);
        }
      }
      if (tp->length > 1)
      {
        this->PolyLines->InsertNextCell(tp->length, TempIds.data());
      }
      this->Vertices->InsertNextCell(1, &VertexId);
    }
  }

  pathLines->SetPoints(this->LineCoordinates);
  pathLines->SetLines(this->PolyLines);
  outPD->AddArray(this->TrailId);
  outPD->AddArray(trackLength);
  outPD->SetActiveScalars(this->TrailId->GetName());

  // Vertex at Front of Trail
  particles->SetPoints(this->VertexCoordinates);
  particles->SetVerts(this->Vertices);
}

//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::Execute(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  using vtkSDDP = vtkStreamingDemandDrivenPipeline;

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  //
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* selection =
    selInfo ? vtkDataSet::SafeDownCast(selInfo->Get(vtkDataObject::DATA_OBJECT())) : nullptr;
  vtkPolyData* pathLines = vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* particles = vtkPolyData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));

  if (outInfo0->Get(vtkSDDP::INCOMPLETE_TIME_STEPS()) == vtkSDDP::INCOMPLETE_TIME_STEPS_RESET)
  {
    this->Flush();
  }

  if (this->Internals->TimeSteps.empty())
  {
    this->InitializeExecute(input, pathLines);
  }

  if (outInfo0->Has(vtkSDDP::UPDATE_TIME_STEP()))
  {
    this->AccumulateTrails(input, selection);

    this->Internals->TimeSteps.push_back(
      outInfo0->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));
  }
  pathLines->Initialize();
  particles->Initialize();
  this->PostExecute(input, pathLines, particles);
  if (!this->Internals->TimeSteps.empty())
  {
    vtkNew<vtkDoubleArray> tsteps;
    tsteps->SetName("time_steps");
    tsteps->SetNumberOfTuples(this->Internals->TimeSteps.size());
    double* tsteps_ptr = tsteps->GetPointer(0);
    std::copy(this->Internals->TimeSteps.begin(), this->Internals->TimeSteps.end(), tsteps_ptr);
    pathLines->GetFieldData()->AddArray(tsteps.GetPointer());
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalPathLineFilter::ExecuteStreaming(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  //
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* selection =
    selInfo ? vtkDataSet::SafeDownCast(selInfo->Get(vtkDataObject::DATA_OBJECT())) : nullptr;
  vtkPolyData* pathLines = vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* particles = vtkPolyData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));

  if (this->CurrentTimeIndex == 0)
  {
    this->Flush();
    this->InitializeExecute(input, pathLines);
  }

  this->AccumulateTrails(input, selection);

  this->CurrentTimeIndex++;

  if (this->CurrentTimeIndex < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) &&
    !this->CheckAbort())
  {
    // There is still more to do.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    // We are done.  Finish up.
    this->PostExecute(input, pathLines, particles);
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
  }

  return 1;
}
//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::Flush()
{
  this->LineCoordinates->Initialize();
  this->PolyLines->Initialize();
  this->Vertices->Initialize();
  this->TrailId->Initialize();
  this->Internals->Trails.clear();
  this->Internals->TimeStepSequence.clear();
  this->Internals->TrailFieldNames.clear();
  this->Internals->TimeSteps.clear();
  ParticleTrail::UniqueId = 0;
}
//------------------------------------------------------------------------------
void vtkTemporalPathLineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaskPoints: " << this->MaskPoints << "\n";
  os << indent << "MaxTrackLength: " << this->MaxTrackLength << "\n";
  os << indent << "IdChannelArray: " << (this->IdChannelArray ? this->IdChannelArray : "None")
     << "\n";
  os << indent << "MaxStepDistance: {" << this->MaxStepDistance[0] << ","
     << this->MaxStepDistance[1] << "," << this->MaxStepDistance[2] << "}\n";
  os << indent << "KeepDeadTrails: " << this->KeepDeadTrails << "\n";
}
VTK_ABI_NAMESPACE_END
