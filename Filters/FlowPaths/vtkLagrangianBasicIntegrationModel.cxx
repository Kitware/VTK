/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianBasicIntegrationModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianBasicIntegrationModel.h"

#include "vtkBilinearQuadIntersection.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkGenericCell.h"
#include "vtkIntArray.h"
#include "vtkLagrangianParticle.h"
#include "vtkLagrangianParticleTracker.h"
#include "vtkLagrangianThreadedData.h"
#include "vtkLongLongArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLocator.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

#include <cassert>
#include <mutex>
#include <set>
#include <sstream>
#include <vector>

#define USER_SURFACE_TYPE 100 // Minimal value for user defined surface type

//----------------------------------------------------------------------------
typedef std::vector<vtkSmartPointer<vtkAbstractCellLocator> > LocatorsTypeBase;
class vtkLocatorsType : public LocatorsTypeBase
{
};

typedef std::vector<vtkSmartPointer<vtkDataSet> > DataSetsTypeBase;
class vtkDataSetsType : public DataSetsTypeBase
{
};

typedef std::pair<unsigned int, vtkSmartPointer<vtkDataSet> > SurfaceItem;
typedef std::vector<SurfaceItem> SurfaceTypeBase;
class vtkSurfaceType : public SurfaceTypeBase
{
};

typedef std::pair<unsigned int, double> PassThroughItem;
typedef std::set<PassThroughItem> PassThroughSetType;

//----------------------------------------------------------------------------
vtkLagrangianBasicIntegrationModel::vtkLagrangianBasicIntegrationModel()
  : Locator(nullptr)
  , Tolerance(1.0e-8)
  , NonPlanarQuadSupport(false)
  , UseInitialIntegrationTime(false)
  , Tracker(nullptr)
{
  SurfaceArrayDescription surfaceTypeDescription;
  surfaceTypeDescription.nComp = 1;
  surfaceTypeDescription.type = VTK_INT;
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_MODEL, "ModelDefined"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_TERM, "Terminate"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_BOUNCE, "Bounce"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_BREAK, "BreakUp"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_PASS, "PassThrough"));
  this->SurfaceArrayDescriptions["SurfaceType"] = surfaceTypeDescription;

  this->SeedArrayNames->InsertNextValue("ParticleInitialVelocity");
  this->SeedArrayComps->InsertNextValue(3);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);
  this->SeedArrayNames->InsertNextValue("ParticleInitialIntegrationTime");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);

  this->Locators = new vtkLocatorsType;
  this->DataSets = new vtkDataSetsType;
  this->Surfaces = new vtkSurfaceType;
  this->SurfaceLocators = new vtkLocatorsType;

  // Using a vtkStaticCellLocator by default
  vtkNew<vtkStaticCellLocator> locator;
  this->SetLocator(locator);
  this->LocatorsBuilt = false;
}

