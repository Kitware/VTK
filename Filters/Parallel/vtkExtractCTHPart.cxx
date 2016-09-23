/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCTHPart.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractCTHPart.h"

#include "vtkAppendPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkClipPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkExtractCTHPart.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkUniformGrid.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkExtractCTHPart);
vtkCxxSetObjectMacro(vtkExtractCTHPart,ClipPlane,vtkPlane);
vtkCxxSetObjectMacro(vtkExtractCTHPart,Controller,vtkMultiProcessController);

const double CTH_AMR_SURFACE_VALUE=0.499;
const double CTH_AMR_SURFACE_VALUE_FLOAT=1;
const double CTH_AMR_SURFACE_VALUE_UNSIGNED_CHAR=255;

//-----------------------------------------------------------------------------
//=============================================================================
class vtkExtractCTHPartInternal
{
public:
  typedef std::vector<std::string> VolumeArrayNamesType;
  VolumeArrayNamesType VolumeArrayNames;
  vtkBoundingBox GlobalInputBounds;

  // Counter used to scale progress events.
  int TotalNumberOfDatasets;
};

class vtkExtractCTHPart::VectorOfFragments :
  public std::vector<vtkSmartPointer<vtkPolyData> >
{
};

class vtkExtractCTHPart::ScaledProgress
{
  vtkExtractCTHPart* Self;
  double Shift;
  double Scale;
public:
  ScaledProgress(double shift, double scale, vtkExtractCTHPart* self)
  {
    assert((self != NULL) &&
      (shift >= 0.0) && (shift <= 1.0) &&
      (scale >= 0.0) && (scale <= 1.0));

    this->Self = self;
    this->Shift = self->ProgressShift;
    this->Scale = self->ProgressScale;

    self->ProgressShift += shift * self->ProgressScale;
    self->ProgressScale *= scale;
    //cout << "Shift-Scale Push: " << self->ProgressShift << ", " <<
    //  self->ProgressScale << endl;
  }

  ~ScaledProgress()
  {
    this->WorkDone();
  }

  void WorkDone()
  {
    if (this->Self)
    {
      this->Self->ProgressScale = this->Scale;
      this->Self->ProgressShift = this->Shift;
      //cout << "Shift-Scale Pop: " << this->Self->ProgressShift << ", " <<
      //  this->Self->ProgressScale << endl;
      this->Self = NULL;
    }
  }
};


