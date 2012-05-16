/*=========================================================================

  Project:   vtkCSCS
  Module:    vtkTemporalPathLineFilter.cxx

  Copyright (c) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing this
  copyright notice appears on all copies of source code and an
  acknowledgment appears with any substantial usage of the code.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

=========================================================================*/
#include "vtkTemporalPathLineFilter.h"
#include "vtkPolyData.h"
#include "vtkPointSet.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMergePoints.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
//
#include <vector>
#include <list>
#include <map>
#include <string>
#include <stdexcept>
#include <cmath>
//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalPathLineFilter);
//----------------------------------------------------------------------------
//
typedef struct { double x[3]; }   Position;
typedef std::vector<Position>  CoordList;
typedef std::vector<vtkIdType> IdList;
typedef std::vector<vtkSmartPointer<vtkAbstractArray> > FieldList;

class ParticleTrail : public vtkObject {
  public:
    static ParticleTrail *New();
    vtkTypeMacro(ParticleTrail, vtkObject);
    //
    unsigned int  firstpoint;
    unsigned int  lastpoint;
    unsigned int  length;
    long int      GlobalId;
    vtkIdType     TrailId;
    vtkIdType     FrontPointId;
    bool          alive;
    bool          updated;
    CoordList     Coords;
    FieldList     Fields;
    //
    ParticleTrail() {
      this->TrailId      = 0;
      this->FrontPointId = 0;
      this->GlobalId     = ParticleTrail::UniqueId++;
    }

    static long int UniqueId;
};
vtkStandardNewMacro(ParticleTrail);

long int ParticleTrail::UniqueId=0;

typedef vtkSmartPointer<ParticleTrail> TrailPointer;
typedef std::pair<vtkIdType, TrailPointer> TrailMapType;

class vtkTemporalPathLineFilterInternals : public vtkObject {
  public:
    static vtkTemporalPathLineFilterInternals *New();
    vtkTypeMacro(vtkTemporalPathLineFilterInternals, vtkObject);
    //
    typedef std::map<vtkIdType, TrailPointer>::iterator TrailIterator;
    std::map<vtkIdType, TrailPointer> Trails;
    //
    std::string                  LastIdArrayName;
    std::map<int, double>        TimeStepSequence;
    //
    // This specifies the order of the arrays in the trails fields.  These are
    // valid in between calls to RequestData.
    std::vector<vtkStdString>    TrailFieldNames;
    // Input arrays corresponding to the entries in TrailFieldNames.  NULL arrays
    // indicate missing arrays.  This field is only valid during a call to
    // RequestData.
    std::vector<vtkAbstractArray*> InputFieldArrays;
};
vtkStandardNewMacro(vtkTemporalPathLineFilterInternals);

