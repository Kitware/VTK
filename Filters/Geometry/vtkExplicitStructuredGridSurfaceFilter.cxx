/*=========================================================================
 Copyright (c) Kitware SAS 2014
 All rights reserved.
 More information http://www.kitware.fr
=========================================================================*/
#include "vtkExplicitStructuredGridSurfaceFilter.h"

#include "vtkCellData.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <vector>

vtkStandardNewMacro(vtkExplicitStructuredGridSurfaceFilter);

//----------------------------------------------------------------------------
vtkExplicitStructuredGridSurfaceFilter::vtkExplicitStructuredGridSurfaceFilter()
{
  this->PieceInvariant = 0;

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;

  this->OriginalCellIdsName = nullptr;
  this->SetOriginalCellIdsName("vtkOriginalCellIds");

  this->OriginalPointIdsName = nullptr;
  this->SetOriginalPointIdsName("vtkOriginalPointIds");
}

//----------------------------------------------------------------------------
vtkExplicitStructuredGridSurfaceFilter::~vtkExplicitStructuredGridSurfaceFilter()
{
  this->SetOriginalCellIdsName(nullptr);
  this->SetOriginalPointIdsName(nullptr);
}

// ----------------------------------------------------------------------------
int vtkExplicitStructuredGridSurfaceFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  inputVector[0]->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent);
  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridSurfaceFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int ghostLevels;
  ghostLevels = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  ghostLevels = std::max(ghostLevels, 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);

  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridSurfaceFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  // Get the input and output
  vtkExplicitStructuredGrid* input = vtkExplicitStructuredGrid::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  vtkIdType numCells = input->GetNumberOfCells();
  if (input->CheckAttributes() || numCells == 0)
  {
    return 1;
  }

  inputVector[0]->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent);

  return this->ExtractSurface(input, output);
}

static int hexaFaces[6][4] = {
  { 0, 4, 7, 3 },
  { 1, 2, 6, 5 },
  { 0, 1, 5, 4 },
  { 3, 7, 6, 2 },
  { 0, 3, 2, 1 },
  { 4, 5, 6, 7 },
};

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridSurfaceFilter::ExtractSurface(
  vtkExplicitStructuredGrid* input, vtkPolyData* output)
{
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (numCells == 0)
  {
    return 1;
  }

  vtkDebugMacro(<< "Executing explicit structured grid surface filter");

  vtkPointData* pd = input->GetPointData();
  vtkCellData* cd = input->GetCellData();
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();

  vtkNew<vtkIdTypeArray> originalCellIds;
  if (this->PassThroughCellIds)
  {
    originalCellIds->SetName(this->GetOriginalCellIdsName());
    originalCellIds->SetNumberOfComponents(1);
    originalCellIds->Allocate(numCells);
    outputCD->AddArray(originalCellIds.GetPointer());
  }
  vtkNew<vtkIdTypeArray> originalPointIds;
  if (this->PassThroughPointIds)
  {
    originalPointIds->SetName(this->GetOriginalPointIdsName());
    originalPointIds->SetNumberOfComponents(1);
    originalPointIds->Allocate(numPts);
    outputPD->AddArray(originalPointIds.GetPointer());
  }

  vtkNew<vtkIdList> cellIds;
  vtkUnsignedCharArray* connectivityFlags = nullptr;

  char* facesConnectivityFlagsArrayName = input->GetFacesConnectivityFlagsArrayName();
  if (facesConnectivityFlagsArrayName)
  {
    connectivityFlags = vtkUnsignedCharArray::SafeDownCast(
      input->GetCellData()->GetAbstractArray(facesConnectivityFlagsArrayName));
    if (!connectivityFlags)
    {
      vtkErrorMacro("Make sure Connectivity Flags have been computed before using this filter");
      return 0;
    }
  }

  vtkPoints* points = input->GetPoints();
  vtkCellArray* cells = input->GetCells();
  if (!points || !cells)
  {
    return 1;
  }

  // Allocate
  vtkNew<vtkPoints> newPts;
  newPts->SetDataType(points->GetDataType());
  newPts->Allocate(numPts, numPts / 2);
  output->SetPoints(newPts);

  vtkNew<vtkCellArray> newCells;
  newCells->AllocateEstimate(numCells / 10, 4);
  output->SetPolys(newCells);

  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd, numPts);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells);

  // Traverse cells to extract geometry
  int abort = 0;
  vtkIdType progressInterval = numCells / 20 + 1;
  vtkIdType npts;
  const vtkIdType* pts;
  cells->InitTraversal();
  std::vector<vtkIdType> pointIdVector(numPts, -1);

  bool mayBlank = input->HasAnyBlankCells();
  bool mayBlankOrGhost = mayBlank || input->HasAnyGhostCells();
  for (vtkIdType cellId = 0; cells->GetNextCell(npts, pts) && !abort; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      vtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    // Ignore blank cells and ghost cells
    if (mayBlankOrGhost && input->GetCellGhostArray()->GetValue(cellId) > 0)
    {
      continue;
    }

    vtkIdType neighbors[6];
    input->GetCellNeighbors(cellId, neighbors);
    unsigned char cflag = connectivityFlags->GetValue(cellId);
    // Traverse hexahedron cell faces
    for (int f = 0; f < 6; f++)
    {
      bool nonBlankNeighbor =
        !mayBlank || (neighbors[f] >= 0 && input->IsCellVisible(neighbors[f]));

      // Connected faces with non blank neighbor are skipped
      if (cflag & (1 << f) && nonBlankNeighbor)
      {
        continue;
      }

      vtkIdType facePtIds[4];
      for (int p = 0; p < 4; p++)
      {
        vtkIdType ptId = pts[hexaFaces[f][p]];
        vtkIdType pt = pointIdVector[ptId];
        if (pt == -1)
        {
          double x[3];
          points->GetPoint(ptId, x);
          pt = newPts->InsertNextPoint(x);
          pointIdVector[ptId] = pt;
          outputPD->CopyData(pd, ptId, pt);
          if (this->PassThroughPointIds)
          {
            originalPointIds->InsertValue(pt, ptId);
          }
        }
        facePtIds[p] = pt;
      }
      vtkIdType newCellId = newCells->InsertNextCell(4, facePtIds);
      outputCD->CopyData(cd, cellId, newCellId);
      if (this->PassThroughCellIds)
      {
        originalCellIds->InsertValue(newCellId, cellId);
      }

    } // for all faces
  }   // for all cells
  // free storage
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridSurfaceFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkExplicitStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PieceInvariant: " << this->PieceInvariant << endl;
  os << indent << "PassThroughCellIds: " << (this->PassThroughCellIds ? "On\n" : "Off\n");
  os << indent << "PassThroughPointIds: " << (this->PassThroughPointIds ? "On\n" : "Off\n");

  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;
}
