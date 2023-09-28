// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridWriter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridIOQuery.h"
#include "vtkCellMetadata.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkIOCellGrid.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOptions.h"
#include "vtkVariant.h"

#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)

#include <fstream>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridWriter);

vtkCellGridWriter::vtkCellGridWriter()
  : FileName{ nullptr }
{
  // Ensure vtkCellGridIOQuery is registered.
  vtkIOCellGrid::RegisterCellsAndResponders();
}

vtkCellGridWriter::~vtkCellGridWriter()
{
  this->SetFileName(nullptr);
}

void vtkCellGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << "\n";
}

vtkCellGrid* vtkCellGridWriter::GetInput()
{
  return vtkCellGrid::SafeDownCast(this->Superclass::GetInputDataObject(0, 0));
}

vtkCellGrid* vtkCellGridWriter::GetInput(int port)
{
  return vtkCellGrid::SafeDownCast(this->Superclass::GetInputDataObject(port, 0));
}

int vtkCellGridWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
  return 1;
}

std::string dataTypeToString(int dataType)
{
  switch (dataType)
  {
#if VTK_SIZEOF_CHAR == 1
    case VTK_CHAR:
      return "vtktypeint8";
    case VTK_SIGNED_CHAR:
      return "vtktypeint8";
    case VTK_UNSIGNED_CHAR:
      return "vtktypeuint8";
#else
#error "Unhandled char size " VTK_SIZEOF_CHAR
#endif

    case VTK_DOUBLE:
      return "double";
    case VTK_FLOAT:
      return "float";

#if VTK_SIZEOF_INT == 2
    case VTK_INT:
      return "vtktypeint16";
    case VTK_UNSIGNED_INT:
      return "vtktypeuint16";
#elif VTK_SIZEOF_INT == 4
    case VTK_INT:
      return "vtktypeint32";
    case VTK_UNSIGNED_INT:
      return "vtktypeuint32";
#elif VTK_SIZEOF_INT == 8
    case VTK_INT:
      return "vtktypeint64";
    case VTK_UNSIGNED_INT:
      return "vtktypeuint64";
#else
#error "Unhandled int size " VTK_SIZEOF_INT
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG:
      return "vtktypeint32";
    case VTK_UNSIGNED_LONG:
      return "vtktypeuint32";
#elif VTK_SIZEOF_LONG == 8
    case VTK_LONG:
      return "vtktypeint32";
    case VTK_UNSIGNED_LONG:
      return "vtktypeuint32";
#else
#error "Unhandled int size " VTK_SIZEOF_INT
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG_LONG:
      return "vtktypeint32";
    case VTK_UNSIGNED_LONG_LONG:
      return "vtktypeuint32";
#elif VTK_SIZEOF_LONG == 8
    case VTK_LONG_LONG:
      return "vtktypeint64";
    case VTK_UNSIGNED_LONG_LONG:
      return "vtktypeuint64";
#else
#error "Unhandled long size " VTK_SIZEOF_LONG
#endif

#if VTK_SIZEOF_SHORT == 1
    case VTK_SHORT:
      return "vtktypeint8";
    case VTK_UNSIGNED_SHORT:
      return "vtktypeuint8";
#elif VTK_SIZEOF_SHORT == 2
    case VTK_SHORT:
      return "vtktypeint16";
    case VTK_UNSIGNED_SHORT:
      return "vtktypeuint16";
#elif VTK_SIZEOF_SHORT == 4
    case VTK_SHORT:
      return "vtktypeint32";
    case VTK_UNSIGNED_SHORT:
      return "vtktypeuint32";
#else
#error "Unhandled short size " VTK_SIZEOF_SHORT
#endif

#ifdef VTK_USE_64BIT_IDS
    case VTK_ID_TYPE:
      return "vtktypeint64";
#else
    case VTK_ID_TYPE:
      return "vtktypeint32";
#endif

    default:
      break;
  }
  return "unhandled";
}

