/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTrimmedExtrusionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTrimmedExtrusionFilter.h"

#include "vtkAbstractCellLocator.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLocator.h"

#include <cmath>

vtkStandardNewMacro(vtkTrimmedExtrusionFilter);
vtkCxxSetObjectMacro(vtkTrimmedExtrusionFilter, Locator, vtkAbstractCellLocator);

namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm.
template <typename T>
struct ExtrudePoints
{
  vtkIdType NPts;
  T* InPoints;
  T* Points;
  unsigned char* Hits;
  vtkAbstractCellLocator* Locator;
  double ExtrusionDirection[3];
  double BoundsCenter[3];
  double BoundsLength;
  double Tol;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage eliminates lots of new/delete.
  vtkSMPThreadLocalObject<vtkGenericCell> Cell;

  ExtrudePoints(vtkIdType npts, T* inPts, T* points, unsigned char* hits,
    vtkAbstractCellLocator* loc, double ed[3], double bds[6])
    : NPts(npts)
    , InPoints(inPts)
    , Points(points)
    , Hits(hits)
    , Locator(loc)
  {
    this->ExtrusionDirection[0] = ed[0];
    this->ExtrusionDirection[1] = ed[1];
    this->ExtrusionDirection[2] = ed[2];
    vtkMath::Normalize(this->ExtrusionDirection);

    this->BoundsCenter[0] = (bds[0] + bds[1]) / 2.0;
    this->BoundsCenter[1] = (bds[2] + bds[3]) / 2.0;
    this->BoundsCenter[2] = (bds[4] + bds[5]) / 2.0;

    this->BoundsLength = sqrt((bds[1] - bds[0]) * (bds[1] - bds[0]) +
      (bds[3] - bds[2]) * (bds[3] - bds[2]) + (bds[5] - bds[4]) * (bds[5] - bds[4]));

    this->Tol = 0.000001 * this->BoundsLength;
  }

  void Initialize() {}

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const T* xi = this->InPoints + 3 * ptId;
    T* x = this->Points + 3 * ptId;
    T* xo = this->Points + 3 * (this->NPts + ptId);
    double len, p0[3], p1[3];
    const double* ed = this->ExtrusionDirection;
    double t, pc[3], xint[3];
    vtkIdType cellId;
    int subId;
    unsigned char* hits = this->Hits + ptId;
    vtkGenericCell*& cell = this->Cell.Local();

    for (; ptId < endPtId; ++ptId, xi += 3, x += 3, xo += 3, ++hits)
    {
      // Copy input points to output
      x[0] = xi[0];
      x[1] = xi[1];
      x[2] = xi[2];

      // Find a extrusion ray of appropriate length
      len = sqrt((x[0] - this->BoundsCenter[0]) * (x[0] - this->BoundsCenter[0]) +
              (x[1] - this->BoundsCenter[1]) * (x[1] - this->BoundsCenter[1]) +
              (x[2] - this->BoundsCenter[2]) * (x[2] - this->BoundsCenter[2])) +
        this->BoundsLength;

      p0[0] = x[0] - len * ed[0];
      p0[1] = x[1] - len * ed[1];
      p0[2] = x[2] - len * ed[2];
      p1[0] = x[0] + len * ed[0];
      p1[1] = x[1] + len * ed[1];
      p1[2] = x[2] + len * ed[2];

      // Intersect the surface and update whether a successful intersection hit or not
      *hits = this->Locator->IntersectWithLine(p0, p1, this->Tol, t, xint, pc, subId, cellId, cell);
      if (*hits > 0)
      {
        xo[0] = static_cast<T>(xint[0]);
        xo[1] = static_cast<T>(xint[1]);
        xo[2] = static_cast<T>(xint[2]);
      }
      else
      {
        xo[0] = xi[0];
        xo[1] = xi[1];
        xo[2] = xi[2];
      }
    }
  }

  void Reduce() {}

  static void Execute(vtkIdType numPts, T* inPts, T* points, unsigned char* hits,
    vtkAbstractCellLocator* loc, double ed[3], double bds[6])
  {
    ExtrudePoints extrude(numPts, inPts, points, hits, loc, ed, bds);
    vtkSMPTools::For(0, numPts, extrude);
  }
}; // ExtrudePoints

} // anonymous namespace