//=============================================================================
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkExtractCTHPart::vtkExtractCTHPart()
{
  this->Internals = new vtkExtractCTHPartInternal();
  this->ClipPlane = NULL;
  this->GenerateTriangles = true;
  this->Capping = true;
  this->RemoveGhostCells = true;
  this->VolumeFractionSurfaceValueInternal = CTH_AMR_SURFACE_VALUE;
  this->VolumeFractionSurfaceValue = CTH_AMR_SURFACE_VALUE;
  this->ProgressScale = 1.0;
  this->ProgressShift = 0.0;

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkExtractCTHPart::~vtkExtractCTHPart()
{
  this->SetController(NULL);
  this->SetClipPlane(NULL);

  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If clip plane is modified,
// then this object is modified as well.
vtkMTimeType vtkExtractCTHPart::GetMTime()
{
  vtkMTimeType mTime= this->Superclass::GetMTime();
  if (this->ClipPlane)
  {
    vtkMTimeType time = this->ClipPlane->GetMTime();
    return time > mTime? time : mTime;
  }

  return mTime;
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::RemoveVolumeArrayNames()
{
  this->Internals->VolumeArrayNames.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::AddVolumeArrayName(const char* arrayName)
{
  if (arrayName !=0 &&
    arrayName[0] != 0 &&
    std::find(this->Internals->VolumeArrayNames.begin(),
      this->Internals->VolumeArrayNames.end(), std::string(arrayName))==
    this->Internals->VolumeArrayNames.end())
  {
    this->Internals->VolumeArrayNames.push_back(arrayName);

    // ensure that the volume arrays are in determinate order. I should just
    // change the code to use a std::set.
    std::sort(this->Internals->VolumeArrayNames.begin(),
      this->Internals->VolumeArrayNames.end());
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkExtractCTHPart::GetNumberOfVolumeArrayNames()
{
  return static_cast<int>(this->Internals->VolumeArrayNames.size());
}

//-----------------------------------------------------------------------------
const char* vtkExtractCTHPart::GetVolumeArrayName(int idx)
{
  if ( idx < 0 ||
       idx > static_cast<int>(this->Internals->VolumeArrayNames.size()) )
  {
    return 0;
  }

  return this->Internals->VolumeArrayNames[idx].c_str();
}

//----------------------------------------------------------------------------
int vtkExtractCTHPart::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port,info))
  {
    return 0;
  }

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkNonOverlappingAMR");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractCTHPart::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  const int number_of_volume_arrays = static_cast<int>(this->Internals->VolumeArrayNames.size());
  if (number_of_volume_arrays == 0)
  {
    // nothing to do.
    return 1;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkSmartPointer<vtkCompositeDataSet> inputCD = vtkCompositeDataSet::SafeDownCast(inputDO);
  vtkRectilinearGrid* inputRG = vtkRectilinearGrid::SafeDownCast(inputDO);
  assert(inputCD != NULL || inputRG != NULL);

  if (inputRG)
  {
    vtkNew<vtkMultiBlockDataSet> mb;
    mb->SetBlock(0, inputRG);
    inputCD = mb.GetPointer();
  }

  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);

  // initialize output multiblock-dataset. It will always have as many blocks as
  // the number-of-volume arrays requested.
  output->SetNumberOfBlocks(number_of_volume_arrays);

  // Compute global bounds for the input dataset. This is used to generate
  // external surface for the dataset.
  if (!this->ComputeGlobalBounds(inputCD))
  {
    vtkErrorMacro("Failed to compute global bounds.");
    return 0;
  }

  if (!this->Internals->GlobalInputBounds.IsValid())
  {
    // empty input, do nothing.
    return 1;
  }

  unsigned int array_index = 0;
  for (vtkExtractCTHPartInternal::VolumeArrayNamesType::iterator iter =
    this->Internals->VolumeArrayNames.begin();
    iter != this->Internals->VolumeArrayNames.end(); ++iter, ++array_index)
  {
    // this loop is doing the 1/(num-arrays)'th work for the entire task.
    ScaledProgress sp(
      array_index * 1.0 / this->Internals->VolumeArrayNames.size(),
      1.0/this->Internals->VolumeArrayNames.size(),
      this);

    output->GetMetaData(array_index)->Set(vtkCompositeDataSet::NAME(), iter->c_str());

    vtkNew<vtkPolyData> contour;
    vtkGarbageCollector::DeferredCollectionPush();
    if (this->ExtractContour(contour.GetPointer(), inputCD, iter->c_str()) &&
      (contour->GetNumberOfPoints() > 0))
    {
      // Add extra arrays.
      vtkNew<vtkIntArray> partArray;
      partArray->SetName("Part Index");
      partArray->SetNumberOfComponents(1);
      partArray->SetNumberOfTuples(contour->GetNumberOfPoints());
      partArray->FillComponent(0, static_cast<double>(array_index));
      contour->GetPointData()->AddArray(partArray.GetPointer());

      // I'm not adding the "Name" array that was added in previous
      // implementation. Don't think that's much of use.

      output->SetBlock(array_index, contour.GetPointer());
    }
    vtkGarbageCollector::DeferredCollectionPop();
  }
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkExtractCTHPart::ComputeGlobalBounds(vtkCompositeDataSet *input)
{
  assert("pre: input_exists" && input!=0);
  this->Internals->GlobalInputBounds.Reset();

  this->Internals->TotalNumberOfDatasets = 0;

  vtkCompositeDataIterator* iter = input->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet *ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!ds)// can be null if on another processor
    {
      continue;
    }
    double realBounds[6];
    ds->GetBounds(realBounds);
    this->Internals->GlobalInputBounds.AddBounds(realBounds);

    ++this->Internals->TotalNumberOfDatasets;
  }
  iter->Delete();

  // Here we have the bounds according to our local datasets.
  // If we are not running in parallel then the local
  // bounds are the global bounds
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    return true;
  }

  const double *min_point = this->Internals->GlobalInputBounds.GetMinPoint();
  const double *max_point = this->Internals->GlobalInputBounds.GetMaxPoint();
  double min_result[3], max_result[3];

  if (!this->Controller->AllReduce(
      min_point, min_result, 3, vtkCommunicator::MIN_OP))
  {
    return false;
  }
  if (!this->Controller->AllReduce(
      max_point, max_result, 3, vtkCommunicator::MAX_OP))
  {
    return false;
  }

  this->Internals->GlobalInputBounds.SetBounds(
    min_result[0], max_result[0], min_result[1], max_result[1],
    min_result[2], max_result[2]);
  // At this point, the global bounds is set in each processor.
  return true;
}

//-----------------------------------------------------------------------------
// return false on error.
bool vtkExtractCTHPart::ExtractContour(
  vtkPolyData* output, vtkCompositeDataSet* input, const char*arrayName)
{
  assert(output!=0 && input!=0 && arrayName!=0 && arrayName[0]!=0);

  bool warn_once = true;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());

  // this loop is first 95% of the work.
  ScaledProgress sp1(0.0, 0.95, this);

  int counter = 0;
  vtkExtractCTHPart::VectorOfFragments fragments;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), ++counter)
  {
    // each iteration is 1/(total num of datasets)'th for the work.
    ScaledProgress sp(
      counter * 1.0/this->Internals->TotalNumberOfDatasets,
      1.0/this->Internals->TotalNumberOfDatasets, this);

    if (counter % 1000 == 0)
    {
      this->TriggerProgressEvent(0.0);
    }

    vtkDataObject *dataObj = iter->GetCurrentDataObject();
    vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(dataObj);
    vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(dataObj);

    if (ug)
    {
      if (!this->ExtractClippedContourOnBlock<vtkUniformGrid>(fragments, ug, arrayName))
      {
        return false;
      }
    }
    else if (rg)
    {
      if (!this->ExtractClippedContourOnBlock<vtkRectilinearGrid>(fragments, rg, arrayName))
      {
        return false;
      }
    }
    else if (warn_once && dataObj)
    {
      warn_once = false;
      vtkWarningMacro(<< dataObj->GetClassName() << " will be ignored.");
    }
    if ((counter % 1000) == 0)
    {
      this->TriggerProgressEvent(1.0);
    }
  }

  if (fragments.size() == 0)
  {
    // empty contour. Not an error though, hence we don't return false.
    return true;
  }
  sp1.WorkDone();

  // Now, the last .05 % of the work.
  ScaledProgress sp2(0.95, 0.05, this);
  this->TriggerProgressEvent(0.0);
  vtkNew<vtkAppendPolyData> appender;
  for (size_t cc=0; cc < fragments.size(); cc++)
  {
    appender->AddInputData(fragments[cc].GetPointer());
  }
  appender->Update();
  output->ShallowCopy(appender->GetOutputDataObject(0));
  this->TriggerProgressEvent(1.0);
  return true;
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkExtractCTHPart::ExtractClippedContourOnBlock(
  vtkExtractCTHPart::VectorOfFragments& fragments, T* dataset, const char* arrayName)
{
  assert(arrayName!=0 && arrayName[0]!=0 && dataset != 0);

  vtkDataArray* volumeFractionArray = dataset->GetCellData()->GetArray(arrayName);
  if (!volumeFractionArray)
  {
    // skip this block.
    return true;
  }

  // determine the true value to use for the contour based on the data-type.
  switch (volumeFractionArray->GetDataType())
  {
  case VTK_UNSIGNED_CHAR:
    this->VolumeFractionSurfaceValueInternal =
      CTH_AMR_SURFACE_VALUE_UNSIGNED_CHAR * this->VolumeFractionSurfaceValue;
    break;

  default:
    this->VolumeFractionSurfaceValueInternal =
      CTH_AMR_SURFACE_VALUE_FLOAT * this->VolumeFractionSurfaceValue;
  }

  // We create a clone so we can modify the dataset (i.e. add new arrays to it).
  vtkNew<T> inputClone;
  inputClone->ShallowCopy(dataset);

  // Convert cell-data-2-point-data so we can contour.
  vtkNew<vtkDoubleArray> pointVolumeFractionArray;
  this->ExecuteCellDataToPointData(volumeFractionArray,
    pointVolumeFractionArray.GetPointer(), inputClone->GetDimensions());
  inputClone->GetPointData()->SetScalars(pointVolumeFractionArray.GetPointer());

  VectorOfFragments blockFragments;
  if (!this->ExtractContourOnBlock<T>(blockFragments, inputClone.GetPointer(), arrayName))
  {
    return false;
  }

  if (!this->ClipPlane)
  {
    fragments.insert(fragments.end(),
      blockFragments.begin(), blockFragments.end());
    return true;
  }

  // Clip-n-cap the fragments using the clip plane.

  // for the clip.
  for (size_t cc=0; cc < blockFragments.size(); cc++)
  {
    vtkNew<vtkClipPolyData> clipper;
    clipper->SetClipFunction(this->ClipPlane);
    clipper->SetInputDataObject(blockFragments[cc]);
    clipper->Update();
    fragments.push_back(clipper->GetOutput());
  }

  //// for the cap.
  if (this->Capping)
  {
    vtkNew<vtkCutter> cutter;
    cutter->SetCutFunction(this->ClipPlane);
    cutter->SetGenerateTriangles(this->GenerateTriangles? 1 : 0);
    cutter->SetInputDataObject(inputClone.GetPointer());

    vtkNew<vtkClipPolyData> scalarClipper;
    scalarClipper->SetInputConnection(cutter->GetOutputPort());
    scalarClipper->SetValue(this->VolumeFractionSurfaceValueInternal);
    scalarClipper->Update();
    fragments.push_back(scalarClipper->GetOutput());
  }
  return true;
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkExtractCTHPart::ExtractContourOnBlock(
  vtkExtractCTHPart::VectorOfFragments& fragments, T* dataset, const char* arrayName)
{
  assert(arrayName!=0 && arrayName[0]!=0 && dataset != 0);

  vtkDataArray* volumeFractionArray = dataset->GetPointData()->GetArray(arrayName);
  assert(volumeFractionArray !=0);
  assert(dataset->GetPointData()->GetArray(arrayName) !=0);

  // Contour only if necessary.
  double range[2];
  volumeFractionArray->GetRange(range);
  if (range[1] < this->VolumeFractionSurfaceValueInternal)
  {
    // this block doesn't have the material of interest.
    return true;
  }

  // Extract exterior surface. Adds the surface polydata to fragments, if any.
  if (this->Capping)
  {
    this->ExtractExteriorSurface(fragments, dataset);
  }

  if (this->ClipPlane == NULL && range[0] > this->VolumeFractionSurfaceValueInternal)
  {
    // no need to extract contour.
    return true;
  }

  // Extract contour.
  vtkNew<vtkContourFilter> contourer;
  contourer->SetInputData(dataset);
  contourer->SetValue(0, this->VolumeFractionSurfaceValueInternal);
  contourer->SetComputeScalars(0);
  contourer->SetGenerateTriangles(this->GenerateTriangles? 1: 0);
  contourer->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, arrayName);
  contourer->Update();

  vtkPolyData* output =
    vtkPolyData::SafeDownCast(contourer->GetOutputDataObject(0));
  if (!output || output->GetNumberOfPoints()== 0)
  {
    return true;
  }
  if (!this->RemoveGhostCells)
  {
    // BUG #14291. Rather than renaming the array, we remove the GhostArray
    // from the output since it may not be present on all ranks and will cause array count mismatch
    output->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  }

  fragments.push_back(output);
  return true;
}

