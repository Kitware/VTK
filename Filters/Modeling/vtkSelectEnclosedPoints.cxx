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

#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkExecutive.h"
#include "vtkFeatureEdges.h"
#include "vtkCellLocator.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkGarbageCollector.h"

vtkStandardNewMacro(vtkSelectEnclosedPoints);

//----------------------------------------------------------------------------
// Construct object.
vtkSelectEnclosedPoints::vtkSelectEnclosedPoints()
{
  this->SetNumberOfInputPorts(2);

  this->CheckSurface = 0;
  this->InsideOut = 0;
  this->Tolerance = 0.001;

  this->InsideOutsideArray = NULL;

  this->CellLocator = vtkCellLocator::New();
  this->CellIds = vtkIdList::New();
  this->Cell = vtkGenericCell::New();
}

//----------------------------------------------------------------------------
vtkSelectEnclosedPoints::~vtkSelectEnclosedPoints()
{
  if ( this->InsideOutsideArray )
    {
    this->InsideOutsideArray->Delete();
    }

  if ( this->CellLocator )
    {
    vtkCellLocator *loc = this->CellLocator;
    this->CellLocator = NULL;
    loc->Delete();
    }

  this->CellIds->Delete();
  this->Cell->Delete();
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *in2Info = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the two inputs and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *surface = vtkPolyData::SafeDownCast(
    in2Info->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro("Selecting enclosed points");

  // If requested, check that the surface is closed
  if ( this->CheckSurface && ! this->IsSurfaceClosed(surface) )
    {
    return 0;
    }

  // Initiailize search structures
  this->Initialize(surface);

  // Create array to mark inside/outside
  if ( this->InsideOutsideArray )
    {
    this->InsideOutsideArray->Delete();
    }
  this->InsideOutsideArray = vtkUnsignedCharArray::New();
  vtkUnsignedCharArray *marks = this->InsideOutsideArray;
  marks->SetName("SelectedPointsArray");

  // Loop over all input points determining inside/outside
  vtkIdType numPts = input->GetNumberOfPoints();
  marks->SetNumberOfValues(numPts);
  vtkIdType ptId;
  double x[3];

  int abort=0;
  vtkIdType progressInterval=numPts/20+1;
  for ( ptId=0; ptId < numPts && !abort; ptId++ )
    {
    if ( ! (ptId % progressInterval) ) //manage progress / early abort
      {
      this->UpdateProgress ((double)ptId / numPts);
      abort = this->GetAbortExecute();
      }

    input->GetPoint(ptId,x);

    if ( this->IsInsideSurface(x) )
      {
      marks->SetValue(ptId,(this->InsideOut?0:1));
      }
    else
      {
      marks->SetValue(ptId,(this->InsideOut?1:0));
      }
    }

  // Copy all the input geometry and data to the output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Add the new scalars array to the output.
  marks->SetName("SelectedPoints");
  output->GetPointData()->SetScalars(marks);

  // release memory
  this->Complete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::IsSurfaceClosed(vtkPolyData *surface)
{
  vtkPolyData *checker = vtkPolyData::New();
  checker->CopyStructure(surface);

  vtkFeatureEdges *features = vtkFeatureEdges::New();
  features->SetInputData(checker);
  features->BoundaryEdgesOn();
  features->NonManifoldEdgesOn();
  features->ManifoldEdgesOff();
  features->FeatureEdgesOff();
  features->Update();

  vtkIdType numCells = features->GetOutput()->GetNumberOfCells();
  features->Delete();
  checker->Delete();

  if ( numCells > 0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
void vtkSelectEnclosedPoints::Initialize(vtkPolyData *surface)
{
  if ( ! this->CellLocator )
    {
    this->CellLocator = vtkCellLocator::New();
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
  if ( !this->InsideOutsideArray ||
       this->InsideOutsideArray->GetValue(inputPtId) == 0 )
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


#define VTK_MAX_ITER 10    //Maximum iterations for ray-firing
#define VTK_VOTE_THRESHOLD 3
//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::IsInsideSurface(double x[3])
{
  // do a quick bounds check
  if ( x[0] < this->Bounds[0] || x[0] > this->Bounds[1] ||
       x[1] < this->Bounds[2] || x[1] > this->Bounds[3] ||
       x[2] < this->Bounds[4] || x[2] > this->Bounds[5])
    {
    return 0;
    }

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
  double tol = this->Tolerance*this->Length;

  for (deltaVotes = 0, iterNumber = 1;
       (iterNumber < VTK_MAX_ITER) && (abs(deltaVotes) < VTK_VOTE_THRESHOLD);
       iterNumber++)
    {
    //  Define a random ray to fire.
    rayMag = 0.0;
    while (rayMag == 0.0 )
      {
      for (i=0; i<3; i++)
        {
        ray[i] = vtkMath::Random(-1.0,1.0);
        }
      rayMag = vtkMath::Norm(ray);
      }

    // The ray must be appropriately sized wrt the bounding box. (It has to go
    // all the way through the bounding box.)
    for (i=0; i<3; i++)
      {
      xray[i] = x[i] + (this->Length/rayMag)*ray[i];
      }

    // Retrieve the candidate cells from the locator
    this->CellLocator->FindCellsAlongLine(x,xray,tol,this->CellIds);

    // Intersect the line with each of the candidate cells
    numInts = 0;
    numCells = this->CellIds->GetNumberOfIds();
    for ( idx=0; idx < numCells; idx++ )
      {
      this->Surface->GetCell(this->CellIds->GetId(idx), this->Cell);
      if ( this->Cell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId) )
        {
        numInts++;
        }
      } //for all candidate cells

    // Count the result
    if ( (numInts % 2) == 0)
      {
      --deltaVotes;
      }
    else
      {
      ++deltaVotes;
      }
    } //try another ray

  //   If the number of votes is positive, the point is inside
  //
  return ( deltaVotes < 0 ? 0 : 1 );
}

#undef VTK_MAX_ITER
#undef VTK_VOTE_THRESHOLD


//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkSelectEnclosedPoints::SetSurfaceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkSelectEnclosedPoints::SetSurfaceData(vtkPolyData *pd)
{
  this->SetInputData(1, pd);
}

//----------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
vtkPolyData *vtkSelectEnclosedPoints::GetSurface()
{
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSelectEnclosedPoints::GetSurface(vtkInformationVector *sourceInfo)
{
  vtkInformation *info = sourceInfo->GetInformationObject(1);
  if (!info)
    {
    return NULL;
    }
  return vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int vtkSelectEnclosedPoints::FillInputPortInformation(int port, vtkInformation *info)
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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Check Surface: "
     << (this->CheckSurface ? "On\n" : "Off\n");

  os << indent << "Inside Out: "
     << (this->InsideOut ? "On\n" : "Off\n");

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