//----------------------------------------------------------------------------
vtkLagrangianBasicIntegrationModel::~vtkLagrangianBasicIntegrationModel()
{
  this->ClearDataSets();
  this->ClearDataSets(true);
  this->SetLocator(nullptr);
  delete this->Locators;
  delete this->DataSets;
  delete this->Surfaces;
  delete this->SurfaceLocators;
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Locator)
  {
    os << indent << "Locator: " << endl;
    this->Locator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: " << this->Locator << endl;
  }
  os << indent << "Tolerance: " << this->Tolerance << endl;
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::SetTracker(vtkLagrangianParticleTracker* tracker)
{
  this->Tracker = tracker;
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::AddDataSet(
  vtkDataSet* dataset, bool surface, unsigned int surfaceFlatIndex)
{
  // Sanity check
  if (!dataset || dataset->GetNumberOfPoints() == 0 || dataset->GetNumberOfCells() == 0)
  {
    vtkErrorMacro(<< "Dataset is null or empty");
    return;
  }

  if (!this->Locator)
  {
    vtkErrorMacro(<< "Locator is null");
    return;
  }

  // There seems to be some kind of problem with the garbage collector
  // and the referencing of datasets and locators.
  // In order to avoid leaks we shallow copy the dataset.
  // This could be removed once this problem is fixed.
  vtkSmartPointer<vtkDataObject> dob;
  dob.TakeReference(vtkDataObjectTypes::NewDataObject(dataset->GetDataObjectType()));
  vtkDataSet* datasetCpy = vtkDataSet::SafeDownCast(dob);
  datasetCpy->ShallowCopy(dataset);

  // insert the dataset into DataSet vector
  if (surface)
  {
    this->Surfaces->push_back(std::make_pair(surfaceFlatIndex, datasetCpy));
  }
  else
  {
    this->DataSets->push_back(datasetCpy);
  }

  // insert a locator into Locators vector, non-null only for vtkPointSet
  vtkSmartPointer<vtkAbstractCellLocator> locator = nullptr;
  if (dataset->IsA("vtkPointSet"))
  {
    if (surface)
    {
      locator.TakeReference(vtkStaticCellLocator::New());
    }
    else
    {
      locator.TakeReference(this->Locator->NewInstance());
    }

    locator->SetDataSet(datasetCpy);
    locator->CacheCellBoundsOn();
    locator->AutomaticOn();
    locator->BuildLocator();
  }
  else
  {
    // for non-vtkPointSet vtkDataSet, we are using their internal locator
    // It is required to do a findCell call before the threaded code
    // so the locator is built first.
    double x[3];
    dataset->GetPoint(0, x);

    vtkNew<vtkGenericCell> cell;
    dataset->GetCell(0, cell);

    int subId;
    double pcoords[3];
    std::vector<double> weights(dataset->GetMaxCellSize());
    dataset->FindCell(x, nullptr, cell, 0, 0, subId, pcoords, weights.data());
  }

  // Add locator
  if (surface)
  {
    this->SurfaceLocators->push_back(locator);
  }
  else
  {
    this->Locators->push_back(locator);

    int size = dataset->GetMaxCellSize();
    if (size > static_cast<int>(this->SharedWeights.size()))
    {
      this->SharedWeights.resize(size);
    }
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::ClearDataSets(bool surface)
{
  if (surface)
  {
    this->Surfaces->clear();
    this->SurfaceLocators->clear();
  }
  else
  {
    this->DataSets->clear();
    this->Locators->clear();
    this->SharedWeights.clear();
  }
}

//----------------------------------------------------------------------------
int vtkLagrangianBasicIntegrationModel::FunctionValues(double* x, double* f, void* userData)
{
  // Sanity check
  if (this->DataSets->empty())
  {
    vtkErrorMacro(<< "Please add a dataset to the integration model before integrating.");
    return 0;
  }
  vtkLagrangianParticle* particle = static_cast<vtkLagrangianParticle*>(userData);
  if (!particle)
  {
    vtkErrorMacro(<< "Could not recover vtkLagrangianParticle");
    return 0;
  }
  vtkAbstractCellLocator* loc;
  vtkDataSet* ds;
  vtkIdType cellId;
  double* weights = particle->GetLastWeights();
  if (this->FindInLocators(x, particle, ds, cellId, loc, weights))
  {
    // Evaluate integration model velocity field with the found cell
    return this->FunctionValues(particle, ds, cellId, weights, x, f);
  }

  // Can't evaluate
  return 0;
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::SetLocator(vtkAbstractCellLocator* locator)
{
  if (this->Locator != locator)
  {
    vtkAbstractCellLocator* temp = this->Locator;
    this->Locator = locator;
    if (this->Locator)
    {
      this->Locator->Register(this);
    }
    if (temp)
    {
      temp->UnRegister(this);
    }
    this->Modified();
    this->LocatorsBuilt = false;
  }
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianBasicIntegrationModel::ComputeSurfaceInteraction(
  vtkLagrangianParticle* particle, std::queue<vtkLagrangianParticle*>& particles,
  unsigned int& surfaceFlatIndex, PassThroughParticlesType& passThroughParticles)
{
  vtkDataSet* surface = nullptr;
  double interFactor = 1.0;
  vtkIdType cellId = -1;
  int surfaceType = -1;
  PassThroughSetType passThroughInterSet;
  bool perforation;
  do
  {
    passThroughInterSet.clear();
    perforation = false;
    for (size_t iDs = 0; iDs < this->Surfaces->size(); iDs++)
    {
      vtkAbstractCellLocator* loc = (*this->SurfaceLocators)[iDs];
      vtkDataSet* tmpSurface = (*this->Surfaces)[iDs].second;
      vtkGenericCell* cell = particle->GetThreadedData()->GenericCell;
      vtkIdList* cellList = particle->GetThreadedData()->IdList;
      cellList->Reset();
      loc->FindCellsAlongLine(
        particle->GetPosition(), particle->GetNextPosition(), this->Tolerance, cellList);
      for (vtkIdType i = 0; i < cellList->GetNumberOfIds(); i++)
      {
        double tmpFactor;
        double tmpPoint[3];
        vtkIdType tmpCellId = cellList->GetId(i);
        tmpSurface->GetCell(tmpCellId, cell);
        if (this->IntersectWithLine(particle, cell->GetRepresentativeCell(),
              particle->GetPosition(), particle->GetNextPosition(), this->Tolerance, tmpFactor,
              tmpPoint) == 0)
        {
          // FindCellAlongsLines sometimes get false positives
          continue;
        }
        if (tmpFactor < interFactor)
        {
          // Recover surface type for this cell
          double surfaceTypeDbl;

          // "SurfaceType" is at index 2
          int surfaceIndex = 2;

          vtkIdType surfaceTupleId = tmpCellId;

          // When using field data surface type, tuple index is 0
          int ret = this->GetFlowOrSurfaceDataFieldAssociation(surfaceIndex);
          if (ret == -1)
          {
            vtkErrorMacro(<< "Surface Type is not correctly set in surface dataset");
            return nullptr;
          }
          if (ret == vtkDataObject::FIELD_ASSOCIATION_NONE)
          {
            surfaceTupleId = 0;
          }
          if (!this->GetFlowOrSurfaceData(
                particle, surfaceIndex, tmpSurface, surfaceTupleId, nullptr, &surfaceTypeDbl))
          {
            vtkErrorMacro(
              << "Surface Type is not set in surface dataset or"
                 " have incorrect number of components, cannot use surface interaction");
            return nullptr;
          }
          int tmpSurfaceType = static_cast<int>(surfaceTypeDbl);
          if (tmpSurfaceType == vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS)
          {
            // Pass Through Surface, store for later
            passThroughInterSet.insert(std::make_pair((*this->Surfaces)[iDs].first, tmpFactor));
          }
          else
          {
            if (tmpSurface == particle->GetLastSurfaceDataSet() &&
              tmpCellId == particle->GetLastSurfaceCellId())
            {
              perforation = this->CheckSurfacePerforation(particle, tmpSurface, tmpCellId);
              if (perforation)
              {
                break;
              }
              continue;
            }

            // Interacting surface
            interFactor = tmpFactor;
            surface = tmpSurface;
            surfaceFlatIndex = (*this->Surfaces)[iDs].first;
            surfaceType = tmpSurfaceType;
            cellId = tmpCellId;
          }
        }
      }
    }
  } while (perforation);

  PassThroughSetType::iterator it;
  for (it = passThroughInterSet.begin(); it != passThroughInterSet.end(); ++it)
  {
    PassThroughItem item = *it;
    // As one cas see in the test above, if a pass through surface intersects at the exact
    // same location than the point computed using the intersection factor,
    // we do not store the intersection.
    // pass through are considered non prioritary, and do not intersects
    // when at the exact the same place as the main intersection
    if (item.second < interFactor)
    {
      vtkLagrangianParticle* clone = particle->CloneParticle();
      clone->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_PASS);
      this->InterpolateNextParticleVariables(clone, item.second);
      passThroughParticles.push(std::make_pair(item.first, clone));
    }
  }

  // Store surface cache (even nullptr one)
  particle->SetLastSurfaceCell(surface, cellId);

  bool recordInteraction = false;
  vtkLagrangianParticle* interactionParticle = nullptr;
  if (cellId != -1)
  {
    // There is an actual interaction
    // Position next point onto surface
    this->InterpolateNextParticleVariables(particle, interFactor, true);
    interactionParticle = particle->CloneParticle();
    switch (surfaceType)
    {
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_TERM:
        recordInteraction = this->TerminateParticle(particle);
        break;
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BOUNCE:
        recordInteraction = this->BounceParticle(particle, surface, cellId);
        break;
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BREAK:
        recordInteraction = this->BreakParticle(particle, surface, cellId, particles);
        break;
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS:
        vtkErrorMacro(<< "Something went wrong with pass-through surface, "
                         "next results will be invalid.");
        return nullptr;
      default:
        if (surfaceType != SURFACE_TYPE_MODEL && surfaceType < USER_SURFACE_TYPE)
        {
          vtkWarningMacro("Please do not use user defined surface type under "
            << USER_SURFACE_TYPE
            << " as they may be used in the future by the Lagrangian Particle Tracker");
        }
        recordInteraction =
          this->InteractWithSurface(surfaceType, particle, surface, cellId, particles);
        break;
    }
    interactionParticle->SetInteraction(particle->GetInteraction());
  }
  if (!recordInteraction)
  {
    delete interactionParticle;
    interactionParticle = nullptr;
  }
  return interactionParticle;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::TerminateParticle(vtkLagrangianParticle* particle)
{
  particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_SURF_TERMINATED);
  particle->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_TERMINATED);
  return true;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::BounceParticle(
  vtkLagrangianParticle* particle, vtkDataSet* surface, vtkIdType cellId)
{
  particle->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_BOUNCE);

  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != nullptr);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Change velocity for bouncing and set interaction point
  double* nextVel = particle->GetNextVelocity();
  double dot = vtkMath::Dot(normal, nextVel);
  for (int i = 0; i < 3; i++)
  {
    nextVel[i] = nextVel[i] - 2 * dot * normal[i];
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::BreakParticle(vtkLagrangianParticle* particle,
  vtkDataSet* surface, vtkIdType cellId, std::queue<vtkLagrangianParticle*>& particles)
{
  // Terminate particle
  particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_SURF_BREAK);
  particle->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_BREAK);

  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != nullptr);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Create new particles
  vtkLagrangianParticle* particle1 = particle->NewParticle(this->Tracker->GetNewParticleId());
  vtkLagrangianParticle* particle2 = particle->NewParticle(this->Tracker->GetNewParticleId());

  // Compute bounce for each new particle
  double* nextVel = particle->GetNextVelocity();
  double* part1Vel = particle1->GetVelocity();
  double* part2Vel = particle2->GetVelocity();
  double dot = vtkMath::Dot(normal, nextVel);
  double cross[3];
  vtkMath::Cross(normal, nextVel, cross);
  double bounceNorm = vtkMath::Norm(nextVel);

  for (int i = 0; i < 3; i++)
  {
    part1Vel[i] = nextVel[i] - 2 * dot * normal[i] + cross[i];
    part2Vel[i] = nextVel[i] - 2 * dot * normal[i] - cross[i];
  }
  double part1Norm = vtkMath::Norm(part1Vel);
  double part2Norm = vtkMath::Norm(part2Vel);
  for (int i = 0; i < 3; i++)
  {
    if (part1Norm != 0.0)
    {
      part1Vel[i] = part1Vel[i] / part1Norm * bounceNorm;
    }
    if (part2Norm != 0.0)
    {
      part2Vel[i] = part2Vel[i] / part2Norm * bounceNorm;
    }
  }

  // push new particle in queue
  // Mutex Locked Area
  std::lock_guard<std::mutex> guard(this->ParticleQueueMutex);
  particles.push(particle1);
  particles.push(particle2);
  return true;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::InteractWithSurface(int vtkNotUsed(surfaceType),
  vtkLagrangianParticle* particle, vtkDataSet* vtkNotUsed(surface), vtkIdType vtkNotUsed(cellId),
  std::queue<vtkLagrangianParticle*>& vtkNotUsed(particles))
{
  return this->TerminateParticle(particle);
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::IntersectWithLine(vtkLagrangianParticle* particle,
  vtkCell* cell, double p1[3], double p2[3], double tol, double& t, double x[3])
{
  // Non planar quad support
  if (this->NonPlanarQuadSupport)
  {
    vtkQuad* quad = vtkQuad::SafeDownCast(cell);
    if (quad)
    {
      if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
      {
        // the 2 points are the same, no intersection
        return false;
      }

      // create 4 points and fill the bqi
      vtkPoints* points = quad->GetPoints();
      vtkBilinearQuadIntersection* bqi = particle->GetThreadedData()->BilinearQuadIntersection;
      points->GetPoint(0, bqi->GetP00Data());
      points->GetPoint(3, bqi->GetP01Data());
      points->GetPoint(1, bqi->GetP10Data());
      points->GetPoint(2, bqi->GetP11Data());

      // Create the ray
      vtkVector3d r(p1[0], p1[1], p1[2]);                         // origin of the ray
      vtkVector3d q(p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]); // a ray direction

      // the original t before q is normalised
      double tOrig = q.Norm();
      q.Normalize();

      vtkVector3d uv;                     // variables returned
      if (bqi->RayIntersection(r, q, uv)) // run intersection test
      {
        // we have an intersection
        t = uv.GetZ() / tOrig;
        if (t >= 0.0 && t <= 1.0)
        {
          // Recover intersection between p1 and p2
          vtkVector3d intersec = bqi->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
          x[0] = intersec.GetX();
          x[1] = intersec.GetY();
          x[2] = intersec.GetZ();
          return true;
        }
        else
        {
          // intersection outside of p1p2
          return false;
        }
      }
      else
      {
        // no intersection
        return false;
      }
    }
  }

  // Standard cell intersection
  double pcoords[3];
  int subId;
  int ret = cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
  if (ret != 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InterpolateNextParticleVariables(
  vtkLagrangianParticle* particle, double interpolationFactor, bool forceInside)
{
  if (forceInside)
  {
    // Reducing interpolationFactor to ensure we stay in domain
    double magnitude = particle->GetPositionVectorMagnitude();
    interpolationFactor *= (magnitude - this->Tolerance / interpolationFactor) / magnitude;
  }

  double* current = particle->GetEquationVariables();
  double* next = particle->GetNextEquationVariables();
  for (int i = 0; i < particle->GetNumberOfVariables(); i++)
  {
    next[i] = current[i] + (next[i] - current[i]) * interpolationFactor;
  }
  double& stepTime = particle->GetStepTimeRef();
  stepTime *= interpolationFactor;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::CheckSurfacePerforation(
  vtkLagrangianParticle* particle, vtkDataSet* surface, vtkIdType cellId)
{
  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != nullptr);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Recover particle vector
  double prevToCurr[3];
  double currToNext[3];
  for (int i = 0; i < 3; i++)
  {
    prevToCurr[i] = particle->GetPosition()[i] - particle->GetPrevPosition()[i];
    currToNext[i] = particle->GetNextPosition()[i] - particle->GetPosition()[i];
  }

  // Check directions
  double dot = vtkMath::Dot(normal, currToNext);
  double prevDot = vtkMath::Dot(normal, prevToCurr);
  double* nextVel = particle->GetNextVelocity();
  double velDot = vtkMath::Dot(normal, nextVel);
  if (dot == 0 || prevDot == 0 || prevDot * dot > 0)
  {
    // vector does not project on the same directions, perforation !
    for (int i = 0; i < 3; i++)
    {
      // Simple perforation management via symmetry
      currToNext[i] = currToNext[i] - 2 * dot * normal[i];
      particle->GetNextPosition()[i] = particle->GetPosition()[i] + currToNext[i];
      nextVel[i] = nextVel[i] - 2 * velDot * normal[i];
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  // Store the array metadata
  vtkLagrangianBasicIntegrationModel::ArrayVal vals;
  vals.val[0] = port;
  vals.val[1] = connection;
  vals.val[2] = fieldAssociation;
  ArrayMapVal array = ArrayMapVal(vals, name);
  this->InputArrays[idx] = array;
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::FindInLocators(double* x, vtkLagrangianParticle* particle)
{
  vtkIdType cellId;
  vtkDataSet* dataset;
  return this->FindInLocators(x, particle, dataset, cellId);
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::FindInLocators(
  double* x, vtkLagrangianParticle* particle, vtkDataSet*& dataset, vtkIdType& cellId)
{
  vtkAbstractCellLocator* loc;
  double* weights = this->SharedWeights.data();
  return this->FindInLocators(x, particle, dataset, cellId, loc, weights);
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::FindInLocators(double* x, vtkLagrangianParticle* particle,
  vtkDataSet*& dataset, vtkIdType& cellId, vtkAbstractCellLocator*& loc, double*& weights)
{
  // Sanity check
  if (this->DataSets->empty())
  {
    return false;
  }

  vtkGenericCell* cell = particle->GetThreadedData()->GenericCell;

  // Try the provided cache
  dataset = particle->GetLastDataSet();
  loc = particle->GetLastLocator();
  cellId = particle->GetLastCellId();
  double* lastPosition = particle->GetLastCellPosition();
  if (dataset)
  {
    // Check the last cell
    if (cellId != -1)
    {
      // Check if previous call was the same
      if (lastPosition[0] == x[0] && lastPosition[1] == x[1] && lastPosition[2] == x[2])
      {
        return true;
      }

      // If not, check if new position is in the same cell
      double pcoords[3];
      int subId;
      double dist2;
      dataset->GetCell(cellId, cell);
      if (cell->EvaluatePosition(x, nullptr, subId, pcoords, dist2, weights) == 1)
      {
        return true;
      }
    }

    // Not in provided cell cache, try the whole dataset
    cellId = this->FindInLocator(dataset, loc, x, cell, weights);
    if (cellId != -1)
    {
      particle->SetLastCell(loc, dataset, cellId, x);
      return true;
    }
  }

  // No cache or Cache miss, try other datasets
  vtkDataSet* lastDataSet = dataset;
  for (size_t iDs = 0; iDs < this->DataSets->size(); iDs++)
  {
    loc = (*this->Locators)[iDs];
    dataset = (*this->DataSets)[iDs];
    if (dataset != lastDataSet)
    {
      cellId = this->FindInLocator(dataset, loc, x, cell, weights);
      if (cellId != -1)
      {
        // Store the found cell for caching purpose
        particle->SetLastCell(loc, dataset, cellId, x);
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangianBasicIntegrationModel::FindInLocator(
  vtkDataSet* ds, vtkAbstractCellLocator* loc, double* x, vtkGenericCell* cell, double* weights)
{
  double pcoords[3];
  vtkIdType cellId;
  if (loc)
  {
    // Use locator to find the cell containing x
    cellId = loc->FindCell(x, this->Tolerance, cell, pcoords, weights);
  }
  else
  {
    // No locator, ds is vtkImageData or vtkRectilinearGrid,
    // which does not require any cellToUse when calling FindCell.
    int subId;
    cellId = ds->FindCell(x, nullptr, cell, 0, this->Tolerance, subId, pcoords, weights);
  }

  // Ignore Ghost cells
  if (cellId != -1 && ds->GetCellGhostArray() &&
    ds->GetCellGhostArray()->GetValue(cellId) & vtkDataSetAttributes::DUPLICATECELL)
  {
    return -1;
  }
  return cellId;
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkLagrangianBasicIntegrationModel::GetSeedArray(
  int idx, vtkLagrangianParticle* particle)
{
  return this->GetSeedArray(idx, particle->GetSeedData());
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkLagrangianBasicIntegrationModel::GetSeedArray(int idx, vtkPointData* pointData)
{
  // Check the provided index
  if (this->InputArrays.count(idx) == 0)
  {
    vtkErrorMacro(<< "No arrays at index:" << idx);
    return nullptr;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 1 for Source
  if (arrayIndexes.first.val[0] != 1)
  {
    vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a particle data array");
    return nullptr;
  }

  // Check connection, should be 0, no multiple connection
  if (arrayIndexes.first.val[1] != 0)
  {
    vtkErrorMacro(<< "This filter does not support multiple connections by port");
    return nullptr;
  }

  // Check field association
  switch (arrayIndexes.first.val[2])
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      // Recover array
      vtkAbstractArray* array = pointData->GetAbstractArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
      }
      return array;
    }
    default:
      vtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS are supported in particle data input");
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceDataNumberOfComponents(
  int idx, vtkDataSet* dataSet)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    vtkErrorMacro(<< "No arrays at index:" << idx);
    return -1;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input or 2 for Surface
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a flow or surface data array");
    return -1;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    vtkErrorMacro(<< "This filter does not support multiple connections by port");
    return -1;
  }

  // Check dataset is present
  if (!dataSet)
  {
    vtkErrorMacro(<< "Please provide a dataSet when calling this method "
                     "for input arrays coming from the flow or surface");
    return -1;
  }

  // Check fieldAssociation
  switch (arrayIndexes.first.val[2])
  {
    // Point needs interpolation
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      vtkDataArray* array = dataSet->GetPointData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return -1;
      }
      return array->GetNumberOfComponents();
    }
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    {
      vtkDataArray* array = dataSet->GetCellData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return -1;
      }
      return array->GetNumberOfComponents();
    }
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
    {
      vtkDataArray* array = dataSet->GetFieldData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return false;
      }
      return array->GetNumberOfComponents();
    }
    default:
      vtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS "
                    << "are supported in this method");
  }
  return -1;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceData(vtkLagrangianParticle* particle,
  int idx, vtkDataSet* dataSet, vtkIdType tupleId, double* weights, double* data)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    vtkErrorMacro(<< "No arrays at index:" << idx);
    return false;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input or 2 for Surface
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a flow or surface data array");
    return false;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    vtkErrorMacro(<< "This filter does not support multiple connections by port");
    return false;
  }

  // Check dataset is present
  if (!dataSet)
  {
    vtkErrorMacro(<< "Please provide a dataSet when calling this method "
                     "for input arrays coming from the flow or surface");
    return false;
  }

  // Check fieldAssociation
  switch (arrayIndexes.first.val[2])
  {
    // Point needs interpolation
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      if (!weights)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " is a PointData, yet no weights have been provided");
        return false;
      }
      vtkDataArray* array = dataSet->GetPointData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return false;
      }
      if (tupleId >= dataSet->GetNumberOfCells())
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " does not contain cellId :" << tupleId << " . Please check arrays.");
        return false;
      }

      // Manual interpolation of data at particle location
      vtkIdList* idList = particle->GetThreadedData()->IdList;
      dataSet->GetCellPoints(tupleId, idList);
      for (int j = 0; j < array->GetNumberOfComponents(); j++)
      {
        data[j] = 0;
        for (int i = 0; i < idList->GetNumberOfIds(); i++)
        {
          data[j] += weights[i] * array->GetComponent(idList->GetId(i), j);
        }
      }
      return true;
    }
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    {
      if (tupleId >= dataSet->GetNumberOfCells())
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " does not contain cellId :" << tupleId << " . Please check arrays.");
        return false;
      }
      vtkDataArray* array = dataSet->GetCellData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return false;
      }
      array->GetTuple(tupleId, data);
      return true;
    }
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
    {
      vtkDataArray* array = dataSet->GetFieldData()->GetArray(arrayIndexes.second.c_str());
      if (!array || tupleId >= array->GetNumberOfTuples())
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found in FieldData or does not contain"
                         "tuple index: "
                      << tupleId << " , please check arrays.");
        return false;
      }
      array->GetTuple(tupleId, data);
      return true;
    }
    default:
      vtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS "
                    << "are supported in this method");
  }
  return false;
}