//-----------------------------------------------------------------------------
// Create object with normal extrusion type, capping on, scale factor=1.0,
// vector (0,0,1), and extrusion point (0,0,0).
vtkTrimmedExtrusionFilter::vtkTrimmedExtrusionFilter()
{
  this->SetNumberOfInputPorts(2);

  this->Capping = 1;

  this->ExtrusionDirection[0] = 0.0;
  this->ExtrusionDirection[1] = 0.0;
  this->ExtrusionDirection[2] = 1.0;

  this->ExtrusionStrategy = vtkTrimmedExtrusionFilter::BOUNDARY_EDGES;
  this->CappingStrategy = vtkTrimmedExtrusionFilter::MAXIMUM_DISTANCE;

  this->Locator = nullptr;
}

//-----------------------------------------------------------------------------
// Destructor
vtkTrimmedExtrusionFilter::~vtkTrimmedExtrusionFilter()
{
  this->SetLocator(nullptr);
}

//-----------------------------------------------------------------------------
int vtkTrimmedExtrusionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* in2Info = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDebugMacro(<< "Executing trimmed extrusion");

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* surface = vtkPolyData::SafeDownCast(in2Info->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input || !output)
  {
    vtkErrorMacro(<< "Missing input and/or output!");
    return 1;
  }

  if (!surface)
  {
    vtkErrorMacro(<< "Missing trim surface!");
    return 1;
  }
  if (surface->GetNumberOfPoints() < 1 || surface->GetNumberOfCells() < 1)
  {
    vtkErrorMacro(<< "Empty trim surface!");
    return 1;
  }

  // Initialize / check input
  //
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
  {
    vtkErrorMacro(<< "No data to extrude!");
    return 1;
  }

  if (vtkMath::Norm(this->ExtrusionDirection) <= 0.0)
  {
    vtkErrorMacro(<< "Must have nonzero extrusion direction");
    return 1;
  }

  // Generate the new points. Basically replicate points, except the new
  // point lies at the intersection of ray (in the extrusion direction)
  // against the trim surface. Also keep track if there are misses and use
  // this information later for capping (if necessary).
  vtkPointData* pd = input->GetPointData();
  vtkPointData* outputPD = output->GetPointData();
  outputPD->CopyNormalsOff();
  outputPD->CopyAllocate(pd, 2 * numPts);
  for (vtkIdType i = 0; i < numPts; ++i)
  {
    outputPD->CopyData(pd, i, i);
    outputPD->CopyData(pd, i, numPts + i);
  }

  vtkPoints* newPts = vtkPoints::New();
  newPts->SetDataType(input->GetPoints()->GetDataType());
  newPts->SetNumberOfPoints(2 * numPts);
  output->SetPoints(newPts);

  // Extrude the points by intersecting with the trim surface. Use a cell
  // locator to accelerate intersection operations.
  //
  if (this->Locator == nullptr)
  {
    this->Locator = vtkStaticCellLocator::New();
  }
  this->Locator->SetDataSet(surface);
  this->Locator->BuildLocator();
  double surfaceBds[6];
  surface->GetBounds(surfaceBds);

  // This performs the intersection of the extrusion ray. If a hit, the xyz
  // of the intersection point is used and hit[i] set to 1. If not, the xyz
  // is set to the xyz of the generating point and hit[i] remains 0. Later we
  // can use hit value to control the extrusion.
  unsigned char* hits = new unsigned char[numPts];
  void* inPtr = input->GetPoints()->GetVoidPointer(0);
  void* outPtr = newPts->GetVoidPointer(0);
  switch (newPts->GetDataType())
  {
    vtkTemplateMacro(ExtrudePoints<VTK_TT>::Execute(numPts, (VTK_TT*)inPtr, (VTK_TT*)outPtr, hits,
      this->Locator, this->ExtrusionDirection, surfaceBds));
  }

  // Prepare to generate the topology. Different topolgy is built depending
  // on extrusion strategy
  if (this->ExtrusionStrategy == vtkTrimmedExtrusionFilter::BOUNDARY_EDGES)
  {
    input->BuildLinks();
  }
  else // every edge is swept
  {
    input->BuildCells();
  }

  // Depending on the capping strategy, update the point coordinates. This has to be
  // done on a cell-by-cell basis. The adjustment is done in place.
  if (this->CappingStrategy != vtkTrimmedExtrusionFilter::INTERSECTION)
  {
    this->AdjustPoints(input, numPts, numCells, hits, newPts);
  }

  // Now generate the topology.
  this->ExtrudeEdges(input, output, numPts, numCells);

  // Cleanup, add the points to the output and clean up.
  newPts->Delete();
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
// Based on the capping strategy, adjust the point coordinates along the
// extrusion ray. This requires looping over all cells, grabbing the cap
// points, and then adjusting them as appropriate. Note this could be
// templated / sped up if necessary.
void vtkTrimmedExtrusionFilter::AdjustPoints(
  vtkPolyData* input, vtkIdType numPts, vtkIdType numCells, unsigned char* hits, vtkPoints* newPts)
{
  vtkIdType cellId;
  vtkIdType npts;
  const vtkIdType* ptIds;
  vtkIdType pId;
  vtkIdType i;
  double len, sum, min, max, p0[3], p1[3], ed[3];
  double p10[3], dir, minDir = 1.0, maxDir = 1.0, mDir = 1.0;
  vtkIdType numHits;

  ed[0] = this->ExtrusionDirection[0];
  ed[1] = this->ExtrusionDirection[1];
  ed[2] = this->ExtrusionDirection[2];
  vtkMath::Normalize(ed);

  for (cellId = 0; cellId < numCells; ++cellId)
  {
    input->GetCellPoints(cellId, npts, ptIds);

    // Gather information about cell
    min = VTK_FLOAT_MAX;
    max = VTK_FLOAT_MIN;
    sum = 0.0;
    numHits = 0;
    for (i = 0; i < npts; ++i)
    {
      pId = ptIds[i];
      if (hits[pId] > 0)
      {
        numHits++;
        newPts->GetPoint(pId, p0);
        newPts->GetPoint(numPts + pId, p1);

        vtkMath::Subtract(p1, p0, p10);
        dir = vtkMath::Dot(p10, ed);
        dir = (dir > 0.0 ? 1.0 : -1.0);

        len = sqrt(vtkMath::Distance2BetweenPoints(p0, p1));

        if (len < min)
        {
          min = len;
          minDir = dir;
        }
        if (len > max)
        {
          max = len;
          maxDir = dir;
        }
        sum += dir * len;
      }
    } // over primitive points

    // Adjust points if there was an intersection. Note that the extrusion
    // intersection is along the estrusion ray in either the negative or
    // positive direction.
    if (numHits > 0)
    {
      len = fabs(sum / static_cast<double>(numHits));
      if (this->CappingStrategy == vtkTrimmedExtrusionFilter::AVERAGE_DISTANCE)
      {
        mDir = 1.0;
      }
      else if (this->CappingStrategy == vtkTrimmedExtrusionFilter::MINIMUM_DISTANCE)
      {
        mDir = minDir;
      }
      else // if ( this->CappingStrategy == vtkTrimmedExtrusionFilter::MAXIMUM_DISTANCE )
      {
        mDir = maxDir;
      }

      for (i = 0; i < npts; ++i)
      {
        pId = ptIds[i];
        newPts->GetPoint(pId, p0);
        p1[0] = p0[0] + mDir * len * ed[0];
        p1[1] = p0[1] + mDir * len * ed[1];
        p1[2] = p0[2] + mDir * len * ed[2];
        newPts->SetPoint(numPts + pId, p1);
      }
    } // if valid polygon

  } // for all cells
}

