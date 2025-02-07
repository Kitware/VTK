// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNetCDFUGRIDReader.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkUnstructuredGrid.h"

#include <array>

#include <vtk_netcdf.h>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkNetCDFUGRIDReader);

//--------------------------------------------------------------------------------------------------
vtkNetCDFUGRIDReader::vtkNetCDFUGRIDReader()
  : PointDataArraySelection{ vtkSmartPointer<vtkDataArraySelection>::New() }
  , CellDataArraySelection{ vtkSmartPointer<vtkDataArraySelection>::New() }
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//--------------------------------------------------------------------------------------------------
vtkNetCDFUGRIDReader::~vtkNetCDFUGRIDReader()
{
  this->Close();
}

//--------------------------------------------------------------------------------------------------
int vtkNetCDFUGRIDReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->Open())
  {
    vtkErrorMacro("Can not open file.");
    return 0;
  }

  if (!this->ParseHeader())
  {
    vtkErrorMacro("Can not parse header.");
    this->Close();
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // look for the "time" dimension in the top level block, it should contain timestep count
  // of data arrays.
  int timeDimId{};
  if (nc_inq_dimid(this->NcId, "time", &timeDimId) == NC_NOERR)
  {
    std::size_t timeStepCount{};
    if (!this->CheckError(nc_inq_dimlen(this->NcId, timeDimId, &timeStepCount)))
    {
      this->Close();
      return 0;
    }

    int timeVarId{};
    if (!this->CheckError(nc_inq_varid(this->NcId, "time", &timeVarId)))
    {
      vtkErrorMacro("`time` dimension is defined, but `time` variable is not.");
      this->Close();
      return 0;
    }

    this->TimeSteps.resize(timeStepCount);
    if (!this->CheckError(nc_get_var_double(this->NcId, timeVarId, this->TimeSteps.data())))
    {
      this->Close();
      return 0;
    }

    const std::array<double, 2> range = { this->TimeSteps.front(), this->TimeSteps.back() };

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps.data(),
      static_cast<int>(this->TimeSteps.size()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range.data(), 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  this->Close();

  return 1;
}

//--------------------------------------------------------------------------------------------------
int vtkNetCDFUGRIDReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  this->UpdateProgress(0.0);

  if (!this->Open())
  {
    vtkErrorMacro("Can not open file.");
    return 0;
  }

  this->UpdateProgress(0.125);

  if (!this->ParseHeader())
  {
    vtkErrorMacro("Can not parse header.");
    this->Close();
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformationDoubleKey* timeKey =
    vtkInformationDoubleKey::SafeDownCast(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double time{};
  if (outInfo->Has(timeKey))
  {
    time = outInfo->Get(timeKey);
  }

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), time);

  std::size_t timeStep{};
  for (std::size_t step{}; step < this->TimeSteps.size(); step++)
  {
    if (this->TimeSteps[step] >= time)
    {
      timeStep = step;
      break;
    }
  }

  this->UpdateProgress(0.25);

  if (!this->FillPoints(output))
  {
    vtkErrorMacro("Failed to get point data");
    this->Close();
    return 0;
  }

  this->UpdateProgress(0.5);

  if (!this->FillCells(output))
  {
    vtkErrorMacro("Failed to get cell data");
    this->Close();
    return 0;
  }

  this->UpdateProgress(0.75);

  if (!this->FillArrays(output, timeStep))
  {
    vtkErrorMacro("Failed to get array data");
    this->Close();
    return 0;
  }

  this->UpdateProgress(1.0);

  this->Close();

  return 1;
}

//--------------------------------------------------------------------------------------------------
int vtkNetCDFUGRIDReader::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  return 1;
}

//--------------------------------------------------------------------------------------------------
int vtkNetCDFUGRIDReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//--------------------------------------------------------------------------------------------------
int vtkNetCDFUGRIDReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//--------------------------------------------------------------------------------------------------
const char* vtkNetCDFUGRIDReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//--------------------------------------------------------------------------------------------------
const char* vtkNetCDFUGRIDReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//--------------------------------------------------------------------------------------------------
int vtkNetCDFUGRIDReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->GetArraySetting(name);
}

