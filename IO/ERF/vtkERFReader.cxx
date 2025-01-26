// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkERFReader.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkHDF5Helper.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <vector>

#define H5_USE_16_API
#include "vtk_hdf5.h"

#include "vtkHDF5ScopedHandle.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkERFReader);

namespace
{
// Attributes described in the format for the System Block
constexpr std::array<const char*, 4> SYSTEM_ATTRIBUTES = { "solver_name", "solver_vers", "sys",
  "title" };

const std::string NODE_GROUP = "COORDINATE";
};

//----------------------------------------------------------------------------
vtkERFReader::vtkERFReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Add observer for array selection update
  this->VariablesSelection->AddObserver(vtkCommand::ModifiedEvent, this, &vtkERFReader::Modified);
  this->StagesSelection->AddObserver(vtkCommand::ModifiedEvent, this, &vtkERFReader::Modified);
  this->BlocksSelection->AddObserver(vtkCommand::ModifiedEvent, this, &vtkERFReader::Modified);
}

//----------------------------------------------------------------------------
vtkERFReader::~vtkERFReader() = default;

//------------------------------------------------------------------------------
int vtkERFReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkHDF::ScopedH5FHandle fileId = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    vtkErrorMacro("Could not open ERF-HDF5 file '" << this->FileName << "'");
    return 0;
  }

  // Find all stages
  this->CurrentName = "/";
  std::vector<std::string> stages = vtkHDF5Helper::GetChildren(fileId, this->CurrentName);
  for (std::size_t i = 0; i < stages.size(); i++)
  {
    bool isDefaultStage = stages[i] == "post";
    this->StagesSelection->AddArray(stages[i].c_str(), isDefaultStage);
  }

  // 'post' represents a symbolic links to the default stage, it will always be present.
  this->CurrentName = "/post";
  if (!this->IsValidERFDataset(fileId))
  {
    return 0;
  }

  std::vector<std::string> blocks = vtkHDF5Helper::GetChildren(fileId, this->CurrentName);
  for (const auto& block : blocks)
  {
    if (block == "multistate")
    {
      // unsupported for now, skip.
      continue;
    }

    if (block == "erfheader")
    {
      // As this block is mandatory, we always parse it so no need to add it here
      continue;
    }

    this->BlocksSelection->AddArray(block.c_str(), true);
  }

  // In order to retrieve data arrays, we need to list all subgroup of 'variable' (which
  // should be inside the group 'constant')
  std::string constantPath = vtkHDF5Helper::GetPathFromName(fileId, "/", "constant");
  if (constantPath.empty())
  {
    vtkWarningMacro(<< "Can't find group named 'constant' but it's required.");
    return 1;
  }

  std::string variablePath = vtkHDF5Helper::GetPathFromName(fileId, constantPath, "variable");
  if (variablePath.empty())
  {
    vtkWarningMacro(<< "Can't find group named 'variable'.");
    return 1;
  }

  std::vector<std::string> variables = vtkHDF5Helper::GetChildren(fileId, variablePath);
  for (const std::string& variable : variables)
  {
    // data array isn't case sensitive
    std::string lowercase = variable;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
      [](unsigned char c) { return std::tolower(c); });

    this->VariablesSelection->AddArray(lowercase.c_str(), true);
  }

  // Check optional temporal information
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (this->ExtractTemporalData(fileId))
  {
    this->AddTemporalInformation(outInfo);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkERFReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("No output information");
    return 0;
  }

  // Retrieve the potential timestep selected by user
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    this->CurrentTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    outInfo->Set(vtkDataObject::DATA_TIME_STEP(), this->CurrentTimeValue);
  }

  vtkPartitionedDataSetCollection* pdc = vtkPartitionedDataSetCollection::GetData(outInfo);
  if (!pdc)
  {
    vtkErrorMacro("No output data object");
    return 0;
  }

  this->AddTemporalInformationAsFieldData(pdc);

  return this->TraverseStage(pdc);
}

