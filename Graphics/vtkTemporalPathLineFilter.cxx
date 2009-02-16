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
#include "vtkTemporalDataSet.h"
#include "vtkMergePoints.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
//
#include <vtkstd/vector>
#include <vtkstd/list>
#include <vtkstd/map>
#include <vtkstd/string>
#include <stdexcept>
#include <cmath>
//---------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkTemporalPathLineFilter, "1.6");
vtkStandardNewMacro(vtkTemporalPathLineFilter);
//----------------------------------------------------------------------------
//
typedef struct { double x[3]; }   Position;
typedef vtkstd::vector<Position>  CoordList;
typedef vtkstd::vector<vtkIdType> IdList;
typedef vtkstd::vector<float>     ScalarList;

class ParticleTrail : public vtkObject {
  public:
    static ParticleTrail *New();
    vtkTypeMacro(ParticleTrail, vtkObject);
    //
    unsigned int  firstpoint;
    unsigned int  lastpoint;
    unsigned int  length;
    long int      GlobalId;
    vtkIdType     Id;
    bool          alive;
    bool          updated;
    CoordList     Coords;
    ScalarList    Scalars;
    //
    ParticleTrail() { 
      this->Id = 0;
      this->GlobalId = ParticleTrail::UniqueId++; 
    }

    static long int UniqueId;
};
vtkStandardNewMacro(ParticleTrail);

long int ParticleTrail::UniqueId=0;

typedef vtkSmartPointer<ParticleTrail> TrailPointer;
typedef vtkstd::pair<vtkIdType, TrailPointer> TrailMapType;

class vtkTemporalPathLineFilterInternals : public vtkObject {
  public:
    static vtkTemporalPathLineFilterInternals *New();
    vtkTypeMacro(vtkTemporalPathLineFilterInternals, vtkObject);
    //
    typedef vtkstd::map<vtkIdType, TrailPointer>::iterator TrailIterator;
    vtkstd::map<vtkIdType, TrailPointer> Trails;
    //
    vtkstd::string                  LastIdArrayName;
    vtkstd::map<int, double>        TimeStepSequence;
};
vtkStandardNewMacro(vtkTemporalPathLineFilterInternals);

