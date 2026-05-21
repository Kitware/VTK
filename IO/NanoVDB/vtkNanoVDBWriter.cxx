// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNanoVDBWriter.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringFormatter.h"

#include <string>
#include <vector>

#include <nanovdb/util/CreateNanoGrid.h>
#include <nanovdb/util/IO.h>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
std::string GetGridName(const char* arrayName, int component, int numberOfComponents)
{
  std::string name = (arrayName && arrayName[0] != '\0') ? arrayName : "array";
  if (numberOfComponents != 1 && numberOfComponents != 3)
  {
    name += "_" + vtk::to_string(component);
  }
  return name;
}

// Guard against very small positive spacing values, which can produce unstable transforms.
void EnsureValidSpacing(double& dx, double& dy, double& dz)
{
  while ((dx > 0.0 && dx < 1e-4) || (dy > 0.0 && dy < 1e-4) || (dz > 0.0 && dz < 1e-4))
  {
    dx *= 2.0;
    dy *= 2.0;
    dz *= 2.0;
  }
}

double SafeInverseSpacing(double value)
{
  return value != 0.0 ? 1.0 / value : 0.0;
}

nanovdb::Map GetNanoVDBMap(const double origin[3], double dx, double dy, double dz)
{
  const double mat[3][3] = { { dx, 0.0, 0.0 }, { 0.0, dy, 0.0 }, { 0.0, 0.0, dz } };
  const double invMat[3][3] = { { SafeInverseSpacing(dx), 0.0, 0.0 },
    { 0.0, SafeInverseSpacing(dy), 0.0 }, { 0.0, 0.0, SafeInverseSpacing(dz) } };

  nanovdb::Map map;
  map.set(
    mat, invMat, nanovdb::Vec3d(origin[0] + dx * 0.5, origin[1] + dy * 0.5, origin[2] + dz * 0.5));
  return map;
}

// Get filename based on whether the file is being written in parallel and the number of time steps.
std::string GetFileName(const char* baseFileName, bool writeAllTimeSteps, int currentTimeIndex,
  int numberOfTimeSteps, int localProcessId, int numberOfProcesses)
{
  std::string fileName = baseFileName;
  std::string path = vtksys::SystemTools::GetFilenamePath(fileName);
  std::string fileNameBase = vtksys::SystemTools::GetFilenameWithoutExtension(fileName);
  std::string ext = vtksys::SystemTools::GetFilenameExtension(fileName);
  std::ostringstream oss;
  oss << std::setw(5) << std::setfill('0') << currentTimeIndex;
  std::string newFileName;

  if (numberOfProcesses == 1)
  {
    if (writeAllTimeSteps && numberOfTimeSteps > 1)
    {
      newFileName = path + "/" + fileNameBase + "_" + oss.str() + ext;
    }
    else
    {
      newFileName = baseFileName;
    }
  }
  else
  {
    if (writeAllTimeSteps && numberOfTimeSteps > 1)
    {
      newFileName =
        path + "/" + fileNameBase + "_" + vtk::to_string(localProcessId) + "_" + oss.str() + ext;
    }
    else
    {
      newFileName = path + "/" + fileNameBase + "_" + vtk::to_string(localProcessId) + ext;
    }
  }
  return newFileName;
}
}

vtkStandardNewMacro(vtkNanoVDBWriter);
vtkCxxSetObjectMacro(vtkNanoVDBWriter, Controller, vtkMultiProcessController);

//-----------------------------------------------------------------------------
vtkNanoVDBWriter::vtkNanoVDBWriter()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkNanoVDBWriter::~vtkNanoVDBWriter()
{
  this->SetFileName(nullptr);
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
int vtkNanoVDBWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkNanoVDBWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      (this->Controller ? this->Controller->GetNumberOfProcesses() : 1));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      (this->Controller ? this->Controller->GetLocalProcessId() : 0));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

    if (this->WriteAllTimeSteps)
    {
      double* inTimes = inputVector[0]->GetInformationObject(0)->Get(
        vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      if (inTimes && this->WriteAllTimeSteps)
      {
        double timeReq = inTimes[this->CurrentTimeIndex];
        inputVector[0]->GetInformationObject(0)->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
      }
    }
    return 1;
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      // reset the CurrentTimeIndex in case we're writing out all of the time steps
      this->CurrentTimeIndex = 0;
      this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
    else
    {
      this->NumberOfTimeSteps = 1;
    }
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (this->WriteAllTimeSteps && this->CurrentTimeIndex == 0)
    {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
  }

  int retVal = this->Superclass::ProcessRequest(request, inputVector, outputVector);

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (this->WriteAllTimeSteps && this->CurrentTimeIndex == this->NumberOfTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
    }
  }

  return retVal;
}

//-----------------------------------------------------------------------------
bool vtkNanoVDBWriter::WriteDataAndReturn()
{
  if (!this->FileName || this->FileName[0] == '\0')
  {
    vtkErrorMacro("A FileName must be specified.");
    return false;
  }

  vtkImageData* imageData = vtkImageData::SafeDownCast(this->GetInput());
  if (!imageData)
  {
    vtkErrorMacro("vtkNanoVDBWriter requires vtkImageData input.");
    return false;
  }

  bool success = this->WriteImageData(imageData);
  this->CurrentTimeIndex++;

  return success;
}

