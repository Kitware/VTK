// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "fidesdefs.h" // For build/config preprocessor defs

#if VTK_HAVE_CONDUIT_PYTHON
#include "vtkPython.h" // must be first
#endif

#include "vtkFidesReader.h"

// Fides includes
#include <vtk_fides.h>
// clang-format off
#include VTK_FIDES(fides/ExternalDataRegistry.h)
#include VTK_FIDES(fides/DataSetReader.h)
// clang-format on

#include "vtkDataArraySelection.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmlib/ImageDataConverter.h"
#include "vtkmlib/UnstructuredGridConverter.h"
#include "vtksys/SystemTools.hxx"

#ifdef IOFIDES_HAVE_MPI
#include "vtkMPI.h"
#include "vtkMPIController.h"
#endif

#if VTK_HAVE_CONDUIT
#include "conduit.hpp"
#include "conduit_cpp_to_c.hpp"
#if VTK_HAVE_CONDUIT_PYTHON
#include "conduit_python.hpp"
#endif
#endif

#include <viskores/filter/clean_grid/CleanGrid.h>

// clang-format off
#if __has_include(VTK_FIDES(fides/DataContainer.h))
#define VTK_FIDES_HAS_DATA_CONTAINER 1
#else
#define VTK_FIDES_HAS_DATA_CONTAINER 0
#endif
// clang-format on

#include <numeric>
#include <stdexcept>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkFidesReader);
vtkCxxSetObjectMacro(vtkFidesReader, Controller, vtkMultiProcessController);

struct vtkFidesReader::vtkFidesReaderImpl
{
  std::unique_ptr<fides::io::DataSetReader> Reader;
  std::unordered_map<std::string, std::string> Paths;
  bool HasParsedDataModel{ false };
  bool UsePresetModel{ false };
  bool SkipNextPrepareCall{ false };
  int NumberOfDataSources{ 0 };
  bool UseInlineEngine{ false };
  fides::Params AllParams;
  vtkNew<vtkStringArray> SourceNames;

#if VTK_HAVE_CONDUIT
  std::unordered_map<std::string, std::string> ConduitTokens;

#if VTK_HAVE_CONDUIT_PYTHON
  std::unordered_map<std::string, vtkPyObjectFwd*> PythonNodes;
#endif

  // Helper to remove a single node by name
  bool RemoveConduitNode(const std::string& name)
  {
    auto tokenIt = this->ConduitTokens.find(name);
    if (tokenIt == this->ConduitTokens.end())
    {
      return false; // Not found, nothing to do
    }

    // Clear Conduit token and path
    fides::io::ExternalDataRegistry::Instance().Unregister(tokenIt->second);
    this->ConduitTokens.erase(tokenIt);
    this->Paths.erase(name);

#if VTK_HAVE_CONDUIT_PYTHON
    // Clear corresponding Python node if it exists
    auto pyNodeIt = this->PythonNodes.find(name);
    if (pyNodeIt != this->PythonNodes.end())
    {
      vtkPythonScopeGilEnsurer gilEnsurer;
      Py_DECREF(pyNodeIt->second);
      this->PythonNodes.erase(pyNodeIt);
    }
#endif

    return true; // state changed
  }

  // Helper to clear all nodes
  bool ClearConduitNodes()
  {
    if (this->ConduitTokens.empty())
    {
      return false;
    }

    // Unregister external memory from Fides
    for (const auto& pair : this->ConduitTokens)
    {
      fides::io::ExternalDataRegistry::Instance().Unregister(pair.second);
    }

    // Clear local maps
    this->ConduitTokens.clear();
    this->Paths.clear();

#if VTK_HAVE_CONDUIT_PYTHON
    // Decrement ref count on all stored Python nodes
    if (!this->PythonNodes.empty())
    {
      vtkPythonScopeGilEnsurer gilEnsurer;
      for (const auto& pair : this->PythonNodes)
      {
        Py_DECREF(pair.second);
      }
      this->PythonNodes.clear();
    }
#endif
    return true; // state changed
  }
#endif // VTK_HAVE_CONDUIT

  // Metadata of an individual group in ADIOS file
  // This metadata is populated in RequestInformation.
  // and subsequently used in RequestData
  struct GroupMetaData
  {
    std::size_t NumberOfBlocks;
    std::string Name;
    std::set<std::string> PointDataArrays;
    std::set<std::string> CellDataArrays;
    std::set<std::string> FieldDataArrays;
  };
  std::vector<GroupMetaData> GroupMetaDataCollection;

  // first -> source name, second -> address of IO object
  std::pair<std::string, std::string> IOObjectInfo;