typedef vtkstd::map<int, double>::iterator TimeStepIterator;
//----------------------------------------------------------------------------
vtkTemporalPathLineFilter::vtkTemporalPathLineFilter()
{
  this->NumberOfTimeSteps    = 0;
  this->MaskPoints           = 200;
  this->MaxTrackLength       = 10;
  this->LastTrackLength      = 10;
  this->FirstTime            = 1;
  this->UsePointIndexForIds  = 1;
  this->IdChannelArray       = NULL;
  this->ScalarArray          = NULL;
  this->LatestTime           = 01E10;
  this->MaxStepDistance[0]   = 0.0001;
  this->MaxStepDistance[1]   = 0.0001;
  this->MaxStepDistance[2]   = 0.0001;
  this->MaxStepDistance[0]   = 1;
  this->MaxStepDistance[1]   = 1;
  this->MaxStepDistance[2]   = 1;
  this->KeepDeadTrails       = 0;
  this->ParticleCoordinates  = vtkSmartPointer<vtkPoints>::New();
  this->ParticlePolyLines    = vtkSmartPointer<vtkCellArray>::New();
  this->PointOpacity         = vtkSmartPointer<vtkFloatArray>::New();
  this->PointId              = vtkSmartPointer<vtkFloatArray>::New();
  this->PointScalars         = vtkSmartPointer<vtkFloatArray>::New();
  this->Internals            = vtkSmartPointer<vtkTemporalPathLineFilterInternals>::New();
  this->PointOpacity->SetName("Opacity");
  this->PointScalars->SetName("Scalars");
  this->SetNumberOfInputPorts(2);
}
//----------------------------------------------------------------------------
vtkTemporalPathLineFilter::~vtkTemporalPathLineFilter()
{
  if (this->IdChannelArray)
    {
    delete [] this->IdChannelArray;
    this->IdChannelArray = NULL;
    }
  if (this->ScalarArray)
    {
    delete [] this->ScalarArray;
    this->ScalarArray = NULL;
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
void vtkTemporalPathLineFilter::SetSelectionConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
//----------------------------------------------------------------------------
void vtkTemporalPathLineFilter::SetSelection(vtkDataSet *input)
{
  this->SetInput(1, input);
}
 //----------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

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
    vtkstd::pair<vtkTemporalPathLineFilterInternals::TrailIterator, bool> result =
      this->Internals->Trails.insert(TrailMapType(i,trail));
    if (!result.second) {
      throw vtkstd::runtime_error("Unexpected map error");
    }
    // new trail created, reserve memory now for efficiency
    trail = result.first->second;
    trail->Coords.assign(this->MaxTrackLength,Position());
    trail->Scalars.assign(this->MaxTrackLength,0);
    trail->lastpoint  = 0;
    trail->firstpoint = 0;
    trail->length     = 0;
    trail->alive      = 1;
    trail->updated    = 0;
    trail->Id         = i;
  } 
  else 
  {
    trail = t->second;
  }
  return trail;
}
//---------------------------------------------------------------------------
void vtkTemporalPathLineFilter::IncrementTrail(
  TrailPointer trail, vtkDataSet *input,
  vtkDataArray *inscalars, vtkIdType i) 
{
  // if for some reason, the particle ID appeared more than once
  // in the data, only update once - and use the point that is closest
  // to the last point on the trail
  if (trail->updated && trail->length>0) {
    unsigned int lastindex = (trail->lastpoint-2)%this->MaxTrackLength;
    unsigned int thisindex = (trail->lastpoint-1)%this->MaxTrackLength;
    double *coord0  = trail->Coords[lastindex].x;
    double *coord1a = trail->Coords[thisindex].x;
    double *coord1b = input->GetPoint(i);
    if (vtkMath::Distance2BetweenPoints(coord0, coord1b)<
        vtkMath::Distance2BetweenPoints(coord0, coord1a)) 
    {
      // new point is closer to previous than the one already present.
      // replace with this one.
      input->GetPoint(i,coord1a);
      if (inscalars) {
        trail->Scalars[trail->lastpoint] = inscalars->GetTuple(i)[0];
      }
    }
    // all indices have been updated already, so just exit 
    return;
  }
  //
  // Copy coord and scalar into trail
  //
  double *coord = trail->Coords[trail->lastpoint].x;
  input->GetPoint(i,coord);
  if (inscalars) {
    trail->Scalars[trail->lastpoint] = inscalars->GetTuple(i)[0];
  }
  // make sure the increment is within our allowed range
  double dist = 1.0;
  if (trail->length>0) {
    unsigned int lastindex = (trail->lastpoint-1)%this->MaxTrackLength;
    double *lastcoord = trail->Coords[lastindex].x;
    //
    double distx = fabs(lastcoord[0]-coord[0]);
    double disty = fabs(lastcoord[1]-coord[1]);
    double distz = fabs(lastcoord[2]-coord[2]);
    dist  = sqrt(dist);
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
  trail->alive = 1;
}
//---------------------------------------------------------------------------
int vtkTemporalPathLineFilter::RequestData(
  vtkInformation *vtkNotUsed(information),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  //
  vtkDataSet     *input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *selection = selInfo ? vtkDataSet::SafeDownCast(selInfo->Get(vtkDataObject::DATA_OBJECT())) : NULL;
  vtkPolyData   *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  //
  vtkInformation *doInfo = input->GetInformation();
  vtkstd::vector<double> timesteps;
  if (doInfo->Has(vtkDataObject::DATA_TIME_STEPS()))
  {
    int NumberOfInputTimeSteps = doInfo->Length(vtkDataObject::DATA_TIME_STEPS());
    timesteps.resize(NumberOfInputTimeSteps);
    doInfo->Get(vtkDataObject::DATA_TIME_STEPS(), &timesteps[0]);
  }
  else {
    return 0;
  }
  double CurrentTimeStep = timesteps[0];

  //
  // Ids
  //
  vtkDataArray *Ids = NULL;
  vtkDataArray *inscalars = NULL;
  if (!this->UsePointIndexForIds && this->IdChannelArray) {
    Ids = vtkDataArray::SafeDownCast(input->GetPointData()->GetArray(this->IdChannelArray));
  }
  if (this->ScalarArray) {
    inscalars = vtkDataArray::SafeDownCast(input->GetPointData()->GetArray(this->ScalarArray));
  }
  //
  // Get Ids if they are there and check they didn't change
  //
  if (!Ids) {
    this->Internals->LastIdArrayName = "";
  }
  else {
    if (this->Internals->LastIdArrayName!=vtkstd::string(this->IdChannelArray)) {
      this->FirstTime = 1;
      this->Internals->LastIdArrayName = this->IdChannelArray;
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
    vtkDataArray *selectionIds = vtkDataArray::SafeDownCast(selection->GetPointData()->GetArray(this->IdChannelArray));
    if (selectionIds) {
      vtkIdType N  = selectionIds->GetNumberOfTuples();
//      vtkIdType N1 = input->GetNumberOfPoints();
//      vtkIdType N2 = inscalars->GetNumberOfTuples();
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
        IncrementTrail(trail, input, inscalars, i); // i is current point index
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
      IncrementTrail(trail, input, inscalars, i);
    }
  }
  else {
    vtkIdType N = input->GetNumberOfPoints();
    for (vtkIdType i=0; i<N; i++) {
      vtkIdType ID = static_cast<vtkIdType>(Ids->GetTuple1(i));
      if (ID%this->MaskPoints==0) {
        TrailPointer trail = this->GetTrail(ID);    // ID is map key and particle ID
        IncrementTrail(trail, input, inscalars, i); // i is current point index
      }
    }
  }
  //
  // check the 'alive' flag and remove any that are dead
  //
  if (!this->KeepDeadTrails) {
    vtkstd::vector<vtkIdType> deadIds;
    deadIds.reserve(this->Internals->Trails.size());
    for (vtkTemporalPathLineFilterInternals::TrailIterator t=
      this->Internals->Trails.begin(); 
      t!=this->Internals->Trails.end(); t++) 
    {
      if (!t->second->alive) deadIds.push_back(t->first);
    }
    for (vtkstd::vector<vtkIdType>::iterator it=deadIds.begin(); it!=deadIds.end(); ++it) {
      this->Internals->Trails.erase(*it);
    }
  }

  //
  // Create the polydata output
  //
  this->ParticleCoordinates = vtkSmartPointer<vtkPoints>::New();
  this->PointScalars        = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticlePolyLines   = vtkSmartPointer<vtkCellArray>::New();
  this->PointId             = vtkSmartPointer<vtkFloatArray>::New();
  //
  size_t size = this->Internals->Trails.size();
  this->ParticleCoordinates->Allocate(size*this->MaxTrackLength);
  this->ParticlePolyLines->Allocate(2*size*this->MaxTrackLength);
  this->PointScalars->Allocate(size*this->MaxTrackLength);
  if (this->ScalarArray) {
    this->PointScalars->SetName(this->ScalarArray);
  }
  this->PointId->Allocate(size*this->MaxTrackLength);
  this->PointId->SetName("TrackId");
  //
  vtkstd::vector<vtkIdType> TempIds(this->MaxTrackLength);
  //
  for (vtkTemporalPathLineFilterInternals::TrailIterator t=
    this->Internals->Trails.begin(); 
    t!=this->Internals->Trails.end(); t++) 
  {
    TrailPointer tp = t->second;
    if (tp->length>0) {
      for (unsigned int p=0; p<tp->length; p++) {
        unsigned int index = (tp->firstpoint+p)%this->MaxTrackLength;
        double *coord = tp->Coords[index].x;
        TempIds[p] = this->ParticleCoordinates->InsertNextPoint(coord);
        if (inscalars) {
          float *scalar = &tp->Scalars[index];
          this->PointScalars->InsertNextTuple(scalar);
        }
        this->PointId->InsertNextTuple1(static_cast<double>(tp->Id));
      }
      this->ParticlePolyLines->InsertNextCell(tp->length,&TempIds[0]);
    }
  }

  output->GetPointData()->Initialize();
//  output->GetPointData()->AddArray(this->PointOpacity);
  output->GetPointData()->AddArray(this->PointId);
  output->GetPointData()->AddArray(this->PointScalars);
  if (inscalars) {
    output->GetPointData()->SetScalars(this->PointScalars);
  }
  else if (this->ScalarArray) {
//    if (strcmp(this->ScalarArray, "Opacity")==0) output->GetPointData()->SetScalars(this->PointOpacity);
//    else output->GetPointData()->SetScalars(this->PointId);
  }
  else {
    output->GetPointData()->SetScalars(this->PointId);
  }
  output->SetPoints(this->ParticleCoordinates);
  output->SetLines(this->ParticlePolyLines);

  return 1;
}
//---------------------------------------------------------------------------
void vtkTemporalPathLineFilter::Flush()
{
  this->ParticleCoordinates->Initialize();
  this->ParticlePolyLines->Initialize();
  this->PointOpacity->Initialize();
  this->PointId->Initialize();
  this->PointScalars->Initialize();
  this->Internals->Trails.clear();
  this->Internals->TimeStepSequence.clear();
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
  os << indent << "UsePointIndexForIds: " 
    << this->UsePointIndexForIds << "\n";
  os << indent << "IdChannelArray: " 
    << (this->IdChannelArray ? this->IdChannelArray : "None") << "\n";
  os << indent << "ScalarArray: " 
    << (this->ScalarArray ? this->ScalarArray : "None") << "\n";
  os << indent << "MaxStepDistance: {" 
     << this->MaxStepDistance[0] << ","
     << this->MaxStepDistance[1] << ","
     << this->MaxStepDistance[2] << "}\n";
  os << indent << "KeepDeadTrails: " 
    << this->KeepDeadTrails << "\n";
}
//---------------------------------------------------------------------------
