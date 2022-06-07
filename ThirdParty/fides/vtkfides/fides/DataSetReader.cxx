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
#include <fides/predefined/DataModelFactory.h>
#include <fides/predefined/DataModelHelperFunctions.h>
#include <fides/predefined/InternalMetadataSource.h>

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

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <fides/CellSet.h>
#include <fides/CoordinateSystem.h>
#include <fides/DataSource.h>
#include <fides/Field.h>
#include <fides/Keys.h>

namespace fides
{
namespace io
{

using DataSourceType = fides::io::DataSource;
using DataSourcesType = std::unordered_map<std::string, std::shared_ptr<DataSourceType>>;

class DataSetReader::DataSetReaderImpl
{
public:
  DataSetReaderImpl(const std::string dataModel, DataModelInput inputType, const Params& params)
  {
    this->Cleanup();
    if (inputType == DataModelInput::BPFile)
    {
      // in this case the bp file passed in becomes our MetadataSource
      // which is used to select a predefined data model
      this->MetadataSource.reset(new fides::predefined::InternalMetadataSource(dataModel));
      auto dm = predefined::DataModelFactory::GetInstance().CreateDataModel(this->MetadataSource);
      this->ReadJSON(dm->GetDOM());
    }
    else
    {
      rapidjson::Document doc = this->GetJSONDocument(dataModel, inputType);
      this->ParsingChecks(doc, dataModel, inputType);
      this->ReadJSON(doc);
    }

    this->SetDataSourceParameters(params);
  }

  virtual ~DataSetReaderImpl() { this->Cleanup(); }

  void Cleanup()
  {
    this->DataSources.clear();
    this->CoordinateSystem.reset();
    this->CellSet.reset();
  }

  rapidjson::Document GetJSONDocument(const std::string& dataModel, DataModelInput inputType)
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

  void SetDataSourceParameters(const std::string source, const DataSourceParams& params)
  {
    auto it = this->DataSources.find(source);
    if (it == this->DataSources.end())
    {
      throw std::runtime_error("Source name was not found in DataSources.");
    }
    auto& ds = *(it->second);
    ds.SetDataSourceParameters(params);
  }

  void SetDataSourceIO(const std::string source, void* io)
  {
    auto it = this->DataSources.find(source);
    if (it == this->DataSources.end())
    {
      throw std::runtime_error("Source name was not found in DataSources.");
    }
    auto& ds = *(it->second);
    ds.SetDataSourceIO(io);
  }

  void SetDataSourceIO(const std::string source, const std::string& io)
  {
    auto it = this->DataSources.find(source);
    if (it == this->DataSources.end())
    {
      throw std::runtime_error("Source name was not found in DataSources.");
    }
    auto& ds = *(it->second);
    ds.SetDataSourceIO(io);
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
      if (!dataSource.GetObject().HasMember("filename_mode"))
      {
        throw std::runtime_error("data_source objects must have filename_mode.");
      }
      std::string filename_mode = dataSource.GetObject()["filename_mode"].GetString();
      if (filename_mode.empty())
      {
        throw std::runtime_error("data_source filename_mode must be a non-empty string.");
      }
      auto source = std::make_shared<DataSourceType>();
      if (filename_mode == "input")
      {
        source->Mode = fides::io::FileNameMode::Input;
      }
      else if (filename_mode == "relative")
      {
        source->Mode = fides::io::FileNameMode::Relative;
        if (!dataSource.GetObject().HasMember("filename"))
        {
          throw std::runtime_error("data_source objects must have filename.");
        }
        source->FileName = dataSource.GetObject()["filename"].GetString();
      }
      else
      {
        throw std::runtime_error("data_source filename_mode must be input or relative.");
      }

      this->DataSources[name] = source;
    }
  }

  void ProcessCoordinateSystem(const rapidjson::Value& coordSys)
  {
    this->CoordinateSystem = std::make_shared<fides::datamodel::CoordinateSystem>();
    this->CoordinateSystem->ObjectName = "coordinate_system";

    this->CoordinateSystem->ProcessJSON(coordSys, this->DataSources);
  }

  void ProcessCellSet(const rapidjson::Value& cellSet)
  {
    this->CellSet = std::make_shared<fides::datamodel::CellSet>();
    this->CellSet->ObjectName = "cell_set";

    this->CellSet->ProcessJSON(cellSet, this->DataSources);
  }