  vtkStringArray* GetDataSourceNames()
  {
    if (this->Reader && this->SourceNames->GetNumberOfValues() == 0)
    {
      auto names = this->Reader->GetDataSourceNames();
      for (const auto& name : names)
      {
        this->SourceNames->InsertNextValue(name);
      }
    }
    return this->SourceNames;
  }

  void SetNumberOfDataSources()
  {
    if (this->Reader)
    {
      auto names = this->Reader->GetDataSourceNames();
      this->NumberOfDataSources = static_cast<int>(names.size());
    }
  }

  void SetupInlineEngine()
  {
    if (this->IOObjectInfo.first.empty() || this->IOObjectInfo.second.empty())
    {
      return;
    }

    // params has to be set before setting data source
    fides::DataSourceParams params;
    params["engine_type"] = "Inline";
    this->Reader->SetDataSourceParameters(this->IOObjectInfo.first, params);
    this->Reader->SetDataSourceIO(this->IOObjectInfo.first, this->IOObjectInfo.second);
    this->UseInlineEngine = true;
  }
};

vtkFidesReader::vtkFidesReader()
  : Impl(new vtkFidesReaderImpl())
  , Controller(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->FieldDataArraySelection = vtkDataArraySelection::New();
  this->StreamSteps = false;
  this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
  this->CreateSharedPoints = false;
  this->DebugOn();
}

vtkFidesReader::~vtkFidesReader()
{
  // Close the reader first so Fides finishes all I/O and teardown while the
  // underlying Conduit nodes (and Python memory) are still alive.
  if (this->Impl->Reader)
  {
    this->Impl->Reader->Close();
    this->Impl->Reader.reset();
  }

  // Clean up selections
  this->PointDataArraySelection->Delete();
  this->CellDataArraySelection->Delete();
  this->FieldDataArraySelection->Delete();

  // Clean up Fides+Conduit registry tokens and
#if VTK_HAVE_CONDUIT
  this->Impl->ClearConduitNodes();
#endif

  this->SetController(nullptr);
}

int vtkFidesReader::CanReadFile(const std::string& name)
{
  if (!vtksys::SystemTools::FileExists(name))
  {
    return 0;
  }

  auto parts = vtksys::SystemTools::SplitString(name, '.');
  // though adios by default uses .bp, some users have changed their files to be .bp3, .bp4, .bp5
  if (vtksys::SystemTools::StringStartsWith(parts.back().c_str(), "bp"))
  {
    // it's possible we may not be able to read a bp file, if it doesn't have the json in an
    // attribute or the fides metadata, but there's not really a good way to check at this point
    // that works with all engines.
    // TODO: though I suppose we could do a try/fail. if it's an actual bp file, then we can check
    // it if it's an sst stream then we wouldn't be able to check it.
    return 1;
  }
  if (vtksys::SystemTools::StringEndsWith(name, ".json"))
  {
    return 1;
  }
  // TODO: an sst stream doesn't necessarily have a bp file ending....
  return 0;
}

void vtkFidesReader::SetFileName(const std::string& fname)
{
  this->FileName = fname;
  auto parts = vtksys::SystemTools::SplitString(fname, '.');
  // though adios by default uses .bp, some users have changed their files to be .bp3, .bp4, .bp5
  if (vtksys::SystemTools::StringStartsWith(parts.back().c_str(), "bp"))
  {
    this->Impl->UsePresetModel = true;
    vtkDebugMacro(<< "Using a preset data model");
  }
}