namespace
{
struct VectorCopyToNanoVDB
{
  template <typename TArray>
  void operator()(TArray* sourceArray, nanovdb::build::Vec3fGrid& destGrid, vtkImageData* imageData,
    int extent[6])
  {
    auto accessor = destGrid.getAccessor();
    auto data = vtk::DataArrayTupleRange<3>(sourceArray);
    for (int k = extent[4]; k <= extent[5]; ++k)
    {
      for (int j = extent[2]; j <= extent[3]; ++j)
      {
        for (int i = extent[0]; i <= extent[1]; ++i)
        {
          int vtkijk[3] = { i, j, k };
          vtkIdType pointId = imageData->ComputePointId(vtkijk);
          accessor.setValue(nanovdb::Coord(i, j, k),
            nanovdb::Vec3f(static_cast<float>(data[pointId][0]),
              static_cast<float>(data[pointId][1]), static_cast<float>(data[pointId][2])));
        }
      }
    }
  }
};

struct ScalarCopyToNanoVDB
{
  template <typename TArray>
  void operator()(TArray* sourceArray, nanovdb::build::FloatGrid& destGrid, vtkImageData* imageData,
    int extent[6])
  {
    auto accessor = destGrid.getAccessor();
    auto data = vtk::DataArrayValueRange<1>(sourceArray);
    for (int k = extent[4]; k <= extent[5]; ++k)
    {
      for (int j = extent[2]; j <= extent[3]; ++j)
      {
        for (int i = extent[0]; i <= extent[1]; ++i)
        {
          int vtkijk[3] = { i, j, k };
          vtkIdType pointId = imageData->ComputePointId(vtkijk);
          accessor.setValue(nanovdb::Coord(i, j, k), static_cast<float>(data[pointId]));
        }
      }
    }
  }
};
}

//-----------------------------------------------------------------------------
bool vtkNanoVDBWriter::WriteImageData(vtkImageData* imageData)
{
  int extent[6];
  imageData->GetExtent(extent);

  double origin[3] = { 0.0, 0.0, 0.0 };
  imageData->GetOrigin(origin);

  double dx = 0.0, dy = 0.0, dz = 0.0;
  imageData->GetSpacing(dx, dy, dz);
  ::EnsureValidSpacing(dx, dy, dz);

  nanovdb::Map gridMap = ::GetNanoVDBMap(origin, dx, dy, dz);

  std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> handles;

  vtkPointData* pointData = imageData->GetPointData();
  for (int array = 0; array < pointData->GetNumberOfArrays(); ++array)
  {
    vtkDataArray* data = pointData->GetArray(array);
    if (!data)
    {
      continue;
    }

    const int numberOfComponents = data->GetNumberOfComponents();
    for (int component = 0; component < numberOfComponents; ++component)
    {
      if (numberOfComponents == 3 && component > 0)
      {
        continue;
      }

      std::string gridName = ::GetGridName(data->GetName(), component, numberOfComponents);

      if (numberOfComponents == 3)
      {
        nanovdb::build::Vec3fGrid vectorGrid(
          nanovdb::Vec3f(0.0f, 0.0f, 0.0f), gridName, nanovdb::GridClass::Staggered);
        vectorGrid.mMap = gridMap;

        using VectorCopyDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
        VectorCopyToNanoVDB vectorCopyFunctor;
        if (!VectorCopyDispatch::Execute(data, vectorCopyFunctor, vectorGrid, imageData, extent))
        {
          vectorCopyFunctor(data, vectorGrid, imageData, extent);
        }
        handles.emplace_back(nanovdb::createNanoGrid(vectorGrid));
      }
      else
      {
        nanovdb::build::FloatGrid scalarGrid(0.0f, gridName, nanovdb::GridClass::FogVolume);
        scalarGrid.mMap = gridMap;

        using ScalarCopyDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
        ScalarCopyToNanoVDB scalarCopyFunctor;
        if (!ScalarCopyDispatch::Execute(data, scalarCopyFunctor, scalarGrid, imageData, extent))
        {
          scalarCopyFunctor(data, scalarGrid, imageData, extent);
        }
        handles.emplace_back(nanovdb::createNanoGrid(scalarGrid));
      }
    }
  }

  if (handles.empty())
  {
    vtkWarningMacro("No data arrays found to write.");
    return false;
  }

  int localProcessId = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  int numberOfProcesses = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  std::string fileName = ::GetFileName(this->FileName, this->WriteAllTimeSteps,
    this->CurrentTimeIndex, this->NumberOfTimeSteps, localProcessId, numberOfProcesses);
  try
  {
    nanovdb::io::writeGrids(fileName, handles);
  }
  catch (const std::exception& e)
  {
    vtkErrorMacro("Failed to write NanoVDB file. Reason: " << e.what() << '\n');
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkNanoVDBWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "none") << endl;
  os << indent << "WriteAllTimeSteps: " << this->WriteAllTimeSteps << endl;
}

VTK_ABI_NAMESPACE_END
