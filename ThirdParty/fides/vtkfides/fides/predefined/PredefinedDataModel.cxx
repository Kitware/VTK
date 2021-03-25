//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FidesTypes.h>
#include <fides/predefined/DataModelFactory.h>
#include <fides/predefined/DataModelHelperFunctions.h>
#include <fides/predefined/InternalMetadataSource.h>
#include <fides/predefined/PredefinedDataModel.h>
#include <fides/predefined/SupportedDataModels.h>

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
#include FIDES_RAPIDJSON(rapidjson/prettywriter.h)
#include FIDES_RAPIDJSON(rapidjson/stringbuffer.h)
// clang-format on

#include <iostream>

namespace fides
{
namespace predefined
{

namespace
{

// Registration callbacks for DataModelFactory
std::shared_ptr<PredefinedDataModel> CreateUniform(std::shared_ptr<InternalMetadataSource> source)
{
  return std::shared_ptr<PredefinedDataModel>(new UniformDataModel(source));
}

std::shared_ptr<PredefinedDataModel> CreateUniformFromDataSet(const vtkm::cont::DataSet& dataSet)
{
  return std::shared_ptr<PredefinedDataModel>(new UniformDataModel(dataSet));
}

std::shared_ptr<PredefinedDataModel> CreateRectilinearFromDataSet(
  const vtkm::cont::DataSet& dataSet)
{
  return std::shared_ptr<PredefinedDataModel>(new RectilinearDataModel(dataSet));
}

std::shared_ptr<PredefinedDataModel> CreateUnstructuredSingleTypeFromDataSet(
  const vtkm::cont::DataSet& dataSet)
{
  return std::shared_ptr<PredefinedDataModel>(new UnstructuredSingleTypeDataModel(dataSet));
}

std::shared_ptr<PredefinedDataModel> CreateUnstructuredFromDataSet(
  const vtkm::cont::DataSet& dataSet)
{
  return std::shared_ptr<PredefinedDataModel>(new UnstructuredDataModel(dataSet));
}


std::shared_ptr<PredefinedDataModel> CreateRectilinear(
  std::shared_ptr<InternalMetadataSource> source)
{
  return std::shared_ptr<PredefinedDataModel>(new RectilinearDataModel(source));
}

std::shared_ptr<PredefinedDataModel> CreateUnstructured(
  std::shared_ptr<InternalMetadataSource> source)
{
  return std::shared_ptr<PredefinedDataModel>(new UnstructuredDataModel(source));
}

std::shared_ptr<PredefinedDataModel> CreateUnstructuredSingleType(
  std::shared_ptr<InternalMetadataSource> source)
{
  return std::shared_ptr<PredefinedDataModel>(new UnstructuredSingleTypeDataModel(source));
}

std::shared_ptr<PredefinedDataModel> CreateXGC(std::shared_ptr<InternalMetadataSource> source)
{
  return std::shared_ptr<PredefinedDataModel>(new XGCDataModel(source));
}

// registering data models with the factory
const bool uniformRegistered =
  DataModelFactory::GetInstance().RegisterDataModel(DataModelTypes::UNIFORM, CreateUniform);
const bool rectilinearRegistered =
  DataModelFactory::GetInstance().RegisterDataModel(DataModelTypes::RECTILINEAR, CreateRectilinear);
const bool unstructuredRegistered =
  DataModelFactory::GetInstance().RegisterDataModel(DataModelTypes::UNSTRUCTURED,
                                                    CreateUnstructured);
const bool unstructuredSingleRegistered =
  DataModelFactory::GetInstance().RegisterDataModel(DataModelTypes::UNSTRUCTURED_SINGLE,
                                                    CreateUnstructuredSingleType);
const bool xgcRegistered =
  DataModelFactory::GetInstance().RegisterDataModel(DataModelTypes::XGC, CreateXGC);

const bool uniformDSRegistered =
  DataModelFactory::GetInstance().RegisterDataModelFromDS(DataModelTypes::UNIFORM_FROM_DATASET,
                                                          CreateUniformFromDataSet);
const bool rectilinearDSRegistered =
  DataModelFactory::GetInstance().RegisterDataModelFromDS(DataModelTypes::RECTILINEAR_FROM_DATASET,
                                                          CreateRectilinearFromDataSet);
const bool unstructuredSingleDSRegistered = DataModelFactory::GetInstance().RegisterDataModelFromDS(
  DataModelTypes::UNSTRUCTURED_SINGLE_FROM_DATASET,
  CreateUnstructuredSingleTypeFromDataSet);
const bool unstructuredDSRegistered =
  DataModelFactory::GetInstance().RegisterDataModelFromDS(DataModelTypes::UNSTRUCTURED_FROM_DATASET,
                                                          CreateUnstructuredFromDataSet);

// some helper functions that are used by different data model classes

std::string GetOptionalVariableName(std::shared_ptr<InternalMetadataSource> source,
                                    const std::string& attrName,
                                    const std::string& defaultValue)
{
  if (!source)
  {
    return defaultValue;
  }

  auto vec = source->GetAttribute<std::string>(attrName);
  if (vec.empty())
  {
    return defaultValue;
  }
  return vec[0];
}

std::string GetRequiredVariableName(std::shared_ptr<InternalMetadataSource> source,
                                    const std::string& attrName)
{
  auto dimVarName = source->GetAttribute<std::string>(attrName);
  if (dimVarName.empty())
  {
    throw std::runtime_error(attrName + " must be set for this data model");
  }
  return dimVarName[0];
}


} // end anonymous namespace

//-----------------------------------------------------------------------------
PredefinedDataModel::PredefinedDataModel(std::shared_ptr<InternalMetadataSource> source)
  : MetadataSource(source)
  , FieldsToWriteSet(false)
{
}

PredefinedDataModel::PredefinedDataModel(const vtkm::cont::DataSet& dataSet)
  : MetadataSource(nullptr)
  , DataSetSource(dataSet)
  , FieldsToWriteSet(false)
{
}

rapidjson::Document& PredefinedDataModel::GetDOM(bool print /* = false*/)
{
  this->Doc.SetObject();
  rapidjson::Value root(rapidjson::kObjectType);
  this->CreateDataSources(root);
  this->CreateCoordinateSystem(root);
  this->CreateCellSet(root);
  this->CreateFields(root);
  this->AddStepInformation(root);
  this->AddRootToDocument(root);

  if (print)
  {
    this->PrintJSON();
  }

  return this->Doc;
}

void PredefinedDataModel::CreateDataSources(rapidjson::Value& parent)
{
  rapidjson::Value allSources(rapidjson::kArrayType);
  this->CreateDataSource(allSources, "source", "input");
  parent.AddMember("data_sources", allSources, this->Doc.GetAllocator());
}

void PredefinedDataModel::CreateDataSource(rapidjson::Value& parent,
                                           const std::string& name,
                                           const std::string& mode,
                                           const std::string& filename /*=""*/)
{
  rapidjson::Value source(rapidjson::kObjectType);
  rapidjson::Value n = SetString(this->Doc.GetAllocator(), name);
  source.AddMember("name", n, this->Doc.GetAllocator());

  rapidjson::Value m = SetString(this->Doc.GetAllocator(), mode);
  source.AddMember("filename_mode", m, this->Doc.GetAllocator());

  if (mode == "relative")
  {
    rapidjson::Value fn = SetString(this->Doc.GetAllocator(), filename);
    source.AddMember("filename", fn, this->Doc.GetAllocator());
  }
  parent.PushBack(source, this->Doc.GetAllocator());
}

void PredefinedDataModel::AddStepInformation(rapidjson::Value& parent)
{
  rapidjson::Value stepInfo(rapidjson::kObjectType);
  stepInfo.AddMember("data_source", "source", this->Doc.GetAllocator());
  parent.AddMember("step_information", stepInfo, this->Doc.GetAllocator());
}

void PredefinedDataModel::CreateFields(rapidjson::Value& parent)
{
  if (this->MetadataSource == nullptr)
  {
    vtkm::Id numFields = this->DataSetSource.GetNumberOfFields();
    rapidjson::Value fieldArr(rapidjson::kArrayType);
    for (vtkm::Id i = 0; i < numFields; i++)
    {
      auto field = this->DataSetSource.GetField(i);
      //If field restriction is turned on, then skip those fields.
      if (this->FieldsToWriteSet &&
          this->FieldsToWrite.find(field.GetName()) == this->FieldsToWrite.end())
      {
        continue;
      }

      rapidjson::Value fieldObj(rapidjson::kObjectType);
      auto fieldName = SetString(this->Doc.GetAllocator(), field.GetName());
      fieldObj.AddMember("name", fieldName, this->Doc.GetAllocator());
      std::string assoc = "unknown";
      if (field.IsFieldCell())
        assoc = "cell_set";
      else if (field.IsFieldPoint())
        assoc = "points";
      else
        throw std::runtime_error("Unsupported field association");

      bool isVector = false;
      if (field.GetData().GetNumberOfComponents() > 1)
        isVector = true;

      auto assocStr = SetString(this->Doc.GetAllocator(), assoc);
      fieldObj.AddMember("association", assocStr, this->Doc.GetAllocator());

      rapidjson::Value arrObj(rapidjson::kObjectType);
      arrObj.AddMember("array_type", "basic", this->Doc.GetAllocator());
      arrObj.AddMember("data_source", "source", this->Doc.GetAllocator());
      if (isVector)
        arrObj.AddMember("is_vector", "true", this->Doc.GetAllocator());
      else
        arrObj.AddMember("is_vector", "false", this->Doc.GetAllocator());

      arrObj.AddMember(
        "variable", SetString(this->Doc.GetAllocator(), field.GetName()), this->Doc.GetAllocator());
      fieldObj.AddMember("array", arrObj, this->Doc.GetAllocator());

      fieldArr.PushBack(fieldObj, this->Doc.GetAllocator());
    }

    parent.AddMember("fields", fieldArr, this->Doc.GetAllocator());
    return;
  }

  auto varList = this->MetadataSource->GetAttribute<std::string>("Fides_Variable_List");
  if (varList.empty())
  {
    // In this case there are no fields specified in an ADIOS attribute
    return;
  }
  rapidjson::Value fields(rapidjson::kArrayType);
  rapidjson::Value field(rapidjson::kObjectType);
  field.AddMember("variable_list_attribute_name", "Fides_Variable_List", this->Doc.GetAllocator());
  field.AddMember(
    "variable_association_attribute_name", "Fides_Variable_Associations", this->Doc.GetAllocator());
  rapidjson::Value arrObj(rapidjson::kObjectType);
  CreateArrayBasic(this->Doc.GetAllocator(), arrObj, "source", "");
  field.AddMember("array", arrObj, this->Doc.GetAllocator());
  fields.PushBack(field, this->Doc.GetAllocator());
  parent.AddMember("fields", fields, this->Doc.GetAllocator());
}

void PredefinedDataModel::PrintJSON()
{
  rapidjson::StringBuffer buf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
  this->Doc.Accept(writer);
  std::cout << buf.GetString() << std::endl;
}

//-----------------------------------------------------------------------------
UniformDataModel::UniformDataModel(std::shared_ptr<InternalMetadataSource> source)
  : PredefinedDataModel(source)
{
}

UniformDataModel::UniformDataModel(const vtkm::cont::DataSet& dataSet)
  : PredefinedDataModel(dataSet)
{
}

void UniformDataModel::CreateCoordinateSystem(rapidjson::Value& parent)
{
  if (this->MetadataSource == nullptr)
  {
    auto dcellSet = this->DataSetSource.GetCellSet();
    if (!dcellSet.IsType<StructuredCell3DType>())
    {
      throw "Cellset not uniform 3D.";
    }

    CreateArrayUniformPointCoordinates(
      this->Doc.GetAllocator(), parent, "dims", "origin", "spacing");
    return;
  }

  rapidjson::Value coordSys(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);
  arrObj.AddMember("array_type", "uniform_point_coordinates", this->Doc.GetAllocator());

  auto dimVarName = GetRequiredVariableName(this->MetadataSource, "Fides_Dimension_Variable");
  CreateValueVariableDimensions(
    this->Doc.GetAllocator(), arrObj, "variable_dimensions", "source", dimVarName);

  CreateValueArray(
    this->Doc.GetAllocator(), arrObj, this->MetadataSource, "Fides_Origin", "origin");
  CreateValueArray(
    this->Doc.GetAllocator(), arrObj, this->MetadataSource, "Fides_Spacing", "spacing");

  coordSys.AddMember("array", arrObj, this->Doc.GetAllocator());
  parent.AddMember("coordinate_system", coordSys, this->Doc.GetAllocator());
}

void UniformDataModel::CreateCellSet(rapidjson::Value& parent)
{
  if (this->MetadataSource == nullptr)
  {
    CreateStructuredCellset(this->Doc.GetAllocator(), parent, "dims");
    return;
  }
  rapidjson::Value cellSet(rapidjson::kObjectType);
  cellSet.AddMember("cell_set_type", "structured", this->Doc.GetAllocator());
  auto dimVarName = GetRequiredVariableName(this->MetadataSource, "Fides_Dimension_Variable");
  CreateValueVariableDimensions(
    this->Doc.GetAllocator(), cellSet, "variable_dimensions", "source", dimVarName);
  parent.AddMember("cell_set", cellSet, this->Doc.GetAllocator());
}

void UniformDataModel::AddRootToDocument(rapidjson::Value& root)
{
  this->Doc.AddMember("uniform_grid", root, this->Doc.GetAllocator());
}

//-----------------------------------------------------------------------------
RectilinearDataModel::RectilinearDataModel(std::shared_ptr<InternalMetadataSource> source)
  : PredefinedDataModel(source)
{
}

RectilinearDataModel::RectilinearDataModel(const vtkm::cont::DataSet& dataSet)
  : PredefinedDataModel(dataSet)
{
}

void RectilinearDataModel::CreateCoordinateSystem(rapidjson::Value& parent)
{
  if (this->MetadataSource == nullptr)
  {
    auto dcellSet = this->DataSetSource.GetCellSet();
    if (!dcellSet.IsType<StructuredCell3DType>())
    {
      throw "Cellset not structured 3D.";
    }

    CreateArrayRectilinearPointCoordinates(
      this->Doc.GetAllocator(), parent, "x_array", "y_array", "z_array");
    return;
  }

  rapidjson::Value coordSys(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);
  CreateArrayCartesianProduct(this->Doc.GetAllocator(), arrObj, this->MetadataSource, "source");
  coordSys.AddMember("array", arrObj, this->Doc.GetAllocator());
  parent.AddMember("coordinate_system", coordSys, this->Doc.GetAllocator());
}

void RectilinearDataModel::CreateCellSet(rapidjson::Value& parent)
{
  if (this->MetadataSource == nullptr)
  {
    CreateStructuredCellset(this->Doc.GetAllocator(), parent, "dims");
    return;
  }

  rapidjson::Value cellSet(rapidjson::kObjectType);
  cellSet.AddMember("cell_set_type", "structured", this->Doc.GetAllocator());

  auto dimVarName = GetRequiredVariableName(this->MetadataSource, "Fides_Dimension_Variable");
  CreateValueVariableDimensions(
    this->Doc.GetAllocator(), cellSet, "variable_dimensions", "source", dimVarName);
  parent.AddMember("cell_set", cellSet, this->Doc.GetAllocator());
}

void RectilinearDataModel::AddRootToDocument(rapidjson::Value& root)
{
  this->Doc.AddMember("rectilinear_grid", root, this->Doc.GetAllocator());
}

//-----------------------------------------------------------------------------
UnstructuredDataModel::UnstructuredDataModel(std::shared_ptr<InternalMetadataSource> source)
  : PredefinedDataModel(source)
{
}

UnstructuredDataModel::UnstructuredDataModel(const vtkm::cont::DataSet& dataSet)
  : PredefinedDataModel(dataSet)
{
}

void UnstructuredDataModel::CreateCoordinateSystem(rapidjson::Value& parent)
{
  if (this->MetadataSource == nullptr)
  {
    auto dcellSet = this->DataSetSource.GetCellSet();
    if (!dcellSet.IsType<UnstructuredSingleType>() && !dcellSet.IsType<UnstructuredType>())
    {
      throw std::runtime_error("Cellset is not UnstructuredSingleType.");
    }

    CreateArrayUnstructuredPointCoordinates(this->Doc.GetAllocator(), parent, "coordinates");
    return;
  }

  rapidjson::Value coordSys(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);
  auto varName =
    GetOptionalVariableName(this->MetadataSource, "Fides_Coordinates_Variable", "points");
  CreateArrayBasic(this->Doc.GetAllocator(), arrObj, "source", varName, true);
  coordSys.AddMember("array", arrObj, this->Doc.GetAllocator());
  parent.AddMember("coordinate_system", coordSys, this->Doc.GetAllocator());
}

void UnstructuredDataModel::CreateCellSet(rapidjson::Value& parent)
{
  rapidjson::Value cellSet(rapidjson::kObjectType);
  cellSet.AddMember("cell_set_type", "explicit", this->Doc.GetAllocator());

  rapidjson::Value connectivity(rapidjson::kObjectType);
  std::string connName =
    GetOptionalVariableName(this->MetadataSource, "Fides_Connectivity_Variable", "connectivity");
  CreateArrayBasic(this->Doc.GetAllocator(), connectivity, "source", connName);
  cellSet.AddMember("connectivity", connectivity, this->Doc.GetAllocator());

  rapidjson::Value cellTypes(rapidjson::kObjectType);
  std::string ctName =
    GetOptionalVariableName(this->MetadataSource, "Fides_Cell_Types_Variable", "cell_types");
  CreateArrayBasic(this->Doc.GetAllocator(), cellTypes, "source", ctName);
  cellSet.AddMember("cell_types", cellTypes, this->Doc.GetAllocator());

  rapidjson::Value numVertices(rapidjson::kObjectType);
  std::string vertName =
    GetOptionalVariableName(this->MetadataSource, "Fides_Num_Vertices_Variable", "num_verts");
  CreateArrayBasic(this->Doc.GetAllocator(), numVertices, "source", vertName);
  cellSet.AddMember("number_of_vertices", numVertices, this->Doc.GetAllocator());

  parent.AddMember("cell_set", cellSet, this->Doc.GetAllocator());
}

void UnstructuredDataModel::AddRootToDocument(rapidjson::Value& root)
{
  this->Doc.AddMember("unstructured_grid", root, this->Doc.GetAllocator());
}

//-----------------------------------------------------------------------------
UnstructuredSingleTypeDataModel::UnstructuredSingleTypeDataModel(
  std::shared_ptr<InternalMetadataSource> source)
  : UnstructuredDataModel(source)
{
}

UnstructuredSingleTypeDataModel::UnstructuredSingleTypeDataModel(const vtkm::cont::DataSet& dataSet)
  : UnstructuredDataModel(dataSet)
{
}

void UnstructuredSingleTypeDataModel::CreateCellSet(rapidjson::Value& parent)
{
  if (this->MetadataSource == nullptr)
  {
    auto dcellSet = this->DataSetSource.GetCellSet();
    if (!dcellSet.IsType<UnstructuredSingleType>())
    {
      throw std::runtime_error("Cellset is not UnstructuredSingleType");
    }
    const auto& cellSet = this->DataSetSource.GetCellSet().Cast<UnstructuredSingleType>();
    vtkm::UInt8 shapeId = cellSet.GetCellShape(0);
    std::string cellType = fides::ConvertVTKmCellTypeToFides(shapeId);

    CreateUnstructuredSingleTypeCellset(this->Doc.GetAllocator(), parent, "connectivity", cellType);
    return;
  }

  // structured
  rapidjson::Value cellSet(rapidjson::kObjectType);
  cellSet.AddMember("cell_set_type", "single_type", this->Doc.GetAllocator());

  std::string cellType = this->MetadataSource->GetDataModelCellType();
  rapidjson::Value ct = SetString(this->Doc.GetAllocator(), cellType);
  cellSet.AddMember("cell_type", ct, this->Doc.GetAllocator());
  cellSet.AddMember("data_source", "source", this->Doc.GetAllocator());

  auto connName =
    GetOptionalVariableName(this->MetadataSource, "Fides_Connectivity_Variable", "connectivity");
  auto conn = SetString(this->Doc.GetAllocator(), connName);
  cellSet.AddMember("variable", conn, this->Doc.GetAllocator());

  parent.AddMember("cell_set", cellSet, this->Doc.GetAllocator());
}

void UnstructuredSingleTypeDataModel::AddRootToDocument(rapidjson::Value& root)
{
  this->Doc.AddMember("unstructured_grid_single_cell_type", root, this->Doc.GetAllocator());
}

//-----------------------------------------------------------------------------
XGCDataModel::XGCDataModel(std::shared_ptr<InternalMetadataSource> source)
  : PredefinedDataModel(source)
{
}

rapidjson::Document& XGCDataModel::GetDOM(bool print /* = false*/)
{
  PredefinedDataModel::GetDOM();
  if (!this->Doc.HasMember("xgc"))
  {
    throw std::runtime_error("doc doesn't have xgc member");
  }
  auto& root = this->Doc["xgc"];
  auto nplanes =
    GetOptionalVariableName(this->MetadataSource, "Fides_Number_Of_Planes_Variable", "nphi");
  CreateValueScalar(this->Doc.GetAllocator(), root, "number_of_planes", "scalar", "3d", "nphi");

  if (print)
  {
    this->PrintJSON();
  }

  return this->Doc;
}

void XGCDataModel::CreateDataSources(rapidjson::Value& parent)
{
  rapidjson::Value allSources(rapidjson::kArrayType);
  auto meshFilename =
    GetOptionalVariableName(this->MetadataSource, "Fides_XGC_Mesh_Filename", "xgc.mesh.bp");
  this->CreateDataSource(allSources, "mesh", "relative", meshFilename);
  auto dFilename =
    GetOptionalVariableName(this->MetadataSource, "Fides_XGC_3d_Filename", "xgc.3d.bp");
  this->CreateDataSource(allSources, "3d", "relative", dFilename);
  auto diagFilename =
    GetOptionalVariableName(this->MetadataSource, "Fides_XGC_Diag_Filename", "xgc.oneddiag.bp");
  this->CreateDataSource(allSources, "diag", "relative", diagFilename);
  parent.AddMember("data_sources", allSources, this->Doc.GetAllocator());
}

void XGCDataModel::AddStepInformation(rapidjson::Value& parent)
{
  rapidjson::Value stepInfo(rapidjson::kObjectType);
  stepInfo.AddMember("data_source", "3d", this->Doc.GetAllocator());
  parent.AddMember("step_information", stepInfo, this->Doc.GetAllocator());
}

void XGCDataModel::CreateCoordinateSystem(rapidjson::Value& parent)
{
  rapidjson::Value coordSys(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);
  auto coords = GetOptionalVariableName(this->MetadataSource, "Fides_Coordinates_Variable", "rz");
  CreateArrayXGCCoordinates(this->Doc.GetAllocator(), arrObj, "mesh", coords);
  coordSys.AddMember("array", arrObj, this->Doc.GetAllocator());
  parent.AddMember("coordinate_system", coordSys, this->Doc.GetAllocator());
}

void XGCDataModel::CreateCellSet(rapidjson::Value& parent)
{
  rapidjson::Value cellSet(rapidjson::kObjectType);
  cellSet.AddMember("cell_set_type", "xgc", this->Doc.GetAllocator());
  cellSet.AddMember("periodic", true, this->Doc.GetAllocator());

  auto triConn = GetOptionalVariableName(
    this->MetadataSource, "Fides_Triangle_Connectivity_Variable", "nd_connect_list");
  rapidjson::Value cells(rapidjson::kObjectType);
  CreateArrayBasic(this->Doc.GetAllocator(), cells, "mesh", triConn);
  cells.AddMember("static", true, this->Doc.GetAllocator());
  cells.AddMember("is_vector", "false", this->Doc.GetAllocator());
  cellSet.AddMember("cells", cells, this->Doc.GetAllocator());

  auto planeConn =
    GetOptionalVariableName(this->MetadataSource, "Fides_Plane_Connectivity_Variable", "nextnode");
  rapidjson::Value conn(rapidjson::kObjectType);
  CreateArrayBasic(this->Doc.GetAllocator(), conn, "mesh", planeConn);
  conn.AddMember("static", true, this->Doc.GetAllocator());
  conn.AddMember("is_vector", "false", this->Doc.GetAllocator());
  cellSet.AddMember("plane_connectivity", conn, this->Doc.GetAllocator());

  parent.AddMember("cell_set", cellSet, this->Doc.GetAllocator());
}

void XGCDataModel::CreateFields(rapidjson::Value& parent)
{
  rapidjson::Value fields(rapidjson::kArrayType);
  rapidjson::Value field(rapidjson::kObjectType);
  field.AddMember("variable_list_attribute_name", "Fides_Variable_List", this->Doc.GetAllocator());
  field.AddMember(
    "variable_association_attribute_name", "Fides_Variable_Associations", this->Doc.GetAllocator());
  field.AddMember(
    "variable_sources_attribute_name", "Fides_Variable_Sources", this->Doc.GetAllocator());
  field.AddMember(
    "variable_arrays_attribute_name", "Fides_Variable_Array_Types", this->Doc.GetAllocator());

  rapidjson::Value arrObj(rapidjson::kObjectType);
  CreateArrayBasic(this->Doc.GetAllocator(), arrObj, "", "", false, "");
  field.AddMember("array", arrObj, this->Doc.GetAllocator());

  fields.PushBack(field, this->Doc.GetAllocator());
  parent.AddMember("fields", fields, this->Doc.GetAllocator());
}

void XGCDataModel::AddRootToDocument(rapidjson::Value& root)
{
  this->Doc.AddMember("xgc", root, this->Doc.GetAllocator());
}

}
}