void vtkFidesReader::SetDataSourceIO(const std::string& name, const std::string& ioAddress)
{
  if (name.empty() || ioAddress.empty())
  {
    return;
  }
  // can't call SetDataSourceIO in Fides yet, so just save the address for now
  this->Impl->IOObjectInfo = std::make_pair(name, ioAddress);
  this->StreamSteps = true;
  this->Impl->UseInlineEngine = true;
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkFidesReader::SetDataSourceNode(const std::string& name, vtkPyObjectFwd* conduitNode)
{
#if VTK_HAVE_CONDUIT_PYTHON
  // Passing None or nullptr just clears out any existing resource
  if (!conduitNode || conduitNode == Py_None)
  {
    this->Impl->RemoveConduitNode(name);
    this->Modified();
    return true;
  }

  if (!Py_IsInitialized())
  {
    vtkErrorMacro("Python interpreter is not initialized.");
    return false;
  }

  if (import_conduit() < 0)
  {
    vtkErrorMacro("Failed to import Conduit Python C-API.");
    return false;
  }

  if (!PyConduit_Node_Check(conduitNode))
  {
    vtkErrorMacro("Provided Python object is not a valid conduit.Node.");
    return false;
  }

  // Extract the raw C++ pointer
  conduit::Node* nodePtr = PyConduit_Node_Get_Node_Ptr(conduitNode);
  if (!nodePtr)
  {
    vtkErrorMacro("Failed to extract C++ pointer from Python conduit.Node.");
    return false;
  }

  // Clear any existing node with this name to prevent leaks
  this->Impl->RemoveConduitNode(name);

  // Register with Fides
  std::string token;
  try
  {
    auto wrappedNode = fides::WrapExternal(nodePtr);
    token = fides::io::ExternalDataRegistry::Instance().Register(wrappedNode);
  }
  catch (const std::exception& e)
  {
    vtkErrorMacro("Fides registration failed: " << e.what());
    return false;
  }

  // Store the new state
  this->Impl->ConduitTokens[name] = token;
  this->Impl->Paths[name] = token;
  this->Impl->PythonNodes[name] = conduitNode;

  Py_INCREF(conduitNode);
  this->Modified();

  return true;

#else
  (void)name;
  (void)conduitNode;
  vtkErrorMacro(
    "VTK must be compiled with Python wrapping and Conduit support to use this method!");
  return false;
#endif
}

//------------------------------------------------------------------------------
bool vtkFidesReader::SetDataSourceNode(const std::string& name, conduit_node* conduitNode)
{
#if VTK_HAVE_CONDUIT
  // Intercept nullptr to clear an existing resource
  if (!conduitNode)
  {
    this->Impl->RemoveConduitNode(name);
    this->Modified();
    return true;
  }

  // Convert the C-API node to the C++ API node using Conduit's official bridge
  conduit::Node* cppNode = conduit::cpp_node(conduitNode);
  if (!cppNode)
  {
    vtkErrorMacro("Failed to convert C-API conduit_node to C++ conduit::Node.");
    return false;
  }

  // Clear any existing node, Fides token, and Python DECREF
  this->Impl->RemoveConduitNode(name);

  // Wrap and register with Fides
  std::string token;
  try
  {
    auto wrappedNode = fides::WrapExternal(cppNode);
    token = fides::io::ExternalDataRegistry::Instance().Register(wrappedNode);
  }
  catch (const std::exception& e)
  {
    vtkErrorMacro("Fides registration failed: " << e.what());
    return false;
  }

  // Store the state
  this->Impl->ConduitTokens[name] = token;
  this->Impl->Paths[name] = token;

  // Note: There is no reference counting for pure C++. The caller is strictly
  // responsible for keeping the memory alive until Update() is finished.

  this->Modified();
  return true;

#else
  (void)name;
  (void)conduitNode;
  vtkErrorMacro("VTK must be compiled with Conduit support to use this method!");
  return false;
#endif
}

//------------------------------------------------------------------------------
void vtkFidesReader::RemoveDataSourceNode(const std::string& name)
{
#if VTK_HAVE_CONDUIT
  if (this->Impl->RemoveConduitNode(name))
  {
    this->Modified();
  }
#else
  (void)name;
#endif
}

//------------------------------------------------------------------------------
void vtkFidesReader::RemoveAllDataSourceNodes()
{
#if VTK_HAVE_CONDUIT
  if (!this->Impl->ClearConduitNodes())
  {
    this->Modified();
  }
#endif
}

// This version is used when a json file with the data model is provided
void vtkFidesReader::ParseDataModel(const std::string& fname)
{
  // Should no longer be used; use ParseDataModel() instead
  this->FileName = fname;
  this->ParseDataModel();
}

// This version is used when a pre-defined data model is being used
void vtkFidesReader::ParseDataModel()
{
  // If we have the minimum required info (basically just FileName), then we'll
  // go ahead and create the reader.
  // This opens the reader in a random access mode.
  // If RequestInformation is called again, we may end up deleting it and making
  // a new reader because we have new information about how the reader should
  // actually be opened (e.g., with some type of streaming engine)
  fides::io::DataSetReader::DataModelInput inputType =
    fides::io::DataSetReader::DataModelInput::JSONFile;
  if (this->Impl->UsePresetModel)
  {
    inputType = fides::io::DataSetReader::DataModelInput::BPFile;
  }
  try
  {
    vtkDebugMacro(<< "Setting up Fides DataSetReader with FileName: " << this->FileName
                  << ", inputType: " << static_cast<int>(inputType) << ", StreamSteps: "
                  << this->StreamSteps << ", CreateSharedPoints: " << this->CreateSharedPoints);
    bool usedMpi = false;

#ifdef IOFIDES_HAVE_MPI
    if (this->Controller && this->Controller->GetCommunicator())
    {
      vtkMPICommunicator* vtkComm =
        vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

      if (vtkComm && vtkComm->GetMPIComm())
      {
        MPI_Comm comm = *(vtkComm->GetMPIComm()->GetHandle());
        this->Impl->Reader.reset(new fides::io::DataSetReader(this->FileName, inputType,
          this->StreamSteps, comm, this->Impl->AllParams, this->CreateSharedPoints));

        usedMpi = true;
      }
    }
#endif

    // Serial fallback in case VTK is built without MPI, the controller is null,
    // or the communicator downcast fails
    if (!usedMpi)
    {
      this->Impl->Reader.reset(new fides::io::DataSetReader(this->FileName, inputType,
        this->StreamSteps, this->Impl->AllParams, this->CreateSharedPoints));
    }
  }
  catch (std::exception& e)
  {
    // In some cases it's expected that reading will fail (e.g., not all properties have been set
    // yet), so we don't always want to output the exception. We'll just put it in vtkDebugMacro,
    // so we can just turn it on when we're experiencing some issue.
    vtkWarningMacro(<< "Exception encountered when trying to set up Fides DataSetReader: "
                    << e.what());
    this->Impl->HasParsedDataModel = false;
    return;
  }
  this->Impl->HasParsedDataModel = true;
  this->Impl->SetupInlineEngine();
}

void vtkFidesReader::SetDataSourcePath(const std::string& name, const std::string& path)
{
  if (this->Impl->NumberOfDataSources <= 0)
  {
    this->Impl->SetNumberOfDataSources();
  }
  vtkDebugMacro(<< "Number of data sources: " << this->Impl->NumberOfDataSources);
  vtkDebugMacro(<< "source " << name << "'s path is " << path);
  this->Impl->Paths[name] = path;
  this->Modified();
}

void vtkFidesReader::SetDataSourceEngine(const std::string& name, const std::string& engine)
{
  if (name.empty() || engine.empty())
  {
    return;
  }
  fides::DataSourceParams params;
  params["engine_type"] = engine;
  vtkDebugMacro(<< "for data source " << name << ", setting ADIOS engine to " << engine);
  this->Impl->AllParams.insert(std::make_pair(name, params));
  if (engine == "SST")
  {
    this->StreamSteps = true;
  }
  this->Modified();
}

void vtkFidesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Filename: " << this->FileName << "\n";
  os << indent << "Stream Steps: " << this->StreamSteps << "\n";
  os << indent << "Next step status: " << this->NextStepStatus << "\n";
  os << indent << "Use Preset model: " << this->Impl->UsePresetModel << "\n";
  os << indent << "Has parsed data model: " << this->Impl->HasParsedDataModel << "\n";
  os << indent << "Number of data sources: " << this->Impl->NumberOfDataSources << "\n";
  os << indent << "Create shared points: " << this->CreateSharedPoints << "\n";
}

int vtkFidesReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkFidesReader::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    output = vtkPartitionedDataSetCollection::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
  }
  return 1;
}

namespace
{
#if VTK_FIDES_HAS_DATA_CONTAINER
bool IsPointField(fides::FieldAssociation association);
bool IsCellField(fides::FieldAssociation association);
bool IsWholeDataSetField(fides::FieldAssociation association);
#else
bool IsPointField(viskores::cont::Field::Association association);
bool IsCellField(viskores::cont::Field::Association association);
bool IsWholeDataSetField(viskores::cont::Field::Association association);
#endif
}

int vtkFidesReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (this->StreamSteps && this->NextStepStatus != StepStatus::NotReady)
  {
    // if we're in StreamSteps mode, updating the step status could cause
    // RequestInformation to be called again. In this case, we'll assume
    // that if NextStepStatus is good, that we'll just return here instead
    // of resetting the DataSetReader
    return 1;
  }
  if (this->Impl->UseInlineEngine && this->Impl->HasParsedDataModel)
  {
    // If we're using the Inline engine, we may get unnecessary
    // RequestInformation calls, but we don't want to actually reset the
    // reader
    return 1;
  }

  if (this->Impl->UseInlineEngine)
  {
    // ranks can only access their own data with the inline engine, so
    // we have to set CreateSharedPoints to false. GhostCellsGenerator
    // is currently having a feature added to it, that will fix the gap
    // without Fides needing to do it, that should work for the inline case
    this->CreateSharedPoints = false;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // okay so basically we will reset our reader on any call to RequestInfo
  // (except for the situations described above)
  // because we may later get new metadata that determines how we should actually
  // have created the reader, configured adios, etc
  // at bare minimum, FileName has been set, so we can go ahead and call this
  this->ParseDataModel();

  if (!this->Impl->HasParsedDataModel)
  {
    // for some reason we weren't able to set up the fides reader, so just return
    return 1;
  }

  // reset the number of data sources
  this->Impl->SetNumberOfDataSources();

  // for generated data model, we have to set the paths for sources
  if (this->Impl->UsePresetModel)
  {
    vtkStringArray* sourceNames = this->Impl->GetDataSourceNames();
    vtkDebugMacro(<< this->Impl->NumberOfDataSources << " data sources were found");
    for (int i = 0; i < this->Impl->NumberOfDataSources; ++i)
    {
      // Currently, if there are multiple data sources and we are using a predefined
      // data model, then we'll assume this is XGC. All other predefined data models
      // have only a single data source, and file name is not specified in the data
      // model, so in this case, we need to set the data source path to be the
      // full file name. For XGC, this->FileName is actually to a file containing only
      // attributes, so we just need to grab the directory the attribute file is in
      // to set for each data source.
      std::string path;
      if (this->Impl->NumberOfDataSources == 1)
      {
        path = this->FileName;
      }
      else
      {
        path = vtksys::SystemTools::GetFilenamePath(this->FileName) + "/";
      }
      this->SetDataSourcePath(sourceNames->GetValue(i), path);
    }
  }

  if (this->Impl->NumberOfDataSources == 0)
  {
    // no reason to keep going
    // this can happen when using a JSON file instead of a BP file
    return 1;
  }

  if (this->StreamSteps)
  {
    // This isn't as relevant for earlier BP versions, but for BP5 streaming
    // as well as staging engines, we need to do the BeginStep() call before
    // we can read anything, or even just inquire variables/attributes.
    // Fides does need to get some information about variables/attributes
    // in ReadMetaData()
    this->PrepareNextStep();
    this->Impl->SkipNextPrepareCall = true;
  }

  // collection of group metadata will be rebuilt
  this->Impl->GroupMetaDataCollection.clear();
  // get all group names
  auto groupNames = this->Impl->Reader->GetGroupNames(this->Impl->Paths);
  if (groupNames.empty())
  {
    // this is fine. there are no groups in the file.
    // insert a placeholder empty group name, so that the for loop runs once.
    groupNames.insert("");
  }
  for (const auto& groupName : groupNames)
  {
    fides::metadata::MetaData metaData;
    try
    {
      metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths, groupName);
    }
    catch (std::exception& e)
    {
      // it's possible that we were able to set Fides up, but reading metadata
      // failed, indicating that not all properties have been set before this
      // RequestInformation call. This is not necessarily an error, but may
      // indicate a problem with the JSON that will cause errors when reading
      // the data.
      vtkWarningMacro(<< "Exception encountered when trying to set up Fides DataSetReader: "
                      << e.what());
      this->Impl->HasParsedDataModel = false;
      return 1;
    }
    vtkDebugMacro(<< "MetaData has been read by Fides " << (groupName.empty() ? "for group " : "")
                  << groupName);

    vtkFidesReaderImpl::GroupMetaData groupMetaData;
    groupMetaData.Name = groupName;
    groupMetaData.NumberOfBlocks =
      metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS()).NumberOfItems;
    vtkDebugMacro(<< "Number of blocks found in metadata: " << groupMetaData.NumberOfBlocks);

    if (metaData.Has(fides::keys::FIELDS()))
    {
      vtkDebugMacro(<< "Metadata has fields info");
      auto fields = metaData.Get<fides::metadata::Vector<fides::metadata::FieldInformation>>(
        fides::keys::FIELDS());
      for (auto& field : fields.Data)
      {
        if (IsPointField(field.Association))
        {
          groupMetaData.PointDataArrays.insert(field.Name);
          this->PointDataArraySelection->AddArray(field.Name.c_str());
        }
        else if (IsCellField(field.Association))
        {
          groupMetaData.CellDataArrays.insert(field.Name);
          this->CellDataArraySelection->AddArray(field.Name.c_str());
        }
        else if (IsWholeDataSetField(field.Association))
        {
          groupMetaData.FieldDataArrays.insert(field.Name);
          this->FieldDataArraySelection->AddArray(field.Name.c_str());
        }
      }
    }
    this->Impl->GroupMetaDataCollection.emplace_back(std::move(groupMetaData));
  } // for groupName in groupNames

  fides::metadata::MetaData metaData;
  try
  {
    metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths);
  }
  catch (...)
  {
    // shouldn't happen, cheap insurance.
    return 1;
  }
  if (!this->StreamSteps && metaData.Has(fides::keys::NUMBER_OF_STEPS()))
  {
    // If there's a time array provided, we'll use that, otherwise, just create an array
    // with consecutive integers for the time
    std::vector<double> times;
    int nSteps;
    if (metaData.Has(fides::keys::TIME_ARRAY()))
    {
      times = metaData.Get<fides::metadata::Vector<double>>(fides::keys::TIME_ARRAY()).Data;
      nSteps = static_cast<int>(times.size());
    }
    else
    {
      nSteps = static_cast<int>(
        metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems);

      times.resize(nSteps);
      std::iota(times.begin(), times.end(), 0);
    }

    double timeRange[2];
    timeRange[0] = times[0];
    timeRange[1] = times[nSteps - 1];
    vtkDebugMacro(<< "time min: " << timeRange[0] << ", time max: " << timeRange[1]);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times.data(), nSteps);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  outInfo->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