typedef std::map<int, double>::iterator TimeStepIterator;
//----------------------------------------------------------------------------
vtkTemporalPathLineFilter::vtkTemporalPathLineFilter()
{
  this->NumberOfTimeSteps    = 0;
  this->MaskPoints           = 200;
  this->MaxTrackLength       = 10;
  this->LastTrackLength      = 10;
  this->FirstTime            = 1;
  this->IdChannelArray       = NULL;
  this->LatestTime           = 01E10;
  this->MaxStepDistance[0]   = 0.0001;
  this->MaxStepDistance[1]   = 0.0001;
  this->MaxStepDistance[2]   = 0.0001;
  this->MaxStepDistance[0]   = 1;
  this->MaxStepDistance[1]   = 1;
  this->MaxStepDistance[2]   = 1;
  this->KeepDeadTrails       = 0;
  this->Vertices             = vtkSmartPointer<vtkCellArray>::New();
  this->PolyLines            = vtkSmartPointer<vtkCellArray>::New();
  this->LineCoordinates      = vtkSmartPointer<vtkPoints>::New();
  this->VertexCoordinates    = vtkSmartPointer<vtkPoints>::New();
  this->TrailId              = vtkSmartPointer<vtkFloatArray>::New();
  this->Internals            = vtkSmartPointer<vtkTemporalPathLineFilterInternals>::New();
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2); // Lines and points
}
//----------------------------------------------------------------------------
vtkTemporalPathLineFilter::~vtkTemporalPathLineFilter()
{
  if (this->IdChannelArray)
    {
    delete [] this->IdChannelArray;
    this->IdChannelArray = NULL;
    }
}
//----------------------------------------------------------------------------
int vtkTemporalPathLineFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port==0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  else if (port==1) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}
//----------------------------------------------------------------------------
int vtkTemporalPathLineFilter::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // Lines on 0, First point as Vertex Cell on 1
  if (port==0) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  else if (port==1) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  return 1;
}
//----------------------------------------------------------------------------
void vtkTemporalPathLineFilter::SetSelectionConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
//----------------------------------------------------------------------------
void vtkTemporalPathLineFilter::SetSelectionData(vtkDataSet *input)
{
  this->SetInputData(1, input);
}
 //----------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo0 = outputVector->GetInformationObject(0);
  outInfo0->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS())) {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  return 1;
}
//---------------------------------------------------------------------------
TrailPointer vtkTemporalPathLineFilter::GetTrail(vtkIdType i)
{
  TrailPointer trail;
  vtkTemporalPathLineFilterInternals::TrailIterator t = this->Internals->Trails.find(i);
  if (t==this->Internals->Trails.end()) {
    trail = vtkSmartPointer<ParticleTrail>::New();
    std::pair<vtkTemporalPathLineFilterInternals::TrailIterator, bool> result =
      this->Internals->Trails.insert(TrailMapType(i,trail));
    if (!result.second) {
      throw std::runtime_error("Unexpected map error");
    }
    // new trail created, reserve memory now for efficiency
    trail = result.first->second;
    trail->Coords.assign(this->MaxTrackLength,Position());
    trail->lastpoint  = 0;
    trail->firstpoint = 0;
    trail->length     = 0;
    trail->alive      = 1;
    trail->updated    = 0;
    trail->TrailId    = i;

    trail->Fields.assign(this->Internals->InputFieldArrays.size(), 0);
    for (size_t j = 0; j < this->Internals->InputFieldArrays.size(); j++)
      {
      vtkAbstractArray *inputArray = this->Internals->InputFieldArrays[j];
      if (!inputArray) continue;
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
//---------------------------------------------------------------------------
#define vtkTTFEQUAL(p,q) (p[0]==q[0] && p[1]==q[1] && p[2]==q[2])
//---------------------------------------------------------------------------
void vtkTemporalPathLineFilter::IncrementTrail(
  TrailPointer trail, vtkDataSet *input, vtkIdType id)
{
  //
  // After a clip operation, some points might not exist anymore
  // if the Id is out of bounds, kill the trail
  //
  if (id>=input->GetNumberOfPoints()) {
    trail->alive = 0;
    trail->updated = 1;
    return;
  }
  // if for some reason, two particles have the same ID, only update once
  // and use the point that is closest to the last point on the trail
  if (trail->updated && trail->length>0) {
    unsigned int lastindex = (trail->lastpoint-2)%this->MaxTrackLength;
    unsigned int thisindex = (trail->lastpoint-1)%this->MaxTrackLength;
    double *coord0  = trail->Coords[lastindex].x;
    double *coord1a = trail->Coords[thisindex].x;
    double *coord1b = input->GetPoint(id);
    if (vtkMath::Distance2BetweenPoints(coord0, coord1b)<
        vtkMath::Distance2BetweenPoints(coord0, coord1a))
    {
      // new point is closer to previous than the one already present.
      // replace with this one.
      input->GetPoint(id,coord1a);
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
  double *coord = trail->Coords[trail->lastpoint].x;
  input->GetPoint(id,coord);
  for (size_t fieldId = 0; fieldId < trail->Fields.size(); fieldId++)
    {
    trail->Fields[fieldId]->InsertTuple(
      trail->lastpoint, id, this->Internals->InputFieldArrays[fieldId]);
    }
  // make sure the increment is within our allowed range
  // and disallow zero distances
  double dist = 1.0;
  if (trail->length>0) {
    unsigned int lastindex = (this->MaxTrackLength + trail->lastpoint-1)%this->MaxTrackLength;
    double *lastcoord = trail->Coords[lastindex].x;
    //
    double distx = fabs(lastcoord[0]-coord[0]);
    double disty = fabs(lastcoord[1]-coord[1]);
    double distz = fabs(lastcoord[2]-coord[2]);
    dist = sqrt(distx*distx + disty*disty + distz*distz);
    //
    if (distx>this->MaxStepDistance[0] ||
        disty>this->MaxStepDistance[1] ||
        distz>this->MaxStepDistance[2])
    {
      trail->alive = 0;
      trail->updated = 1;
      return;
    }
  }
  //
  // Extend the trail and wrap accordingly around maxlength
  //
  if (dist>1E-9) {
    trail->lastpoint++;
    trail->length++;
    if (trail->length>=this->MaxTrackLength) {
      trail->lastpoint  = trail->lastpoint%this->MaxTrackLength;
      trail->firstpoint = trail->lastpoint;
      trail->length     = this->MaxTrackLength;
    }
    trail->updated = 1;
  }
  trail->FrontPointId = id;
  trail->alive = 1;
}
//---------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestData(
  vtkInformation *vtkNotUsed(information),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo   = inputVector[0]->GetInformationObject(0);
  vtkInformation *selInfo  = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation *outInfo1 = outputVector->GetInformationObject(1);
  //
  vtkDataSet      *input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet  *selection = selInfo ? vtkDataSet::SafeDownCast(selInfo->Get(vtkDataObject::DATA_OBJECT())) : NULL;
  vtkPolyData   *output0 = vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData   *output1 = vtkPolyData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData  *inputPointData = input->GetPointData();
  vtkPointData  *pointPointData = output1->GetPointData();
  //
  vtkInformation *doInfo = input->GetInformation();
  double timeStep(0);
  if (doInfo->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    timeStep =  doInfo->Get(vtkDataObject::DATA_TIME_STEP());
  }
  else {
    vtkErrorMacro(<<"The input dataset did not have a valid DATA_TIME_STEPS information key");
    return 0;
  }
  double CurrentTimeStep = timeStep;

  //
  // Ids
  //
  vtkDataArray *Ids = NULL;
  if (this->IdChannelArray) {
    Ids = input->GetPointData()->GetArray(this->IdChannelArray);
  }
  if (!Ids) {
    // Try loading the global ids.
    Ids = input->GetPointData()->GetGlobalIds();
  }
  // we don't always know how many trails there will be so guess 1000 for allocation of
  // point scalars on the second Particle output
  pointPointData->Initialize();
  pointPointData->CopyAllocate(inputPointData, 1000);

  //
  // Get Ids if they are there and check they didn't change
  //
  if (!Ids) {
    this->Internals->LastIdArrayName = "";
  }
  else if (this->IdChannelArray) {
    if (this->Internals->LastIdArrayName != this->IdChannelArray) {
      this->FirstTime = 1;
      this->Internals->LastIdArrayName = this->IdChannelArray;
    }
  }
  else {
    if (this->Internals->LastIdArrayName != "") {
      this->FirstTime = 1;
      this->Internals->LastIdArrayName = "";
    }
  }
  //
  // Check time and Track length
  //
  if (CurrentTimeStep<this->LatestTime) this->FirstTime = 1;
  if (this->LastTrackLength!=this->MaxTrackLength) this->FirstTime = 1;

  //
  // Reset everything if we are starting afresh
  //
  if (this->FirstTime) {
    this->Flush();
    this->FirstTime = 0;
  }
  this->LatestTime      = CurrentTimeStep;
  this->LastTrackLength = this->MaxTrackLength;

  // Set up output fields.
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output0->GetPointData();
  outPD->CopyAllocate(input->GetPointData(),
    input->GetNumberOfPoints()*this->MaxTrackLength/this->MaskPoints);
  if (   this->Internals->TrailFieldNames.empty()
      && this->Internals->Trails.empty() )
    {
    this->Internals->TrailFieldNames.resize(outPD->GetNumberOfArrays());
    for (int i = 0; i < outPD->GetNumberOfArrays(); i++)
      {
      this->Internals->TrailFieldNames[i] = outPD->GetArrayName(i);
      }
    }

  std::vector<vtkAbstractArray*> outputFieldArrays;
  this->Internals->InputFieldArrays.resize(
                                       this->Internals->TrailFieldNames.size());
  outputFieldArrays.resize(this->Internals->TrailFieldNames.size());
  for (size_t i = 0; i < this->Internals->TrailFieldNames.size(); i++)
    {
    this->Internals->InputFieldArrays[i]
      = inPD->GetAbstractArray(this->Internals->TrailFieldNames[i]);
    outputFieldArrays[i]
      = outPD->GetAbstractArray(this->Internals->TrailFieldNames[i]);
    }

  //
  // Clear all trails' 'alive' flag so that
  // 'dead' ones can be removed at the end
  // Increment Trail marks the trail as alive
  //
  for (vtkTemporalPathLineFilterInternals::TrailIterator t=
    this->Internals->Trails.begin();
    t!=this->Internals->Trails.end(); t++)
  {
    t->second->alive = 0;
    t->second->updated = 0;
  }

  //
  // If a selection input was provided, Build a list of selected Ids
  //
  this->UsingSelection = 0;
  if (selection && Ids) {
    this->UsingSelection = 1;
    this->SelectionIds.clear();
    vtkDataArray *selectionIds;
    if (this->IdChannelArray)
      {
      selectionIds = selection->GetPointData()->GetArray(this->IdChannelArray);
      }
    else
      {
      selectionIds = selection->GetPointData()->GetGlobalIds();
      }
    if (selectionIds) {
      vtkIdType N  = selectionIds->GetNumberOfTuples();
      for (vtkIdType i=0; i<N; i++) {
        vtkIdType ID = static_cast<vtkIdType>(selectionIds->GetTuple1(i));
        this->SelectionIds.insert(ID);
      }
    }
  }

  //
  // If the user provided a valid selection, we will use the IDs from it
  // to choose particles for building trails
  //
  if (this->UsingSelection) {
    vtkIdType N = input->GetNumberOfPoints();
    for (vtkIdType i=0; i<N; i++) {
      vtkIdType ID = static_cast<vtkIdType>(Ids->GetTuple1(i));
      if (this->SelectionIds.find(ID)!=this->SelectionIds.end()) {
        TrailPointer trail = this->GetTrail(ID);    // ID is map key and particle ID
        IncrementTrail(trail, input, i); // i is current point index
      }
    }
  }
  else if (!Ids) {
    //
    // If no Id array is specified or available, then we can only do every Nth
    // point to build up trails.
    //
    vtkIdType N = input->GetNumberOfPoints();
    for (vtkIdType i=0; i<N; i+=this->MaskPoints) {
      TrailPointer trail = this->GetTrail(i);
      IncrementTrail(trail, input, i);
    }
  }
  else {
    vtkIdType N = input->GetNumberOfPoints();
    for (vtkIdType i=0; i<N; i++) {
      vtkIdType ID = static_cast<vtkIdType>(Ids->GetTuple1(i));
      if (ID%this->MaskPoints==0) {
        TrailPointer trail = this->GetTrail(ID);    // ID is map key and particle ID
        IncrementTrail(trail, input, i); // i is current point index
      }
    }
  }
  //
  // check the 'alive' flag and remove any that are dead
  //
  if (!this->KeepDeadTrails) {
    std::vector<vtkIdType> deadIds;
    deadIds.reserve(this->Internals->Trails.size());
    for (vtkTemporalPathLineFilterInternals::TrailIterator t=
      this->Internals->Trails.begin();
      t!=this->Internals->Trails.end(); t++)
    {
      if (!t->second->alive) deadIds.push_back(t->first);
    }
    for (std::vector<vtkIdType>::iterator it=deadIds.begin(); it!=deadIds.end(); ++it) {
      this->Internals->Trails.erase(*it);
    }
  }

  //
  // Create the polydata outputs
  //
  this->LineCoordinates     = vtkSmartPointer<vtkPoints>::New();
  this->VertexCoordinates   = vtkSmartPointer<vtkPoints>::New();
  this->Vertices            = vtkSmartPointer<vtkCellArray>::New();
  this->PolyLines           = vtkSmartPointer<vtkCellArray>::New();
  this->TrailId             = vtkSmartPointer<vtkFloatArray>::New();
  //
  size_t size = this->Internals->Trails.size();
  this->LineCoordinates->Allocate(size*this->MaxTrackLength);
  this->Vertices->Allocate(size);
  this->VertexCoordinates->Allocate(size);
  this->PolyLines->Allocate(2*size*this->MaxTrackLength);
  this->TrailId->Allocate(size*this->MaxTrackLength);
  this->TrailId->SetName("TrailId");
  //
  std::vector<vtkIdType> TempIds(this->MaxTrackLength);
  vtkIdType VertexId=0;
  //
  for (vtkTemporalPathLineFilterInternals::TrailIterator t=
    this->Internals->Trails.begin();
    t!=this->Internals->Trails.end(); t++)
  {
    TrailPointer tp = t->second;
    if (tp->length>0) {
      for (unsigned int p=0; p<tp->length; p++) {
        // build list of Ids that make line
        unsigned int index = (tp->firstpoint+p)%this->MaxTrackLength;
        double *coord = tp->Coords[index].x;
        TempIds[p] = this->LineCoordinates->InsertNextPoint(coord);
        for (size_t fieldId = 0; fieldId < outputFieldArrays.size(); fieldId++)
          {
          outputFieldArrays[fieldId]->InsertNextTuple(index,
                                                      tp->Fields[fieldId]);
          }
        this->TrailId->InsertNextTuple1(static_cast<double>(tp->TrailId));

        // export the front end of the line as a vertex on Output1
        if (p==(tp->length-1)) {
          VertexId = this->VertexCoordinates->InsertNextPoint(coord);
          // copy all point scalars from input to new point data
          pointPointData->CopyData(inputPointData, tp->FrontPointId, VertexId);
        }
      }
      if (tp->length>1) {
        this->PolyLines->InsertNextCell(tp->length,&TempIds[0]);
      }
      this->Vertices->InsertNextCell(1, &VertexId);
    }
  }

  output0->SetPoints(this->LineCoordinates);
  output0->SetLines(this->PolyLines);
  outPD->AddArray(this->TrailId);
  outPD->SetScalars(this->TrailId);
  this->Internals->InputFieldArrays.resize(0);

  // Vertex at Front of Trail
  output1->SetPoints(this->VertexCoordinates);
  output1->SetVerts(this->Vertices);

  return 1;
}
//---------------------------------------------------------------------------
void vtkTemporalPathLineFilter::Flush()
{
  this->LineCoordinates->Initialize();
  this->PolyLines->Initialize();
  this->Vertices->Initialize();
  this->TrailId->Initialize();
  this->Internals->Trails.clear();
  this->Internals->TimeStepSequence.clear();
  this->Internals->TrailFieldNames.clear();
  this->FirstTime = 1;
  ParticleTrail::UniqueId = 0;
}
//---------------------------------------------------------------------------
void vtkTemporalPathLineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MaskPoints: "
    << this->MaskPoints << "\n";
  os << indent << "MaxTrackLength: "
    << this->MaxTrackLength << "\n";
  os << indent << "IdChannelArray: "
    << (this->IdChannelArray ? this->IdChannelArray : "None") << "\n";
  os << indent << "MaxStepDistance: {"
     << this->MaxStepDistance[0] << ","
     << this->MaxStepDistance[1] << ","
     << this->MaxStepDistance[2] << "}\n";
  os << indent << "KeepDeadTrails: "
    << this->KeepDeadTrails << "\n";
}