//----------------------------------------------------------------------------
bool vtkERFReader::ExtractTemporalData(const hid_t& rootIdx)
{
  std::string singlestatePath = this->CurrentName + "/singlestate";
  vtkHDF::ScopedH5GHandle singlestateHandle = H5Gopen(rootIdx, singlestatePath.c_str());
  if (singlestateHandle < 0)
  {
    // silently do nothing as its optional.
    return false;
  }

  std::vector<std::string> singletstates =
    vtkHDF5Helper::GetChildren(singlestateHandle, singlestatePath);

  this->States.resize(singletstates.size());
  this->TimeValues.resize(singletstates.size());

  vtkSmartPointer<vtkAbstractArray> array;
  for (std::size_t i = 0; i < singletstates.size(); i++)
  {
    std::string statePath = singlestatePath + "/" + singletstates[i] + "/";
    vtkHDF::ScopedH5GHandle stateHandle = H5Gopen(singlestateHandle, statePath.c_str());

    // 'indexident' store the state
    std::string indexidentPath =
      vtkHDF5Helper::GetPathFromName(stateHandle, statePath, "indexident");
    array = vtk::TakeSmartPointer(vtkHDF5Helper::CreateDataArray(stateHandle, indexidentPath));
    if (array)
    {
      // will always be a single double value
      this->States[i] = array->GetVariantValue(0).ToDouble();
    }

    // 'indexval' is for the time value
    std::string indexvalPath = vtkHDF5Helper::GetPathFromName(stateHandle, statePath, "indexval");
    array = vtk::TakeSmartPointer(vtkHDF5Helper::CreateDataArray(stateHandle, indexvalPath));
    if (array)
    {
      this->TimeValues[i] = array->GetVariantValue(0).ToDouble();
    }
  }

  bool hasTemporalData = !this->TimeValues.empty() || !this->States.empty();
  bool sameNumberOfStateThanTimeValue = this->TimeValues.size() == this->States.size();

  return hasTemporalData || sameNumberOfStateThanTimeValue;
}

//----------------------------------------------------------------------------
void vtkERFReader::AddTemporalInformation(vtkInformation* info)
{
  // removed old timesteps
  info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeValues.data(),
    static_cast<int>(this->TimeValues.size()));

  this->TimeRanges[0] = this->TimeValues[0];
  this->TimeRanges[1] = this->TimeValues[this->TimeValues.size() - 1];
  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_RANGE(), static_cast<double*>(this->TimeRanges), 2);
}

