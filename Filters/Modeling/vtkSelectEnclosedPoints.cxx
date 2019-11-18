/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectEnclosedPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectEnclosedPoints.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkFeatureEdges.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRandomPool.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLocator.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkSelectEnclosedPoints);

//----------------------------------------------------------------------------
// Classes support threading. Each point can be processed separately, so the
// in/out containment check is threaded.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm. Thread on point type.
struct SelectInOutCheck
{
  vtkIdType NumPts;
  vtkDataSet* DataSet;
  vtkPolyData* Surface;
  double Bounds[6];
  double Length;
  double Tolerance;
  vtkStaticCellLocator* Locator;
  unsigned char* Hits;
  vtkSelectEnclosedPoints* Selector;
  vtkTypeBool InsideOut;
  vtkRandomPool* Sequence;
  vtkSMPThreadLocal<vtkIntersectionCounter> Counter;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage eliminates lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> CellIds;
  vtkSMPThreadLocalObject<vtkGenericCell> Cell;

  SelectInOutCheck(vtkIdType numPts, vtkDataSet* ds, vtkPolyData* surface, double bds[6],
    double tol, vtkStaticCellLocator* loc, unsigned char* hits, vtkSelectEnclosedPoints* sel,
    vtkTypeBool io)
    : NumPts(numPts)
    , DataSet(ds)
    , Surface(surface)
    , Tolerance(tol)
    , Locator(loc)
    , Hits(hits)
    , Selector(sel)
    , InsideOut(io)
  {
    this->Bounds[0] = bds[0];
    this->Bounds[1] = bds[1];
    this->Bounds[2] = bds[2];
    this->Bounds[3] = bds[3];
    this->Bounds[4] = bds[4];
    this->Bounds[5] = bds[5];
    this->Length = sqrt((bds[1] - bds[0]) * (bds[1] - bds[0]) +
      (bds[3] - bds[2]) * (bds[3] - bds[2]) + (bds[5] - bds[4]) * (bds[5] - bds[4]));

    // Precompute a sufficiently large enough random sequence
    this->Sequence = vtkRandomPool::New();
    this->Sequence->SetSize((numPts > 1500 ? numPts : 1500));
    this->Sequence->GeneratePool();
  }

  ~SelectInOutCheck() { this->Sequence->Delete(); }

  void Initialize()
  {
    vtkIdList*& cellIds = this->CellIds.Local();
    cellIds->Allocate(512);
    vtkIntersectionCounter& counter = this->Counter.Local();
    counter.SetTolerance(this->Tolerance);
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double x[3];
    unsigned char* hits = this->Hits + ptId;
    vtkGenericCell*& cell = this->Cell.Local();
    vtkIdList*& cellIds = this->CellIds.Local();
    vtkIntersectionCounter& counter = this->Counter.Local();

    for (; ptId < endPtId; ++ptId)
    {
      this->DataSet->GetPoint(ptId, x);

      if (this->Selector->IsInsideSurface(x, this->Surface, this->Bounds, this->Length,
            this->Tolerance, this->Locator, cellIds, cell, counter, this->Sequence, ptId))
      {
        *hits++ = (this->InsideOut ? 0 : 1);
      }
      else
      {
        *hits++ = (this->InsideOut ? 1 : 0);
      }
    }
  }

  void Reduce() {}

  static void Execute(vtkIdType numPts, vtkDataSet* ds, vtkPolyData* surface, double bds[6],
    double tol, vtkStaticCellLocator* loc, unsigned char* hits, vtkSelectEnclosedPoints* sel)
  {
    SelectInOutCheck inOut(numPts, ds, surface, bds, tol, loc, hits, sel, sel->GetInsideOut());
    vtkSMPTools::For(0, numPts, inOut);
  }
}; // SelectInOutCheck

} // anonymous namespace

//----------------------------------------------------------------------------
// Construct object.
vtkSelectEnclosedPoints::vtkSelectEnclosedPoints()
{
  this->SetNumberOfInputPorts(2);

  this->CheckSurface = false;
  this->InsideOut = 0;
  this->Tolerance = 0.0001;

  this->InsideOutsideArray = nullptr;

  // These are needed to support backward compatibility
  this->CellLocator = vtkStaticCellLocator::New();
  this->CellIds = vtkIdList::New();
  this->Cell = vtkGenericCell::New();
}