//-----------------------------------------------------------------------------
// Description:
// Append quads for faces of the block that actually on the bounds
// of the hierarchical dataset. Deals with ghost cells.
template <class T>
void vtkExtractCTHPart::ExtractExteriorSurface(
  vtkExtractCTHPart::VectorOfFragments& fragments, T* input)
{
  assert("pre: valid_input" && input!=0 && input->CheckAttributes()==0);

  int result=0;
#if 1
  int dims[3];
  input->GetDimensions(dims);
  int ext[6];
  int originalExtents[6];
  input->GetExtent(ext);
  input->GetExtent(originalExtents);

//  vtkUnsignedCharArray *ghostArray=static_cast<vtkUnsignedCharArray *>(input->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));

  // bounds without taking ghost cells into account
  double bounds[6];

  input->GetBounds(bounds);

#if 0
  // block face min x
  if(this->IsGhostFace(0,0,dims,ghostArray))
  {
    // downsize this!
    bounds[0]+=spacing[0];
    ++ext[0];
  }
  if(this->IsGhostFace(0,1,dims,ghostArray))
  {
    // downsize this!
    bounds[1]-=spacing[0];
    --ext[1];
  }
  if(this->IsGhostFace(1,0,dims,ghostArray))
  {
    // downsize this!
    bounds[2]+=spacing[1];
    ++ext[2];
  }
  if(this->IsGhostFace(1,1,dims,ghostArray))
  {
    // downsize this!
    bounds[3]-=spacing[1];
    --ext[3];
  }
  if(this->IsGhostFace(2,0,dims,ghostArray))
  {
    // downsize this!
    bounds[4]+=spacing[2];
    ++ext[4];
  }
  if(this->IsGhostFace(2,1,dims,ghostArray))
  {
    // downsize this!
    bounds[5]-=spacing[2];
    --ext[5];
  }
#endif
  // here, bounds are real block bounds without ghostcells.

  const double *minP = this->Internals->GlobalInputBounds.GetMinPoint();
  const double *maxP = this->Internals->GlobalInputBounds.GetMaxPoint();
#if 0
  const double epsilon=0.001;
  int doFaceMinX=fabs(bounds[0]- minP[0])<epsilon;
  int doFaceMaxX=fabs(bounds[1]- maxP[0])<epsilon;
  int doFaceMinY=fabs(bounds[2]- minP[1])<epsilon;
  int doFaceMaxY=fabs(bounds[3]- maxP[1])<epsilon;
  int doFaceMinZ=fabs(bounds[4]- minP[2])<epsilon;
  int doFaceMaxZ=fabs(bounds[5]- maxP[2])<epsilon;
#endif

#if 1
  int doFaceMinX=bounds[0]<= minP[0];
  int doFaceMaxX=bounds[1]>= maxP[0];
  int doFaceMinY=bounds[2]<= minP[1];
  int doFaceMaxY=bounds[3]>= maxP[1];
  int doFaceMinZ=bounds[4]<= minP[2];
  int doFaceMaxZ=bounds[5]>= maxP[2];
#endif
#if 0
  int doFaceMinX=1;
  int doFaceMaxX=1;
  int doFaceMinY=1;
  int doFaceMaxY=1;
  int doFaceMinZ=1;
  int doFaceMaxZ=1;
#endif
#if 0
  int doFaceMinX=0;
  int doFaceMaxX=0;
  int doFaceMinY=0;
  int doFaceMaxY=0;
  int doFaceMinZ=0;
  int doFaceMaxZ=0;
#endif
#if 0
  doFaceMaxX=0;
  doFaceMaxY=0;
  doFaceMaxZ=0;
#endif

  result=doFaceMinX||doFaceMaxX||doFaceMinY||doFaceMaxY||doFaceMinZ
    ||doFaceMaxZ;

  if(result)
  {
    vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();

    vtkIdType numPoints=0;
    vtkIdType cellArraySize=0;

//  input->GetExtent(ext);

    // Compute an upper bound for the number of points and cells.
    // xMin face
    if (doFaceMinX && ext[2] != ext[3] && ext[4] != ext[5] && ext[0] != ext[1])
    {
      cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    }
    // xMax face
    if (doFaceMaxX && ext[2] != ext[3] && ext[4] != ext[5])
    {
      cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    }
    // yMin face
    if (doFaceMinY && ext[0] != ext[1] && ext[4] != ext[5] && ext[2] != ext[3])
    {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    }
    // yMax face
    if (doFaceMaxY && ext[0] != ext[1] && ext[4] != ext[5])
    {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    }
    // zMin face
    if (doFaceMinZ && ext[0] != ext[1] && ext[2] != ext[3] && ext[4] != ext[5])
    {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    }
    // zMax face
    if (doFaceMaxZ && ext[0] != ext[1] && ext[2] != ext[3])
    {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    }

    vtkCellArray *outPolys = vtkCellArray::New();
    outPolys->Allocate(cellArraySize);
    output->SetPolys(outPolys);
    outPolys->Delete();

    vtkPoints *outPoints = vtkPoints::New();
    outPoints->Allocate(numPoints);
    output->SetPoints(outPoints);
    outPoints->Delete();

    // Allocate attributes for copying.
    output->GetPointData()->CopyAllocate(input->GetPointData());
    output->GetCellData()->CopyAllocate(input->GetCellData());

    // Extents are already corrected for ghostcells.

    // make each face that is actually on the ds boundary
    if(doFaceMinX)
    {
      this->ExecuteFaceQuads(input,output,0,originalExtents,ext,0,1,2);
    }
    if(doFaceMaxX)
    {
      this->ExecuteFaceQuads(input,output,1,originalExtents,ext,0,2,1);
    }
    if(doFaceMinY)
    {
      this->ExecuteFaceQuads(input,output,0,originalExtents,ext,1,2,0);
    }
    if(doFaceMaxY)
    {
      this->ExecuteFaceQuads(input,output,1,originalExtents,ext,1,0,2);
    }
    if(doFaceMinZ)
    {
      this->ExecuteFaceQuads(input,output,0,originalExtents,ext,2,0,1);
    }
    if(doFaceMaxZ)
    {
      this->ExecuteFaceQuads(input,output,1,originalExtents,ext,2,1,0);
    }
    output->Squeeze();
    assert(output->CheckAttributes() == 0);

    vtkNew<vtkClipPolyData> clipper;
    clipper->SetInputData(output);
    clipper->SetValue(this->VolumeFractionSurfaceValueInternal);
    clipper->Update();
    fragments.push_back(clipper->GetOutput());
  }
#endif
// result=>valid_surface: A=>B !A||B
}