//----------------------------------------------------------------------------
bool vtkERFReader::IsValidERFDataset(const hid_t& fileId) const
{
  std::string systemPath = this->CurrentName + "/constant/system";
  vtkHDF::ScopedH5GHandle systemHandle = H5Gopen(fileId, systemPath.c_str());
  if (systemHandle < 0)
  {
    vtkErrorMacro("Missing system group which is mandatory.");
    return false;
  }

  std::string erfHeaderPath = this->CurrentName + "/erfheader";
  vtkHDF::ScopedH5GHandle erfHeaderHandle = H5Gopen(fileId, erfHeaderPath.c_str());
  if (erfHeaderHandle < 0)
  {
    vtkErrorMacro("Missing erf header group which is mandatory.");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
int vtkERFReader::TraverseStage(vtkPartitionedDataSetCollection* pdc)
{
  this->CurrentName = this->GetStage();

  vtkHDF::ScopedH5FHandle fileId = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    vtkErrorMacro("Could not open ERF-HDF5 file '" << this->FileName << "'");
    return 0;
  }

  vtkNew<vtkDataAssembly> hierarchy;
  hierarchy->Initialize();
  pdc->SetDataAssembly(hierarchy);

  auto groups = hierarchy->AddNodes({ "Constant", "SingleState" });
  int meshNodeId = 0;
  int meshStartId = pdc->GetNumberOfPartitionedDataSets();

  // Treat 'constant' group
  this->AppendConstantGroupData(pdc, fileId);
  this->AppendMeshs(pdc, hierarchy, groups[0], meshNodeId, meshStartId);

  // Treat 'singlestate' group
  meshStartId = pdc->GetNumberOfPartitionedDataSets();
  this->AppendSinglestateGroupData(fileId);
  this->AppendMeshs(pdc, hierarchy, groups[1], meshNodeId, meshStartId);

  return 1;
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendMeshs(vtkPartitionedDataSetCollection* pdc, vtkDataAssembly* hierarchy,
  int& streamNodeId, int& meshNodeId, int& meshStartId)
{
  // Append unstructured grid
  for (auto it = this->Meshs.begin(); it != this->Meshs.end(); it++)
  {
    vtkUnstructuredGrid* mesh = it->second;
    if (!mesh)
    {
      continue;
    }

    std::string fullPath = it->first;
    meshNodeId = hierarchy->AddNode(fullPath.c_str(), streamNodeId);
    meshStartId = pdc->GetNumberOfPartitionedDataSets();
    pdc->SetNumberOfPartitionedDataSets(meshStartId + 1);

    pdc->SetPartition(meshStartId, 0, mesh);
    pdc->GetMetaData(meshStartId)->Set(vtkCompositeDataSet::NAME(), fullPath.c_str());
    hierarchy->AddDataSetIndex(meshNodeId, meshStartId);
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendConstantGroupData(vtkPartitionedDataSetCollection* pdc, hid_t fileId)
{
  // reset state
  this->MeshPoints.clear();
  this->Meshs.clear();

  this->AppendMandatoryBlock(pdc, fileId);
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendSinglestateGroupData(hid_t fileId)
{
  std::string singlestatePath = vtkHDF5Helper::GetPathFromName(fileId, "/", "singlestate");
  vtkHDF::ScopedH5GHandle singlestateHandle = H5Gopen(fileId, singlestatePath.c_str());
  if (singlestateHandle < 0)
  {
    // silently do nothing as its optional.
    return;
  }
  int stateSelected = this->GetTimeValuesIndex();
  std::vector<std::string> allStatesName =
    vtkHDF5Helper::GetChildren(singlestateHandle, singlestatePath);

  this->CurrentName = singlestatePath + allStatesName[stateSelected];

  this->MeshPoints.clear();
  this->Meshs.clear();
  this->AppendSinglestateBlock(fileId);
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendMandatoryBlock(
  vtkPartitionedDataSetCollection* output, const hid_t& fileId)
{
  std::string root = "/";
  std::string initialPath = root + this->CurrentName;
  this->CurrentName = initialPath + "/constant/system/erfblock";
  vtkHDF::ScopedH5GHandle systemHandle = H5Gopen(fileId, this->CurrentName.c_str());
  if (systemHandle < 0)
  {
    vtkErrorMacro("Missing 'erfblock' inside the 'System' group.");
    return;
  }
  this->AppendSystemBlock(output, systemHandle);

  this->CurrentName = initialPath + "/erfheader";
  vtkHDF::ScopedH5GHandle erfHeaderHandle = H5Gopen(fileId, this->CurrentName.c_str());
  if (erfHeaderHandle < 0)
  {
    vtkErrorMacro("Missing 'erfheader' inside the 'Constant' group.");
    return;
  }
  this->AppendERFHeaderBlock(output, erfHeaderHandle);

  // All information at this stage can be optional
  // Now the 'Constant' Block
  // recreate the mesh
  this->CurrentName = initialPath + "/constant";
  vtkHDF::ScopedH5GHandle constantHandle = H5Gopen(fileId, this->CurrentName.c_str());
  if (constantHandle >= 0)
  {
    this->BuildMesh(constantHandle);
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendSinglestateBlock(const hid_t& stateId)
{
  vtkHDF::ScopedH5GHandle singlestateHandle = H5Gopen(stateId, this->CurrentName.c_str());
  if (singlestateHandle < 0)
  {
    return;
  }

  this->BuildMesh(singlestateHandle);
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendSystemBlock(
  vtkPartitionedDataSetCollection* output, const hid_t& systemHandle)
{
  // Check attributes
  for (const auto& attribute : SYSTEM_ATTRIBUTES)
  {
    if (!H5Aexists(systemHandle, attribute))
    {
      vtkWarningMacro("Missing attribute '" << attribute << "'.");
      continue;
    }

    std::string value = this->GetAttributeValueAsStr(systemHandle, attribute);
    if (value.empty())
    {
      continue;
    }

    vtkNew<vtkStringArray> stringArr;
    stringArr->SetName(attribute);
    stringArr->SetNumberOfValues(1);
    stringArr->SetValue(0, value);
    output->GetFieldData()->AddArray(stringArr);
  }

  // Check Datasets
  std::string dataSetPath = "ubid";
  this->AppendFieldDataByName(output, systemHandle, dataSetPath);

  dataSetPath = "ubcon";
  this->AppendFieldDataByName(output, systemHandle, dataSetPath);

  dataSetPath = "ubnam";
  this->AppendFieldDataByName(output, systemHandle, dataSetPath);

  dataSetPath = "ubtyp";
  this->AppendFieldDataByName(output, systemHandle, dataSetPath);
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendERFHeaderBlock(
  vtkPartitionedDataSetCollection* output, const hid_t& erfHeaderHandle)
{
  // No attributes, only a dataset
  std::string dataSetPath = "erfheader";
  this->AppendFieldDataByName(output, erfHeaderHandle, dataSetPath);
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendFieldDataByName(
  vtkPartitionedDataSetCollection* pdc, const hid_t& id, const std::string& name)
{
  vtkSmartPointer<vtkAbstractArray> array;
  array = vtk::TakeSmartPointer(vtkHDF5Helper::CreateDataArray(id, this->CurrentName, name));
  if (array && pdc)
  {
    pdc->GetFieldData()->AddArray(array);
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::BuildMesh(const hid_t& fileId)
{
  vtkHDF::ScopedH5GHandle selectedStateHandle = H5Gopen(fileId, this->CurrentName.c_str());
  if (selectedStateHandle < 0)
  {
    return;
  }

  // Extract all points and point data.
  std::string entityresultsPath =
    vtkHDF5Helper::GetPathFromName(selectedStateHandle, this->CurrentName + "/", "entityresults");
  vtkHDF::ScopedH5GHandle entityresultsHandle =
    H5Gopen(selectedStateHandle, entityresultsPath.c_str());
  if (entityresultsHandle < 0)
  {
    return;
  }

  std::vector<std::string> allEntities =
    vtkHDF5Helper::GetChildren(selectedStateHandle, entityresultsPath);
  std::vector<std::string> remainingEntities;
  for (std::size_t i = 0; i < allEntities.size(); i++)
  {
    std::string entityPath = entityresultsPath + allEntities[i];
    vtkHDF::ScopedH5GHandle entityHandle = H5Gopen(entityresultsHandle, entityPath.c_str());
    if (entityHandle < 0)
    {
      return;
    }

    // 'entityresults' contains also cell data or point data that should be attached on mesh
    // produced by the 'constant' group, treated later.
    if (!vtkHDF5Helper::GroupExists(entityHandle, ::NODE_GROUP.c_str()))
    {
      remainingEntities.push_back(allEntities[i]);
      continue;
    }

    std::string coords = entityPath + "/" + ::NODE_GROUP;
    auto entityArrays = vtkHDF5Helper::GetChildren(entityHandle, entityPath);

    vtkNew<vtkUnstructuredGrid> mesh;
    for (std::size_t arrIdx = 0; arrIdx < entityArrays.size(); arrIdx++)
    {
      std::string fullPath =
        vtkHDF5Helper::GetPathFromName(entityHandle, entityPath + "/", "erfblock");
      if (entityArrays[arrIdx] == ::NODE_GROUP)
      {
        vtkHDF::ScopedH5GHandle erfblockHandle = H5Gopen(entityHandle, fullPath.c_str());
        this->AppendPoints(mesh, fullPath, erfblockHandle);

        this->MeshPoints.insert(std::pair<std::string, vtkUnstructuredGrid*>(allEntities[i], mesh));

        continue;
      }

      std::string lowercase = entityArrays[arrIdx];
      std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
        [](unsigned char c) { return std::tolower(c); });

      if (this->VariablesSelection->GetEnabledArrayIndex(lowercase.c_str()) == -1)
      {
        continue;
      }

      fullPath = vtkHDF5Helper::GetPathFromName(
        entityHandle, entityPath + "/" + entityArrays[arrIdx] + "/", "erfblock");
      vtkHDF::ScopedH5GHandle erfblockHandle = H5Gopen(entityHandle, fullPath.c_str());
      auto array =
        vtk::TakeSmartPointer(vtkHDF5Helper::CreateDataArray(erfblockHandle, fullPath + "res"));

      if (array && mesh->GetPointData()->GetNumberOfTuples() == array->GetNumberOfTuples())
      {
        array->SetName(entityArrays[arrIdx].c_str());
        mesh->GetPointData()->AddArray(array);
      }
    }
  }

  // Extract all cells and cell data.
  std::string connectivitiesPath =
    vtkHDF5Helper::GetPathFromName(selectedStateHandle, this->CurrentName + "/", "connectivities");
  vtkHDF::ScopedH5GHandle connectivitiesHandle =
    H5Gopen(selectedStateHandle, connectivitiesPath.c_str());
  if (connectivitiesHandle < 0)
  {
    return;
  }

  std::vector<std::string> allConnectivities =
    vtkHDF5Helper::GetChildren(selectedStateHandle, connectivitiesPath);
  for (std::size_t i = 0; i < allConnectivities.size(); i++)
  {
    std::string connectivityPath = connectivitiesPath + "/" + allConnectivities[i];
    vtkHDF::ScopedH5GHandle connectivityHandle =
      H5Gopen(connectivitiesHandle, connectivityPath.c_str());
    if (connectivityHandle < 0)
    {
      continue;
    }

    // 'etypnode' is needed to know which point array is associated to the current connectivity
    // array
    std::string erfblockPath =
      vtkHDF5Helper::GetPathFromName(connectivityHandle, connectivityPath + "/", "erfblock");
    vtkHDF::ScopedH5GHandle erfblockHandle = H5Gopen(connectivityHandle, erfblockPath.c_str());
    if (erfblockHandle < 0)
    {
      continue;
    }

    std::string etypnode = this->GetAttributeValueAsStr(erfblockHandle, "etypnode");
    this->MeshPoints.insert({ allConnectivities[i], this->MeshPoints[etypnode] });
    this->AppendCells(this->MeshPoints[etypnode], erfblockPath, erfblockHandle);
    this->Meshs.insert({ allConnectivities[i], this->MeshPoints[etypnode] });
  }

  // Treat remaining data entities which will be data arrays
  for (std::size_t i = 0; i < remainingEntities.size(); i++)
  {
    std::string entityPath = entityresultsPath + "/" + remainingEntities[i];
    vtkHDF::ScopedH5GHandle entityHandle = H5Gopen(entityresultsHandle, entityPath.c_str());
    if (entityHandle < 0)
    {
      return;
    }

    auto entityArrays = vtkHDF5Helper::GetChildren(entityHandle, entityPath);

    for (std::size_t arrIdx = 0; arrIdx < entityArrays.size(); arrIdx++)
    {
      std::string fullPath =
        vtkHDF5Helper::GetPathFromName(entityHandle, entityPath + "/", "erfblock");
      vtkHDF::ScopedH5GHandle erfblockHandle = H5Gopen(entityHandle, fullPath.c_str());
      if (erfblockHandle < 0)
      {
        continue;
      }

      std::string etyp = this->GetAttributeValueAsStr(erfblockHandle, "etyp");
      auto array =
        vtk::TakeSmartPointer(vtkHDF5Helper::CreateDataArray(entityHandle, fullPath, "res"));
      vtkUnstructuredGrid* mesh = this->Meshs[etyp];
      if (!mesh || !array)
      {
        continue;
      }

      std::string lowercase = entityArrays[arrIdx];
      std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
        [](unsigned char c) { return std::tolower(c); });

      bool isSelected = this->VariablesSelection->GetArraySetting(lowercase.c_str());
      if (!isSelected)
      {
        continue;
      }

      array->SetName(entityArrays[arrIdx].c_str());
      if (mesh->GetCellData()->GetNumberOfTuples() == array->GetNumberOfTuples())
      {
        mesh->GetCellData()->AddArray(array);
      }
      else if (mesh->GetPointData()->GetNumberOfTuples() == array->GetNumberOfTuples())
      {
        mesh->GetPointData()->AddArray(array);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendPoints(
  vtkUnstructuredGrid* output, const std::string& nodeAttributePath, const hid_t& fileId)
{
  vtkNew<vtkPoints> points;
  output->SetPoints(points);
  points->SetDataTypeToDouble();

  // Retrieve attributes information about number of points
  vtkHDF::ScopedH5AHandle nentHandler = H5Aopen(fileId, "nent", H5P_DEFAULT);
  vtkHDF::ScopedH5THandle rawType = H5Aget_type(nentHandler);
  vtkHDF::ScopedH5THandle dataType = H5Tget_native_type(rawType, H5T_DIR_ASCEND);

  int numberOfPoints = 0;
  if (H5Aread(nentHandler, dataType, &numberOfPoints) < 0)
  {
    vtkWarningMacro("Can't retrieve 'nent' attributes on NODE");
  }

  points->SetNumberOfPoints(numberOfPoints);

  // Retrieve each point
  std::string resPath = nodeAttributePath + "/res";
  vtkHDF::ScopedH5DHandle arrayId = H5Dopen(fileId, resPath.c_str());
  if (arrayId < 0)
  {
    vtkWarningMacro("No array named " << resPath << " available");
    return;
  }

  vtkHDF::ScopedH5THandle nodeRawType = H5Dget_type(arrayId);
  vtkHDF::ScopedH5THandle nodeDataType = H5Tget_native_type(nodeRawType, H5T_DIR_ASCEND);

  if (H5Tequal(nodeDataType, H5T_NATIVE_DOUBLE))
  {
    std::vector<double> resData;
    resData.resize(numberOfPoints * 3);
    H5Dread(arrayId, nodeDataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, resData.data());
    for (int i = 0; i < numberOfPoints; i++)
    {
      double point[3] = { resData[i * 3], resData[i * 3 + 1], resData[i * 3 + 2] };
      points->SetPoint(i, point);
    }
  }

  // Then point data
  std::string erfPointId = "entid";
  vtkSmartPointer<vtkAbstractArray> array;
  array =
    vtk::TakeSmartPointer(vtkHDF5Helper::CreateDataArray(fileId, nodeAttributePath, erfPointId));
  if (array)
  {
    output->GetPointData()->AddArray(array);
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::AppendCells(
  vtkUnstructuredGrid* output, const std::string& shellAttributePath, const hid_t& fileId)
{
  if (!output)
  {
    return;
  }

  auto* entid = vtkIntArray::SafeDownCast(output->GetPointData()->GetArray("entid"));
  if (!entid)
  {
    vtkWarningMacro("Missing 'entid' point data array which is used to create cell by indice.");
  }

  if (entid->GetNumberOfValues() == 0)
  {
    vtkWarningMacro("'entid' point data array is empty, we can't recreate cell by indice.");
    return;
  }

  int numberOfDimensions = this->GetAttributeValueAsInt(fileId, "ndim");
  if (numberOfDimensions == VTK_INT_MIN)
  {
    return;
  }

  int numberOfIndicePerCell = this->GetAttributeValueAsInt(fileId, "npele");
  if (numberOfIndicePerCell == VTK_INT_MIN)
  {
    return;
  }

  if (!this->IsCellSupported(numberOfDimensions, numberOfIndicePerCell))
  {
    return;
  }

  int numberOfCell = this->GetAttributeValueAsInt(fileId, "nele");
  if (numberOfCell == VTK_INT_MIN)
  {
    return;
  }

  // Retrieve each cell indices
  std::string resPath = shellAttributePath + "/ic";
  vtkHDF::ScopedH5DHandle arrayId = H5Dopen(fileId, resPath.c_str());
  if (arrayId < 0)
  {
    vtkWarningMacro("No array named " << resPath << " available");
    return;
  }

  vtkHDF::ScopedH5THandle shellRawType = H5Dget_type(arrayId);
  vtkHDF::ScopedH5THandle shellDataType = H5Tget_native_type(shellRawType, H5T_DIR_ASCEND);

  if (!H5Tequal(shellDataType, H5T_NATIVE_INT))
  {
    vtkWarningMacro(<< "Can't retrieve the cell id array.");
    return;
  }

  vtkNew<vtkCellArray> cellArray;
  vtkNew<vtkUnsignedCharArray> cellTypes;

  this->FillCellsByType(cellArray, cellTypes, shellDataType, arrayId, entid, numberOfDimensions,
    numberOfIndicePerCell, numberOfCell);
  if (cellArray->GetNumberOfCells() > 0)
  {
    output->SetCells(cellTypes, cellArray);
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::FillCellsByType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes,
  hid_t shellDataType, hid_t arrayId, vtkIntArray* entid, int numberOfDimensions,
  int numberOfIndicePerCell, int numberOfCell)
{
  std::vector<int> resData;
  resData.resize(numberOfCell * numberOfIndicePerCell);
  H5Dread(arrayId, shellDataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, resData.data());

  switch (numberOfDimensions)
  {
    case 0:
      this->Fill0DCellType(
        cellArray, cellTypes, entid, resData, numberOfIndicePerCell, numberOfCell);
      break;
    case 1:
      this->Fill1DCellType(
        cellArray, cellTypes, entid, resData, numberOfIndicePerCell, numberOfCell);
      break;
    case 2:
      this->Fill2DCellType(
        cellArray, cellTypes, entid, resData, numberOfIndicePerCell, numberOfCell);
      break;
    case 3:
      this->Fill3DCellType(
        cellArray, cellTypes, entid, resData, numberOfIndicePerCell, numberOfCell);
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::Fill0DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes,
  vtkIntArray* entid, const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell)
{
  if (!this->Is0DCellSupported(numberOfIndicePerCell))
  {
    vtkWarningMacro(<< "Only vert cell is supported for 0D cell.");
    return;
  }

  for (int i = 0; i < numberOfCell; i++)
  {
    vtkIdType pointId1 = static_cast<vtkIdType>(entid->LookupValue(resData[i]));

    cellArray->InsertNextCell(1, &pointId1);
    cellTypes->InsertNextValue(VTK_VERTEX);
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::Fill1DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes,
  vtkIntArray* entid, const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell)
{
  if (!this->Is1DCellSupported(numberOfIndicePerCell))
  {
    vtkWarningMacro(<< "Only line cell is supported for 1D cell.");
    return;
  }

  for (int i = 0; i < numberOfCell; i++)
  {
    int pointId1 = entid->LookupValue(resData[i * 2]);
    int pointId2 = entid->LookupValue(resData[i * 2 + 1]);

    cellArray->InsertNextCell({ pointId1, pointId2 });
    cellTypes->InsertNextValue(VTK_LINE);
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::Fill2DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes,
  vtkIntArray* entid, const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell)
{
  if (!this->Is2DCellSupported(numberOfIndicePerCell))
  {
    vtkWarningMacro(<< "Only triangle and quad cell are supported for 2D cell.");
    return;
  }

  if (numberOfIndicePerCell == 3)
  {
    for (int i = 0; i < numberOfCell; i++)
    {
      int pointId1 = entid->LookupValue(resData[i * 3]);
      int pointId2 = entid->LookupValue(resData[i * 3 + 1]);
      int pointId3 = entid->LookupValue(resData[i * 3 + 2]);

      cellArray->InsertNextCell({ pointId1, pointId2, pointId3 });
      cellTypes->InsertNextValue(VTK_TRIANGLE);
    }
  }
  else
  {
    for (int i = 0; i < numberOfCell; i++)
    {
      int pointId1 = entid->LookupValue(resData[i * 4]);
      int pointId2 = entid->LookupValue(resData[i * 4 + 1]);
      int pointId3 = entid->LookupValue(resData[i * 4 + 2]);
      int pointId4 = entid->LookupValue(resData[i * 4 + 3]);

      cellArray->InsertNextCell({ pointId1, pointId2, pointId3, pointId4 });
      cellTypes->InsertNextValue(VTK_QUAD);
    }
  }
}

//----------------------------------------------------------------------------
void vtkERFReader::Fill3DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes,
  vtkIntArray* entid, const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell)
{
  if (!this->Is3DCellSupported(numberOfIndicePerCell))
  {
    vtkWarningMacro(<< "Only tetra, pyramid, penta and hex cell are supported for 3D cell.");
    return;
  }

  for (int i = 0; i < numberOfCell; i++)
  {
    // retrieve each point index for the current cell
    vtkNew<vtkIdList> pointIndices;
    pointIndices->SetNumberOfIds(numberOfIndicePerCell);
    for (int j = 0; j < numberOfIndicePerCell; j++)
    {
      int pointId = entid->LookupValue(resData[i * numberOfIndicePerCell + j]);
      pointIndices->SetId(j, pointId);
    }

    // then the correct cell type
    cellArray->InsertNextCell(pointIndices);
    switch (numberOfIndicePerCell)
    {
      case 4:
        cellTypes->InsertNextValue(VTK_TETRA);
        break;
      case 5:
        cellTypes->InsertNextValue(VTK_PYRAMID);
        break;
      case 6:
        cellTypes->InsertNextValue(VTK_PENTAGONAL_PRISM);
        break;
      case 8:
        cellTypes->InsertNextValue(VTK_HEXAHEDRON);
        break;
      default:
        break;
    }
  }
}

//----------------------------------------------------------------------------
std::string vtkERFReader::GetAttributeValueAsStr(
  const hid_t& erfIdx, const std::string& attributeName) const
{
  vtkHDF::ScopedH5AHandle attributeHandler = H5Aopen(erfIdx, attributeName.c_str(), H5P_DEFAULT);
  vtkHDF::ScopedH5THandle rawType = H5Aget_type(attributeHandler);
  vtkHDF::ScopedH5THandle dataType = H5Tget_native_type(rawType, H5T_DIR_ASCEND);

  if (H5Tget_class(dataType) != H5T_STRING)
  {
    return "";
  }

  hsize_t stringLength = H5Aget_storage_size(attributeHandler);
  std::string value;
  value.resize(stringLength + 1);
  // NOLINTNEXTLINE(readability-container-data-pointer)
  if (H5Aread(attributeHandler, dataType, &value[0]) >= 0)
  {
    value.erase(std::remove_if(value.begin(), value.end(),
                  [](char c)
                  {
                    // convert it to avoid potential issue with negative char
                    unsigned char uc = static_cast<unsigned char>(c);
                    return std::isspace(uc) || !std::isalpha(uc);
                  }),
      value.end());
  }

  return value;
}

//----------------------------------------------------------------------------
int vtkERFReader::GetAttributeValueAsInt(
  const hid_t& erfIdx, const std::string& attributeName) const
{
  int value = VTK_INT_MIN;
  vtkHDF::ScopedH5AHandle attrHandler = H5Aopen(erfIdx, attributeName.c_str(), H5P_DEFAULT);
  vtkHDF::ScopedH5THandle attrRawType = H5Aget_type(attrHandler);
  vtkHDF::ScopedH5THandle attrDataType = H5Tget_native_type(attrRawType, H5T_DIR_ASCEND);
  if (H5Aread(attrHandler, attrDataType, &value) < 0)
  {
    vtkWarningMacro("Can't retrieve '" << attributeName << "' attributes");
  }

  return value;
}

//----------------------------------------------------------------------------
bool vtkERFReader::IsCellSupported(int ndim, int npelem)
{
  switch (ndim)
  {
    case 0:
      return this->Is0DCellSupported(npelem);
    case 1:
      return this->Is1DCellSupported(npelem);
    case 2:
      return this->Is2DCellSupported(npelem);
    case 3:
      return this->Is3DCellSupported(npelem);
    default:
      vtkWarningMacro("unsupported cell dimension: " << ndim << ".");
      break;
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkERFReader::Is0DCellSupported(int npelem)
{
  bool isVert = npelem == 1;

  return isVert;
}

//----------------------------------------------------------------------------
bool vtkERFReader::Is1DCellSupported(int npelem)
{
  bool isLine = npelem == 2;

  return isLine;
}

//----------------------------------------------------------------------------
bool vtkERFReader::Is2DCellSupported(int npelem)
{
  bool isTriangle = npelem == 3;
  bool isQuad = npelem == 4;

  return isTriangle || isQuad;
}

//----------------------------------------------------------------------------
bool vtkERFReader::Is3DCellSupported(int npelem)
{
  bool isTetra = npelem == 4;
  bool isPyramid = npelem == 5;
  bool isPenta = npelem == 6;
  bool isHexa = npelem == 8;

  return isTetra || isPyramid || isPenta || isHexa;
}

//----------------------------------------------------------------------------
int vtkERFReader::GetTimeValuesIndex()
{
  return std::distance(this->TimeValues.begin(),
    find(this->TimeValues.begin(), this->TimeValues.end(), this->CurrentTimeValue));
}

// ----------------------------------------------------------------------------
void vtkERFReader::EnableAllVariables()
{
  this->VariablesSelection->EnableAllArrays();
}

//-----------------------------------------------------------------------------
void vtkERFReader::SetVariablesStatus(const char* name, int status)
{
  this->VariablesSelection->SetArraySetting(name, status);
}

// ----------------------------------------------------------------------------
void vtkERFReader::EnableAllBlocks()
{
  this->BlocksSelection->EnableAllArrays();
}

//-----------------------------------------------------------------------------
void vtkERFReader::SetBlocksStatus(const char* name, int status)
{
  this->BlocksSelection->SetArraySetting(name, status);
}

//-----------------------------------------------------------------------------
void vtkERFReader::SetStagesStatus(const char* name, int status)
{
  if (status)
  {
    this->StagesSelection->DisableAllArrays();
    this->StagesSelection->EnableArray(name);
  }
  else
  {
    this->StagesSelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
std::string vtkERFReader::GetStage() const
{
  std::string stage;
  for (int i = 0; i < this->StagesSelection->GetNumberOfArrays(); i++)
  {
    // there is only one stage enabled
    if (this->StagesSelection->GetArraySetting(i))
    {
      stage = this->StagesSelection->GetArrayName(i);
      break;
    }
  }

  return stage;
}

//----------------------------------------------------------------------------
void vtkERFReader::AddTemporalInformationAsFieldData(vtkPartitionedDataSetCollection* pdc)
{
  if (!pdc || this->TimeValues.empty() || this->States.empty())
  {
    return;
  }

  bool hasAlreadyTimeValues = pdc->GetFieldData()->HasArray("Time Values");
  bool hasAlreadyStates = pdc->GetFieldData()->HasArray("States");
  if (hasAlreadyStates || hasAlreadyTimeValues)
  {
    return;
  }

  vtkNew<vtkDoubleArray> timeValues;
  timeValues->SetName("Time Values");
  timeValues->SetNumberOfValues(this->TimeValues.size());
  timeValues->SetVoidArray(static_cast<void*>(this->TimeValues.data()),
    static_cast<vtkIdType>(this->TimeValues.size()), 1);

  pdc->GetFieldData()->AddArray(timeValues);

  vtkNew<vtkIntArray> states;
  states->SetName("States");
  states->SetNumberOfValues(this->States.size());
  states->SetVoidArray(
    static_cast<void*>(this->States.data()), static_cast<vtkIdType>(this->States.size()), 1);

  pdc->GetFieldData()->AddArray(states);
}

//----------------------------------------------------------------------------
void vtkERFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName.empty() ? this->FileName : "(none)") << "\n";
  os << indent << "CurrentName: " << this->CurrentName << "\n";
}

VTK_ABI_NAMESPACE_END