//----------------------------------------------------------------------------
vtkIdType vtkTrimmedExtrusionFilter::GetNeighborCount(
  vtkPolyData* input, vtkIdType inCellId, vtkIdType p1, vtkIdType p2, vtkIdList* cellIds)
{
  if (this->ExtrusionStrategy == vtkTrimmedExtrusionFilter::BOUNDARY_EDGES)
  {
    input->GetCellEdgeNeighbors(inCellId, p1, p2, cellIds);
    return cellIds->GetNumberOfIds();
  }
  else // every edge is swept
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
// Somewhat modified from vtkLinearExtrusionFilter
void vtkTrimmedExtrusionFilter::ExtrudeEdges(
  vtkPolyData* input, vtkPolyData* output, vtkIdType numPts, vtkIdType numCells)
{
  vtkIdType inCellId, outCellId;
  int numEdges, dim;
  const vtkIdType* pts = nullptr;
  vtkIdType npts = 0;
  vtkIdType ptId, ncells, p1, p2;
  vtkIdType i, j;
  vtkCellArray *newLines = nullptr, *newPolys = nullptr, *newStrips = nullptr;
  vtkCell* edge;
  vtkIdList *cellIds, *cellPts;
  cellIds = vtkIdList::New();

  // Here is a big pain about ordering of cells. (Copy CellData)
  vtkIdList* lineIds;
  vtkIdList* polyIds;
  vtkIdList* stripIds;

  // Build cell data structure. Create a local copy
  vtkCellArray* inVerts = input->GetVerts();
  vtkCellArray* inLines = input->GetLines();
  vtkCellArray* inPolys = input->GetPolys();
  vtkCellArray* inStrips = input->GetStrips();

  // Allocate memory for output. We don't copy normals because surface geometry
  // is modified. Copy all points - this is the usual requirement and it makes
  // creation of skirt much easier.
  output->GetCellData()->CopyNormalsOff();
  output->GetCellData()->CopyAllocate(input->GetCellData(), 3 * input->GetNumberOfCells());

  if ((ncells = inVerts->GetNumberOfCells()) > 0)
  {
    newLines = vtkCellArray::New();
    newLines->AllocateEstimate(ncells, 2);
  }

  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells() / 10 +
    inStrips->GetNumberOfCells() / 10;
  ncells = (ncells < 100 ? 100 : ncells);

  newPolys = vtkCellArray::New();
  newPolys->AllocateCopy(inPolys);

  vtkIdType progressInterval = numPts / 10 + 1;
  int abort = 0;

  // We need the cellid to copy cell data. Skip points and lines.
  inCellId = outCellId = 0;
  if (input->GetVerts())
  {
    inCellId += input->GetVerts()->GetNumberOfCells();
  }
  if (input->GetLines())
  {
    inCellId += input->GetLines()->GetNumberOfCells();
  }
  // We need to keep track of input cell ids used to generate
  // output cells so that we can copy cell data at the end.
  // We do not know how many lines, polys and strips we will get
  // before hand.
  lineIds = vtkIdList::New();
  polyIds = vtkIdList::New();
  stripIds = vtkIdList::New();

  // If capping is on, copy 2D cells to output (plus create cap)
  //
  if (this->Capping)
  {
    if (inPolys->GetNumberOfCells() > 0)
    {
      for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
      {
        newPolys->InsertNextCell(npts, pts);
        polyIds->InsertNextId(inCellId);
        newPolys->InsertNextCell(npts);
        for (i = 0; i < npts; i++)
        {
          newPolys->InsertCellPoint(pts[i] + numPts);
        }
        polyIds->InsertNextId(inCellId);
        ++inCellId;
      }
    }

    if (inStrips->GetNumberOfCells() > 0)
    {
      newStrips = vtkCellArray::New();
      newStrips->AllocateEstimate(ncells, 4);
      for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
      {
        newStrips->InsertNextCell(npts, pts);
        stripIds->InsertNextId(inCellId);
        newStrips->InsertNextCell(npts);
        for (i = 0; i < npts; i++)
        {
          newStrips->InsertCellPoint(pts[i] + numPts);
        }
        stripIds->InsertNextId(inCellId);
        ++inCellId;
      }
    }
  }
  this->UpdateProgress(0.4);

  // Loop over all polygons and triangle strips searching for boundary edges.
  // If boundary edge found, extrude quad polygons. (Since the extrusion is
  // linear and guaranteed planar, triangle are not needed.)
  //
  progressInterval = numCells / 10 + 1;
  vtkGenericCell* cell = vtkGenericCell::New();
  for (inCellId = 0; inCellId < numCells && !abort; inCellId++)
  {
    if (!(inCellId % progressInterval)) // manage progress / early abort
    {
      this->UpdateProgress(0.4 + 0.6 * inCellId / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(inCellId, cell);
    cellPts = cell->GetPointIds();

    if ((dim = cell->GetCellDimension()) == 0) // create lines from points
    {
      for (i = 0; i < cellPts->GetNumberOfIds(); i++)
      {
        newLines->InsertNextCell(2);
        ptId = cellPts->GetId(i);
        newLines->InsertCellPoint(ptId);
        newLines->InsertCellPoint(ptId + numPts);
        lineIds->InsertNextId(inCellId);
      }
    }

    else if (dim == 1) // create strips from lines
    {
      for (i = 0; i < (cellPts->GetNumberOfIds() - 1); i++)
      {
        p1 = cellPts->GetId(i);
        p2 = cellPts->GetId(i + 1);
        newPolys->InsertNextCell(4);
        newPolys->InsertCellPoint(p1);
        newPolys->InsertCellPoint(p2);
        newPolys->InsertCellPoint(p2 + numPts);
        newPolys->InsertCellPoint(p1 + numPts);
        polyIds->InsertNextId(inCellId);
      }
    }

    else if (dim == 2) // create strips from boundary edges
    {
      numEdges = cell->GetNumberOfEdges();
      for (i = 0; i < numEdges; i++)
      {
        edge = cell->GetEdge(i);
        for (j = 0; j < (edge->GetNumberOfPoints() - 1); j++)
        {
          p1 = edge->PointIds->GetId(j);
          p2 = edge->PointIds->GetId(j + 1);

          // Check if this is a boundary edge
          if (this->GetNeighborCount(input, inCellId, p1, p2, cellIds) < 1)
          {
            newPolys->InsertNextCell(4);
            newPolys->InsertCellPoint(p1);
            newPolys->InsertCellPoint(p2);
            newPolys->InsertCellPoint(p2 + numPts);
            newPolys->InsertCellPoint(p1 + numPts);
            polyIds->InsertNextId(inCellId);
          }
        } // for each sub-edge
      }   // for each edge
    }     // for each polygon or triangle strip
  }       // for each cell
  cell->Delete();

  // Now Copy cell data.
  outCellId = 0;
  j = lineIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), lineIds->GetId(i), outCellId);
    ++outCellId;
  }
  j = polyIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), polyIds->GetId(i), outCellId);
    ++outCellId;
  }
  j = stripIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), stripIds->GetId(i), outCellId);
    ++outCellId;
  }
  lineIds->Delete();
  stripIds->Delete();
  polyIds->Delete();
  polyIds = nullptr;

  // Send data to output and release memory
  cellIds->Delete();

  if (newLines)
  {
    output->SetLines(newLines);
    newLines->Delete();
  }

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (newStrips)
  {
    output->SetStrips(newStrips);
    newStrips->Delete();
  }
}

//----------------------------------------------------------------------------
// Specify the trim surface
void vtkTrimmedExtrusionFilter::SetTrimSurfaceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkTrimmedExtrusionFilter::SetTrimSurfaceData(vtkPolyData* pd)
{
  this->SetInputData(1, pd);
}

//----------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
vtkPolyData* vtkTrimmedExtrusionFilter::GetTrimSurface()
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
vtkPolyData* vtkTrimmedExtrusionFilter::GetTrimSurface(vtkInformationVector* sourceInfo)
{
  vtkInformation* info = sourceInfo->GetInformationObject(1);
  if (!info)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int vtkTrimmedExtrusionFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkTrimmedExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Extrusion Direction: (" << this->ExtrusionDirection[0] << ", "
     << this->ExtrusionDirection[1] << ", " << this->ExtrusionDirection[2] << ")\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");

  os << indent << "Extrusion Strategy: " << this->ExtrusionStrategy << "\n";
  os << indent << "Capping Strategy: " << this->CappingStrategy << "\n";

  os << indent << "Locator: " << this->Locator << "\n";
}