namespace
{
#if VTK_FIDES_HAS_DATA_CONTAINER
bool IsPointField(fides::FieldAssociation association)
{
  return association == fides::FieldAssociation::Points;
}

bool IsCellField(fides::FieldAssociation association)
{
  return association == fides::FieldAssociation::Cells;
}

bool IsWholeDataSetField(fides::FieldAssociation association)
{
  return association == fides::FieldAssociation::WholeDataSet;
}

fides::metadata::FieldInformation MakeFieldInformation(
  const std::string& name, viskores::cont::Field::Association association)
{
  switch (association)
  {
    case viskores::cont::Field::Association::Points:
      return fides::metadata::FieldInformation(name, fides::FieldAssociation::Points);
    case viskores::cont::Field::Association::Cells:
      return fides::metadata::FieldInformation(name, fides::FieldAssociation::Cells);
    case viskores::cont::Field::Association::WholeDataSet:
      return fides::metadata::FieldInformation(name, fides::FieldAssociation::WholeDataSet);
    default:
      throw std::runtime_error("Unsupported Viskores field association.");
  }
}

viskores::cont::PartitionedDataSet ReadViskoresDataSet(fides::io::DataSetReader& reader,
  const std::unordered_map<std::string, std::string>& paths,
  const fides::metadata::MetaData& selections)
{
  auto data = reader.ReadDataSet(paths, selections, fides::DataSetType::Viskores);
  if (!data)
  {
    throw std::runtime_error("Fides did not return a Viskores dataset.");
  }
  return fides::GetAsViskoresPDS(*data);
}
#else
bool IsPointField(viskores::cont::Field::Association association)
{
  return association == viskores::cont::Field::Association::Points;
}

bool IsCellField(viskores::cont::Field::Association association)
{
  return association == viskores::cont::Field::Association::Cells;
}

bool IsWholeDataSetField(viskores::cont::Field::Association association)
{
  return association == viskores::cont::Field::Association::WholeDataSet;
}

fides::metadata::FieldInformation MakeFieldInformation(
  const std::string& name, viskores::cont::Field::Association association)
{
  return fides::metadata::FieldInformation(name, association);
}

viskores::cont::PartitionedDataSet ReadViskoresDataSet(fides::io::DataSetReader& reader,
  const std::unordered_map<std::string, std::string>& paths,
  const fides::metadata::MetaData& selections)
{
  return reader.ReadDataSet(paths, selections);
}
#endif

fides::metadata::Vector<size_t> DetermineBlocksToRead(int nBlocks, int nPieces, int piece)
{
  int startPiece, endPiece;
  if (nBlocks > nPieces)
  {
    int nLocalPieces = nBlocks / nPieces;
    startPiece = nLocalPieces * piece;
    endPiece = startPiece + nLocalPieces;
    int remain = nBlocks % nPieces;
    if (remain)
    {
      if (piece < remain)
      {
        startPiece += piece;
        endPiece += piece + 1;
      }
      else
      {
        startPiece += remain;
        endPiece += remain;
      }
    }
  }
  else
  {
    if (piece < nBlocks)
    {
      startPiece = piece;
      endPiece = piece + 1;
    }
    else
    {
      startPiece = nBlocks - 1;
      endPiece = startPiece + 1;
    }
  }

  fides::metadata::Vector<size_t> blocksToRead;
  size_t nBlocksToRead = endPiece - startPiece;
  if (nBlocksToRead > 0)
  {
    blocksToRead.Data.resize(nBlocksToRead);
    std::iota(blocksToRead.Data.begin(), blocksToRead.Data.end(), startPiece);
  }
  return blocksToRead;
}

vtkDataSet* ConvertDataSet(const viskores::cont::DataSet& ds)
{
  vtkNew<vtkUnstructuredGrid> dstmp;
  const auto& cs = ds.GetCellSet();
  if (cs.IsType<viskores::cont::CellSetSingleType<>>() ||
    cs.IsType<viskores::cont::CellSetExplicit<>>())
  {
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
    fromvtkm::Convert(ds, ug, dstmp, /*forceViskores*/ true);
    return ug;
  }
  else if (cs.IsType<viskores::cont::CellSetStructured<2>>() ||
    cs.IsType<viskores::cont::CellSetStructured<3>>())
  {
    const auto& coords = ds.GetCoordinateSystem();
    auto array = coords.GetData();
    if (array.IsType<viskores::cont::ArrayHandleUniformPointCoordinates>())
    {
      vtkImageData* image = vtkImageData::New();
      fromvtkm::Convert(ds, image, dstmp);
      return image;
    }
  }
  viskores::filter::clean_grid::CleanGrid filter;
  filter.SetCompactPointFields(false);
  auto result = filter.Execute(ds);
  return ConvertDataSet(result);
}

} // end anon namespace

