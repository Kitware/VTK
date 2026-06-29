//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/DataSetReader.h>
#include <fides/DataSourceFactory.h>
#include <fides/internal/OutputBuilder.h>
#include <fides/internal/predefined/DataModelFactory.h>
#include <fides/internal/predefined/DataModelHelperFunctions.h>
#include <fides/internal/predefined/InternalMetadataSource.h>

#include <ios>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
#include FIDES_RAPIDJSON(rapidjson/error/en.h)
#include FIDES_RAPIDJSON(rapidjson/filereadstream.h)
// clang-format on

#if FIDES_USE_VISKORES
#include <fides/viskores/ViskoresBuilder.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#endif

#if FIDES_USE_VTK
#include <fides/vtk/VTKBuilder.h>
#include <vtkPartitionedDataSet.h>
#include <vtkSmartPointer.h>
#endif

#include <fides/DataSource.h>
#include <fides/Keys.h>
#include <fides/internal/CellGridModel.h>
#include <fides/internal/DataSetModel.h>
#include <fides/internal/DataWrapHelper.h>

namespace fides
{
namespace io
{

namespace
{

std::string baseFileName(std::string const& path)
{
  return path.substr(path.find_last_of("/\\") + 1);
}

/// True iff \c name would be accepted by vtkDataAssembly::IsNodeNameValid.
///
/// Mirrors VTK's rules here (rather than calling into VTK directly,
/// which fides_core does not link against): first char must be a
/// letter or '_'; subsequent chars must be letters, digits, '_', '-',
/// or '.'; the name must not be empty; and names that start with "da"
/// and are longer than 2 characters are reserved by VTK for internal
/// <dataset> tags (e.g. "data", "database", "dataset"). Validating
/// schema-supplied names at parse time means users hit a clean error
/// at the source of the problem rather than seeing their identifiers
/// silently mangled into the materialized vtkDataAssembly.
bool IsValidAssemblyNodeName(const std::string& name)
{
  if (name.empty())
  {
    return false;
  }
  if (name.size() > 2 && name[0] == 'd' && name[1] == 'a')
  {
    return false; // reserved (matches vtkDataAssembly::IsNodeNameReserved)
  }
  const char c0 = name[0];
  if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '_'))
  {
    return false;
  }
  for (size_t i = 1; i < name.size(); ++i)
  {
    const char c = name[i];
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
      c == '_' || c == '-' || c == '.';
    if (!ok)
    {
      return false;
    }
  }
  return true;
}

/// Extracts a double value from a RawArray element, regardless of the underlying type.
double GetRawArrayValueAsDouble(const fides::RawArray& raw, size_t index)
{
  switch (raw.Type)
  {
    case fides::DataType::Float64:
      return raw.GetValue<double>(index);
    case fides::DataType::Float32:
      return static_cast<double>(raw.GetValue<float>(index));
    case fides::DataType::Int8:
      return static_cast<double>(raw.GetValue<int8_t>(index));
    case fides::DataType::Int16:
      return static_cast<double>(raw.GetValue<int16_t>(index));
    case fides::DataType::Int32:
      return static_cast<double>(raw.GetValue<int32_t>(index));
    case fides::DataType::Int64:
      return static_cast<double>(raw.GetValue<int64_t>(index));
    case fides::DataType::UInt8:
      return static_cast<double>(raw.GetValue<uint8_t>(index));
    case fides::DataType::UInt16:
      return static_cast<double>(raw.GetValue<uint16_t>(index));
    case fides::DataType::UInt32:
      return static_cast<double>(raw.GetValue<uint32_t>(index));
    case fides::DataType::UInt64:
      return static_cast<double>(raw.GetValue<uint64_t>(index));
    default:
      throw std::runtime_error("GetRawArrayValueAsDouble: unknown type");
  }
}

/// Copies a RawArray to a vector<double>, converting from the underlying type.
void CopyRawArrayToDoubleVector(const fides::RawArray& raw, std::vector<double>& out)
{
  out.resize(raw.NumValues);
  for (size_t i = 0; i < raw.NumValues; i++)
  {
    out[i] = GetRawArrayValueAsDouble(raw, i);
  }
}

} // end anon namespace

using DataSourceType = fides::io::DataSource;
using DataSourcesType = std::unordered_map<std::string, std::shared_ptr<DataSourceType>>;

class DataSetReader::DataSetReaderImpl
{
public:
  /// One schema-declared dataset. A legacy single-dataset schema yields
  /// one entry with an empty Name; a `datasets[]` schema yields one entry
  /// per child, Name doubling as the ADIOS variable group prefix and the
  /// DATASET_SELECTION key.
  struct DatasetEntry
  {
    std::string Name;
    std::unique_ptr<fides::datamodel::DataObjectModel> Model;
  };

  DataSetReaderImpl(const std::string& dataModel,
                    DataModelInput inputType,
                    bool streamSteps,
                    const Params& params,
                    bool createSharedPoints)
    : StreamingMode(streamSteps)
  {
#if FIDES_USE_MPI
    this->Comm = MPI_COMM_NULL;
#endif
    this->SetupReader(dataModel, inputType, params, createSharedPoints);
  }

#if FIDES_USE_MPI
  DataSetReaderImpl(const std::string& dataModel,
                    DataModelInput inputType,
                    bool streamSteps,
                    MPI_Comm comm,
                    const Params& params,
                    bool createSharedPoints)
    : StreamingMode(streamSteps)
  {
    this->Comm = comm;
    this->SetupReader(dataModel, inputType, params, createSharedPoints);
  }
#endif

  virtual ~DataSetReaderImpl() { this->Cleanup(); }