  std::shared_ptr<fides::datamodel::Field> ProcessField(const rapidjson::Value& fieldJson)
  {
    if (!fieldJson.IsObject())
    {
      throw std::runtime_error("field needs to be an object.");
    }
    auto field = std::make_shared<fides::datamodel::Field>();
    field->ProcessJSON(fieldJson, this->DataSources);
    field->ObjectName = "field";
    return field;
  }

  void ProcessFields(const rapidjson::Value& fields)
  {
    this->Fields.clear();
    if (!fields.IsArray())
    {
      throw std::runtime_error("fields is not an array.");
    }
    auto fieldsArray = fields.GetArray();
    for (auto& field : fieldsArray)
    {
      auto fieldPtr = this->ProcessField(field);
      this->Fields[std::make_pair(fieldPtr->Name, fieldPtr->Association)] = fieldPtr;
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

  template <typename ValueType>
  const rapidjson::Value& FindAndReturnObject(ValueType& root, const std::string name)
  {
    if (!root.HasMember(name.c_str()))
    {
      throw std::runtime_error("Missing " + name + " member.");
    }
    auto& val = root[name.c_str()];
    if (!val.IsObject())
    {
      throw std::runtime_error(name + " is expected to be an object.");
    }
    return val;
  }

  void ParsingChecks(rapidjson::Document& document,
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

  void ReadJSON(rapidjson::Document& document)
  {
    auto m = document.GetObject().begin();
    const auto obj = m->value.GetObject();
    if (!obj.HasMember("data_sources"))
    {
      throw std::runtime_error("Missing data_sources member.");
    }
    this->ProcessDataSources(obj["data_sources"].GetArray());

    if (obj.HasMember("number_of_planes"))
    {
      auto& nPlanes = obj["number_of_planes"];
      fides::datamodel::XGCCommon::ProcessNumberOfPlanes(nPlanes, this->DataSources);
    }

    if (!obj.HasMember("coordinate_system"))
    {
      throw std::runtime_error("Missing coordinate_system member.");
    }
    auto& cs = this->FindAndReturnObject(obj, "coordinate_system");
    this->ProcessCoordinateSystem(cs);

    if (!obj.HasMember("cell_set"))
    {
      throw std::runtime_error("Missing cell_set member.");
    }
    auto& cells = this->FindAndReturnObject(obj, "cell_set");
    this->ProcessCellSet(cells);

    if (obj.HasMember("fields"))
    {
      auto& fields = obj["fields"];
      this->ProcessFields(fields);
    }

    if (obj.HasMember("step_information"))
    {
      auto& sinf = obj["step_information"];
      this->ProcessStepInformation(sinf);
    }
  }

  std::vector<vtkm::cont::CoordinateSystem> ReadCoordinateSystem(
    const std::unordered_map<std::string, std::string>& paths,
    const fides::metadata::MetaData& selections)
  {
    if (!this->CoordinateSystem)
    {
      throw std::runtime_error("Cannot read missing coordinate system.");
    }
    return this->CoordinateSystem->Read(paths, this->DataSources, selections);
  }

  std::vector<vtkm::cont::UnknownCellSet> ReadCellSet(
    const std::unordered_map<std::string, std::string>& paths,
    const fides::metadata::MetaData& selections)
  {
    if (!this->CellSet)
    {
      throw std::runtime_error("Cannot read missing cell set.");
    }
    return this->CellSet->Read(paths, this->DataSources, selections);
  }

  // updates this->Fields if we have any wildcard fields. Should be used
  // in ReadMetaData()
  void ExpandWildcardFields()
  {
    auto it = this->Fields.begin();
    while (it != this->Fields.end())
    {
      auto& origField = it->second;
      // find fields to expand
      if (origField->IsWildcardField())
      {
        auto lists = origField->GetWildcardFieldLists(this->MetadataSource);
        // need to add each name, association pair to Fields
        // as well as create the associated Field object
        auto& names = lists.Names;
        auto& associations = lists.Associations;

        for (size_t i = 0; i < names.size(); ++i)
        {
          std::string isVector = "auto";
          std::string source = "source";
          std::string arrayType = "basic";
          if (!lists.IsVector.empty() && i < lists.IsVector.size())
          {
            isVector = lists.IsVector[i];
          }
          if (!lists.Sources.empty() && i < lists.Sources.size())
          {
            source = lists.Sources[i];
          }
          if (!lists.ArrayTypes.empty() && i < lists.ArrayTypes.size())
          {
            arrayType = lists.ArrayTypes[i];
          }

          // the wildcard field uses an ArrayPlaceholder. Now we have enough info
          // to create the actual JSON for the Array object for this Field. This can
          // then be passed to Field.ProcessExpandedField which will use it to create
          // the actual array object.
          rapidjson::Document arrayObj;
          arrayObj = predefined::CreateFieldArrayDoc(names[i], source, arrayType, isVector);

          if (!arrayObj.HasMember("array"))
          {
            throw std::runtime_error("Field Array Object was not created correctly");
          }
          auto fieldPtr = std::make_shared<fides::datamodel::Field>();
          fieldPtr->ProcessExpandedField(names[i], associations[i], arrayObj, this->DataSources);
          fieldPtr->ObjectName = "field";
          this->Fields[std::make_pair(fieldPtr->Name, fieldPtr->Association)] = fieldPtr;
        }

        // remove the wildcard field now that we're done expanding it
        it = this->Fields.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  struct GetTimeValueFunctor
  {
    template <typename T, typename S>
    VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>& array, double& time) const
    {
      time = static_cast<double>(array.ReadPortal().Get(0));
    }
  };

  fides::metadata::MetaData ReadMetaData(const std::unordered_map<std::string, std::string>& paths)
  {
    if (!this->CoordinateSystem)
    {
      throw std::runtime_error("Cannot read missing coordinate system.");
    }
    size_t nBlocks = this->CoordinateSystem->GetNumberOfBlocks(paths, this->DataSources);
    fides::metadata::MetaData metaData;
    fides::metadata::Size nBlocksM(nBlocks);
    metaData.Set(fides::keys::NUMBER_OF_BLOCKS(), nBlocksM);

    if (!this->Fields.empty())
    {
      // updates this->Fields if necessary
      this->ExpandWildcardFields();
      fides::metadata::Vector<fides::metadata::FieldInformation> fields;
      for (auto& item : this->Fields)
      {
        auto& field = item.second;
        fides::metadata::FieldInformation afield(field->Name, field->Association);
        fields.Data.push_back(afield);
      }
      metaData.Set(fides::keys::FIELDS(), fields);
    }

    size_t nSteps = this->GetNumberOfSteps();
    if (nSteps > 0)
    {
      fides::metadata::Size nStepsM(nSteps);
      metaData.Set(fides::keys::NUMBER_OF_STEPS(), nStepsM);
    }

    auto it = this->DataSources.find(this->StepSource);
    if (it != this->DataSources.end())
    {
      auto ds = it->second;
      auto itr = paths.find(this->StepSource);
      if (itr == paths.end())
      {
        throw std::runtime_error("Could not find data_source with name " + this->StepSource +
                                 " among the input paths.");
      }
      std::string path = itr->second + ds->FileName;
      ds->OpenSource(path);

      // StreamingMode only gets set on the first PrepareNextStep, but
      // for streaming PrepareNextStep has to be called before ReadMetaData anyway
      if (this->StreamingMode)
      {
        auto timeVec = ds->GetScalarVariable(this->TimeVariable, metadata::MetaData());
        if (!timeVec.empty())
        {
          auto& timeAH = timeVec[0];
          if (timeAH.GetNumberOfValues() == 1)
          {
            double timeVal;
            timeAH.CastAndCallForTypes<vtkm::TypeListScalarAll,
                                       vtkm::List<vtkm::cont::StorageTagBasic>>(
              GetTimeValueFunctor(), timeVal);
            fides::metadata::Time time(timeVal);
            metaData.Set(fides::keys::TIME_VALUE(), time);
          }
        }
      }
      else
      {
        auto timeVec = ds->GetTimeArray(this->TimeVariable, metadata::MetaData());
        if (!timeVec.empty())
        {
          auto& timeAH = timeVec[0];
          if (!timeAH.CanConvert<vtkm::cont::ArrayHandle<vtkm::Float64>>())
          {
            std::runtime_error("can't convert time array to double");
          }
          vtkm::cont::ArrayHandle<vtkm::Float64> timeCasted =
            timeAH.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float64>>();

          auto timePortal = timeCasted.ReadPortal();
          fides::metadata::Vector<double> time;
          time.Data.resize(timePortal.GetNumberOfValues());
          vtkm::cont::ArrayPortalToIterators<decltype(timePortal)> iterators(timePortal);
          std::copy(iterators.GetBegin(), iterators.GetEnd(), time.Data.begin());
          metaData.Set(fides::keys::TIME_ARRAY(), time);
        }
      }
    }

    return metaData;
  }

  void PostRead(std::vector<vtkm::cont::DataSet>& pds, const fides::metadata::MetaData& selections)
  {
    this->CoordinateSystem->PostRead(pds, selections);
    this->CellSet->PostRead(pds, selections);
    for (auto& f : this->Fields)
      f.second->PostRead(pds, selections);
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
    StepStatus retVal = StepStatus::EndOfStream;
    for (const auto& source : this->DataSources)
    {
      auto& ds = *(source.second);
      std::string name = source.first;
      auto itr = paths.find(name);
      if (itr == paths.end())
      {
        throw std::runtime_error("Could not find data_source with name " + name +
                                 " among the input paths.");
      }
      std::string path = itr->second + ds.FileName;
      ds.OpenSource(path);
      auto rc = ds.BeginStep();
      while (rc == StepStatus::NotReady)
      {
        rc = ds.BeginStep();
      }
      if (rc == StepStatus::OK)
      {
        retVal = StepStatus::OK;
      }
    }
    return retVal;
  }

  void EndStep()
  {
    for (const auto& source : this->DataSources)
    {
      source.second->EndStep();
    }
  }

  DataSourcesType DataSources;
  std::shared_ptr<fides::predefined::InternalMetadataSource> MetadataSource = nullptr;
  std::shared_ptr<fides::datamodel::CoordinateSystem> CoordinateSystem = nullptr;
  std::shared_ptr<fides::datamodel::CellSet> CellSet = nullptr;
  using FieldsKeyType = std::pair<std::string, vtkm::cont::Field::Association>;
  std::map<FieldsKeyType, std::shared_ptr<fides::datamodel::Field>> Fields;
  std::string StepSource;
  std::string TimeVariable;
  bool StreamingMode = false;
};

bool DataSetReader::CheckForDataModelAttribute(const std::string& filename,
                                               const std::string& attrName /*="Fides_Data_Model"*/)
{
  bool found = false;
  auto source = std::make_shared<DataSourceType>();
  source->Mode = fides::io::FileNameMode::Relative;
  source->FileName = filename;

  // Here we want to make sure when creating a data source that we don't start ADIOS in
  // MPI mode. vtkFidesReader::CanReadFile uses this function to see if it can read
  // a given ADIOS file. When running in parallel, only rank 0 will execute this, so we need
  // to make sure that we don't do collective calls.
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

DataSetReader::DataSetReader(const std::string dataModel,
                             DataModelInput inputType /*=DataModelInput::JSONFile*/,
                             const Params& params)
  : Impl(nullptr)
{
  if (inputType != DataModelInput::BPFile)
  {
    this->Impl.reset(new DataSetReaderImpl(dataModel, inputType, params));
    return;
  }

  // we have a BPFile and we need to look for either Fides_Data_Model
  // or fides/schema attributes
  std::string fidesAttr = "Fides_Data_Model";
  auto source = std::make_shared<DataSourceType>();
  source->Mode = fides::io::FileNameMode::Relative;
  source->FileName = dataModel; // in this case dataModel should be bp filename
  source->OpenSource(dataModel);
  if (source->GetAttributeType(fidesAttr) == "string")
  {
    std::vector<std::string> result = source->ReadAttribute<std::string>(fidesAttr);
    if (!result.empty())
    {
      if (predefined::DataModelSupported(result[0]))
      {
        this->Impl.reset(new DataSetReaderImpl(dataModel, inputType, params));
        return;
      }
    }
  }

  // Fides_Data_Model either not found or value was incorrect, now look for fides/schema
  std::string schemaAttr = "fides/schema";
  if (source->GetAttributeType(schemaAttr) == "string")
  {
    auto schema = source->ReadAttribute<std::string>(schemaAttr);
    if (!schema.empty())
    {
      this->Impl.reset(new DataSetReaderImpl(schema[0], DataModelInput::JSONString, params));
      return;
    }
  }

  throw std::runtime_error("InputType is a BP File, but valid 'Fides_Data_Model' or "
                           "'fides/schema' attributes could not be found in the file.");
}

DataSetReader::~DataSetReader() = default;

fides::metadata::MetaData DataSetReader::ReadMetaData(
  const std::unordered_map<std::string, std::string>& paths)
{
  return this->Impl->ReadMetaData(paths);
}

vtkm::cont::PartitionedDataSet DataSetReader::ReadDataSet(
  const std::unordered_map<std::string, std::string>& paths,
  const fides::metadata::MetaData& selections)
{
  auto ds = this->ReadDataSetInternal(paths, selections);
  if (this->Impl->StreamingMode)
  {
    this->Impl->EndStep();
  }
  else
  {
    this->Impl->DoAllReads();
  }
  this->Impl->PostRead(ds, selections);

  // for(size_t i=0; i<ds.GetNumberOfPartitions(); i++)
  // {
  //   vtkm::io::writer::VTKDataSetWriter writer(
  //     "output" + std::to_string(i) + ".vtk");
  //   writer.WriteDataSet(ds.GetPartition(i));
  // }

  return vtkm::cont::PartitionedDataSet(ds);
}

StepStatus DataSetReader::PrepareNextStep(const std::unordered_map<std::string, std::string>& paths)
{
  this->Impl->StreamingMode = true;
  return this->Impl->BeginStep(paths);
}

vtkm::cont::PartitionedDataSet DataSetReader::ReadStep(
  const std::unordered_map<std::string, std::string>& paths,
  const fides::metadata::MetaData& selections)
{
  return this->ReadDataSet(paths, selections);
}

// Returning vector of DataSets instead of PartitionedDataSet because
// PartitionedDataSet::GetPartition always returns a const DataSet, but
// we may need to update the DataSet in the PostRead call
std::vector<vtkm::cont::DataSet> DataSetReader::ReadDataSetInternal(
  const std::unordered_map<std::string, std::string>& paths,
  const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::CoordinateSystem> coordSystems =
    this->Impl->ReadCoordinateSystem(paths, selections);
  std::vector<vtkm::cont::UnknownCellSet> cellSets = this->Impl->ReadCellSet(paths, selections);
  size_t nPartitions = cellSets.size();
  std::vector<vtkm::cont::DataSet> dataSets(nPartitions);
  for (size_t i = 0; i < nPartitions; i++)
  {
    if (i < coordSystems.size())
    {
      dataSets[i].AddCoordinateSystem(coordSystems[i]);
    }
    if (i < cellSets.size())
    {
      dataSets[i].SetCellSet(cellSets[i]);
    }
  }

  if (selections.Has(fides::keys::FIELDS()))
  {
    using FieldInfoType = fides::metadata::Vector<fides::metadata::FieldInformation>;
    auto& fields = selections.Get<FieldInfoType>(fides::keys::FIELDS());
    for (auto& field : fields.Data)
    {
      auto itr = this->Impl->Fields.find(std::make_pair(field.Name, field.Association));
      if (itr != this->Impl->Fields.end())
      {
        std::vector<vtkm::cont::Field> fieldVec =
          itr->second->Read(paths, this->Impl->DataSources, selections);
        for (size_t i = 0; i < nPartitions; i++)
        {
          if (i < fieldVec.size())
          {
            dataSets[i].AddField(fieldVec[i]);
          }
        }
      }
    }
  }
  else
  {
    for (auto& field : this->Impl->Fields)
    {
      std::vector<vtkm::cont::Field> fields =
        field.second->Read(paths, this->Impl->DataSources, selections);
      for (size_t i = 0; i < nPartitions; i++)
      {
        if (i < fields.size())
        {
          dataSets[i].AddField(fields[i]);
        }
      }
    }
  }

  return dataSets;
}

void DataSetReader::SetDataSourceParameters(const std::string source,
                                            const DataSourceParams& params)
{
  this->Impl->SetDataSourceParameters(source, params);
}

void DataSetReader::SetDataSourceIO(const std::string source, void* io)
{
  this->Impl->SetDataSourceIO(source, io);
}

void DataSetReader::SetDataSourceIO(const std::string source, const std::string& io)
{
  this->Impl->SetDataSourceIO(source, io);
}

FIDES_DEPRECATED_SUPPRESS_BEGIN
std::shared_ptr<fides::datamodel::FieldDataManager> DataSetReader::GetFieldData()
{
  // Function to be removed in next version
  return nullptr;
}
FIDES_DEPRECATED_SUPPRESS_END

std::vector<std::string> DataSetReader::GetDataSourceNames()
{
  std::vector<std::string> names;
  for (const auto& source : this->Impl->DataSources)
  {
    names.push_back(source.first);
  }
  return names;
}

} // end namespace io
} // end namespace fides