struct WriteDataArrayWorker
{
  WriteDataArrayWorker(nlohmann::json& result)
    : m_result(result)
  {
  }

  template <typename InArrayT>
  void operator()(InArrayT* inArray)
  {
    using T = vtk::GetAPIType<InArrayT>;
    const auto inRange = vtk::DataArrayValueRange(inArray);
    T val;
    for (const auto& value : inRange)
    {
      val = value;
      m_result.push_back(val);
    }
  }

  nlohmann::json& m_result;
};

nlohmann::json serializeArrayValues(vtkAbstractArray* arr)
{
  auto result = nlohmann::json::array();
  if (!arr)
  {
    return result;
  }

  if (auto* darr = vtkDataArray::SafeDownCast(arr))
  {
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
    WriteDataArrayWorker worker(result);
    if (!Dispatcher::Execute(darr, worker))
    {
      worker(darr);
    }
  }
  else
  {
    for (vtkIdType ii = 0; ii < arr->GetNumberOfValues(); ++ii)
    {
      result.push_back(arr->GetVariantValue(ii).ToString());
    }
  }
  return result;
}

void vtkCellGridWriter::WriteData()
{
  if (!this->FileName || !this->FileName[0])
  {
    vtkErrorMacro("No filename set.");
    return;
  }

  auto* grid = this->GetInput();
  if (!grid)
  {
    vtkErrorMacro("No input dataset to write to \"" << this->FileName << "\"");
    return;
  }

  std::ofstream output(this->FileName);
  if (!output.good())
  {
    vtkErrorMacro("Could not open \"" << this->FileName << "\" for writing.");
    return;
  }

  auto cellTypes = nlohmann::json::array();
  for (const auto& cellType : grid->CellTypeArray())
  {
    auto entry = nlohmann::json::object({ { "type", cellType.Data() } });
    cellTypes.push_back(entry);
  }
  // Now provide vtkCellMetadata subclasses with a chance to add to \a cellTypes.
  vtkNew<vtkCellGridIOQuery> query;
  query->PrepareToSerialize(cellTypes);
  if (!grid->Query(query))
  {
    vtkErrorMacro("Could not prepare cell metadata.");
    return;
  }

  // Iterate all the vtkDataSetAttributes held by the grid.
  // As we go, store a map from each array in each vtkDataSetAttributes
  // to a "location" for the array so we can refer to the arrays later
  // by a persistent name instead of by pointer.
  std::map<vtkAbstractArray*, nlohmann::json> arrayLocations;
  auto arrayGroups = nlohmann::json::object();
  for (const auto& entry : grid->GetArrayGroups())
  {
    std::string groupName;
    vtkStringToken groupToken(entry.first);
    if (!groupToken.Data().empty())
    {
      groupName = groupToken.Data();
    }
    else
    {
      std::ostringstream groupId;
      groupId << entry.first;
      groupName = groupId.str();
    }
    nlohmann::json arraysInGroup;

    // Fetch arrays serving in specific roles.
    vtkDataArray* groupScalars = entry.second->GetScalars();
    vtkDataArray* groupVectors = entry.second->GetVectors();
    vtkDataArray* groupTensors = entry.second->GetTensors();
    vtkDataArray* groupTCoords = entry.second->GetTCoords();
    vtkDataArray* groupTangents = entry.second->GetTangents();
    vtkDataArray* groupGlobalIds = entry.second->GetGlobalIds();
    vtkAbstractArray* groupPedigreeIds = entry.second->GetPedigreeIds();
    vtkDataArray* groupRationalWeights = entry.second->GetRationalWeights();
    vtkDataArray* groupHigherOrderDegrees = entry.second->GetHigherOrderDegrees();

    for (vtkIdType ii = 0; ii < entry.second->GetNumberOfArrays(); ++ii)
    {
      vtkAbstractArray* arr = entry.second->GetAbstractArray(ii);
      if (!arr)
      {
        continue;
      }
      arrayLocations[arr] = nlohmann::json::array({ groupName, arr->GetName() });
      nlohmann::json arrayRecord{ { "name", arr->GetName() },
        { "tuples", arr->GetNumberOfTuples() }, { "components", arr->GetNumberOfComponents() },
        { "type", dataTypeToString(arr->GetDataType()) }, { "data", serializeArrayValues(arr) } };
      if (arr == groupScalars)
      {
        arrayRecord["default_scalars"] = true;
      }
      if (arr == groupVectors)
      {
        arrayRecord["default_vectors"] = true;
      }
      if (arr == groupTensors)
      {
        arrayRecord["default_tensors"] = true;
      }
      if (arr == groupTCoords)
      {
        arrayRecord["default_tcoords"] = true;
      }
      if (arr == groupTangents)
      {
        arrayRecord["default_tangents"] = true;
      }
      if (arr == groupGlobalIds)
      {
        arrayRecord["default_global_ids"] = true;
      }
      if (arr == groupPedigreeIds)
      {
        arrayRecord["default_pedigree_ids"] = true;
      }
      if (arr == groupRationalWeights)
      {
        arrayRecord["default_rational_weights"] = true;
      }
      if (arr == groupHigherOrderDegrees)
      {
        arrayRecord["default_higher_order_degrees"] = true;
      }
      arraysInGroup.push_back(arrayRecord);
    }
    if (!arraysInGroup.empty())
    {
      arrayGroups[groupName] = arraysInGroup;
    }
  }

  auto attributes = nlohmann::json::array();
  auto* shapeAtt = grid->GetShapeAttribute();
  for (const auto cellAttId : grid->GetCellAttributeIds())
  {
    auto* cellAtt = grid->GetCellAttributeById(cellAttId);
    if (cellAtt)
    {
      nlohmann::json record{ { "name", cellAtt->GetName().Data() },
        { "type", cellAtt->GetAttributeType().Data() }, { "space", cellAtt->GetSpace().Data() },
        { "components", cellAtt->GetNumberOfComponents() } };
      auto arraysForCellType = nlohmann::json::object();
      for (const auto& cellTypeObj : cellTypes)
      {
        auto cellType = cellTypeObj.at("type").get<std::string>();
        auto arraysByRole = nlohmann::json::object();
        for (const auto& entry : cellAtt->GetArraysForCellType(cellType))
        {
          auto it = arrayLocations.find(entry.second);
          if (it == arrayLocations.end() || !entry.second)
          {
            vtkErrorMacro("Array " << entry.second << " not held by any attributes object.");
            continue;
          }
          arraysByRole[entry.first.Data()] = it->second;
        }
        if (!arraysByRole.empty())
        {
          arraysForCellType[cellType] = arraysByRole;
        }
      }
      if (!arraysForCellType.empty())
      {
        record["arrays"] = arraysForCellType;
      }
      if (cellAtt == shapeAtt)
      {
        record["shape"] = true;
      }
      attributes.push_back(record);
    }
  }

  auto schemaName = grid->GetSchemaName();
  if (!schemaName.IsValid())
  {
    schemaName = "dg leaf";
  }
  nlohmann::json data = {
    { "data-type", "cell-grid" }, { "arrays", arrayGroups }, { "attributes", attributes },
    { "cell-types", cellTypes },
    { "format-version", 1 }, // A version number for the file format (i.e.., JSON)
    { "schema-name",
      schemaName.Data() }, // A name for the schema (key/value structure) of this file's content.
    { "schema-version", grid->GetSchemaVersion() }, // A version number for the file's schema.
    { "content-version",
      grid->GetContentVersion() } // A version number for the file's content (key/value data).
  };

  output << data;
  output.close();
}

VTK_ABI_NAMESPACE_END