  void SetupReader(const std::string& dataModel,
                   DataModelInput inputType,
                   const Params& params,
                   bool createSharedPoints)
  {
    this->Cleanup();
    if (inputType == DataModelInput::BPFile)
    {
      this->ADIOSStreamSetup(dataModel, params);
      // due to BP5 (also applies to SST), we have to make sure
      // that we call BeginStep on this source before we can try
      // to read the metadata from it. This only applies when
      // we're using the streaming mode instead of random access.
      if (this->StreamingMode)
      {
        this->InternalSourceBeginStep();
      }
      this->ParseDataModel();
    }
    else
    {
      this->DataModelDocument = std::make_shared<rapidjson::Document>(
        DataSetReaderImpl::GetJSONDocument(dataModel, inputType));
      DataSetReaderImpl::ParsingChecks(*this->DataModelDocument, dataModel, inputType);
      this->ReadJSON(*this->DataModelDocument);
    }
    std::string relativePath;
    if (inputType != DataModelInput::JSONString)
    {
      size_t pos = dataModel.find_last_of("\\/");
      if (pos != std::string::npos)
      {
        relativePath = dataModel.substr(0, pos + 1);
      }
      else
      {
        relativePath = "./";
      }
    }
    for (const auto& it : this->DataSources)
    {
      ADIOSDataSource* adiosSrc = dynamic_cast<ADIOSDataSource*>(it.second.get());
      if (adiosSrc)
      {
        // Only ADIOSDataSource supports CreateSharedPoints / StreamingMode
        adiosSrc->CreateSharedPoints = createSharedPoints;
        adiosSrc->RelativePath = relativePath;
        // Propagate the reader's streaming choice. ADIOSDataSource defaults
        // StreamingMode=true, but the reader's streamSteps argument is the
        // ground truth; downstream code (any path that opens a source) must
        // see the right mode from the start, not after a per-method override.
        adiosSrc->StreamingMode = this->StreamingMode;
      }
    }

    this->SetDataSourceParameters(params);
  }

  void ADIOSStreamSetup(const std::string& filename, const Params& params)
  {
    // need to check here for which engine is being used, but at this point we don't
    // actually know which source name maps to the adios file we're opening (unless there's
    // only one source).
    // Because of this, we need to set some conditions. At least for now, we need to limit
    // to a single SST source (but there can be a BP source with it as well).
    // Also in the case of multiple sources where one is SST, we require that the JSON be provided
    // in the BP file.
    std::string engineType = "BPFile";
    if (params.size() == 1)
    {
      const auto& source = params.begin()->second;
      for (const auto& param : source)
      {
        if (param.first == "engine_type" && param.second == "SST")
        {
          engineType = "SST";
        }
      }
    }
    else if (params.size() > 1)
    {
      int numSST = 0;
      for (const auto& source : params)
      {
        for (const auto& param : source.second)
        {
          if (param.first == "engine_type" && param.second == "SST")
          {
            numSST++;
          }
        }
      }

      if (numSST > 1)
      {
        throw std::runtime_error("Currently only one SST source is allowed.");
      }
    }

    // when the filename is an ADIOS file/stream
    // we need to create a temporary holder for this initial data source so we can open it
    // and read the fides/schema attribute or other fides metadata from it.
    // once we've processed the json, then we can set this source to the actual data source
    // and get rid of this temporary holder for it. Note that it will still be the same
    // DataSource object, because in cases like SST, we don't want to have to open another reader.
#if FIDES_USE_MPI
    this->InternalSource = std::make_shared<fides::io::ADIOSDataSource>(this->Comm);
#else
    this->InternalSource = std::make_shared<fides::io::ADIOSDataSource>();
#endif
    this->InternalSource->Mode = fides::io::FileNameMode::Input;
    this->InternalSource->FileName = filename;
    this->InternalSource->StreamingMode = this->StreamingMode;
    this->InternalSource->SetEngineType(engineType);
    this->InternalSource->OpenSource(filename);
  }

  // Only used when we are loading JSON/metadata from an adios stream/file
  void ParseDataModel()
  {
    // check first for fides/schema and use that if it exists
    if (this->InternalSource->GetAttributeType("fides/schema") == "string")
    {
      auto schema = this->InternalSource->ReadAttribute<std::string>("fides/schema");
      if (!schema.empty())
      {
        this->DataModelDocument = std::make_shared<rapidjson::Document>(
          DataSetReaderImpl::GetJSONDocument(schema[0], DataModelInput::JSONString));
        DataSetReaderImpl::ParsingChecks(
          *this->DataModelDocument, schema[0], DataModelInput::JSONString);
        this->ReadJSON(*this->DataModelDocument);
        this->UpdateDataSources();
        return;
      }
    }

    // fides/schema didn't exist, so see if we can find the fides metadata
    // in this case the bp file passed in becomes our MetadataSource
    // which is used to select a predefined data model
    this->MetadataSource.reset(new fides::predefined::InternalMetadataSource(this->InternalSource));
    auto dm = predefined::DataModelFactory::GetInstance().CreateDataModel(this->MetadataSource);
    this->ReadJSON(dm->GetDOM());
    this->UpdateDataSources();
  }

  void UpdateDataSources()
  {
    // we have InternalSource which is the source we opened to read the json, now we need to
    // reconcile it with a source in DataSources. In the case of BP files, this isn't really that
    // important, because we could open the file multiple times, but for other engines, we want to
    // ensure we only open it once.
    // Note: we have been supporting the case where the passed in BP file only contains the schema or
    // metadata (for example, see the xgc test). So we may not necessarily match up InternalSource
    // with one of the DataSources, but that's not an issue since we won't need to read any other data
    // from that file.
    if (!this->InternalSource)
    {
      return;
    }

    // the file that contains the json can use relative or input for filename_mode
    int numInputFiles = 0;
    std::string tmpName;
    for (auto& ds : this->DataSources)
    {
      ADIOSDataSource* source = dynamic_cast<ADIOSDataSource*>(ds.second.get());

      if (source == nullptr)
      {
        throw std::runtime_error("Metadata can only be provided by an ADIOS data source");
      }

      if (source->Mode == FileNameMode::Input)
      {
        numInputFiles++;
        if (numInputFiles > 1)
        {
          tmpName.clear();
          continue;
        }
        tmpName = ds.first;
      }
      else
      {
        auto internalName = baseFileName(this->InternalSource->FileName);
        auto sourceName = baseFileName(source->FileName);
        if (internalName == sourceName)
        {
          this->DataSources[ds.first] = this->InternalSource;
          this->InternalSource = nullptr;
        }
      }
    }

    if (!tmpName.empty() and this->InternalSource)
    {
      this->DataSources[tmpName] = this->InternalSource;
      this->InternalSource = nullptr;
    }
  }

  void Cleanup()
  {
    this->DataSources.clear();
    this->Datasets.clear();
  }

  static rapidjson::Document GetJSONDocument(const std::string& dataModel, DataModelInput inputType)
  {
    rapidjson::Document d;
    if (inputType == DataModelInput::JSONFile)
    {
      FILE* fp = std::fopen(dataModel.c_str(), "rb");
      if (!fp)
      {
        throw std::ios_base::failure("Unable to open metadata file; does '" + dataModel +
                                     "' exist?");
      }
      std::vector<char> buffer(65536);
      rapidjson::FileReadStream is(fp, buffer.data(), buffer.size());
      d.ParseStream(is);
      std::fclose(fp);
    }
    else if (inputType == DataModelInput::JSONString)
    {
      rapidjson::StringStream s(dataModel.c_str());
      d.ParseStream(s);
    }
    else
    {
      throw std::runtime_error(
        "DataModelInput should be either Filename or String containing JSON");
    }
    return d;
  }