//--------------------------------------------------------------------------------------------------
int vtkNetCDFUGRIDReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->GetArraySetting(name);
}

//--------------------------------------------------------------------------------------------------
void vtkNetCDFUGRIDReader::SetPointArrayStatus(const char* name, int status)
{
  this->PointDataArraySelection->SetArraySetting(name, status);
  this->Modified();
}

//--------------------------------------------------------------------------------------------------
void vtkNetCDFUGRIDReader::SetCellArrayStatus(const char* name, int status)
{
  this->CellDataArraySelection->SetArraySetting(name, status);
  this->Modified();
}

//--------------------------------------------------------------------------------------------------
bool vtkNetCDFUGRIDReader::Open()
{
  if (!this->FileName)
  {
    vtkErrorMacro("No filename specified.");
    return false;
  }

  this->Close();

  int id{};
  int error{ nc_open(this->FileName, 0, &id) };
  if (error != NC_NOERR)
  {
    vtkErrorMacro("Failed to open file \"" << this->FileName << "\": " << nc_strerror(error));
    return false;
  }

  this->NcId = id;

  return true;
}

//--------------------------------------------------------------------------------------------------
bool vtkNetCDFUGRIDReader::ParseHeader()
{
  int varCount{};
  if (!this->CheckError(nc_inq_nvars(this->NcId, &varCount)))
  {
    return false;
  }

  if (varCount == 0)
  {
    vtkErrorMacro("No variable defined in file.");
    return false;
  }

  std::vector<int> vars{};
  vars.resize(static_cast<std::size_t>(varCount));
  if (!this->CheckError(nc_inq_varids(this->NcId, &varCount, vars.data())))
  {
    return false;
  }

  std::vector<int> meshIds{};
  std::vector<int> faceIds{};
  std::vector<int> nodeIds{};

  for (int var : vars)
  {
    int attCount{};
    if (!this->CheckError(nc_inq_varnatts(this->NcId, var, &attCount)))
    {
      return false;
    }

    for (int i = 0; i < attCount; ++i)
    {
      const auto name(this->GetAttributeName(var, i));

      nc_type type{ NC_NAT };
      if (!this->CheckError(nc_inq_atttype(this->NcId, var, name.c_str(), &type)))
      {
        return false;
      }

      if (type != NC_CHAR)
      {
        continue;
      }

      // if the cf_role attribute is "mesh_topology" then the var is a mesh
      if (name == "cf_role")
      {
        const auto value = this->GetAttributeString(var, name);
        if (value == "mesh_topology")
        {
          meshIds.emplace_back(var);
        }
      }
      else if (name == "location")
      {
        // location attribute tells us if this data is associated to cells or points
        const auto value = this->GetAttributeString(var, name);
        if (value == "face")
        {
          faceIds.emplace_back(var);
        }
        else if (value == "node")
        {
          nodeIds.emplace_back(var);
        }
      }
    }
  }

  if (meshIds.empty())
  {
    vtkErrorMacro("File does not contain a mesh");
    return false;
  }

  if (meshIds.size() > 1)
  {
    vtkWarningMacro("Multi-meshes is not supported. Only the first mesh will be read.");
  }

  this->FaceArrayVarIds = std::move(faceIds);
  this->NodeArrayVarIds = std::move(nodeIds);

  if (!this->FillArraySelection(this->FaceArrayVarIds, this->CellDataArraySelection))
  {
    return false;
  }

  if (!this->FillArraySelection(this->NodeArrayVarIds, this->PointDataArraySelection))
  {
    return false;
  }

  this->MeshVarId = meshIds[0]; // only single mesh is supported
  int topologyDimension{};

  if (!this->CheckError(
        nc_get_att_int(this->NcId, this->MeshVarId, "topology_dimension", &topologyDimension)))
  {
    vtkErrorMacro(
      "Invalid mesh #" << this->MeshVarId << ". Missing required attribute topology_dimension");
    return false;
  }

  if (topologyDimension != 2)
  {
    vtkErrorMacro("Unsupported topology dimension " << topologyDimension);
    return false;
  }

  // face_node_connectivity variable contains the cells
  const std::string faceVarName{ this->GetAttributeString(
    this->MeshVarId, "face_node_connectivity") };

  if (!this->CheckError(nc_inq_varid(this->NcId, faceVarName.c_str(), &this->FaceVarId)))
  {
    return false;
  }

  int faceDimCount{};
  if (!this->CheckError(nc_inq_varndims(this->NcId, FaceVarId, &faceDimCount)))
  {
    return false;
  }

  if (faceDimCount != 2)
  {
    vtkErrorMacro("face_node_connectivity must be a two dimension array");
    return false;
  }

  std::array<int, 2> faceDimIds{};
  if (!this->CheckError(nc_inq_vardimid(this->NcId, FaceVarId, faceDimIds.data())))
  {
    return false;
  }

  std::vector<std::size_t> faceDimSize{};
  faceDimSize.resize(faceDimCount);
  for (std::size_t i = 0; i < faceDimSize.size(); ++i)
  {
    if (!this->CheckError(nc_inq_dimlen(this->NcId, faceDimIds[i], &faceDimSize[i])))
    {
      return false;
    }
  }

  // cells data may be either an array of int[cellcount][cellsize] (default) or
  // int[cellsize][cellcount] face_dimension variable helps us disambiguate by telling us which one
  // is `cellcount`
  int faceDimId{};
  if (nc_inq_attid(this->NcId, this->MeshVarId, "face_dimension", &faceDimId) != NC_NOERR)
  {
    this->FaceCount = faceDimSize[0];
    this->NodesPerFace = faceDimSize[1];
    this->FaceStride = this->NodesPerFace;
    this->NodesPerFaceStride = 1;
  }
  else
  {
    const auto name(this->GetDimensionName(faceDimIds[0]));
    if (this->GetAttributeString(this->MeshVarId, "face_dimension") == name)
    {
      this->FaceCount = faceDimSize[0];
      this->NodesPerFace = faceDimSize[1];
      this->FaceStride = this->NodesPerFace;
      this->NodesPerFaceStride = 1;
    }
    else
    {
      this->FaceCount = faceDimSize[1];
      this->NodesPerFace = faceDimSize[0];
      this->FaceStride = 1;
      this->NodesPerFaceStride = this->FaceCount;
    }
  }

  // node_coordinates attributes help us get the 2 vars that correspond to x and y
  const std::string nodeVarName{ this->GetAttributeString(this->MeshVarId, "node_coordinates") };
  const std::string nodeXVarName{ nodeVarName.begin(),
    std::find(nodeVarName.begin(), nodeVarName.end(), ' ') };
  const std::string nodeYVarName{ std::find(nodeVarName.rbegin(), nodeVarName.rend(), ' ').base(),
    nodeVarName.end() };

  if (!this->CheckError(nc_inq_varid(this->NcId, nodeXVarName.c_str(), &this->NodeXVarId)))
  {
    vtkErrorMacro("X array \"" << nodeXVarName.c_str() << "\" is undefined.");
    return false;
  }

  if (!this->CheckError(nc_inq_varid(this->NcId, nodeYVarName.c_str(), &this->NodeYVarId)))
  {
    vtkErrorMacro("Y array \"" << nodeYVarName.c_str() << "\" is undefined.");
    return false;
  }

  int nodeDimCount{};
  if (!this->CheckError(nc_inq_varndims(this->NcId, this->NodeXVarId, &nodeDimCount)))
  {
    return false;
  }

  std::vector<int> nodeXDimIds{};
  nodeXDimIds.resize(nodeDimCount);
  if (!this->CheckError(nc_inq_vardimid(this->NcId, this->NodeXVarId, nodeXDimIds.data())))
  {
    return false;
  }

  if (!this->CheckError(nc_inq_dimlen(this->NcId, nodeXDimIds[0], &this->NodeCount)))
  {
    return false;
  }

  if (this->NodesPerFace > 3) // may be mixed mesh
  {
    if (!this->CheckError(
          nc_get_att(this->NcId, this->FaceVarId, "_FillValue", &this->FaceFillValue)))
    {
      vtkErrorMacro("_FillValue attribute missing - The connectivity variable has to specify a "
                    "_FillValue attribute because it has more than 3 nodes per face");
      return false;
    }
  }

  if (nc_get_att(this->NcId, this->FaceVarId, "start_index", &this->FaceStartIndex) != NC_NOERR)
  {
    this->FaceStartIndex = 0;
  }

  if (!this->CheckError(nc_inq_vartype(this->NcId, this->NodeXVarId, &this->NodeType)))
  {
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
bool vtkNetCDFUGRIDReader::FillArraySelection(
  const std::vector<int>& varIds, vtkDataArraySelection* selection)
{
  for (int var : varIds)
  {
    selection->AddArray(this->GetVariableName(var).c_str());
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
struct PointsExtractor
{
  template <typename OutArray>
  void operator()(OutArray* output, int NcId, int xVar, int yVar, int& result)
  {
    using T = vtk::GetAPIType<OutArray>;

    std::vector<T> x{};
    x.resize(output->GetNumberOfTuples());
    std::vector<T> y{};
    y.resize(output->GetNumberOfTuples());

    result = nc_get_var(NcId, xVar, x.data());
    if (result != NC_NOERR)
    {
      return;
    }

    result = nc_get_var(NcId, yVar, y.data());
    if (result != NC_NOERR)
    {
      return;
    }

    auto range = vtk::DataArrayTupleRange<3>(output);
    for (vtkIdType i = 0; i < output->GetNumberOfTuples(); ++i)
    {
      range[i][0] = x[i];
      range[i][1] = y[i];
      range[i][2] = 0.0f;
    }
  }
};

//--------------------------------------------------------------------------------------------------
bool vtkNetCDFUGRIDReader::FillPoints(vtkUnstructuredGrid* output)
{
  vtkNew<vtkPoints> points;

  if (this->NodeType == NC_FLOAT)
  {
    points->SetDataTypeToFloat();
  }
  else if (this->NodeType == NC_DOUBLE)
  {
    points->SetDataTypeToDouble();
  }
  else
  {
    vtkErrorMacro("Invalid mesh has nodes that are not floating point values");
    return false;
  }

  points->SetNumberOfPoints(this->NodeCount);

  PointsExtractor worker{};
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  int result{ NC_NOERR };
  Dispatcher::Execute(
    points->GetData(), worker, this->NcId, this->NodeXVarId, this->NodeYVarId, result);

  if (!this->CheckError(result))
  {
    return false;
  }

  output->SetPoints(points);

  return true;
}

//--------------------------------------------------------------------------------------------------
bool vtkNetCDFUGRIDReader::FillCells(vtkUnstructuredGrid* output)
{
  std::vector<int> faces{};
  faces.resize(this->NodesPerFace * this->FaceCount);
  if (!this->CheckError(nc_get_var(this->NcId, this->FaceVarId, faces.data())))
  {
    return false;
  }

  output->Allocate(this->FaceCount);

  std::vector<vtkIdType> pointIds{};
  pointIds.resize(this->NodesPerFace);
  for (std::size_t i = 0; i < this->FaceCount; ++i)
  {
    VTKCellType cell_type{ VTK_TRIANGLE };
    vtkIdType point_count{ 3 };

    for (std::size_t j = 0; j < this->NodesPerFace; ++j)
    {
      const vtkIdType id = faces[j * this->NodesPerFaceStride + i * this->FaceStride];

      if (this->NodesPerFace > 3 && id == this->FaceFillValue)
      {
        cell_type = VTK_TRIANGLE;
        point_count = 3;
        continue;
      }
      else if (this->NodesPerFace > 3)
      {
        cell_type = VTK_QUAD;
        point_count = 4;
      }
      else
      {
        cell_type = VTK_TRIANGLE;
        point_count = 3;
      }

      pointIds[j] = id - this->FaceStartIndex;
    }

    output->InsertNextCell(cell_type, point_count, pointIds.data());
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
bool vtkNetCDFUGRIDReader::FillArrays(vtkUnstructuredGrid* output, std::size_t timeStep)
{
  for (std::size_t i = 0; i < this->FaceArrayVarIds.size(); ++i)
  {
    if (!this->CellDataArraySelection->GetArraySetting(static_cast<int>(i)))
    {
      continue;
    }

    const auto array(this->GetArrayData(this->FaceArrayVarIds[i], timeStep, this->FaceCount));
    if (!array)
    {
      return false;
    }

    output->GetCellData()->AddArray(array);
  }

  for (std::size_t i = 0; i < this->NodeArrayVarIds.size(); ++i)
  {
    if (!this->PointDataArraySelection->GetArraySetting(static_cast<int>(i)))
    {
      continue;
    }

    const auto array(this->GetArrayData(this->NodeArrayVarIds[i], timeStep, this->NodeCount));
    if (!array)
    {
      return false;
    }

    output->GetPointData()->AddArray(array);
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
void vtkNetCDFUGRIDReader::Close()
{
  if (this->NcId == -1) // not opened
  {
    return;
  }

  int error{ nc_close(this->NcId) };
  if (error != NC_NOERR)
  {
    vtkErrorMacro("Failed to close file, memory may leak: " << nc_strerror(error));
  }

  this->NcId = -1;
}

//--------------------------------------------------------------------------------------------------
bool vtkNetCDFUGRIDReader::CheckError(int error)
{
  if (error != NC_NOERR)
  {
    vtkErrorMacro(
      "Failed to read information of file \"" << this->FileName << "\": " << nc_strerror(error));
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
std::string vtkNetCDFUGRIDReader::GetVariableName(int var)
{
  std::array<char, NC_MAX_NAME> buffer{};

  if (!this->CheckError(nc_inq_varname(this->NcId, var, buffer.data())))
  {
    vtkErrorMacro("Can not query var #" << var << " name");
    return {};
  }

  return std::string{ buffer.data() };
}

//--------------------------------------------------------------------------------------------------
std::string vtkNetCDFUGRIDReader::GetAttributeName(int var, int att)
{
  std::array<char, NC_MAX_NAME> buffer{};

  if (!this->CheckError(nc_inq_attname(this->NcId, var, att, buffer.data())))
  {
    vtkErrorMacro("Can not query var #" << var << "'s att #" << att << "name");
    return {};
  }

  return std::string{ buffer.data() };
}

//--------------------------------------------------------------------------------------------------
std::string vtkNetCDFUGRIDReader::GetDimensionName(int dim)
{
  std::array<char, NC_MAX_NAME> buffer{};

  if (!this->CheckError(nc_inq_dimname(this->NcId, dim, buffer.data())))
  {
    vtkErrorMacro("Can not query dim #" << dim << " name");
    return {};
  }

  return std::string{ buffer.data() };
}

//--------------------------------------------------------------------------------------------------
std::string vtkNetCDFUGRIDReader::GetAttributeString(int var, std::string name)
{
  std::size_t size{};
  if (!this->CheckError(nc_inq_attlen(this->NcId, var, name.c_str(), &size)))
  {
    vtkErrorMacro("Invalid mesh #" << var << ". Missing attribute " << name);
    return "";
  }

  std::string output{};
  output.resize(size);
  // NOLINTNEXTLINE(readability-container-data-pointer)
  if (!this->CheckError(nc_get_att_text(this->NcId, var, name.c_str(), &output[0])))
  {
    vtkErrorMacro("Invalid mesh #" << var << ". Missing attribute " << name);
    return "";
  }

  return output;
}

//--------------------------------------------------------------------------------------------------
static vtkSmartPointer<vtkDataArray> MakeDataArray(nc_type type)
{
  switch (type)
  {
    case NC_BYTE:
      return vtkSmartPointer<vtkSignedCharArray>::New();
    case NC_CHAR:
      return vtkSmartPointer<vtkCharArray>::New();
    case NC_SHORT:
      return vtkSmartPointer<vtkShortArray>::New();
    case NC_INT:
      return vtkSmartPointer<vtkIntArray>::New();
    case NC_FLOAT:
      return vtkSmartPointer<vtkFloatArray>::New();
    case NC_DOUBLE:
      return vtkSmartPointer<vtkDoubleArray>::New();
    case NC_UBYTE:
      return vtkSmartPointer<vtkUnsignedCharArray>::New();
    case NC_USHORT:
      return vtkSmartPointer<vtkUnsignedShortArray>::New();
    case NC_UINT:
      return vtkSmartPointer<vtkUnsignedIntArray>::New();
    case NC_INT64:
      return vtkSmartPointer<vtkLongLongArray>::New();
    case NC_UINT64:
      return vtkSmartPointer<vtkUnsignedLongLongArray>::New();
    default:
      return nullptr;
  }
}

//--------------------------------------------------------------------------------------------------
struct DataArrayExtractor
{
  template <typename OutArray>
  void operator()(OutArray* output, int NcId, int var, std::size_t time, std::size_t size,
    bool isTemporal, bool replaceFill, int& result)
  {
    using T = vtk::GetAPIType<OutArray>;

    output->SetNumberOfComponents(1);
    output->SetNumberOfTuples(size);

    if (isTemporal)
    {
      const std::array<std::size_t, 2> start{ time, 0 };
      const std::array<std::size_t, 2> count{ 1, size };
      result = nc_get_vara(NcId, var, start.data(), count.data(), output->GetPointer(0));
    }
    else
    {
      const std::array<std::size_t, 1> start{ 0 };
      const std::array<std::size_t, 1> count{ size };
      result = nc_get_vara(NcId, var, start.data(), count.data(), output->GetPointer(0));
    }

    if (result != NC_NOERR)
    {
      return;
    }

    if (replaceFill && (std::is_same<T, float>::value || std::is_same<T, double>::value))
    {
      T fillValue{};
      if (nc_get_att(NcId, var, "_FillValue", &fillValue) != NC_NOERR)
      {
        vtkDebugWithObjectMacro(output, "No fill value defined");
        return;
      }

      auto range(vtk::DataArrayValueRange(output));
      std::replace(range.begin(), range.end(), fillValue, static_cast<T>(vtkMath::Nan()));
    }
  }
};

//--------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkNetCDFUGRIDReader::GetArrayData(
  int var, std::size_t time, std::size_t size)
{
  nc_type type{ NC_NAT };
  if (!this->CheckError(nc_inq_vartype(this->NcId, var, &type)))
  {
    return nullptr;
  }

  DataArrayExtractor worker{};
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  auto output(MakeDataArray(type));
  if (!output)
  {
    vtkErrorMacro("Unsupported data array type");
    return nullptr;
  }

  output->SetName(this->GetVariableName(var).c_str());

  // Check if variable is time-dependent
  int varDimCount{};
  if (!this->CheckError(nc_inq_varndims(this->NcId, var, &varDimCount)))
  {
    vtkErrorMacro(
      "Could not obtain number of dimensions for variable " << this->GetVariableName(var));
    return nullptr;
  }
  const bool isTemporal = (varDimCount > 1);

  int result{ NC_NOERR };
  Dispatcher::Execute(
    output, worker, this->NcId, var, time, size, isTemporal, this->ReplaceFillValueWithNan, result);

  if (!this->CheckError(result))
  {
    return nullptr;
  }

  return output;
}

//--------------------------------------------------------------------------------------------------
void vtkNetCDFUGRIDReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Filename  : " << this->FileName << '\n';
  os << indent << "Node count : " << this->NodeCount << '\n';
  os << indent << "Face count : " << this->FaceCount << '\n';
  os << indent << "Face fill value : " << this->FaceFillValue << '\n';
  os << indent << "Face start index : " << this->FaceStartIndex << '\n';
  os << indent << "Max node per face : " << this->NodesPerFace << '\n';

  os << indent << "Timesteps: " << '\n';
  for (std::size_t i = 0; i < this->TimeSteps.size(); ++i)
  {
    os << indent << "  #" << i << ": " << this->TimeSteps[i] << '\n';
  }

  os << indent << "Point data arrays: " << '\n';
  for (int i = 0; i < this->PointDataArraySelection->GetNumberOfArrays(); ++i)
  {
    os << indent << "  #" << i << ": " << this->PointDataArraySelection->GetArrayName(i) << '\n';
  }

  os << indent << "Cell data arrays: " << '\n';
  for (int i = 0; i < this->CellDataArraySelection->GetNumberOfArrays(); ++i)
  {
    os << indent << "  #" << i << ": " << this->CellDataArraySelection->GetArrayName(i) << '\n';
  }
}

VTK_ABI_NAMESPACE_END