//----------------------------------------------------------------------------
int vtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceDataFieldAssociation(int idx)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    vtkErrorMacro(<< "No arrays at index:" << idx);
    return -1;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    vtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a flow or surface data array");
    return -1;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    vtkErrorMacro(<< "This filter does not support multiple connections by port");
    return -1;
  }

  return arrayIndexes.first.val[2];
}

//---------------------------------------------------------------------------
vtkStringArray* vtkLagrangianBasicIntegrationModel::GetSeedArrayNames()
{
  return this->SeedArrayNames;
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSeedArrayComps()
{
  return this->SeedArrayComps;
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSeedArrayTypes()
{
  return this->SeedArrayTypes;
}

//---------------------------------------------------------------------------
vtkStringArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayNames()
{
  this->SurfaceArrayNames->SetNumberOfValues(0);
  for (auto it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayNames->InsertNextValue(it->first.c_str());
  }
  return this->SurfaceArrayNames;
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayComps()
{
  this->SurfaceArrayComps->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayComps->InsertNextValue(it->second.nComp);
  }
  return this->SurfaceArrayComps;
}

//---------------------------------------------------------------------------
int vtkLagrangianBasicIntegrationModel::GetWeightsSize()
{
  return static_cast<int>(this->SharedWeights.size());
}

//---------------------------------------------------------------------------
vtkStringArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayEnumValues()
{
  this->SurfaceArrayEnumValues->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayEnumValues->InsertVariantValue(
      this->SurfaceArrayEnumValues->GetNumberOfValues(), it->second.enumValues.size());
    for (size_t i = 0; i < it->second.enumValues.size(); i++)
    {
      this->SurfaceArrayEnumValues->InsertVariantValue(
        this->SurfaceArrayEnumValues->GetNumberOfValues(), it->second.enumValues[i].first);
      this->SurfaceArrayEnumValues->InsertNextValue(it->second.enumValues[i].second.c_str());
    }
  }
  return this->SurfaceArrayEnumValues;
}

//---------------------------------------------------------------------------
vtkDoubleArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayDefaultValues()
{
  this->SurfaceArrayDefaultValues->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    std::vector<double> defaultValues(it->second.nComp);
    for (size_t iDs = 0; iDs < this->Surfaces->size(); iDs++)
    {
      this->ComputeSurfaceDefaultValues(
        it->first.c_str(), (*this->Surfaces)[iDs].second, it->second.nComp, defaultValues.data());
      this->SurfaceArrayDefaultValues->InsertNextTuple(defaultValues.data());
    }
  }
  return this->SurfaceArrayDefaultValues;
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayTypes()
{
  this->SurfaceArrayTypes->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayTypes->InsertNextValue(it->second.type);
  }
  return this->SurfaceArrayTypes;
}