  void SetDataSourceParameters(const Params& params)
  {
    for (const auto& p : params)
    {
      this->SetDataSourceParameters(p.first, p.second);
    }
  }

  void SetDataSourceParameters(const std::string& source, const DataSourceParams& params)
  {
    auto it = this->DataSources.find(source);
    if (it == this->DataSources.end())
    {
      throw std::runtime_error("Source name was not found in DataSources.");
    }
    auto& ds = *(it->second);
    ds.SetDataSourceParameters(params);
  }

  void SetDataSourceIO(const std::string& source, void* io)
  {
    auto it = this->DataSources.find(source);
    if (it == this->DataSources.end())
    {
      throw std::runtime_error("Source name was not found in DataSources.");
    }
    ADIOSDataSource* adiosSrc = dynamic_cast<ADIOSDataSource*>(it->second.get());
    if (!adiosSrc)
    {
      std::cerr << "Ignoring SetDataSourceIO() call for non-ADIOS data source " << it->first
                << std::endl;
      return;
    }
    adiosSrc->SetDataSourceIO(io);
  }

  void SetDataSourceIO(const std::string& source, const std::string& io)
  {
    auto it = this->DataSources.find(source);
    if (it == this->DataSources.end())
    {
      throw std::runtime_error("Source name was not found in DataSources.");
    }
    ADIOSDataSource* adiosSrc = dynamic_cast<ADIOSDataSource*>(it->second.get());
    if (!adiosSrc)
    {
      std::cerr << "Ignoring SetDataSourceIO() call for non-ADIOS data source " << it->first
                << std::endl;
      return;
    }
    adiosSrc->SetDataSourceIO(io);
  }

  template <typename ValueType>
  void ProcessDataSources(const ValueType& dataSources)
  {
    for (auto& dataSource : dataSources)
    {
      if (!dataSource.IsObject())
      {
        throw std::runtime_error("data_sources must contain data_source objects.");
      }
      if (!dataSource.GetObject().HasMember("name"))
      {
        throw std::runtime_error("data_source objects must have name.");
      }
      std::string name = dataSource.GetObject()["name"].GetString();
      if (name.empty())
      {
        throw std::runtime_error("data_source name must be a non-empty string.");
      }
      std::string type = "adios";
      if (dataSource.GetObject().HasMember("type"))
      {
        type = dataSource.GetObject()["type"].GetString();
      }
#if FIDES_USE_MPI
      std::shared_ptr<DataSourceType> source = fides::io::MakeDataSource(type, this->Comm);
#else
      std::shared_ptr<DataSourceType> source = fides::io::MakeDataSource(type);
#endif
      source->SetSchemaDocument(this->DataModelDocument);
      if (type == "adios")
      {
        ADIOSDataSource* ads = dynamic_cast<ADIOSDataSource*>(source.get());

        if (ads == nullptr)
        {
          throw std::logic_error("Internal error: Factory failed to return ADIOSDataSource");
        }

        if (!dataSource.GetObject().HasMember("filename_mode"))
        {
          throw std::runtime_error("ADIOS data_source objects must have filename_mode.");
        }
        std::string filename_mode = dataSource.GetObject()["filename_mode"].GetString();
        if (filename_mode.empty())
        {
          throw std::runtime_error("ADIOS data_source objects must have non-empty filename_mode.");
        }
        if (filename_mode == "input")
        {
          ads->Mode = fides::io::FileNameMode::Input;
        }
        else if (filename_mode == "relative")
        {
          ads->Mode = fides::io::FileNameMode::Relative;
          if (!dataSource.GetObject().HasMember("filename"))
          {
            throw std::runtime_error("data_source objects must have filename.");
          }
          ads->FileName = dataSource.GetObject()["filename"].GetString();
        }
        else
        {
          throw std::runtime_error("data_source filename_mode must be input or relative.");
        }
      }

      this->DataSources[name] = source;
    }
  }

  size_t GetNumberOfSteps()
  {
    if (this->StepSource.empty())
    {
      return 0;
    }
    auto sourceIter = this->DataSources.find(this->StepSource);
    if (sourceIter == this->DataSources.end())
    {
      return 0;
    }
    return sourceIter->second->GetNumberOfSteps();
  }

  void ProcessStepInformation(const rapidjson::Value& sinf)
  {
    if (!sinf.IsObject())
    {
      throw std::runtime_error("step_information needs to be an object.");
    }
    auto sInf = sinf.GetObject();
    if (!sInf.HasMember("data_source"))
    {
      throw std::runtime_error("step_information needs a data_source.");
    }
    this->StepSource = sInf["data_source"].GetString();
    if (sInf.HasMember("variable"))
    {
      this->TimeVariable = sInf["variable"].GetString();
    }
  }

  static void ParsingChecks(rapidjson::Document& document,
                            const std::string& fileName,
                            DataModelInput inputType)
  {
    std::string nameStr;
    if (inputType == DataModelInput::JSONFile)
    {
      nameStr = fileName;
    }
    else if (inputType == DataModelInput::JSONString)
    {
      nameStr = "the passed string";
    }

    if (document.HasParseError())
    {
      throw std::logic_error("Unable to parse " + nameStr + " as a json file. Error: " +
                             rapidjson::GetParseError_En(document.GetParseError()));
    }
    if (!document.IsObject())
    {
      throw std::logic_error("Unable to parse '" + nameStr + "' as a json file; is it valid json?");
    }

    auto m = document.GetObject().begin();
    if (m == document.GetObject().end())
    {
      throw std::logic_error("There is no data in '" + nameStr +
                             "'; there is nothing that can be achieved with this file/string.");
    }
    if (!m->value.IsObject())
    {
      throw std::logic_error("Unable to create a sensible object from '" + nameStr +
                             "'; aborting.");
    }
  }

  /// True when the schema declared a `datasets[]` block (multi-dataset
  /// PDC). More than one entry, or one entry with a non-empty name.
  bool IsMultiDataset() const
  {
    return this->Datasets.size() > 1 ||
      (this->Datasets.size() == 1 && !this->Datasets.front().Name.empty());
  }