//----------------------------------------------------------------------------
// Description:
// Is block face on axis0 (either min or max depending on the maxFlag)
// composed of only ghost cells?
int vtkExtractCTHPart::IsGhostFace(int axis0,
                                   int maxFlag,
                                   int dims[3],
                                   vtkUnsignedCharArray *ghostArray)
{
  assert("pre: valid_axis0" && axis0>=0 && axis0<=2);

  int axis1=axis0+1;
  if(axis1>2)
  {
    axis1=0;
  }
  int axis2=axis0+2;
  if(axis2>2)
  {
    axis2=0;
  }

  int ijk[3]; // index of the cell.

  if(maxFlag)
  {
    ijk[axis0]=dims[axis0]-2;
  }
  else
  {
    ijk[axis0]=0;
  }

  // We test the center cell of the block face.
  // in the worst case (2x2 cells), we need to know if at least
  // three cells are ghost to say that the face is a ghost face.

  ijk[axis1]=dims[axis1]/2-1; // (dims[axis1]-2)/2
  ijk[axis2]=dims[axis2]/2-1; // (dims[axis2]-2)/2
  int result=ghostArray->GetValue(vtkStructuredData::ComputeCellId(dims,ijk));

  if(dims[axis1]==3)
  {
    // axis1 requires 2 cells to be tested.
    // if so, axis1index=0 and axis1index+1=1
    ijk[axis1]=1;
    result=result &&
      ghostArray->GetValue(vtkStructuredData::ComputeCellId(dims,ijk));
  }

  if(dims[axis2]==3)
  {
    // herem axis1 may have moved from the previous test.
    // axis2 requires 2 cells to be tested.
    // if so, axis2index=0 and axis2index+1=1
    ijk[axis2]=1;
    result=result &&
      ghostArray->GetValue(vtkStructuredData::ComputeCellId(dims,ijk));
  }
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Merly the same implementation than in vtkDataSetSurfaceFilter, without
// dealing with the whole extents.
void vtkExtractCTHPart::ExecuteFaceQuads(vtkDataSet *input,
                                         vtkPolyData *output,
                                         int maxFlag,
                                         int originalExtents[6],
                                         int ext[6],
                                         int aAxis,
                                         int bAxis,
                                         int cAxis)
{
  assert("pre: input_exists" && input!=0);
  assert("pre: output_exists" && output!=0);
  assert("pre: originalExtents_exists" && originalExtents!=0);
  assert("pre: ext_exists" && ext!=0);
  assert("pre: valid_axes"
         && aAxis>=0 && aAxis<=2
         && bAxis>=0 && bAxis<=2
         && cAxis>=0 && cAxis<=2
         && aAxis!=bAxis
         && aAxis!=cAxis
         && bAxis!=cAxis);

  vtkPoints    *outPts;
  vtkCellArray *outPolys;
  vtkPointData *inPD, *outPD;
  vtkCellData  *inCD, *outCD;
  int          pInc[3];
  int          qInc[3];
  int          cOutInc;
  double        pt[3];
  vtkIdType    inStartPtId;
  vtkIdType    inStartCellId;
  vtkIdType    outStartPtId;
  vtkIdType    outPtId;
  vtkIdType    inId, outId;
  int          ib, ic;
  int          aA2, bA2, cA2;

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();

  pInc[0] = 1;
  pInc[1] = (originalExtents[1]-originalExtents[0]+1);
  pInc[2] = (originalExtents[3]-originalExtents[2]+1) * pInc[1];
  // quad increments (cell incraments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = originalExtents[1]-originalExtents[0];
  // The conditions are for when we have one or more degenerate axes (2d or 1d cells).
  if (qInc[1] == 0)
  {
    qInc[1] = 1;
  }
  qInc[2] = (originalExtents[3]-originalExtents[2]) * qInc[1];
  if (qInc[2] == 0)
  {
    qInc[2] = qInc[1];
  }

  // Temporary variables to avoid many multiplications.
  aA2 = aAxis<<1; // * 2;
  bA2 = bAxis<<1; // * 2;
  cA2 = cAxis<<1; //  * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2+1] || ext[cA2] == ext[cA2+1])
  {
    return;
  }
#if 0
  if (maxFlag)
  {
    if (ext[aA2+1] < wholeExt[aA2+1])
    {
      return;
    }
  }
  else
  { // min faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2+1] || ext[aA2] > wholeExt[aA2])
    {
      return;
    }
  }
#endif

  if(!maxFlag)
  {
    if (ext[aA2] == ext[aA2+1])
    {
      return;
    }
  }

  // Assuming no ghost cells ...
//  inStartPtId = inStartCellId = 0;
  inStartPtId=0; //ext[aA2];
  inStartCellId=0; //ext[aA2];


  // I put this confusing conditional to fix a regression test.
  // If we are creating a maximum face, then we indeed have to offset the input cell Ids.
  // However, vtkGeometryFilter created a 2d image as a max face, but the cells are copied
  // as a min face (no offset).  Hence maxFlag = 1 and there should be no offset.
  if (maxFlag && ext[aA2] < ext[1+aA2])
  {
    inStartPtId = pInc[aAxis]*(ext[aA2+1]-originalExtents[aA2]); // -ext[aA2]
    inStartCellId = qInc[aAxis]*(ext[aA2+1]-originalExtents[aA2]-1); // -ext[aA2]
  }

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2+1]; ++ic)
  {
    for (ib = ext[bA2]; ib <= ext[bA2+1]; ++ib)
    {
//      inId = inStartPtId + (ib-ext[bA2]+originExtents[bAxis])*pInc[bAxis]
//                         + (ic-ext[cA2]+originExtents[cAxis])*pInc[cAxis];

      inId = inStartPtId + (ib-originalExtents[bA2])*pInc[bAxis]
        + (ic-originalExtents[cA2])*pInc[cAxis];

      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD,inId,outId);
    }
  }

  // Do the cells.
  cOutInc = ext[bA2+1] - ext[bA2] + 1;

  outPolys = output->GetPolys();

  // Old method for creating quads (needed for cell data.).
  for (ic = ext[cA2]; ic < ext[cA2+1]; ++ic)
  {
    for (ib = ext[bA2]; ib < ext[bA2+1]; ++ib)
    {
      outPtId = outStartPtId + (ib-ext[bA2]) + (ic-ext[cA2])*cOutInc;
//      inId = inStartCellId + (ib-ext[bA2]+originExtents[bAxis])*qInc[bAxis] + (ic-ext[cA2]+originExtents[cAxis])*qInc[cAxis];

      inId = inStartCellId + (ib-originalExtents[bA2])*qInc[bAxis] + (ic-originalExtents[cA2])*qInc[cAxis];

      outId = outPolys->InsertNextCell(4);
      outPolys->InsertCellPoint(outPtId);
      outPolys->InsertCellPoint(outPtId+cOutInc);
      outPolys->InsertCellPoint(outPtId+cOutInc+1);
      outPolys->InsertCellPoint(outPtId+1);

      // Copy cell data.
      outCD->CopyData(inCD,inId,outId);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::ExecuteCellDataToPointData(
  vtkDataArray *cellVolumeFraction, vtkDoubleArray *pointVolumeFraction, const int *dims)
{
  int count;
  int i, j, k;
  int iEnd, jEnd, kEnd;
  int jInc, kInc;
  double *pPoint;

  pointVolumeFraction->SetName(cellVolumeFraction->GetName());
  pointVolumeFraction->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);

  iEnd = dims[0]-1;
  jEnd = dims[1]-1;
  kEnd = dims[2]-1;

  // Deals with none 3D images, otherwise it will never enter into the loop.
  // And then the data will be not initialized and the output of the contour
  // will be empty.

  int dimensionality=3;

  if(kEnd==0)
  {
    --dimensionality;
    kEnd=1;
  }

  // Increments are for the point array.
  jInc = dims[0];
  kInc = (dims[1]) * jInc;

  pPoint = pointVolumeFraction->GetPointer(0);
//  pCell = static_cast<double*>(cellVolumeFraction->GetVoidPointer(0));

  // Initialize the point data to 0.
  memset(pPoint, 0,  dims[0]*dims[1]*dims[2]*sizeof(double));

#ifndef NDEBUG
  // for debugging and check out of range.
  double *endPtr=pPoint+dims[0]*dims[1]*dims[2];
#endif

//  float delProgress = (maxProgress - minProgress) / (kEnd*jEnd*iEnd) / 2;
//  vtkIdType counter = 0;

  int index=0;
  // Loop thorugh the cells.
  for (k = 0; k < kEnd; ++k)
  {
    for (j = 0; j < jEnd; ++j)
    {
      for (i = 0; i < iEnd; ++i)
      {
        //if (counter % 1000 == 0 && reportProgress)
        //  {
        //  this->UpdateProgress(minProgress + delProgress*(i+j*iEnd+k*iEnd*jEnd));
        //  }
        //counter++;
        // Add cell value to all points of cell.
        double value=cellVolumeFraction->GetTuple1(index);

        assert("check: valid_range" && pPoint<endPtr);
        assert("check: valid_range" && pPoint+1<endPtr);
        assert("check: valid_range" && pPoint+jInc<endPtr);
        assert("check: valid_range" && pPoint+jInc+1<endPtr);

        *pPoint += value;
        pPoint[1] += value;
        pPoint[jInc] += value;
        pPoint[1+jInc] += value;

        if(dimensionality==3)
        {
          assert("check: valid_range" && pPoint+kInc<endPtr);
          assert("check: valid_range" && pPoint+kInc+1<endPtr);
          assert("check: valid_range" && pPoint+kInc+jInc<endPtr);
          assert("check: valid_range" && pPoint+kInc+jInc+1<endPtr);

          pPoint[kInc] += value;
          pPoint[kInc+1] += value;
          pPoint[kInc+jInc] += value;
          pPoint[kInc+jInc+1] += value;
        }

        // Increment pointers
        ++pPoint;
        ++index;
      }
      // Skip over last point to the start of the next row.
      ++pPoint;
    }
    // Skip over the last row to the start of the next plane.
    pPoint += jInc;
  }

  // Now a second pass to normalize the point values.
  // Loop through the points.
  count = 1;
  pPoint = pointVolumeFraction->GetPointer(0);

  // because we eventually modified iEnd, jEnd, kEnd to handle the
  // 2D image case, we have to recompute them.
  iEnd = dims[0]-1;
  jEnd = dims[1]-1;
  kEnd = dims[2]-1;

  // counter = 0;
  for (k = 0; k <= kEnd; ++k)
  {
    // Just a fancy fast way to compute the number of cell neighbors of a
    // point.
    if (k == 1)
    {
      count = count << 1;
    }
    if (k == kEnd && kEnd>0)
    {
      // only in 3D case, otherwise count may become zero
      // and be involved in a division by zero later on
      count = count >> 1;
    }
    for (j = 0; j <= jEnd; ++j)
    {
      // Just a fancy fast way to compute the number of cell neighbors of a
      // point.
      if (j == 1)
      {
        count = count << 1;
      }
      if (j == jEnd)
      {
        count = count >> 1;
      }
      for (i = 0; i <= iEnd; ++i)
      {
        //if (counter % 1000 == 0 && reportProgress)
        //  {
        //  this->UpdateProgress(minProgress + delProgress/2 + delProgress*(i+j*iEnd+k*iEnd*jEnd));
        //  }
        //counter++;
        // Just a fancy fast way to compute the number of cell neighbors of a
        // point.
        if (i == 1)
        {
          count = count << 1;
        }
        if (i == iEnd)
        {
          count = count >> 1;
        }
        assert("check: valid_range" && pPoint<endPtr);
        assert("check: strictly_positive_count" && count>0);
        *pPoint = *pPoint / static_cast<double>(count);
        ++pPoint;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::TriggerProgressEvent(double val)
{
  double progress = this->ProgressShift + val*this->ProgressScale;
  //cout << "Progress: " << progress << endl;
  this->UpdateProgress(progress);
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VolumeArrayNames: \n";
  vtkIndent i2 = indent.GetNextIndent();
  std::vector<std::string>::iterator it;
  for ( it = this->Internals->VolumeArrayNames.begin();
    it != this->Internals->VolumeArrayNames.end();
    ++ it )
  {
    os << i2 << it->c_str() << endl;
  }
  os << indent << "VolumeFractionSurfaceValue: "
    << this->VolumeFractionSurfaceValue << endl;
  os << indent << "Capping: " << this->Capping << endl;
  os << indent << "GenerateTriangles: " << this->GenerateTriangles << endl;
  os << indent << "RemoveGhostCells: " << this->RemoveGhostCells << endl;

  if (this->ClipPlane)
  {
    os << indent << "ClipPlane:\n";
    this->ClipPlane->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ClipPlane: NULL\n";
  }

  if ( this->Controller!=0)
  {
    os << "Controller:" << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "No Controller." << endl;
  }
}
