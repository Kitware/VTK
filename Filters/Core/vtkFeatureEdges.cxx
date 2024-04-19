// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFeatureEdges.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFeatureEdges);

namespace
{
constexpr unsigned char CELL_NOT_VISIBLE =
  vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::DUPLICATECELL;
} // anonymous namespace

//------------------------------------------------------------------------------
// Construct object with feature angle = 30; all types of edges, except
// manifold edges, are extracted and colored.
vtkFeatureEdges::vtkFeatureEdges()
{
  this->FeatureAngle = 30.0;
  this->BoundaryEdges = true;
  this->FeatureEdges = true;
  this->NonManifoldEdges = true;
  this->ManifoldEdges = false;
  this->PassLines = false;
  this->RemoveGhostInterfaces = true;
  this->Coloring = true;
  this->Locator = nullptr;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//------------------------------------------------------------------------------
vtkFeatureEdges::~vtkFeatureEdges()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

VTK_ABI_NAMESPACE_END

//------------------------------------------------------------------------------
// Generate feature edges for mesh
VTK_ABI_NAMESPACE_BEGIN
int vtkFeatureEdges::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* inPts;
  vtkPoints* newPts;
  vtkFloatArray* newScalars = nullptr;
  vtkCellArray* newLines;
  vtkPolyData* Mesh;
  int i;
  vtkIdType j, numNei;
  vtkIdType numBEdges, numNonManifoldEdges, numFedges, numManifoldEdges;
  double scalar, n[3], x1[3], x2[3];
  double cosAngle = 0;
  vtkIdType lineIds[2];
  vtkIdType npts = 0;
  const vtkIdType* pts = nullptr;
  vtkCellArray *inPolys, *inStrips, *newPolys;
  vtkFloatArray* polyNormals = nullptr;
  vtkIdType numPts, numCells, numPolys, numStrips, numLines, nei;
  vtkIdList* neighbors;
  vtkIdType p1, p2, newId;
  vtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();

  outPD->CopyGlobalIdsOn();
  outCD->CopyGlobalIdsOn();

  unsigned char* ghosts = nullptr;
  vtkDebugMacro(<< "Executing feature edges");

  vtkDataArray* temp = nullptr;
  if (cd)
  {
    temp = cd->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    ghosts = static_cast<vtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  //  Check input
  //
  inPts = input->GetPoints();
  numCells = input->GetNumberOfCells();
  numPolys = input->GetNumberOfPolys();
  numStrips = input->GetNumberOfStrips();
  numLines = this->PassLines ? input->GetNumberOfLines() : 0;
  if ((numPts = input->GetNumberOfPoints()) < 1 || !inPts ||
    (numPolys < 1 && numStrips < 1 && numLines < 1))
  {
    vtkDebugMacro(<< "No input data!");
    return 1;
  }

  if (!this->BoundaryEdges && !this->NonManifoldEdges && !this->FeatureEdges &&
    !this->ManifoldEdges)
  {
    vtkDebugMacro(<< "All edge types turned off!");
  }

  // Build cell structure.  Might have to triangulate the strips.
  Mesh = vtkPolyData::New();
  Mesh->SetPoints(inPts);
  inPolys = input->GetPolys();
  vtkIdType numberOfNewPolys = numPolys;

  vtkNew<vtkIdList> polyIdToCellIdMap;
  vtkNew<vtkIdList> stripIdToCellIdMap;
  vtkNew<vtkIdList> lineIdToCellIdMap;
  std::map<vtkIdType, vtkIdType> decomposedStripIdToStripIdMap;

  // We need to remap cells if there are other cell arrays than polys
  if (numPolys != numCells)
  {
    polyIdToCellIdMap->SetNumberOfIds(numPolys);
    stripIdToCellIdMap->SetNumberOfIds(numStrips);
    lineIdToCellIdMap->SetNumberOfIds(numLines);
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      switch (input->GetCellType(cellId))
      {
        case VTK_EMPTY_CELL:
        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
          break;
        case VTK_TRIANGLE:
        case VTK_QUAD:
        case VTK_POLYGON:
          polyIdToCellIdMap->SetId(input->GetCellIdRelativeToCellArray(cellId), cellId);
          break;
        case VTK_TRIANGLE_STRIP:
          stripIdToCellIdMap->SetId(input->GetCellIdRelativeToCellArray(cellId), cellId);
          break;
        case VTK_LINE:
        case VTK_POLY_LINE:
          if (this->PassLines)
          {
            lineIdToCellIdMap->SetId(input->GetCellIdRelativeToCellArray(cellId), cellId);
          }
          break;
        default:
          vtkErrorMacro(<< "Wrong cell type in poly data input.");
          break;
      }
    }
  }

  if (numStrips > 0)
  {
    newPolys = vtkCellArray::New();
    if (numPolys > 0)
    {
      newPolys->DeepCopy(inPolys);
    }
    else
    {
      newPolys->AllocateEstimate(numStrips, 5);
    }
    inStrips = input->GetStrips();
    vtkIdType stripId = -1;
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
    {
      numberOfNewPolys += npts - 2;
      decomposedStripIdToStripIdMap.insert({ numberOfNewPolys, ++stripId });
      vtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
    }

    Mesh->SetPolys(newPolys);
    newPolys->Delete();
  }
  else
  {
    newPolys = inPolys;
    Mesh->SetPolys(newPolys);
  }
  Mesh->BuildLinks();

  // Allocate storage for lines/points (arbitrary allocation sizes)
  //
  newPts = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  newPts->Allocate(numPts / 10, numPts);
  newLines = vtkCellArray::New();
  newLines->AllocateEstimate(numPts / 20, 2);
  if (this->Coloring)
  {
    newScalars = vtkFloatArray::New();
    newScalars->SetName("Edge Types");
    newScalars->Allocate(numCells / 10, numCells);
  }

  outPD->CopyAllocate(pd, numPts);
  outCD->CopyAllocate(cd, numCells);

  // Get our locator for merging points
  //
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds());

  // Loop over all polygons generating boundary, non-manifold,
  // and feature edges
  //
  if (this->FeatureEdges)
  {
    polyNormals = vtkFloatArray::New();
    polyNormals->SetNumberOfComponents(3);
    polyNormals->Allocate(3 * newPolys->GetNumberOfCells());

    vtkIdType cellId;
    for (cellId = 0, newPolys->InitTraversal(); newPolys->GetNextCell(npts, pts); cellId++)
    {
      vtkPolygon::ComputeNormal(inPts, npts, pts, n);
      polyNormals->InsertTuple(cellId, n);
    }

    cosAngle = cos(vtkMath::RadiansFromDegrees(this->FeatureAngle));
  }

  neighbors = vtkIdList::New();
  neighbors->Allocate(VTK_CELL_SIZE);

  bool abort = false;
  vtkIdType progressInterval = newPolys->GetNumberOfCells() / 20 + 1;

  numBEdges = numNonManifoldEdges = numFedges = numManifoldEdges = 0;
  vtkIdType newCellId, cellId;

  // When filling output cells, to respect the same order as in vtkPolyData,
  // we need to fill lines, then polys, then strips.
  vtkIdType numOutLines = 0;
  if (numLines)
  {
    vtkCellArray* lines = input->GetLines();
    vtkIdType lineId = 0;
    for (lines->InitTraversal(); lines->GetNextCell(npts, pts); ++lineId)
    {
      cellId = lineIdToCellIdMap->GetId(lineId);
      if (ghosts && ghosts[cellId] & CELL_NOT_VISIBLE)
      {
        continue;
      }

      for (vtkIdType pointId = 0; pointId < npts - 1; ++pointId)
      {
        p1 = pts[pointId];
        p2 = pts[pointId + 1];

        Mesh->GetPoint(p1, x1);
        Mesh->GetPoint(p2, x2);

        if (this->Locator->InsertUniquePoint(x1, lineIds[0]))
        {
          outPD->CopyData(pd, p1, lineIds[0]);
        }

        if (this->Locator->InsertUniquePoint(x2, lineIds[1]))
        {
          outPD->CopyData(pd, p2, lineIds[1]);
        }

        newId = newLines->InsertNextCell(2, lineIds);
        outCD->CopyData(cd, cellId, newId);
        if (this->Coloring)
        {
          newScalars->InsertTuple1(newId, 0.888889);
        }
        ++numOutLines;
      }
    }
  }

  for (newCellId = 0, newPolys->InitTraversal(); newPolys->GetNextCell(npts, pts) && !abort;
       newCellId++)
  {
    if (!(newCellId % progressInterval)) // manage progress / early abort
    {
      this->UpdateProgress(static_cast<double>(newCellId) / numCells);
      abort = this->CheckAbort();
    }

    if (numPolys == numCells) // Input only has Polys
    {
      cellId = newCellId;
    }
    else if (newCellId < numPolys) // Input has mixed types, and we currently are on a Poly
    {
      cellId = polyIdToCellIdMap->GetId(newCellId);
    }
    else // Input has mixed types and we are dealing with triangle strips
    {
      auto it = decomposedStripIdToStripIdMap.lower_bound(newCellId + 1);
      cellId = stripIdToCellIdMap->GetId(it->second);
    }

    if (ghosts && ghosts[cellId] & CELL_NOT_VISIBLE)
    {
      continue;
    }

    // Used with non manifold edges when there are ghost cells in the input
    vtkNew<vtkIdList> edgesRemapping;

    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];

      Mesh->GetCellEdgeNeighbors(newCellId, p1, p2, neighbors);
      numNei = neighbors->GetNumberOfIds();

      vtkIdType numNeiWithoutGhosts = numNei;
      vtkIdType firstNeighbor = 0;
      if (ghosts)
      {
        for (j = 0; j < numNei; ++j)
        {
          vtkIdType neiId = neighbors->GetId(j);
          vtkIdType neighborCellIdInInput;
          if (numPolys == numCells)
          {
            neighborCellIdInInput = neiId;
          }
          else if (neiId < numPolys)
          {
            neighborCellIdInInput = polyIdToCellIdMap->GetId(neiId);
          }
          else
          {
            auto it = decomposedStripIdToStripIdMap.lower_bound(neiId + 1);
            neighborCellIdInInput = stripIdToCellIdMap->GetId(it->second);
          }
          if (ghosts[neighborCellIdInInput] & CELL_NOT_VISIBLE)
          {
            if (this->NonManifoldEdges)
            {
              edgesRemapping->InsertNextId(j);
            }
            if (j == firstNeighbor)
            {
              ++firstNeighbor;
            }
            --numNeiWithoutGhosts;
          }
        }
      }
      // Ignoring edges that are not visible
      if (numNeiWithoutGhosts != numNei && this->RemoveGhostInterfaces)
      {
        continue;
      }

      if (this->BoundaryEdges && numNeiWithoutGhosts < 1)
      {
        numBEdges++;
        scalar = 0.0;
      }

      else if (this->NonManifoldEdges && numNeiWithoutGhosts > 1)
      {
        // check to make sure that this edge hasn't been created before
        for (j = 0; j < (ghosts ? edgesRemapping->GetNumberOfIds() : numNei); j++)
        {
          if (neighbors->GetId(ghosts ? edgesRemapping->GetId(j) : j) < newCellId)
          {
            break;
          }
        }
        edgesRemapping->Reset();
        if (j >= numNeiWithoutGhosts)
        {
          numNonManifoldEdges++;
          scalar = 0.222222;
        }
        else
        {
          continue;
        }
      }
      else if (this->FeatureEdges && numNeiWithoutGhosts == 1 &&
        (nei = neighbors->GetId(firstNeighbor)) > newCellId)
      {
        double neiTuple[3];
        double cellTuple[3];
        polyNormals->GetTuple(nei, neiTuple);
        polyNormals->GetTuple(newCellId, cellTuple);
        if (vtkMath::Dot(neiTuple, cellTuple) <= cosAngle)
        {
          numFedges++;
          scalar = 0.444444;
        }
        else
        {
          continue;
        }
      }
      else if (this->ManifoldEdges && numNeiWithoutGhosts == 1 &&
        neighbors->GetId(firstNeighbor) > newCellId)
      {
        numManifoldEdges++;
        scalar = 0.666667;
      }
      else
      {
        continue;
      }

      // Add edge to output
      Mesh->GetPoint(p1, x1);
      Mesh->GetPoint(p2, x2);

      if (this->Locator->InsertUniquePoint(x1, lineIds[0]))
      {
        outPD->CopyData(pd, p1, lineIds[0]);
      }

      if (this->Locator->InsertUniquePoint(x2, lineIds[1]))
      {
        outPD->CopyData(pd, p2, lineIds[1]);
      }

      newId = newLines->InsertNextCell(2, lineIds);
      outCD->CopyData(cd, cellId, newId);
      if (this->Coloring)
      {
        newScalars->InsertTuple(newId, &scalar);
      }
    }
  }

  vtkDebugMacro(<< "Created " << numBEdges << " boundary edges, " << numNonManifoldEdges
                << " non-manifold edges, " << numFedges << " feature edges, " << numManifoldEdges
                << " manifold edges," << numOutLines << " lines.");
  (void)numBEdges;
  (void)numOutLines;

  //  Update ourselves.
  //
  if (this->FeatureEdges)
  {
    polyNormals->Delete();
  }

  Mesh->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  neighbors->Delete();

  output->SetLines(newLines);
  newLines->Delete();
  this->Locator->Initialize(); // release any extra memory
  if (this->Coloring)
  {
    int idx = outCD->AddArray(newScalars);
    outCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkFeatureEdges::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = vtkMergePoints::New();
  }
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkFeatureEdges::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkMTimeType vtkFeatureEdges::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//------------------------------------------------------------------------------
int vtkFeatureEdges::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int numPieces, ghostLevel;

  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel + 1);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkFeatureEdges::ExtractAllEdgeTypesOn()
{
  this->BoundaryEdgesOn();
  this->FeatureEdgesOn();
  this->NonManifoldEdgesOn();
  this->ManifoldEdgesOn();
  this->PassLinesOn();
}

//------------------------------------------------------------------------------
void vtkFeatureEdges::ExtractAllEdgeTypesOff()
{
  this->BoundaryEdgesOff();
  this->FeatureEdgesOff();
  this->NonManifoldEdgesOff();
  this->ManifoldEdgesOff();
  this->PassLinesOff();
}

//------------------------------------------------------------------------------
void vtkFeatureEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Boundary Edges: " << (this->BoundaryEdges ? "On\n" : "Off\n");
  os << indent << "Feature Edges: " << (this->FeatureEdges ? "On\n" : "Off\n");
  os << indent << "Non-Manifold Edges: " << (this->NonManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Manifold Edges: " << (this->ManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");
  os << indent << "Coloring: " << (this->Coloring ? "On\n" : "Off\n");

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