  /// The first/primary model — the single model for legacy schemas, or
  /// the first dataset of a `datasets[]` schema (used for whole-schema
  /// queries like step/time that are shared across PDC peers).
  fides::datamodel::DataObjectModel& PrimaryModel() const { return *this->Datasets.front().Model; }

  /// Build a model for a schema body, dispatching on required members:
  /// `cell_attributes` → CellGridModel, `coordinate_system` → DataSetModel.
  /// Works for legacy bodies and `datasets[]` entry bodies alike.
  std::unique_ptr<fides::datamodel::DataObjectModel> BuildModelFor(const rapidjson::Value& obj)
  {
    const bool hasCellAttributes = obj.HasMember("cell_attributes");
    const bool hasCoordinateSystem = obj.HasMember("coordinate_system");
    if (hasCellAttributes && hasCoordinateSystem)
    {
      throw std::runtime_error("Schema contains both 'cell_attributes' and "
                               "'coordinate_system' — model type is ambiguous.");
    }
    if (hasCellAttributes)
    {
      return std::make_unique<fides::datamodel::CellGridModel>();
    }
    if (hasCoordinateSystem)
    {
      return std::make_unique<fides::datamodel::DataSetModel>();
    }
    throw std::runtime_error(
      "Schema body must contain either 'cell_attributes' (cell_grid model) or "
      "'coordinate_system' (dataset model). Predefined data models (uniform, "
      "rectilinear, unstructured, unstructured_single, xgc, gtc) are activated by "
      "the Fides_Data_Model ADIOS attribute on a .bp file, not by JSON, and are "
      "not currently supported inside a datasets[] collection; expand them into "
      "the full coordinate_system + cell_set + fields body or use a single-dataset "
      "schema instead.");
  }

  /// Recursively translate the JSON assembly tree into an
  /// OutputBuilder::AssemblyNode, validating every leaf `datasets`
  /// reference against the declared dataset names.
  void ParseAssemblyTree(const rapidjson::Value& json,
                         fides::OutputBuilder::AssemblyNode& out,
                         const std::set<std::string>& declaredNames)
  {
    if (!json.IsObject())
    {
      throw std::runtime_error("Schema 'assembly' node must be an object.");
    }
    if (json.HasMember("name"))
    {
      if (!json["name"].IsString())
      {
        throw std::runtime_error("Schema 'assembly' node 'name' must be a string.");
      }
      out.Name = json["name"].GetString();
      if (!IsValidAssemblyNodeName(out.Name))
      {
        throw std::runtime_error(
          "Schema 'assembly' node name '" + out.Name +
          "' is not a valid vtkDataAssembly node name. Names must start with a "
          "letter or underscore; subsequent characters may be letters, digits, "
          "underscores, hyphens, or periods; and the names \"da*\" (longer than "
          "two characters) are reserved by VTK.");
      }
      if (out.Name == fides::kAutoNamesAssemblySubtree)
      {
        throw std::runtime_error(
          "Schema 'assembly' node name '" + out.Name +
          "' is reserved by Fides for the reader-synthesized auto-names subtree.");
      }
    }
    if (json.HasMember("datasets"))
    {
      if (!json["datasets"].IsArray())
      {
        throw std::runtime_error(
          "Schema 'assembly' node 'datasets' must be an array of dataset name strings.");
      }
      std::set<std::string> seenHere;
      for (const auto& ds : json["datasets"].GetArray())
      {
        if (!ds.IsString())
        {
          throw std::runtime_error("Schema 'assembly' 'datasets' entries must be strings.");
        }
        std::string name = ds.GetString();
        if (declaredNames.count(name) == 0)
        {
          throw std::runtime_error("Schema 'assembly' references unknown dataset '" + name +
                                   "'. Every name in a leaf 'datasets' list must match a "
                                   "schema-declared datasets[].name.");
        }
        if (!seenHere.insert(name).second)
        {
          throw std::runtime_error("Schema 'assembly' leaf lists dataset '" + name +
                                   "' twice. Each name may appear at most once per leaf.");
        }
        out.Datasets.push_back(name);
      }
    }
    if (json.HasMember("children"))
    {
      if (!json["children"].IsArray())
      {
        throw std::runtime_error("Schema 'assembly' 'children' must be an array.");
      }
      for (const auto& child : json["children"].GetArray())
      {
        out.Children.emplace_back();
        this->ParseAssemblyTree(child, out.Children.back(), declaredNames);
      }
    }
  }

  void ReadJSON(rapidjson::Document& document)
  {
    // Fides schemas are wrapped in a single, arbitrary root key (its name
    // identifies the data model but is otherwise ignored). Picking the
    // schema body via .begin() would silently consume the textually-first
    // member if a sibling were ever introduced, so insist on the
    // single-key invariant up front.
    if (document.GetObject().MemberCount() != 1)
    {
      throw std::runtime_error(
        "Fides schema must contain exactly one top-level key wrapping the schema body (got " +
        std::to_string(document.GetObject().MemberCount()) + ").");
    }
    auto m = document.GetObject().begin();
    const rapidjson::Value& obj = m->value;

    if (!obj.HasMember("data_sources"))
    {
      throw std::runtime_error("Missing data_sources member.");
    }
    this->ProcessDataSources(obj["data_sources"].GetArray());

    this->Datasets.clear();
    this->DataSetNames.clear();

    if (obj.HasMember("datasets"))
    {
      // Multi-dataset (PDC) schema: a top-level `datasets[]` array, each
      // entry a named child carrying its own `data_set` / `cell_grid`
      // body (inline or wrapped). Name doubles as the ADIOS group prefix
      // and the DATASET_SELECTION key.
      const auto& datasets = obj["datasets"];
      if (!datasets.IsArray())
      {
        throw std::runtime_error("Schema 'datasets' member must be an array.");
      }
      this->Datasets.reserve(datasets.Size());
      std::set<std::string> seenNames;
      for (rapidjson::SizeType i = 0; i < datasets.Size(); ++i)
      {
        const auto& entry = datasets[i];
        if (!entry.IsObject() || !entry.HasMember("name"))
        {
          throw std::runtime_error("Schema 'datasets' entry " + std::to_string(i) +
                                   " missing 'name'.");
        }
        const std::string name = entry["name"].GetString();
        if (!IsValidAssemblyNodeName(name))
        {
          throw std::runtime_error(
            "Schema 'datasets' entry name '" + name +
            "' is not a valid vtkDataAssembly node name. Names must start with a "
            "letter or underscore; subsequent characters may be letters, digits, "
            "underscores, hyphens, or periods; and the names \"da*\" (longer than "
            "two characters) are reserved by VTK.");
        }
        if (!seenNames.insert(name).second)
        {
          throw std::runtime_error("Schema 'datasets' lists name '" + name +
                                   "' twice. Each entry must have a unique name.");
        }
        const rapidjson::Value* body = &entry;
        if (entry.HasMember("data_set"))
        {
          body = &entry["data_set"];
        }
        else if (entry.HasMember("cell_grid"))
        {
          body = &entry["cell_grid"];
        }
        // PDC peers share one step axis; reject a per-dataset
        // step_information that would silently disagree with the
        // top-level StepSource.
        if (body->HasMember("step_information") || entry.HasMember("step_information"))
        {
          throw std::runtime_error(
            "step_information must be at the schema top level, not inside dataset '" + name +
            "'. PDC peers share a single step axis; move it next to 'datasets'.");
        }
        auto model = this->BuildModelFor(*body);
        model->ProcessJSON(*body, this->DataSources);
        this->Datasets.push_back({ name, std::move(model) });
        this->DataSetNames.push_back(name);
      }
    }
    else
    {
      // Legacy single-dataset schema: the whole body IS the model body;
      // one nameless entry.
      auto model = this->BuildModelFor(obj);
      model->ProcessJSON(obj, this->DataSources);
      this->Datasets.push_back({ std::string{}, std::move(model) });
    }

    if (obj.HasMember("step_information"))
    {
      this->ProcessStepInformation(obj["step_information"]);
    }

    // Optional top-level assembly (multi-dataset only). Name-referenced;
    // validated against declared dataset names so typos fail at parse.
    this->HasAssembly = false;
    this->Assembly = fides::OutputBuilder::AssemblyNode{};
    if (obj.HasMember("assembly"))
    {
      if (!this->IsMultiDataset())
      {
        throw std::runtime_error("Schema 'assembly' requires a multi-dataset 'datasets[]' block.");
      }
      std::set<std::string> declaredNames(this->DataSetNames.begin(), this->DataSetNames.end());
      this->ParseAssemblyTree(obj["assembly"], this->Assembly, declaredNames);
      this->HasAssembly = true;
    }
  }