//---------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::ManualIntegration(
  vtkInitialValueProblemSolver* vtkNotUsed(integrator), double* vtkNotUsed(xcur),
  double* vtkNotUsed(xnext), double vtkNotUsed(t), double& vtkNotUsed(delT),
  double& vtkNotUsed(delTActual), double vtkNotUsed(minStep), double vtkNotUsed(maxStep),
  double vtkNotUsed(maxError), double vtkNotUsed(cellLength), double& vtkNotUsed(error),
  int& vtkNotUsed(integrationResult), vtkLagrangianParticle* vtkNotUsed(particle))
{
  return false;
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::ComputeSurfaceDefaultValues(
  const char* arrayName, vtkDataSet* vtkNotUsed(dataset), int nComponents, double* defaultValues)
{
  double defVal =
    (strcmp(arrayName, "SurfaceType") == 0) ? static_cast<double>(SURFACE_TYPE_TERM) : 0.0;
  std::fill(defaultValues, defaultValues + nComponents, defVal);
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InitializeParticleData(
  vtkFieldData* particleData, int maxTuple)
{
  vtkNew<vtkIntArray> particleStepNumArray;
  particleStepNumArray->SetName("StepNumber");
  particleStepNumArray->SetNumberOfComponents(1);
  particleStepNumArray->Allocate(maxTuple);
  particleData->AddArray(particleStepNumArray);

  vtkNew<vtkDoubleArray> particleVelArray;
  particleVelArray->SetName("ParticleVelocity");
  particleVelArray->SetNumberOfComponents(3);
  particleVelArray->Allocate(maxTuple * 3);
  particleData->AddArray(particleVelArray);

  vtkNew<vtkDoubleArray> particleIntegrationTimeArray;
  particleIntegrationTimeArray->SetName("IntegrationTime");
  particleIntegrationTimeArray->SetNumberOfComponents(1);
  particleIntegrationTimeArray->Allocate(maxTuple);
  particleData->AddArray(particleIntegrationTimeArray);
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InitializePathData(vtkFieldData* data)
{
  vtkNew<vtkLongLongArray> particleIdArray;
  particleIdArray->SetName("Id");
  particleIdArray->SetNumberOfComponents(1);
  data->AddArray(particleIdArray);

  vtkNew<vtkLongLongArray> particleParentIdArray;
  particleParentIdArray->SetName("ParentId");
  particleParentIdArray->SetNumberOfComponents(1);
  data->AddArray(particleParentIdArray);

  vtkNew<vtkLongLongArray> particleSeedIdArray;
  particleSeedIdArray->SetName("SeedId");
  particleSeedIdArray->SetNumberOfComponents(1);
  data->AddArray(particleSeedIdArray);

  vtkNew<vtkIntArray> particleTerminationArray;
  particleTerminationArray->SetName("Termination");
  particleTerminationArray->SetNumberOfComponents(1);
  data->AddArray(particleTerminationArray);
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InitializeInteractionData(vtkFieldData* data)
{
  vtkNew<vtkIntArray> interactionArray;
  interactionArray->SetName("Interaction");
  interactionArray->SetNumberOfComponents(1);
  data->AddArray(interactionArray);
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InsertParticleSeedData(
  vtkLagrangianParticle* particle, vtkFieldData* data)
{
  // Check for max number of tuples in arrays
  vtkIdType maxTuples = 0;
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
  {
    maxTuples = std::max(data->GetArray(i)->GetNumberOfTuples(), maxTuples);
  }

  // Copy seed data in not yet written array only
  // ie not yet at maxTuple
  vtkPointData* seedData = particle->GetSeedData();
  for (int i = 0; i < seedData->GetNumberOfArrays(); i++)
  {
    const char* name = seedData->GetArrayName(i);
    vtkDataArray* arr = data->GetArray(name);
    if (arr->GetNumberOfTuples() < maxTuples)
    {
      arr->InsertNextTuple(particle->GetSeedArrayTupleIndex(), seedData->GetArray(i));
    }
  }
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InsertPathData(
  vtkLagrangianParticle* particle, vtkFieldData* data)
{
  vtkLongLongArray::SafeDownCast(data->GetArray("Id"))->InsertNextValue(particle->GetId());
  vtkLongLongArray::SafeDownCast(data->GetArray("ParentId"))
    ->InsertNextValue(particle->GetParentId());
  vtkLongLongArray::SafeDownCast(data->GetArray("SeedId"))->InsertNextValue(particle->GetSeedId());
  vtkIntArray::SafeDownCast(data->GetArray("Termination"))
    ->InsertNextValue(particle->GetTermination());
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InsertInteractionData(
  vtkLagrangianParticle* particle, vtkFieldData* data)
{
  vtkIntArray::SafeDownCast(data->GetArray("Interaction"))
    ->InsertNextValue(particle->GetInteraction());
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InsertParticleData(
  vtkLagrangianParticle* particle, vtkFieldData* data, int stepEnum)
{
  switch (stepEnum)
  {
    case vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV:
      vtkIntArray::SafeDownCast(data->GetArray("StepNumber"))
        ->InsertNextValue(particle->GetNumberOfSteps() - 1);
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetPrevVelocity());
      data->GetArray("IntegrationTime")->InsertNextTuple1(particle->GetPrevIntegrationTime());
      break;
    case vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT:
      vtkIntArray::SafeDownCast(data->GetArray("StepNumber"))
        ->InsertNextValue(particle->GetNumberOfSteps());
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetVelocity());
      data->GetArray("IntegrationTime")->InsertNextTuple1(particle->GetIntegrationTime());
      break;
    case vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT:
      vtkIntArray::SafeDownCast(data->GetArray("StepNumber"))
        ->InsertNextValue(particle->GetNumberOfSteps() + 1);
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetNextVelocity());
      data->GetArray("IntegrationTime")
        ->InsertNextTuple1(particle->GetIntegrationTime() + particle->GetStepTimeRef());
      break;
    default:
      break;
  }
}