void vtkFidesReader::PrepareNextStep()
{
  if (!this->Impl->Reader)
  {
    vtkErrorMacro(<< "vtkFidesReader::PrepareNextStep() has been called,"
                     " but Fides has not been set up yet");
    this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
    return;
  }
  if (this->Impl->SkipNextPrepareCall)
  {
    this->Impl->SkipNextPrepareCall = false;
    return;
  }
  try
  {
    this->NextStepStatus =
      static_cast<StepStatus>(this->Impl->Reader->PrepareNextStep(this->Impl->Paths));
  }
  catch (...)
  {
    return;
  }
  vtkDebugMacro(<< "PrepareNextStep() NextStepStatus = " << this->NextStepStatus);
  this->StreamSteps = true;
  this->Modified();
}

int vtkFidesReader::GetNextStepStatus()
{
  vtkDebugMacro(<< "GetNextStepStatus = " << this->NextStepStatus);
  return this->NextStepStatus;
}

double vtkFidesReader::GetTimeOfCurrentStep()
{
  if (!this->StreamSteps)
  {
    vtkErrorMacro("GetTimeOfCurrentStep() can only be called in streaming mode");
    return 0.0;
  }

  if (this->Impl->NumberOfDataSources == 0)
  {
    this->Impl->SetNumberOfDataSources();
  }

  if (!this->Impl->HasParsedDataModel)
  {
    vtkErrorMacro(<< "data model has not been parsed");
    return 0.0;
  }

  auto metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths);
  if (metaData.Has(fides::keys::TIME_VALUE()))
  {
    return metaData.Get<fides::metadata::Time>(fides::keys::TIME_VALUE()).Data;
  }

  vtkErrorMacro(<< "Couldn't grab the time from the Fides metadata");
  return 0.0;
}

int vtkFidesReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->Impl->HasParsedDataModel)
  {
    vtkErrorMacro("RequestData() DataModel must be parsed before RequestData()");
    return 0;
  }

  if (this->StreamSteps && this->NextStepStatus != StepStatus::OK)
  {
    // This doesn't usually happen, but when using Catalyst Live with
    // Fides, sometimes there's a situation where Catalyst gets updated
    // state from Live and it has NextStepStatus == NotReady. In that case
    // (usually only when running with MPI), one rank will think it needs
    // to call RequestData(), in vtkLiveInsituLink::InsituPostProcess().
    // But PrepareNextStep() will not be called, and so ADIOS will throw
    // an error because EndStep() was called without BeginStep().
    return 1;
  }

  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outputVector);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  output->SetNumberOfPartitionedDataSets(0);

#if VTK_HAVE_CONDUIT
  // Determine if we are relying on an in-memory Conduit node
  bool isConduit = !this->Impl->ConduitTokens.empty();
#else
  bool isConduit = false;
#endif

  fides::metadata::MetaData selections;

  // Select time step if downstream requested a specific time step,
  // but skip this if we are dealing with a local Conduit snapshot
  // due to general lack of temporal indexing.
  if (!this->StreamSteps && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) &&
    !isConduit)
  {
    auto step = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int index = 0;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      auto nSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      std::vector<double> allSteps(nSteps);
      outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), allSteps.data());

      double minDiff = VTK_DOUBLE_MAX;
      for (unsigned int i = 0; i < allSteps.size(); i++)
      {
        double diff = std::fabs(allSteps[i] - step);
        if (diff < minDiff)
        {
          minDiff = diff;
          index = i;
        }
      }
    }
    vtkDebugMacro(<< "RequestData() Not streaming and we have update time step request for step "
                  << step << " with index " << index);
    fides::metadata::Index idx(index);
    selections.Set(fides::keys::STEP_SELECTION(), idx);
  }

  unsigned int pdsIdx = 0;
  for (const auto& groupMetaData : this->Impl->GroupMetaDataCollection)
  {
    int nBlocks = static_cast<int>(groupMetaData.NumberOfBlocks);
    int nPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    vtkDebugMacro(<< "nBlocks: " << nBlocks << ", nPieces: " << nPieces << ", piece: " << piece
                  << (groupMetaData.Name.empty() ? "" : ", groupName: ") << groupMetaData.Name);

    fides::metadata::Vector<size_t> blocksToRead;

    if (isConduit)
    {
      // Conduit nodes reflect local in-memory data, and since nBlocks is the
      // number of local blocks on this rank, let's read all of them
      for (int b = 0; b < nBlocks; ++b)
      {
        blocksToRead.Data.push_back(static_cast<size_t>(b));
      }
    }
    else
    {
      // ADIOS2 global block distribution
      blocksToRead = DetermineBlocksToRead(nBlocks, nPieces, piece);
    }

    if (blocksToRead.Data.empty())
    {
      // nothing to read on this rank
      output->SetNumberOfPartitions(pdsIdx, 0);
      vtkDebugMacro(<< "No blocks to read on this rank; returning");
      continue;
    }
    // Select blocks to read.
    selections.Set(fides::keys::BLOCK_SELECTION(), blocksToRead);
    // Select group.
    selections.Set(fides::keys::GROUP_SELECTION(), fides::metadata::String(groupMetaData.Name));

    using FieldInfoType = fides::metadata::Vector<fides::metadata::FieldInformation>;
    FieldInfoType arraySelection;
    // pick selected arrays from the global data array selection instances.
    for (const auto& aname : groupMetaData.PointDataArrays)
    {
      if (this->PointDataArraySelection->ArrayIsEnabled(aname.c_str()))
      {
        // if this array was enabled on the global point data array selection.
        arraySelection.Data.emplace_back(
          MakeFieldInformation(aname, viskores::cont::Field::Association::Points));
      }
    }
    for (const auto& aname : groupMetaData.CellDataArrays)
    {
      if (this->CellDataArraySelection->ArrayIsEnabled(aname.c_str()))
      {
        // if this array was enabled on the global cell data array selection.
        arraySelection.Data.emplace_back(
          MakeFieldInformation(aname, viskores::cont::Field::Association::Cells));
      }
    }
    for (const auto& aname : groupMetaData.FieldDataArrays)
    {
      if (this->FieldDataArraySelection->ArrayIsEnabled(aname.c_str()))
      {
        // if this array was enabled on the global field data array selection.
        arraySelection.Data.emplace_back(
          MakeFieldInformation(aname, viskores::cont::Field::Association::WholeDataSet));
      }
    }
    selections.Set(fides::keys::FIELDS(), arraySelection);

    viskores::cont::PartitionedDataSet datasets;
    try
    {
      vtkDebugMacro(<< "RequestData() calling ReadDataSet");
      datasets = ReadViskoresDataSet(*this->Impl->Reader, this->Impl->Paths, selections);
      if (this->StreamSteps)
      {
        this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
      }
    }
    catch (std::exception& e)
    {
      vtkErrorMacro(<< e.what());
      return 0;
    }
    viskores::Id nParts = datasets.GetNumberOfPartitions();
    output->SetNumberOfPartitions(pdsIdx, nParts);
    std::string datasetName;
    {
      const auto parts = vtksys::SystemTools::SplitString(groupMetaData.Name);
      datasetName = parts.empty() ? "mesh" : parts.back();
    }
    output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), datasetName.c_str());

    for (viskores::Id i = 0; i < nParts; i++)
    {
      auto& ds = datasets.GetPartition(i);
      vtkDataSet* vds = ConvertDataSet(ds);
      if (vds)
      {
        output->SetPartition(pdsIdx, static_cast<unsigned int>(i), vds);
        vds->Delete();
      }
    }
    pdsIdx++;
  }

  return 1;
}

int vtkFidesReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

int vtkFidesReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

const char* vtkFidesReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

int vtkFidesReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

void vtkFidesReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

int vtkFidesReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

const char* vtkFidesReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

int vtkFidesReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

void vtkFidesReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

int vtkFidesReader::GetNumberOfFieldArrays()
{
  return this->FieldDataArraySelection->GetNumberOfArrays();
}

const char* vtkFidesReader::GetFieldArrayName(int index)
{
  return this->FieldDataArraySelection->GetArrayName(index);
}

int vtkFidesReader::GetFieldArrayStatus(const char* name)
{
  return this->FieldDataArraySelection->ArrayIsEnabled(name);
}

void vtkFidesReader::SetFieldArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->FieldDataArraySelection->EnableArray(name);
  }
  else
  {
    this->FieldDataArraySelection->DisableArray(name);
  }
}

vtkMTimeType vtkFidesReader::GetMTime()
{
  auto curMax = std::max(this->Superclass::GetMTime(), this->PointDataArraySelection->GetMTime());
  curMax = std::max(curMax, this->CellDataArraySelection->GetMTime());
  return std::max(curMax, this->FieldDataArraySelection->GetMTime());
}

VTK_ABI_NAMESPACE_END