  fides::metadata::MetaData ReadMetaData(const std::unordered_map<std::string, std::string>& paths,
                                         const std::string& groupName)
  {
    fides::metadata::MetaData metaData;

    if (this->IsMultiDataset() && groupName.empty())
    {
      // Aggregated view across all datasets: FIELDS is the union deduped
      // by (name, association). NUMBER_OF_BLOCKS is per-dataset only —
      // omit it here; callers query a specific dataset via
      // ReadMetaData(<name>).
      fides::metadata::Vector<fides::metadata::FieldInformation> fields;
      std::set<std::pair<std::string, fides::FieldAssociation>> seen;
      for (const auto& ds : this->Datasets)
      {
        auto infos = ds.Model->CollectFieldInformation(this->MetadataSource, this->DataSources);
        for (auto& f : infos)
        {
          if (seen.emplace(f.Name, f.Association).second)
          {
            fields.Data.push_back(std::move(f));
          }
        }
      }
      if (!fields.Data.empty())
      {
        metaData.Set(fides::keys::FIELDS(), fields);
      }
    }
    else
    {
      // Single dataset, or a specific dataset/group requested. In
      // multi-dataset mode the group name selects the dataset (its name
      // is the group prefix); fall back to the primary model otherwise.
      fides::datamodel::DataObjectModel* model = &this->PrimaryModel();
      if (this->IsMultiDataset() && !groupName.empty())
      {
        for (const auto& ds : this->Datasets)
        {
          if (ds.Name == groupName)
          {
            model = ds.Model.get();
            break;
          }
        }
      }
      size_t nBlocks = model->GetNumberOfBlocks(paths, this->DataSources, groupName);
      metaData.Set(fides::keys::NUMBER_OF_BLOCKS(), fides::metadata::Size(nBlocks));

      auto fieldInfos = model->CollectFieldInformation(this->MetadataSource, this->DataSources);
      if (!fieldInfos.empty())
      {
        fides::metadata::Vector<fides::metadata::FieldInformation> fields;
        fields.Data = std::move(fieldInfos);
        metaData.Set(fides::keys::FIELDS(), fields);
      }
    }

    auto it = this->DataSources.find(this->StepSource);
    if (it != this->DataSources.end())
    {
      auto ds = it->second;
      ds->OpenSource(paths, this->StepSource);

      // StreamingMode only gets set on the first PrepareNextStep, but
      // for streaming PrepareNextStep has to be called before ReadMetaData anyway
      if (this->StreamingMode)
      {
        // assumes time value is defined globally in this step, i.e for all groups.
        // if the time value is defined inside the group, then metadata::MetaData() will
        // need to set GROUP_SELECTION
        auto timeVec = ds->GetScalarVariable(this->TimeVariable, metadata::MetaData());
        if (!timeVec.empty())
        {
          auto& timeRA = timeVec[0];
          if (timeRA.NumValues == 1)
          {
            double timeVal = GetRawArrayValueAsDouble(timeRA, 0);
            fides::metadata::Time time(timeVal);
            metaData.Set(fides::keys::TIME_VALUE(), time);
          }
        }
      }
      else
      {
        // assumes time array is defined globally, i.e for all groups.
        // if the time array is defined inside the group, then metadata::MetaData() will
        // need to set GROUP_SELECTION
        auto timeVec = ds->GetTimeArray(this->TimeVariable, metadata::MetaData());
        if (!timeVec.empty())
        {
          auto& timeRA = timeVec[0];
          fides::metadata::Vector<double> time;
          CopyRawArrayToDoubleVector(timeRA, time.Data);
          metaData.Set(fides::keys::TIME_ARRAY(), time);
        }
      }
    }

    size_t nSteps = this->GetNumberOfSteps();
    if (nSteps > 0)
    {
      fides::metadata::Size nStepsM(nSteps);
      metaData.Set(fides::keys::NUMBER_OF_STEPS(), nStepsM);
    }

    return metaData;
  }

  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>& paths)
  {
    // In multi-dataset mode the "groups" are the declared dataset names
    // (the unification — dataset name = group prefix).
    if (this->IsMultiDataset())
    {
      return std::set<std::string>(this->DataSetNames.begin(), this->DataSetNames.end());
    }
    auto it = this->DataSources.find(this->StepSource);
    if (it != this->DataSources.end())
    {
      auto ds = it->second;
      ds->OpenSource(paths, this->StepSource);
      return this->PrimaryModel().GetGroupNames(paths, this->DataSources);
    }
    return {};
  }

  /// Unified item enumeration. For a `datasets[]` schema returns the
  /// schema-declared names in declaration order; \c paths is ignored.
  /// For a single-dataset schema read against a multi-group `.bp`,
  /// queries the step source via \c GetGroupNames and returns the
  /// discovered groups in name-sorted order. With an empty \c paths and
  /// a single-dataset schema, no source can be opened so the result is
  /// empty.
  std::vector<std::string> GetDataSetNames(
    const std::unordered_map<std::string, std::string>& paths)
  {
    if (this->IsMultiDataset())
    {
      return this->DataSetNames;
    }
    if (paths.empty())
    {
      return {};
    }
    auto groups = this->GetGroupNames(paths);
    return std::vector<std::string>(groups.begin(), groups.end());
  }

  void PostRead(DataContainer& container, const fides::metadata::MetaData& selections) const
  {
    if (!this->IsMultiDataset())
    {
      this->PrimaryModel().PostRead(container, selections);
      return;
    }

    // Multi-dataset: PostRead is per item, each against its own
    // single-dataset container view, dispatched to the item's model.
    // (For VTK the component ProcessVTK hooks are no-ops — the builder
    // fully materializes — but we still route through PostRead for
    // symmetry and future-proofing.)
#if FIDES_USE_VISKORES
    if (auto* coll = fides::GetDataAs<std::vector<viskores::cont::PartitionedDataSet>>(container))
    {
      for (size_t i = 0; i < coll->size() && i < this->ItemModels.size(); ++i)
      {
        std::vector<viskores::cont::DataSet> parts;
        for (viskores::Id p = 0; p < (*coll)[i].GetNumberOfPartitions(); ++p)
        {
          parts.push_back((*coll)[i].GetPartition(p));
        }
        auto tmp = fides::internal::Wrap(std::move(parts));
        this->ItemModels[i]->PostRead(*tmp, selections);
        if (auto* mutated = fides::GetDataAs<std::vector<viskores::cont::DataSet>>(*tmp))
        {
          (*coll)[i] = viskores::cont::PartitionedDataSet(*mutated);
        }
      }
      return;
    }
#endif
#if FIDES_USE_VTK
    if (auto* pdc = fides::GetDataAs<fides::VTKPDC>(container))
    {
      const unsigned int n = (*pdc)->GetNumberOfPartitionedDataSets();
      for (unsigned int i = 0; i < n && i < this->ItemModels.size(); ++i)
      {
        vtkSmartPointer<vtkPartitionedDataSet> pds = (*pdc)->GetPartitionedDataSet(i);
        auto tmp = fides::internal::Wrap(pds);
        this->ItemModels[i]->PostRead(*tmp, selections);
      }
      return;
    }
#endif
  }

  /// Any dataset requiring VTK forces the whole collection to VTK output.
  bool RequiresVTK() const
  {
    for (const auto& ds : this->Datasets)
    {
      if (ds.Model && ds.Model->RequiresVTK())
      {
        return true;
      }
    }
    return false;
  }

  std::unique_ptr<OutputBuilder> CreateBuilder(fides::DataSetType dsType) const
  {
    if (this->RequiresVTK() && dsType != fides::DataSetType::VTK)
    {
      throw std::runtime_error(
        "The active Fides data model only supports VTK output (e.g. a cell_grid "
        "schema produces a vtkCellGrid). Pass fides::DataSetType::VTK to ReadDataSet.");
    }
    if (dsType == fides::DataSetType::Viskores)
    {
#if FIDES_USE_VISKORES
      return std::make_unique<ViskoresBuilder>();
#else
      throw std::runtime_error(
        "Cannot create ViskoresBuilder, Viskores was not enabled at configure time");
#endif
    }
    else if (dsType == fides::DataSetType::VTK)
    {
#if FIDES_USE_VTK
      return std::make_unique<VTKBuilder>();
#else
      throw std::runtime_error("Cannot create VTKBuilder, VTK was not enabled at configure time");
#endif
    }
    else
    {
      std::cerr << "Unknown data set type: " << static_cast<int>(dsType) << std::endl;
      throw std::runtime_error("Unrecognized data set type");
    }
  }

  std::unique_ptr<fides::DataContainer> WrapBuilderOutput(OutputBuilder& builder)
  {
    const bool multiItem = builder.IsMultiItem();

    // Try to cast to ViskoresBuilder
#if FIDES_USE_VISKORES
    if (auto* ptr = dynamic_cast<fides::ViskoresBuilder*>(&builder))
    {
      if (multiItem)
      {
        return fides::internal::Wrap(ptr->GetResultCollection());
      }
      return fides::internal::Wrap(std::move(ptr->GetDataSets()));
    }
#endif

    // Try to cast to VTKBuilder
#if FIDES_USE_VTK
    if (auto* ptr = dynamic_cast<fides::VTKBuilder*>(&builder))
    {
      if (multiItem)
      {
        return fides::internal::Wrap(ptr->GetResultCollection());
      }
      // No std::move: GetResult() returns by value, so the prvalue is elided
      // directly into Wrap's parameter. std::move would disable that elision.
      return fides::internal::Wrap(ptr->GetResult());
    }
#endif

    throw std::runtime_error("Unable to wrap OutputBuilder results.");
  }

  void DoAllReads()
  {
    for (const auto& source : this->DataSources)
    {
      source.second->DoAllReads();
    }
  }

  // For BeginStep, we loop on a DataSource if its status is NotReady, because
  // otherwise with multiple sources, we can get into a weird situation where
  // step i for DataSource A may take longer to write than step i for DataSource B.
  // So if we return NotReady in this situation, then on the next call to BeginStep,
  // DataSource A may finally be ready for step i, but it's possible that DataSource
  // B is ready for i+1. Or we could run into an ADIOS error, due to not having an
  // EndStep for DataSource B before the next BeginStep in this type of situation.
  // Also this function only returns EndOfStream when all DataSources have reached
  // EndOfStream. In a situation with multiple source, users shouldn't need to worry
  // about when a single DataSource reaches EndOfStream, because Fides handles this internally
  // (e.g., having the variables making up a mesh marked as static and only reading initially).
  // So the user should only care about PrepareNextStep returning EndOfStream when all
  // DataSources are at the end of their Streams.
  StepStatus BeginStep(const std::unordered_map<std::string, std::string>& paths)
  {
    // PrepareNextStep flips this->StreamingMode to true on first call even if
    // the reader was constructed with streamSteps=false. Propagate so any
    // source not yet opened is opened in streaming mode rather than the
    // SetupReader-time setting; without this, BeginStep against a BP5 source
    // opened in random-access mode throws. StreamingMode lives on
    // ADIOSDataSource only, so cast first.
    for (const auto& source : this->DataSources)
    {
      ADIOSDataSource* adiosSrc = dynamic_cast<ADIOSDataSource*>(source.second.get());
      if (adiosSrc)
      {
        adiosSrc->StreamingMode = this->StreamingMode;
      }
    }
    // We can't have OpenSource and BeginStep in the same loop because if we have multiple
    // sources and they are SST, we may get a hang depending on the settings of SST.
    // Note that if the SST writer settings has RendezvousReaderCount >= 1, then we may
    // still hang anyway.
    for (const auto& source : this->DataSources)
    {
      auto& ds = *(source.second);
      std::string name = source.first;
      ds.OpenSource(paths, name);
    }

    StepStatus retVal = StepStatus::EndOfStream;
    for (const auto& source : this->DataSources)
    {
      auto& ds = *(source.second);
      std::string name = source.first;
      auto rc = ds.BeginStep();
      while (rc == StepStatus::NotReady)
      {
        rc = ds.BeginStep();
      }
      if (rc == StepStatus::OK)
      {
        retVal = StepStatus::OK;
      }
      else if (rc == StepStatus::OtherError)
      {
        // Before we were changing StepStatus to NotReady in this case, but it causes an infinite loop.
        // The only time I've seen OtherError is in SST when the writer crashed or was interrupted by me.
        // I think we should just warn the user that there was an OtherError but change it to EndOfStream
        // and try to process other sources.
        std::cerr << "WARNING: BeginStep for source '" << name << "' returned OtherError"
                  << std::endl;
        retVal = StepStatus::EndOfStream;
      }
    }
    return retVal;
  }

  StepStatus InternalSourceBeginStep()
  {
    // In the case of streaming mode (whether it's SST or BP5), we may only have the InternalSource
    // at this point, so we need to BeginStep for it. This is separate so we can call it separately
    // from other data sources. The DataSource has a flag for DoNotCallNextBeginStep, which we set to
    // true in this case. Then when the user calls reader.PrepareNextStep(), BeginStep will only get
    // called on the other DataSources, not the one that is the InternalSource.
    StepStatus retVal = StepStatus::EndOfStream;
    if (this->InternalSource)
    {
      auto rc = this->InternalSource->BeginStep();
      while (rc == StepStatus::NotReady)
      {
        rc = this->InternalSource->BeginStep();
      }
      if (rc == StepStatus::OK)
      {
        retVal = StepStatus::OK;
      }
    }
    this->InternalSource->DoNotCallNextBeginStep = true;
    return retVal;
  }

  void EndStep()
  {
    for (const auto& source : this->DataSources)
    {
      source.second->EndStep();
    }
  }

  // Core orchestration: reads data model objects into the OutputBuilder
  /// Drop FIELDS entries the model does not own, so a caller selection
  /// spanning PDC peers doesn't trip a sub-model's "unknown field" guard.
  void FilterFieldsForModel(fides::metadata::MetaData& sel,
                            fides::datamodel::DataObjectModel& model)
  {
    if (!sel.Has(fides::keys::FIELDS()))
    {
      return;
    }
    using FieldVec = fides::metadata::Vector<fides::metadata::FieldInformation>;
    const auto& wanted = sel.Get<FieldVec>(fides::keys::FIELDS());
    auto known = model.CollectFieldInformation(this->MetadataSource, this->DataSources);
    std::set<std::pair<std::string, fides::FieldAssociation>> knownSet;
    for (const auto& f : known)
    {
      knownSet.emplace(f.Name, f.Association);
    }
    FieldVec filtered;
    for (const auto& f : wanted.Data)
    {
      if (knownSet.count({ f.Name, f.Association }))
      {
        filtered.Data.push_back(f);
      }
    }
    sel.Set(fides::keys::FIELDS(), filtered);
  }

  void ReadDataSetInternal(const std::unordered_map<std::string, std::string>& paths,
                           const fides::metadata::MetaData& selections,
                           fides::OutputBuilder& builder)
  {
    // DATASET_SELECTION: the unified item filter (empty/absent = all),
    // applied to both the datasets[] path and the group-iteration path.
    std::set<std::string> wanted;
    if (selections.Has(fides::keys::DATASET_SELECTION()))
    {
      const auto& names = selections.Get<fides::metadata::Vector<fides::metadata::String>>(
        fides::keys::DATASET_SELECTION());
      for (const auto& n : names.Data)
      {
        wanted.insert(n.Data);
      }
    }
    auto isWanted = [&](const std::string& name) {
      return wanted.empty() || wanted.count(name) > 0;
    };
    this->ItemModels.clear();

    if (this->IsMultiDataset())
    {
      // One collection item per selected datasets[] entry; the entry name
      // is the ADIOS group prefix.
      for (const auto& ds : this->Datasets)
      {
        if (!isWanted(ds.Name))
        {
          continue;
        }
        builder.CreateItem(ds.Name);
        this->ItemModels.push_back(ds.Model.get());
        fides::metadata::MetaData perDataset = selections;
        perDataset.Remove(fides::keys::DATASET_SELECTION());
        perDataset.Set(fides::keys::GROUP_SELECTION(), fides::metadata::String(ds.Name));
        this->FilterFieldsForModel(perDataset, *ds.Model);
        ds.Model->Read(paths, this->DataSources, perDataset, builder);
      }
      if (this->HasAssembly)
      {
        builder.SetAssembly(this->Assembly);
      }
      return;
    }

    // Single-dataset schema. Group-iteration unification: a source with
    // multiple ADIOS variable groups and no explicit GROUP_SELECTION
    // produces one collection item per (selected) group.
    auto& model = this->PrimaryModel();
    if (!selections.Has(fides::keys::GROUP_SELECTION()))
    {
      auto groups = model.GetGroupNames(paths, this->DataSources);
      if (groups.size() > 1)
      {
        for (const auto& g : groups)
        {
          if (!isWanted(g))
          {
            continue;
          }
          builder.CreateItem(g);
          this->ItemModels.push_back(&model);
          fides::metadata::MetaData perGroup = selections;
          perGroup.Remove(fides::keys::DATASET_SELECTION());
          perGroup.Set(fides::keys::GROUP_SELECTION(), fides::metadata::String(g));
          model.Read(paths, this->DataSources, perGroup, builder);
        }
        return;
      }
    }

    // Legacy single read (one group or fewer, or an explicit
    // GROUP_SELECTION). Byte-identical to the pre-PDC path.
    model.Read(paths, this->DataSources, selections, builder);
  }