//----------------------------------------------------------------------------
vtkSelectEnclosedPoints::~vtkSelectEnclosedPoints()
{
  if (this->InsideOutsideArray)
  {
    this->InsideOutsideArray->Delete();
  }

  if (this->CellLocator)
  {
    vtkAbstractCellLocator* loc = this->CellLocator;
    this->CellLocator = nullptr;
    loc->Delete();
  }

  this->CellIds->Delete();
  this->Cell->Delete();
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* in2Info = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the two inputs and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* surface = vtkPolyData::SafeDownCast(in2Info->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro("Selecting enclosed points");

  // If requested, check that the surface is closed
  if (this->CheckSurface && !this->IsSurfaceClosed(surface))
  {
    return 0;
  }

  // Initiailize search structures
  this->Initialize(surface);

  // Create array to mark inside/outside
  if (this->InsideOutsideArray)
  {
    this->InsideOutsideArray->Delete();
  }
  this->InsideOutsideArray = vtkUnsignedCharArray::New();
  vtkUnsignedCharArray* hits = this->InsideOutsideArray;

  // Loop over all input points determining inside/outside
  vtkIdType numPts = input->GetNumberOfPoints();
  hits->SetNumberOfValues(numPts);
  unsigned char* hitsPtr = static_cast<unsigned char*>(hits->GetVoidPointer(0));

  // Process the points in parallel
  SelectInOutCheck::Execute(
    numPts, input, surface, this->Bounds, this->Tolerance, this->CellLocator, hitsPtr, this);

  // Copy all the input geometry and data to the output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Add the new scalars array to the output.
  hits->SetName("SelectedPoints");
  output->GetPointData()->SetScalars(hits);

  // release memory
  this->Complete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::IsSurfaceClosed(vtkPolyData* surface)
{
  vtkPolyData* checker = vtkPolyData::New();
  checker->CopyStructure(surface);

  vtkFeatureEdges* features = vtkFeatureEdges::New();
  features->SetInputData(checker);
  features->BoundaryEdgesOn();
  features->NonManifoldEdgesOn();
  features->ManifoldEdgesOff();
  features->FeatureEdgesOff();
  features->Update();

  vtkIdType numCells = features->GetOutput()->GetNumberOfCells();
  features->Delete();
  checker->Delete();

  if (numCells > 0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
void vtkSelectEnclosedPoints::Initialize(vtkPolyData* surface)
{
  if (!this->CellLocator)
  {
    this->CellLocator = vtkStaticCellLocator::New();
  }

  this->Surface = surface;
  surface->GetBounds(this->Bounds);
  this->Length = surface->GetLength();

  // Set up structures for acceleration ray casting
  this->CellLocator->SetDataSet(surface);
  this->CellLocator->BuildLocator();
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::IsInside(vtkIdType inputPtId)
{
  if (!this->InsideOutsideArray || this->InsideOutsideArray->GetValue(inputPtId) == 0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::IsInsideSurface(double x, double y, double z)
{
  double xyz[3];
  xyz[0] = x;
  xyz[1] = y;
  xyz[2] = z;
  return this->IsInsideSurface(xyz);
}

//----------------------------------------------------------------------------
// This is done to preserve backward compatibility. However it is not thread
// safe due to the use of the data member CellIds and Cell.
int vtkSelectEnclosedPoints::IsInsideSurface(double x[3])
{
  vtkIntersectionCounter counter(this->Tolerance, this->Length);

  return this->IsInsideSurface(x, this->Surface, this->Bounds, this->Length, this->Tolerance,
    this->CellLocator, this->CellIds, this->Cell, counter);
}

//----------------------------------------------------------------------------
// General method uses ray casting to determine in/out. Since this is a
// numerically delicate operation, we use a crude "statistical" method (based
// on voting) to provide a better answer. Plus there is a process to merge
// nearly conincident points along the intersection rays.
//
// This is a static method so it can be used by other filters; hence the
// many parameters used.
//
// Provision for reproducible threaded random number generation is made by
// supporting the precomputation of a random sequence (see vtkRandomPool).
//
#define VTK_MAX_ITER 10      // Maximum iterations for ray-firing
#define VTK_VOTE_THRESHOLD 2 // Vote margin for test

int vtkSelectEnclosedPoints::IsInsideSurface(double x[3], vtkPolyData* surface, double bds[6],
  double length, double tolerance, vtkAbstractCellLocator* locator, vtkIdList* cellIds,
  vtkGenericCell* genCell, vtkIntersectionCounter& counter, vtkRandomPool* seq, vtkIdType seqIdx)
{
  // do a quick inside bounds check against the surface bounds
  if (x[0] < bds[0] || x[0] > bds[1] || x[1] < bds[2] || x[1] > bds[3] || x[2] < bds[4] ||
    x[2] > bds[5])
  {
    return 0;
  }

  // Shortly we are going to start firing rays. It's important that the rays
  // are long enough to go from the test point all the way through the
  // enclosing surface. So compute a vector from the test point to the center
  // of the surface, and then add in the length (diagonal of bounding box) of
  // the surface.
  double offset[3], totalLength;
  offset[0] = x[0] - ((bds[0] + bds[1]) / 2.0);
  offset[1] = x[1] - ((bds[2] + bds[3]) / 2.0);
  offset[2] = x[2] - ((bds[4] + bds[5]) / 2.0);
  totalLength = length + vtkMath::Norm(offset);

  //  Perform in/out by shooting random rays. Multiple rays are fired
  //  to improve accuracy of the result.
  //
  //  The variable iterNumber counts the number of rays fired and is
  //  limited by the defined variable VTK_MAX_ITER.
  //
  //  The variable deltaVotes keeps track of the number of votes for
  //  "in" versus "out" of the surface.  When deltaVotes > 0, more votes
  //  have counted for "in" than "out".  When deltaVotes < 0, more votes
  //  have counted for "out" than "in".  When the delta_vote exceeds or
  //  equals the defined variable VTK_VOTE_THRESHOLD, then the
  //  appropriate "in" or "out" status is returned.
  //
  double rayMag, ray[3], xray[3], t, pcoords[3], xint[3];
  int i, numInts, iterNumber, deltaVotes, subId;
  vtkIdType idx, numCells;
  double tol = tolerance * length;

  for (deltaVotes = 0, iterNumber = 1;
       (iterNumber < VTK_MAX_ITER) && (abs(deltaVotes) < VTK_VOTE_THRESHOLD); iterNumber++)
  {
    //  Define a random ray to fire.
    rayMag = 0.0;
    while (rayMag == 0.0)
    {
      if (seq == nullptr) // in serial mode
      {
        ray[0] = vtkMath::Random(-1.0, 1.0);
        ray[1] = vtkMath::Random(-1.0, 1.0);
        ray[2] = vtkMath::Random(-1.0, 1.0);
      }
      else // threading, have to scale sequence -1<=x<=1
      {
        ray[0] = 2.0 * (0.5 - seq->GetValue(seqIdx++));
        ray[1] = 2.0 * (0.5 - seq->GetValue(seqIdx++));
        ray[2] = 2.0 * (0.5 - seq->GetValue(seqIdx++));
      }
      rayMag = vtkMath::Norm(ray);
    }

    // The ray must be appropriately sized wrt the bounding box. (It has to
    // go all the way through the bounding box. Remember though that an
    // "inside bounds" check was done previously so diagonal length should
    // be long enough.)
    for (i = 0; i < 3; i++)
    {
      xray[i] = x[i] + 2.0 * totalLength * (ray[i] / rayMag);
    }

    // Retrieve the candidate cells from the locator to limit the
    // intersections to be attempted.
    locator->FindCellsAlongLine(x, xray, tol, cellIds);
    numCells = cellIds->GetNumberOfIds();

    counter.Reset();
    for (idx = 0; idx < numCells; idx++)
    {
      surface->GetCell(cellIds->GetId(idx), genCell);
      if (genCell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId))
      {
        counter.AddIntersection(t);
      }
    } // for all candidate cells along this ray

    numInts = counter.CountIntersections();

    if ((numInts % 2) == 0) // if outside
    {
      --deltaVotes;
    }
    else // if inside
    {
      ++deltaVotes;
    }
  } // try another ray

  //   If the number of votes is positive, the point is inside
  //
  return (deltaVotes < 0 ? 0 : 1);
}

#undef VTK_MAX_ITER
#undef VTK_VOTE_THRESHOLD

//----------------------------------------------------------------------------
// Specify the second enclosing surface input via a connection
void vtkSelectEnclosedPoints::SetSurfaceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Specify the second enclosing surface input data
void vtkSelectEnclosedPoints::SetSurfaceData(vtkPolyData* pd)
{
  this->SetInputData(1, pd);
}

//----------------------------------------------------------------------------
// Return the enclosing surface
vtkPolyData* vtkSelectEnclosedPoints::GetSurface()
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSelectEnclosedPoints::GetSurface(vtkInformationVector* sourceInfo)
{
  vtkInformation* info = sourceInfo->GetInformationObject(1);
  if (!info)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSelectEnclosedPoints::Complete()
{
  this->CellLocator->FreeSearchStructure();
}

//----------------------------------------------------------------------------
void vtkSelectEnclosedPoints::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->CellLocator, "CellLocator");
}

//----------------------------------------------------------------------------
void vtkSelectEnclosedPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Check Surface: " << (this->CheckSurface ? "On\n" : "Off\n");

  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
