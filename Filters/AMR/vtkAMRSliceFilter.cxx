/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRSliceFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRSliceFilter.h"
#include "vtkAMRBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkParallelAMRUtilities.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cassert>
#include <sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAMRSliceFilter);

//-----------------------------------------------------------------------------
vtkAMRSliceFilter::vtkAMRSliceFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->OffsetFromOrigin = 0.0;
  this->Normal = X_NORMAL;
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->MaxResolution = 1;
}

//-----------------------------------------------------------------------------
void vtkAMRSliceFilter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkAMRSliceFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRSliceFilter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkOverlappingAMR");
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkAMRSliceFilter::IsAMRData2D(vtkOverlappingAMR* input)
{
  assert("pre: Input AMR dataset is nullptr" && (input != nullptr));

  if (input->GetGridDescription() != VTK_XYZ_GRID)
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
vtkPlane* vtkAMRSliceFilter::GetCutPlane(vtkOverlappingAMR* inp)
{
  assert("pre: AMR dataset should not be nullptr" && (inp != nullptr));

  vtkTimerLog::MarkStartEvent("AMRSlice::GetCutPlane");

  vtkPlane* pl = vtkPlane::New();

  // Get global bounds
  double minBounds[3];
  double maxBounds[3];
  inp->GetMin(minBounds);
  inp->GetMax(maxBounds);

  double porigin[3];
  for (int i = 0; i < 3; ++i)
    porigin[i] = minBounds[i];

  auto offset = vtkMath::ClampValue(
    this->OffsetFromOrigin, 0.0, maxBounds[this->Normal / 2] - minBounds[this->Normal / 2]);

  switch (this->Normal)
  {
    case X_NORMAL:
      pl->SetNormal(1.0, 0.0, 0.0);
      porigin[0] += offset;
      break;
    case Y_NORMAL:
      pl->SetNormal(0.0, 1.0, 0.0);
      porigin[1] += offset;
      break;
    case Z_NORMAL:
      pl->SetNormal(0.0, 0.0, 1.0);
      porigin[2] += offset;
      break;
    default:
      vtkErrorMacro("Undefined plane normal");
  }
  pl->SetOrigin(porigin);

  vtkTimerLog::MarkEndEvent("AMRSlice::GetCutPlane");
  return (pl);
}

//-----------------------------------------------------------------------------
vtkUniformGrid* vtkAMRSliceFilter::GetSlice(
  double origin[3], int* dims, double* gorigin, double* spacing)
{
  //  vtkTimerLog::MarkStartEvent( "AMRSlice::GetSliceForBlock" );

  vtkUniformGrid* slice = vtkUniformGrid::New();

  // Storage for dimensions of the 2-D slice grid & its origin
  int sliceDims[3];
  double sliceOrigin[3];

  switch (this->Normal)
  {
    case X_NORMAL: // -- YZ plane
      sliceDims[0] = 1;
      sliceDims[1] = dims[1];
      sliceDims[2] = dims[2];

      sliceOrigin[0] = origin[0];
      sliceOrigin[1] = gorigin[1];
      sliceOrigin[2] = gorigin[2];

      slice->SetOrigin(sliceOrigin);
      slice->SetDimensions(sliceDims);
      slice->SetSpacing(spacing);
      assert(slice->GetGridDescription() == VTK_YZ_PLANE);
      break;
    case Y_NORMAL: // -- XZ plane
      sliceDims[0] = dims[0];
      sliceDims[1] = 1;
      sliceDims[2] = dims[2];

      sliceOrigin[0] = gorigin[0];
      sliceOrigin[1] = origin[1];
      sliceOrigin[2] = gorigin[2];

      slice->SetOrigin(sliceOrigin);
      slice->SetDimensions(sliceDims);
      slice->SetSpacing(spacing);
      assert(slice->GetGridDescription() == VTK_XZ_PLANE);
      break;
    case Z_NORMAL: // -- XY plane
      sliceDims[0] = dims[0];
      sliceDims[1] = dims[1];
      sliceDims[2] = 1;

      sliceOrigin[0] = gorigin[0];
      sliceOrigin[1] = gorigin[1];
      sliceOrigin[2] = origin[2];

      slice->SetOrigin(sliceOrigin);
      slice->SetDimensions(sliceDims);
      slice->SetSpacing(spacing);
      assert(slice->GetGridDescription() == VTK_XY_PLANE);
      break;
    default:
      vtkErrorMacro("Undefined normal");
  }

  //  vtkTimerLog::MarkEndEvent( "AMRSlice::GetSliceForBlock" );

  return (slice);
}

//-----------------------------------------------------------------------------
bool vtkAMRSliceFilter::PlaneIntersectsAMRBox(double plane[4], double bounds[6])
{
  bool lowPnt = false;
  bool highPnt = false;

  for (int i = 0; i < 8; ++i)
  {
    // Get box coordinates
    double x = (i & 1) ? bounds[1] : bounds[0];
    double y = (i & 2) ? bounds[3] : bounds[2];
    double z = (i & 4) ? bounds[5] : bounds[4];

    // Plug-in coordinates to the plane equation
    double v = plane[3] - plane[0] * x - plane[1] * y - plane[2] * z;

    if (v == 0.0) // Point is on a plane
    {
      return true;
    }

    if (v < 0.0)
    {
      lowPnt = true;
    }
    else
    {
      highPnt = true;
    }
    if (lowPnt && highPnt)
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkAMRSliceFilter::ComputeAMRBlocksToLoad(vtkPlane* p, vtkOverlappingAMR* metadata)
{
  assert("pre: plane object is nullptr" && (p != nullptr));
  assert("pre: metadata object is nullptr" && (metadata != nullptr));

  vtkTimerLog::MarkStartEvent("AMRSlice::ComputeAMRBlocksToLoad");

  // Store A,B,C,D from the plane equation
  double plane[4];
  plane[0] = p->GetNormal()[0];
  plane[1] = p->GetNormal()[1];
  plane[2] = p->GetNormal()[2];
  plane[3] = p->GetNormal()[0] * p->GetOrigin()[0] + p->GetNormal()[1] * p->GetOrigin()[1] +
    p->GetNormal()[2] * p->GetOrigin()[2];

  vtkSmartPointer<vtkUniformGridAMRDataIterator> iter;
  iter.TakeReference(vtkUniformGridAMRDataIterator::SafeDownCast(metadata->NewIterator()));
  iter->SetSkipEmptyNodes(false);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (iter->GetCurrentLevel() <= this->MaxResolution)
    {
      double* bounds = iter->GetCurrentMetaData()->Get(vtkDataObject::BOUNDING_BOX());
      if (this->PlaneIntersectsAMRBox(plane, bounds))
      {
        unsigned int amrGridIdx = iter->GetCurrentFlatIndex();
        this->BlocksToLoad.push_back(amrGridIdx);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkAMRSliceFilter::GetAMRSliceInPlane(
  vtkPlane* p, vtkOverlappingAMR* inp, vtkOverlappingAMR* out)
{
  assert("pre: input AMR dataset is nullptr" && (inp != nullptr));
  assert("pre: output AMR dataset is nullptr" && (out != nullptr));
  assert("pre: cut plane is nullptr" && (p != nullptr));

  int description = 0;
  switch (this->Normal)
  {
    case X_NORMAL:
      description = VTK_YZ_PLANE;
      break;
    case Y_NORMAL:
      description = VTK_XZ_PLANE;
      break;
    case Z_NORMAL:
      description = VTK_XY_PLANE;
      break;
    default:
      vtkErrorMacro("Undefined normal");
  }

  if (this->BlocksToLoad.empty())
  {
    this->ComputeAMRBlocksToLoad(p, inp);
  }

  auto numLevels = vtkMath::Min(this->MaxResolution + 1, inp->GetNumberOfLevels());
  std::vector<int> blocksPerLevel(numLevels, 0);
  for (unsigned int i = 0; i < this->BlocksToLoad.size(); i++)
  {
    unsigned int flatIndex = this->BlocksToLoad[i];
    unsigned int level;
    unsigned int dataIdx;
    inp->GetLevelAndIndex(flatIndex, level, dataIdx);
    assert(level < numLevels);
    blocksPerLevel[level]++;
  }

  for (int i = static_cast<int>(blocksPerLevel.size() - 1); i >= 0; i--)
  {
    if (blocksPerLevel[i] == 0)
    {
      blocksPerLevel.pop_back();
    }
    else
    {
      break;
    }
  }

  out->Initialize(static_cast<int>(blocksPerLevel.size()), &blocksPerLevel[0]);
  out->SetGridDescription(description);
  out->SetOrigin(p->GetOrigin());
  vtkTimerLog::MarkStartEvent("AMRSlice::GetAMRSliceInPlane");

  std::vector<int> dataIndices(out->GetNumberOfLevels(), 0);
  for (unsigned int i = 0; i < this->BlocksToLoad.size(); i++)
  {
    int flatIndex = this->BlocksToLoad[i];
    unsigned int level;
    unsigned int dataIdx;
    inp->GetLevelAndIndex(flatIndex, level, dataIdx);
    vtkUniformGrid* grid = inp->GetDataSet(level, dataIdx);
    vtkUniformGrid* slice = nullptr;

    if (grid)
    {
      // Get the 3-D Grid dimensions
      int dims[3];
      grid->GetDimensions(dims);
      slice = this->GetSlice(p->GetOrigin(), dims, grid->GetOrigin(), grid->GetSpacing());
      assert("Dimension of slice must be 2-D" && (slice->GetDataDimension() == 2));
      assert("2-D slice is nullptr" && (slice != nullptr));
      this->GetSliceCellData(slice, grid);
      this->GetSlicePointData(slice, grid);
    }
    else
    {
      int dims[3];
      double spacing[3];
      double origin[3];
      inp->GetSpacing(level, spacing);
      inp->GetAMRBox(level, dataIdx).GetNumberOfNodes(dims);
      inp->GetOrigin(level, dataIdx, origin);
      slice = this->GetSlice(p->GetOrigin(), dims, origin, spacing);
    }

    vtkAMRBox box(slice->GetOrigin(), slice->GetDimensions(), slice->GetSpacing(), out->GetOrigin(),
      out->GetGridDescription());
    out->SetSpacing(level, slice->GetSpacing());
    out->SetAMRBox(level, dataIndices[level], box);
    if (grid)
    {
      out->SetDataSet(level, dataIndices[level], slice);
    }
    slice->Delete();
    dataIndices[level]++;
  }

  vtkTimerLog::MarkEndEvent("AMRSlice::GetAMRSliceInPlane");

  vtkTimerLog::MarkStartEvent("AMRSlice::Generate Blanking");
  vtkParallelAMRUtilities::BlankCells(out, this->Controller);
  vtkTimerLog::MarkEndEvent("AMRSlice::Generate Blanking");
}

//-----------------------------------------------------------------------------
void vtkAMRSliceFilter::ComputeCellCenter(vtkUniformGrid* ug, const int cellIdx, double centroid[3])
{
  assert("pre: Input grid is nullptr" && (ug != nullptr));
  assert(
    "pre: cell index out-of-bounds!" && ((cellIdx >= 0) && (cellIdx < ug->GetNumberOfCells())));

  vtkCell* myCell = ug->GetCell(cellIdx);
  assert("post: cell is nullptr" && (myCell != nullptr));

  double pCenter[3];
  double weights[8];
  int subId = myCell->GetParametricCenter(pCenter);
  myCell->EvaluateLocation(subId, pCenter, centroid, weights);
}

//-----------------------------------------------------------------------------
int vtkAMRSliceFilter::GetDonorCellIdx(double x[3], vtkUniformGrid* ug)
{
  const double* x0 = ug->GetOrigin();
  const double* h = ug->GetSpacing();
  int* dims = ug->GetDimensions();

  int ijk[3];
  for (int i = 0; i < 3; ++i)
  {
    ijk[i] = static_cast<int>(floor((x[i] - x0[i]) / h[i]));
    ijk[i] = vtkMath::ClampValue(ijk[i], 0, vtkMath::Max(1, dims[i] - 1) - 1);
  }

  return (vtkStructuredData::ComputeCellId(dims, ijk));
}

//----------------------------------------------------------------------------
int vtkAMRSliceFilter::GetDonorPointIdx(double x[3], vtkUniformGrid* ug)
{
  const double* x0 = ug->GetOrigin();
  const double* h = ug->GetSpacing();
  int* dims = ug->GetDimensions();

  int ijk[3];
  for (int i = 0; i < 3; ++i)
  {
    ijk[i] = std::floor((x[i] - x0[i]) / h[i]);
    ijk[i] = vtkMath::ClampValue(ijk[i], 0, vtkMath::Max(1, dims[i] - 1));
  }

  return vtkStructuredData::ComputePointId(dims, ijk);
}

//-----------------------------------------------------------------------------
void vtkAMRSliceFilter::GetSliceCellData(vtkUniformGrid* slice, vtkUniformGrid* grid3D)
{
  // STEP 1: Allocate data-structures
  vtkCellData* sourceCD = grid3D->GetCellData();
  vtkCellData* targetCD = slice->GetCellData();

  if (sourceCD->GetNumberOfArrays() == 0)
  {
    // nothing to do here
    return;
  }

  // NOTE:
  // Essentially the same as CopyAllocate
  // However CopyAllocate causes visual errors in the slice
  // if ghost cells are present
  vtkIdType numCells = slice->GetNumberOfCells();
  for (int arrayIdx = 0; arrayIdx < sourceCD->GetNumberOfArrays(); ++arrayIdx)
  {
    vtkDataArray* array = sourceCD->GetArray(arrayIdx)->NewInstance();
    array->Initialize();
    array->SetName(sourceCD->GetArray(arrayIdx)->GetName());
    array->SetNumberOfComponents(sourceCD->GetArray(arrayIdx)->GetNumberOfComponents());
    array->SetNumberOfTuples(numCells);
    targetCD->AddArray(array);
    vtkUnsignedCharArray* uca = vtkArrayDownCast<vtkUnsignedCharArray>(array);
    if (uca != nullptr && uca == slice->GetCellGhostArray())
    {
      // initiallize the ghost array
      memset(uca->WritePointer(0, numCells), 0, numCells);
    }
    array->Delete();
  } // END for all arrays

  // STEP 2: Fill in slice data-arrays
  for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
  {
    double probePnt[3];
    this->ComputeCellCenter(slice, cellIdx, probePnt);
    int sourceCellIdx = this->GetDonorCellIdx(probePnt, grid3D);

    // NOTE:
    // Essentially the same as CopyData, but since CopyAllocate is not
    // working properly the loop has to stay for now.
    for (int arrayIdx = 0; arrayIdx < sourceCD->GetNumberOfArrays(); ++arrayIdx)
    {
      vtkDataArray* sourceArray = sourceCD->GetArray(arrayIdx);
      const char* name = sourceArray->GetName();
      vtkDataArray* targetArray = targetCD->GetArray(name);
      targetArray->SetTuple(cellIdx, sourceCellIdx, sourceArray);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkAMRSliceFilter::GetSlicePointData(vtkUniformGrid* slice, vtkUniformGrid* grid3D)
{
  // STEP 1: Allocate data-structures
  vtkPointData* sourcePD = grid3D->GetPointData();
  vtkPointData* targetPD = slice->GetPointData();

  if (sourcePD->GetNumberOfArrays() == 0)
  {
    // nothing to do here
    return;
  }

  // NOTE:
  // Essentially the same as CopyAllocate
  // For the same reasons as with cell data above,
  // this code is used instead as a precaution, for now.
  vtkIdType numPoints = slice->GetNumberOfPoints();
  for (int arrayIdx = 0; arrayIdx < sourcePD->GetNumberOfArrays(); ++arrayIdx)
  {
    vtkDataArray* array = sourcePD->GetArray(arrayIdx)->NewInstance();
    array->Initialize();
    array->SetName(sourcePD->GetArray(arrayIdx)->GetName());
    array->SetNumberOfComponents(sourcePD->GetArray(arrayIdx)->GetNumberOfComponents());
    array->SetNumberOfTuples(numPoints);
    targetPD->AddArray(array);
    vtkUnsignedCharArray* uca = vtkArrayDownCast<vtkUnsignedCharArray>(array);
    if (uca != nullptr && uca == slice->GetPointGhostArray())
    {
      // initialize the ghost array
      memset(uca->WritePointer(0, numPoints), 0, numPoints);
    }
    array->Delete();
  }

  // STEP 2: Fill in slice data-arrays
  for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx)
  {
    double point[3];
    slice->GetPoint(pointIdx, point);
    int sourcePointIdx = this->GetDonorPointIdx(point, grid3D);

    // NOTE:
    // Essentially the same as CopyData, but since CopyAllocate is not
    // working properly the loop has to stay for now.
    for (int arrayIdx = 0; arrayIdx < sourcePD->GetNumberOfArrays(); ++arrayIdx)
    {
      vtkDataArray* sourceArray = sourcePD->GetArray(arrayIdx);
      const char* name = sourceArray->GetName();
      vtkDataArray* targetArray = targetPD->GetArray(name);
      targetArray->SetTuple(pointIdx, sourcePointIdx, sourceArray);
    }
  }
}

//-----------------------------------------------------------------------------
int vtkAMRSliceFilter::RequestInformation(vtkInformation* vtkNotUsed(rqst),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  this->BlocksToLoad.clear();

  vtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));

  // Check if metadata are passed downstream
  if (input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()))
  {
    vtkOverlappingAMR* metadata = vtkOverlappingAMR::SafeDownCast(
      input->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));

    vtkPlane* cutPlane = this->GetCutPlane(metadata);
    assert("Cut plane is nullptr" && (cutPlane != nullptr));

    this->ComputeAMRBlocksToLoad(cutPlane, metadata);
    cutPlane->Delete();
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRSliceFilter::RequestUpdateExtent(vtkInformation*, vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  assert("pre: inInfo is nullptr" && (inInfo != nullptr));

  // Send upstream request for higher resolution
  if (this->BlocksToLoad.size() > 0)
  {
    inInfo->Set(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &this->BlocksToLoad[0],
      static_cast<int>(this->BlocksToLoad.size()));
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRSliceFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  std::ostringstream oss;
  oss.clear();
  oss << "AMRSlice::Request-" << this->MaxResolution;

  std::string eventName = oss.str();
  vtkTimerLog::MarkStartEvent(eventName.c_str());

  // STEP 0: Get input object
  vtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));

  vtkOverlappingAMR* inputAMR =
    vtkOverlappingAMR::SafeDownCast(input->Get(vtkDataObject::DATA_OBJECT()));

  // STEP 1: Get output object
  vtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr" && (output != nullptr));
  vtkOverlappingAMR* outputAMR =
    vtkOverlappingAMR::SafeDownCast(output->Get(vtkDataObject::DATA_OBJECT()));

  if (this->IsAMRData2D(inputAMR))
  {
    outputAMR->ShallowCopy(inputAMR);
    return 1;
  }

  // STEP 2: Compute global origin
  vtkPlane* cutPlane = this->GetCutPlane(inputAMR);
  assert("Cut plane is nullptr" && (cutPlane != nullptr));

  // STEP 3: Get the AMR slice
  this->GetAMRSliceInPlane(cutPlane, inputAMR, outputAMR);
  cutPlane->Delete();

  vtkTimerLog::MarkEndEvent(eventName.c_str());
  return 1;
}