#if FIDES_USE_MPI
  MPI_Comm Comm = MPI_COMM_NULL;
#endif

  std::shared_ptr<rapidjson::Document> DataModelDocument;
  DataSourcesType DataSources;
  std::shared_ptr<fides::predefined::InternalMetadataSource> MetadataSource = nullptr;
  /// Schema-declared datasets. Single-dataset schema = 1 entry, empty
  /// Name; `datasets[]` schema = N named entries.
  std::vector<DatasetEntry> Datasets;
  /// Schema-declared dataset names (`datasets[]` order). Empty for a
  /// legacy single-dataset schema (the "is this a PDC schema?" probe).
  std::vector<std::string> DataSetNames;
  /// Optional assembly tree (multi-dataset schemas only).
  bool HasAssembly = false;
  fides::OutputBuilder::AssemblyNode Assembly;
  /// Model backing each collection item, in CreateItem order. Populated
  /// by ReadDataSetInternal so PostRead can run per item.
  std::vector<fides::datamodel::DataObjectModel*> ItemModels;
  std::string StepSource;
  std::string TimeVariable;
  bool StreamingMode = false;

  std::shared_ptr<fides::io::ADIOSDataSource> InternalSource = nullptr;
};

DataSetReader::DataSetReader() = default;

bool DataSetReader::CheckForDataModelAttribute(const std::string& filename,
                                               const std::string& attrName /*="Fides_Data_Model"*/)
{
  bool found = false;
  auto source = std::make_shared<ADIOSDataSource>();
  source->Mode = fides::io::FileNameMode::Relative;
  source->FileName = filename;

  // Here we want to make sure when creating a data source that we don't start ADIOS in
  // MPI mode. vtkFidesReader::CanReadFile uses this function to see if it can read
  // a given ADIOS file. When running in parallel, only rank 0 will execute this, so we need
  // to make sure that we don't do collective calls.
  source->StreamingMode = false;
  source->OpenSource(filename, false);
  if (source->GetAttributeType(attrName) == "string")
  {
    std::vector<std::string> result = source->ReadAttribute<std::string>(attrName);
    if (!result.empty())
    {
      found = predefined::DataModelSupported(result[0]);
    }
  }
  if (!found)
  {
    // Fides_Data_Model attribute not found, now look for a fides/schema attribute
    std::string schemaAttr = "fides/schema";
    if (source->GetAttributeType(schemaAttr) == "string")
    {
      found = true;
    }
  }
  return found;
}

DataSetReader::DataSetReader(const std::string& dataModel,
                             DataModelInput inputType /*=DataModelInput::JSONFile*/,
                             const Params& params,
                             bool createSharedPoints /*=false*/)
  : DataSetReader(dataModel, inputType, false, params, createSharedPoints)
{
}

DataSetReader::DataSetReader(const std::string& dataModel,
                             DataModelInput inputType,
                             bool streamSteps,
                             const Params& params,
                             bool createSharedPoints /*=false*/)
  : Impl(new DataSetReaderImpl(dataModel, inputType, streamSteps, params, createSharedPoints))
{
}

#if FIDES_USE_MPI
DataSetReader::DataSetReader(const std::string& dataModel,
                             DataModelInput inputType,
                             bool streamSteps,
                             MPI_Comm comm,
                             const Params& params,
                             bool createSharedPoints /*=false*/)
{
  this->Impl.reset(
    new DataSetReaderImpl(dataModel, inputType, streamSteps, comm, params, createSharedPoints));
}
#endif

DataSetReader::~DataSetReader() = default;

fides::metadata::MetaData DataSetReader::ReadMetaData(
  const std::unordered_map<std::string, std::string>& paths,
  const std::string& groupName /*=""*/)
{
  return this->Impl->ReadMetaData(paths, groupName);
}

std::unique_ptr<fides::DataContainer> DataSetReader::ReadDataSet(
  const std::unordered_map<std::string, std::string>& paths,
  const fides::metadata::MetaData& selections,
  fides::DataSetType dsType)
{
  std::unique_ptr<OutputBuilder> builder = this->Impl->CreateBuilder(dsType);

  this->Impl->ReadDataSetInternal(paths, selections, *builder);

  if (this->Impl->StreamingMode)
  {
    this->Impl->EndStep();
  }
  else
  {
    this->Impl->DoAllReads();
  }
  builder->Finalize();

  // Wrap it for passage through public api
  auto container = this->Impl->WrapBuilderOutput(*builder);

  // Pass to PostRead (which mutates it in place)
  this->Impl->PostRead(*container, selections);

  // Return the wrapper to the user
  return container;
}

std::unique_ptr<fides::DataContainer> DataSetReader::ReadDataSet(
  const fides::metadata::MetaData& selections,
  fides::DataSetType dsType)
{
  return this->ReadDataSet(std::unordered_map<std::string, std::string>{}, selections, dsType);
}

void DataSetReader::SetDataSourceParameters(const std::string& source,
                                            const DataSourceParams& params)
{
  this->Impl->SetDataSourceParameters(source, params);
}

void DataSetReader::SetDataSourceIO(const std::string& source, void* io)
{
  this->Impl->SetDataSourceIO(source, io);
}

void DataSetReader::SetDataSourceIO(const std::string& source, const std::string& io)
{
  this->Impl->SetDataSourceIO(source, io);
}

std::vector<std::string> DataSetReader::GetDataSourceNames()
{
  std::vector<std::string> names;
  for (const auto& source : this->Impl->DataSources)
  {
    names.push_back(source.first);
  }
  return names;
}

std::set<std::string> DataSetReader::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths)
{
  return this->Impl->GetGroupNames(paths);
}

std::vector<std::string> DataSetReader::GetDataSetNames(
  const std::unordered_map<std::string, std::string>& paths)
{
  return this->Impl->GetDataSetNames(paths);
}

StepStatus DataSetReader::PrepareNextStep(const std::unordered_map<std::string, std::string>& paths)
{
  this->Impl->StreamingMode = true;
  return this->Impl->BeginStep(paths);
}

void DataSetReader::Close()
{
  for (const auto& source : this->Impl->DataSources)
  {
    source.second->Close();
  }
}

} // end namespace io
} // end namespace fides
