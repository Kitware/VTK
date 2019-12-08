/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <array>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "Core/vtkADIOS2CoreArraySelection.h"
#include "Core/vtkADIOS2CoreTypeTraits.h"
#include "vtkADIOS2CoreImageReader.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataObjectTypes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#ifdef IOADIOS2_HAVE_MPI
#include "vtkMPI.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h" // For the MPI controller member
#endif
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkType.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/SystemTools.hxx"

#include <adios2.h> // adios2
#include <istream>  // istringStream
#include <string>

//----------------------------------------------------------------------------
// Helper functions
//----------------------------------------------------------------------------

bool StringEndsWith(const std::string& a, const std::string& b)
{
  return a.size() >= b.size() && a.compare(a.size() - b.size(), b.size(), b) == 0;
}
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkADIOS2CoreImageReader);
namespace
{
inline std::vector<int> parseDimensions(const std::string& dimsStr)
{
  std::vector<int> dims;
  std::istringstream f(dimsStr);
  std::string token;
  while (std::getline(f, token, ','))
  {
    dims.push_back(std::atoi(token.c_str()));
  }
  return dims;
}
}

//----------------------------------------------------------------------------
struct vtkADIOS2CoreImageReader::vtkADIOS2CoreImageReaderImpl
{

  vtkNew<vtkMultiPieceDataSet> Flatten(vtkMultiBlockDataSet* ibds);

  // ADIOS variables
  std::unique_ptr<adios2::ADIOS> Adios;
  adios2::IO AdiosIO;
  adios2::Engine BpReader;
  InquireVariablesType InquiredVars;
  std::pair<std::string, VarType> ActiveScalar;
  std::map<std::string, adios2::Params> AvailVars;
  std::map<std::string, adios2::Params> AvailAtts;

  size_t BlockStart{ 0 };
  size_t BlockCount{ 0 };
  std::vector<std::array<int, 6> > BlockExtents;

  // VTK variables
  bool HasReadMetaData{ false };
  std::vector<double> TimeSteps;
  // From time to time step(aka reference)
  std::unordered_map<double, size_t> TimeStepsReverseMap;
  // Index of the request time step. If it's a single time step data then it's default to be 0
  size_t RequestStep{ 0 };

  // Select the arrays that should be read in.
  vtkADIOS2ArraySelection ArraySelection;
  // For ParaView GUI display usage.
  vtkNew<vtkStringArray> AvailableArray;
};

vtkNew<vtkMultiPieceDataSet> vtkADIOS2CoreImageReader::vtkADIOS2CoreImageReaderImpl::Flatten(
  vtkMultiBlockDataSet* ibds)
{
  // found out how many pieces we have in current process
  using Opts = vtk::DataObjectTreeOptions;

  // Communicate to find out where the images of current process should go
  int myLen = static_cast<int>(ibds->GetNumberOfBlocks());
  int* allLens{ nullptr };
  int procId{ 0 }, numProcess{ 0 };
#ifdef IOADIOS2_HAVE_MPI
  auto ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl)
  {
    procId = ctrl->GetLocalProcessId();
    numProcess = ctrl->GetNumberOfProcesses();
    allLens = new int[numProcess];
    ctrl->AllGather(&myLen, allLens, 1);
  }
  else
  {
    procId = 0;
    numProcess = 1;
    allLens = new int[1];
    allLens[0] = myLen;
  }
#else
  procId = 0;
  numProcess = 1;
  allLens = new int[1];
  allLens[0] = myLen;
#endif

  unsigned int start{ 0 }, total{ 0 };
  for (int i = 0; i < numProcess; i++)
  {
    if (i < procId)
    {
      start += static_cast<unsigned int>(allLens[i]);
    }
    total += static_cast<unsigned int>(allLens[i]);
  }
  delete[] allLens;

  vtkNew<vtkMultiPieceDataSet> mpds;
  mpds->SetNumberOfPieces(total);
  for (vtkDataObject* obj : vtk::Range(ibds, Opts::VisitOnlyLeaves))
  {
    mpds->SetPiece(start++, obj);
  }
  return mpds;
}

//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::CanReadFile(const std::string& name)
{
  if (!vtksys::SystemTools::FileExists(name))
  {
    return 0;
  }
  if (StringEndsWith(name, ".bp") || StringEndsWith(name, "md.idx"))
  {
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::CanReadFile(const char* fileName)
{
  return this->CanReadFile(std::string(fileName));
}

//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::SetFileName(const char* fileName)
{
  this->FileName = std::string(fileName);
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::GetNumberOfArrays()
{
  return this->Impl->ArraySelection.GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkADIOS2CoreImageReader::GetArrayName(int index)
{
  return this->Impl->ArraySelection.GetArrayName(index);
}

//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::SetArrayStatus(const char* name, int status)
{
  this->Impl->ArraySelection.SetArrayStatus(name, status);
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::GetArrayStatus(const char* name)
{
  return this->Impl->ArraySelection.GetArrayStatus(name);
}

//----------------------------------------------------------------------------
vtkStringArray* vtkADIOS2CoreImageReader::GetAllDimensionArrays()
{
  return this->Impl->AvailableArray;
}

//----------------------------------------------------------------------------
vtkStringArray* vtkADIOS2CoreImageReader::GetAllTimeStepArrays()
{
  return this->Impl->AvailableArray;
}

//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::SetActiveScalar(const std::pair<std::string, VarType>& as)
{
  this->Impl->ActiveScalar = as;
}

//----------------------------------------------------------------------------
std::pair<std::string, vtkADIOS2CoreImageReader::VarType>&
vtkADIOS2CoreImageReader::GetActiveScalar()
{
  return this->Impl->ActiveScalar;
}

//----------------------------------------------------------------------------
const std::pair<std::string, vtkADIOS2CoreImageReader::VarType>&
vtkADIOS2CoreImageReader::GetActiveScalar() const
{
  return this->Impl->ActiveScalar;
}

//----------------------------------------------------------------------------
vtkADIOS2CoreImageReader::StringToParams& vtkADIOS2CoreImageReader::GetAvilableVariables()
{
  return this->Impl->AvailVars;
}

//----------------------------------------------------------------------------
const vtkADIOS2CoreImageReader::StringToParams& vtkADIOS2CoreImageReader::GetAvilableVariables()
  const
{
  return this->Impl->AvailVars;
}

//----------------------------------------------------------------------------
vtkADIOS2CoreImageReader::StringToParams& vtkADIOS2CoreImageReader::GetAvailableAttributes()
{
  return this->Impl->AvailAtts;
}

//----------------------------------------------------------------------------
const vtkADIOS2CoreImageReader::StringToParams& vtkADIOS2CoreImageReader::GetAvailableAttributes()
  const
{
  return this->Impl->AvailAtts;
}

//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::SetController(vtkMultiProcessController* controller)
{
#ifdef IOADIOS2_HAVE_MPI
  vtkMPIController* mpiController = vtkMPIController::SafeDownCast(controller);
  if (controller && !mpiController)
  {
    vtkErrorMacro(
      "vtkADIOS2CoreImageReader is built with MPI but an invalid MPI controller is provided");
    return;
  }
#endif

  this->Controller = controller;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObjectInternal(outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::RequestDataObjectInternal(vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkDataObject> output = vtkDataObject::GetData(outputVector, 0);
  if (!output)
  {
    output = vtk::TakeSmartPointer(vtkDataObjectTypes::NewDataObject(VTK_MULTIBLOCK_DATA_SET));
    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
    this->GetOutputInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
  }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkADIOS2CoreImageReader::OpenAndReadMetaData()
{
  // Is name valid
  if (!this->CanReadFile(this->FileName))
  {
    vtkErrorMacro("cannot read file" << this->FileName);
    return false;
  }

  // Initialize the ADIOS2 data structures
  if (!this->Impl->Adios)
  {
#ifdef IOADIOS2_HAVE_MPI
    // Make sure the ADIOS subsystem is initialized before processing any
    // sort of request.
    if (!this->Controller)
    {
      vtkErrorMacro("The reader is built with MPI support but the application is not launched in "
                    "parallel mode. Abort reading.");
      return false;
    }
    vtkMPICommunicator* comm =
      static_cast<vtkMPICommunicator*>(this->Controller->GetCommunicator());

    this->Impl->Adios.reset(new adios2::ADIOS(*comm->GetMPIComm()->GetHandle(), adios2::DebugON));
#else
    // Make sure the ADIOS subsystem is initialized before processing any
    // sort of request.

    this->Impl->Adios.reset(new adios2::ADIOS(adios2::DebugON));
    // Before processing any request, read the meta data first
#endif
  }
  // Before processing any request, read the meta data first
  try
  {
    this->Impl->AdiosIO = this->Impl->Adios->DeclareIO("vtkADIOS2ImageRead");
    if (StringEndsWith(this->FileName, ".bp"))
    {
      this->Impl->AdiosIO.SetEngine("BPFile");
      this->Impl->BpReader = this->Impl->AdiosIO.Open(this->FileName, adios2::Mode::Read);
    }
    else if (StringEndsWith(this->FileName, "md.idx"))
    {
      this->Impl->AdiosIO.SetEngine("BP4");
      this->Impl->BpReader = this->Impl->AdiosIO.Open(
        this->FileName.substr(0, this->FileName.size() - 6), adios2::Mode::Read);
    }
    else
    {
      throw std::runtime_error("Unsupported file extension");
    }
    this->Impl->AvailVars = this->Impl->AdiosIO.AvailableVariables();
    this->Impl->AvailAtts = this->Impl->AdiosIO.AvailableAttributes();
    // Populate the array selection
    this->Impl->AvailableArray->Allocate(static_cast<vtkIdType>(this->Impl->AvailVars.size()));
    for (auto& iter : this->Impl->AvailVars)
    {
      this->Impl->ArraySelection[iter.first] = true;
      this->Impl->AvailableArray->InsertNextValue(iter.first);
    }
  }
  catch (const std::exception& ex)
  {
    vtkErrorMacro("failed to open and read meta data: " << ex.what());
    return false;
  }

  this->Impl->HasReadMetaData = true;
  return true;
}

//----------------------------------------------------------------------------
vtkADIOS2CoreImageReader::vtkADIOS2CoreImageReader()
  : DimensionArrayAsCell(true)
  , IsColumnMajor(false)
  , Impl(new vtkADIOS2CoreImageReaderImpl)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkADIOS2CoreImageReader::~vtkADIOS2CoreImageReader()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Initialize adios2 variables and read meta data
  if (!this->Impl->HasReadMetaData && !this->OpenAndReadMetaData())
  {
    this->Impl->Adios.reset(nullptr);
    vtkErrorMacro("unable to open file and data");
    return 0;
  }

  if (!this->Impl->AvailVars.size())
  {
    vtkErrorMacro("No variables can be inquired in the provided file. Abort reading");
    return 0;
  }

  // Set extent info
  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);

  this->UpdateDimensionFromDimensionArray();

  int extent[6];
  if (this->IsColumnMajor)
  {
    extent[0] = extent[2] = extent[4] = 0;
    extent[1] = this->Dimension[0];
    extent[3] = this->Dimension[1];
    extent[5] = this->Dimension[2];
  }
  else
  {
    extent[0] = extent[2] = extent[4] = 0;
    extent[1] = this->Dimension[2];
    extent[3] = this->Dimension[1];
    extent[5] = this->Dimension[0];
  }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  if (!this->TimeStepArray.empty() && this->GatherTimeSteps())
  {
    // Publish time information
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->Impl->TimeSteps.data(),
      static_cast<int>(this->Impl->TimeSteps.size()));
    double tRange[2];
    tRange[0] = this->Impl->TimeSteps[0];
    tRange[1] = this->Impl->TimeSteps[this->Impl->TimeSteps.size() - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
  }

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkADIOS2CoreImageReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Convert user selected array names into inquire variables
  this->ConvertArraySelectionToInqVar();

  if (!this->Impl->InquiredVars.size())
  {
    this->Impl->Adios.reset(nullptr);
    vtkErrorMacro("No inquire variable is specified. Abort reading now");
    return 0;
  }
  if (!this->TimeStepArray.empty() &&
    this->Impl->ArraySelection.find(this->TimeStepArray) == this->Impl->ArraySelection.end())
  {
    this->Impl->Adios.reset(nullptr);
    vtkErrorMacro("An invalid time step array name is specified. Abort reading now");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!this->TimeStepArray.empty())
  {
    this->RequestTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    if (!this->Impl->TimeStepsReverseMap.count(this->RequestTimeStep))
    {
      vtkErrorMacro("The requested time step " << this->RequestTimeStep << " is not avaible!");
      return 0;
    }
    this->Impl->RequestStep = this->Impl->TimeStepsReverseMap[this->RequestTimeStep];
  }

  // Initailize work distribution for each rank
  if (!this->InitWorkDistribution())
  {
    this->Impl->Adios.reset(nullptr);
    vtkErrorMacro("unable to initialize work distribution");
    return 0;
  }

  vtkNew<vtkMultiBlockDataSet> mbds;
  mbds->SetNumberOfBlocks(static_cast<unsigned int>(this->Impl->BlockCount));
  this->ReadImageBlocks(mbds);

  vtkSmartPointer<vtkMultiBlockDataSet> rootMB = vtkMultiBlockDataSet::GetData(outInfo);
  vtkNew<vtkMultiPieceDataSet> mpds = this->Impl->Flatten(mbds);
  rootMB->SetBlock(0, mpds);
  if (!this->TimeStepArray.empty())
  {
    rootMB->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->RequestTimeStep);
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkADIOS2CoreImageReader::InitWorkDistribution()
{
  try
  {
    // Determine the blocks that need to be read for the current rank
    std::string varName = this->Impl->InquiredVars.begin()->first;
    std::string typeStr = this->FetchTypeStringFromVarName(varName);
    if (typeStr.empty())
    {
      vtkErrorMacro("Cannot find a type for " << varName << " invalid name is provided");
      return 1;
    }
    // FIXME: adios2 IO object returns an template dependent class instance instead of
    // a pointer or template independent object. Without using std::variant,
    // if statements are used to overcome the limitation
    // Use the adios_types_map in adios2 code base to generate types
    if (typeStr == "string")
    {
      this->CalculateWorkDistribution<std::string>(varName);
    }
    else if (typeStr == "int8_t")
    {
      this->CalculateWorkDistribution<int8_t>(varName);
    }
    else if (typeStr == "uint8_t")
    {
      this->CalculateWorkDistribution<uint8_t>(varName);
    }
    else if (typeStr == "int16_t")
    {
      this->CalculateWorkDistribution<int16_t>(varName);
    }
    else if (typeStr == "uint16_t")
    {
      this->CalculateWorkDistribution<uint16_t>(varName);
    }
    else if (typeStr == "int32_t")
    {
      this->CalculateWorkDistribution<int32_t>(varName);
    }
    else if (typeStr == "uint32_t")
    {
      this->CalculateWorkDistribution<uint32_t>(varName);
    }
    else if (typeStr == "int64_t")
    {
      this->CalculateWorkDistribution<int64_t>(varName);
    }
    else if (typeStr == "uint64_t")
    {
      this->CalculateWorkDistribution<uint64_t>(varName);
    }
    else if (typeStr == "float")
    {
      this->CalculateWorkDistribution<float>(varName);
    }
    else if (typeStr == "double")
    {
      this->CalculateWorkDistribution<double>(varName);
    }
    else if (typeStr == "float complex")
    {
      this->CalculateWorkDistribution<std::complex<float> >(varName);
    }
    else if (typeStr == "double complex")
    {
      this->CalculateWorkDistribution<std::complex<double> >(varName);
    }
  }
  catch (std::exception& e)
  {
    vtkErrorMacro("failed to initialize work distribution: " << e.what());
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
std::string vtkADIOS2CoreImageReader::FetchTypeStringFromVarName(const std::string& name)
{
  return (this->Impl->AvailVars.find(name) == this->Impl->AvailVars.end())
    ? std::string{}
    : this->Impl->AvailVars[name]["Type"];
}
//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::UpdateDimensionFromDimensionArray()
{
  if (this->Impl->AvailVars.find(this->DimensionArray) == this->Impl->AvailVars.end())
  {
    return;
  }
  std::vector<int> dims = parseDimensions(this->Impl->AvailVars[this->DimensionArray]["Shape"]);
  int offset = this->DimensionArrayAsCell ? 1 : 0;
  if (dims.size() == 3)
  {
    this->Dimension[0] = dims[0] + offset;
    this->Dimension[1] = dims[1] + offset;
    this->Dimension[2] = dims[2] + offset;
  }
  else if (dims.size() == 2)
  {
    this->Dimension[0] = dims[0] + offset;
    this->Dimension[1] = dims[1] + offset;
    this->Dimension[2] = 1;
  }
  else
  {
    vtkErrorMacro("Can not use the dimension of array "
      << this->DimensionArray
      << " to set the dimension of image data. Its size is neither 2 nor 3");
  }
}

//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::ConvertArraySelectionToInqVar()
{
  InquireVariablesType inqVars;
  for (auto& iter : this->Impl->ArraySelection)
  {
    if (iter.second) // Enabled by the user
    {
      std::vector<int> dims = parseDimensions(this->Impl->AvailVars[iter.first]["Shape"]);
      std::string arrayName = iter.first;
      if (dims.size() == 2)
      {
        if (dims[0] == this->Dimension[0] && dims[1] == this->Dimension[1])
        {
          inqVars.emplace_back(arrayName, VarType::PointData);
        }
        else if (dims[0] == this->Dimension[0] - 1 && dims[1] == this->Dimension[1] - 1)
        {
          inqVars.emplace_back(arrayName, VarType::CellData);
        }
      }
      else if (dims.size() == 3)
      {
        if (dims[0] == this->Dimension[0] && dims[1] == this->Dimension[1] &&
          dims[2] == this->Dimension[2])
        {
          inqVars.emplace_back(arrayName, VarType::PointData);
        }
        else if (dims[0] == this->Dimension[0] - 1 && dims[1] == this->Dimension[1] - 1 &&
          dims[2] == this->Dimension[2] - 1)
        {
          inqVars.emplace_back(arrayName, VarType::CellData);
        }
      }
      else
      {
        vtkWarningMacro("The dimension of array " << arrayName << " is not supported. Skipping");
      }
    }
  }
  this->Impl->InquiredVars = inqVars;
}

//----------------------------------------------------------------------------
void vtkADIOS2CoreImageReader::ReadImageBlocks(vtkMultiBlockDataSet* mbds)
{
  try
  {
    // One adios block is mapped to one vtk image data.
    size_t blockExtentI{ 0 };
    for (size_t blockI = this->Impl->BlockStart;
         blockI < this->Impl->BlockStart + this->Impl->BlockCount; blockI++)
    {
      vtkNew<vtkImageData> outputImage;
      outputImage->SetOrigin(this->Origin);
      outputImage->SetSpacing(this->Spacing);
      const auto& extents = this->Impl->BlockExtents[blockExtentI++];
      if (this->IsColumnMajor)
      {
        outputImage->SetExtent(
          extents[0], extents[1], extents[2], extents[3], extents[4], extents[5]);
      }
      else
      {
        outputImage->SetExtent(
          extents[4], extents[5], extents[2], extents[3], extents[0], extents[1]);
      }
      // The index of mbds starts from 0
      mbds->SetBlock(static_cast<unsigned int>(blockI - this->Impl->BlockStart), outputImage);
      // Fetch all datas for current image
      for (const auto& iter : this->Impl->InquiredVars)
      {
        std::string varName = iter.first;
        if (this->Impl->AvailVars.find(varName) == this->Impl->AvailVars.end())
        {
          vtkErrorMacro("Inquire variable " << varName << " cannot be found in the provided file");
          continue;
        }
        // TODO: Add validation for inquire variable's dimensions
        std::string typeStr = this->FetchTypeStringFromVarName(varName);
        if (typeStr.empty())
        {
          vtkErrorMacro("Cannot find a type for " << varName << " invalid name is provided");
          continue;
        }

        vtkSmartPointer<vtkAbstractArray> dataArray;
        // Use the adios_types_map in adios2 code base to generate types
        if (typeStr == "string")
        { // vtkStringArray uses vtkStdString instead of std::string. So we manually
          // do the work here
          auto varADIOS2 = this->Impl->AdiosIO.InquireVariable<std::string>(varName);
          varADIOS2.SetBlockSelection(blockI);
          varADIOS2.SetStepSelection({ this->Impl->RequestStep, 1 });

          vtkNew<vtkStringArray> array;
          dataArray = array;
          array->SetNumberOfComponents(1);
          array->SetName(varName.c_str());
          array->SetNumberOfTuples(static_cast<vtkIdType>(varADIOS2.SelectionSize()));
          this->Impl->BpReader.Get(varADIOS2, dynamic_cast<std::string*>(array->GetPointer(0)));
        }
        else if (typeStr == "char")
        {
          dataArray = this->PopulateDataArrayFromVar<char, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "int8_t")
        {
          dataArray = this->PopulateDataArrayFromVar<int8_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "uint8_t")
        {
          dataArray = this->PopulateDataArrayFromVar<uint8_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "int16_t")
        {
          dataArray = this->PopulateDataArrayFromVar<int16_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "uint16_t")
        {
          dataArray = this->PopulateDataArrayFromVar<uint16_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "int32_t")
        {
          dataArray = this->PopulateDataArrayFromVar<int32_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "uint32_t")
        {
          dataArray = this->PopulateDataArrayFromVar<uint32_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "int64_t")
        {
          dataArray = this->PopulateDataArrayFromVar<int64_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "uint64_t")
        {
          dataArray = this->PopulateDataArrayFromVar<uint64_t, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "float")
        {
          dataArray = this->PopulateDataArrayFromVar<float, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "double")
        {
          dataArray = this->PopulateDataArrayFromVar<double, NativeToVTKType>(varName, blockI);
        }
        else if (typeStr == "long double")
        {
          vtkWarningMacro(<< "ADIOS2 type long double is not supported yet. Skipping array "
                          << varName);
          continue;
        }
        else if (typeStr == "float complex")
        {
          vtkWarningMacro(<< "ADIOS2 type float complex is not supported yet. Skipping array "
                          << varName);
          continue;
        }
        else if (typeStr == "double complex")
        {
          vtkWarningMacro(<< "ADIOS2 type double complex is not supported yet. Skipping array "
                          << varName);
          continue;
        }

        if (dataArray && iter.second == VarType::CellData)
        {
          outputImage->GetCellData()->AddArray(dataArray);
        }
        else if (dataArray && iter.second == VarType::PointData)
        {
          outputImage->GetPointData()->AddArray(dataArray);
        }
        // Set active scalars if possible
        if (this->Impl->ActiveScalar.first == varName)
        {
          if (this->Impl->ActiveScalar.second == vtkADIOS2CoreImageReader::VarType::CellData)
          {
            outputImage->GetCellData()->SetActiveScalars(varName.c_str());
          }
          else
          {
            outputImage->GetPointData()->SetActiveScalars(varName.c_str());
          }
        }
        dataArray->Delete();
      }
    }
    this->Impl->BpReader.PerformGets();
  }
  catch (std::exception& e)
  {
    vtkErrorMacro(<< e.what());
  }
}

//----------------------------------------------------------------------------
bool vtkADIOS2CoreImageReader::GatherTimeSteps()
{

  std::string typeStr = this->FetchTypeStringFromVarName(this->TimeStepArray);
  if (typeStr.empty())
  {
    vtkErrorMacro(
      "Cannot find a type for time step " << this->TimeStepArray << " invalid name is provided");
    return false;
  }
  // FIXME: adios2 IO object returns an template dependent class instance instead of
  // a pointer or template independent object. Without using std::variant,
  // if statements are used to overcome the limitation
  // Use the adios_types_map in adios2 code base to generate types

  // It's a safe assumption that the type of time array should be one of the following types.
  if (typeStr == "int8_t")
  {
    this->GatherTimeStepsFromADIOSTimeArray<int8_t>();
  }
  else if (typeStr == "uint8_t")
  {
    this->GatherTimeStepsFromADIOSTimeArray<uint8_t>();
  }
  else if (typeStr == "int16_t")
  {
    this->GatherTimeStepsFromADIOSTimeArray<int16_t>();
  }
  else if (typeStr == "uint16_t")
  {
    this->GatherTimeStepsFromADIOSTimeArray<uint16_t>();
  }
  else if (typeStr == "int32_t")
  {
    this->GatherTimeStepsFromADIOSTimeArray<int32_t>();
  }
  else if (typeStr == "uint32_t")
  {
    this->GatherTimeStepsFromADIOSTimeArray<uint32_t>();
  }
  else if (typeStr == "int64_t")
  {
    this->GatherTimeStepsFromADIOSTimeArray<uint64_t>();
  }
  else if (typeStr == "float")
  {
    this->GatherTimeStepsFromADIOSTimeArray<float>();
  }
  else if (typeStr == "double")
  {
    this->GatherTimeStepsFromADIOSTimeArray<double>();
  }
  else
  {
    vtkErrorMacro("Type " << typeStr << " is not supported yet as a time array type in VTK");
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
template <typename T>
void vtkADIOS2CoreImageReader::CalculateWorkDistribution(const std::string& varName)
{
  this->Impl->BlockExtents.clear();
  auto var = this->Impl->AdiosIO.InquireVariable<T>(varName);
  size_t blockNum = this->Impl->BpReader.BlocksInfo(var, this->Impl->RequestStep).size();

#ifdef IOADIOS2_HAVE_MPI
  size_t rank = static_cast<size_t>(this->Controller->GetLocalProcessId());
  size_t procs = static_cast<size_t>(this->Controller->GetNumberOfProcesses());
#else
  size_t rank{ 0 }, procs{ 1 };
#endif
  // Decide how many blocks that current process shall read
  this->Impl->BlockCount = blockNum / procs;
  size_t leftOver = blockNum % procs;

  if (rank < leftOver)
  {
    this->Impl->BlockCount++;
    this->Impl->BlockStart = rank * (this->Impl->BlockCount);
  }
  else
  {
    this->Impl->BlockStart =
      leftOver * (this->Impl->BlockCount + 1) + (rank - leftOver) * this->Impl->BlockCount;
  }
  // Calculate the extent for each block
  for (size_t i = this->Impl->BlockStart; i < this->Impl->BlockStart + this->Impl->BlockCount; i++)
  {
    auto start = this->Impl->BpReader.BlocksInfo(var, this->Impl->RequestStep)[i].Start;
    auto count = this->Impl->BpReader.BlocksInfo(var, this->Impl->RequestStep)[i].Count;
    if (start.size() == 3 && count.size() == 3)
    {
      // We use the first available var in inquiredVars to init workdistribution
      int offSet = this->Impl->InquiredVars.begin()->second == VarType::PointData ? -1 : 0;
      this->Impl->BlockExtents.push_back(
        { static_cast<int>(start[0]), static_cast<int>(start[0] + count[0]) + offSet,
          static_cast<int>(start[1]), static_cast<int>(start[1] + count[1]) + offSet,
          static_cast<int>(start[2]), static_cast<int>(start[2] + count[2]) + offSet });
    }
    else if (start.size() == 2 && count.size() == 2)
    {
      // We use the first available var in inquiredVars to init workdistribution
      int offSet = this->Impl->InquiredVars.begin()->second == VarType::PointData ? -1 : 0;
      this->Impl->BlockExtents.push_back(
        { static_cast<int>(start[0]), static_cast<int>(start[0] + count[0]) + offSet,
          static_cast<int>(start[1]), static_cast<int>(start[1] + count[1]) + offSet, 0, 1 });
    }
    else
    {
      this->Impl->BlockExtents.push_back({});
    }
  }
}

//----------------------------------------------------------------------------
template <typename T, template <typename...> class U>
vtkSmartPointer<vtkAbstractArray> vtkADIOS2CoreImageReader::PopulateDataArrayFromVar(
  const std::string& varName, size_t blockIndex)
{
  vtkSmartPointer<vtkAbstractArray> array = vtkDataArray::CreateDataArray(U<T>::VTKType);
  try
  {
    auto varADIOS2 = this->Impl->AdiosIO.InquireVariable<T>(varName);
    varADIOS2.SetStepSelection({ this->Impl->RequestStep, 1 });
    varADIOS2.SetBlockSelection(blockIndex);

    array->SetNumberOfComponents(1);
    array->SetName(varName.c_str());
    array->SetNumberOfTuples(static_cast<vtkIdType>(varADIOS2.SelectionSize()));
    this->Impl->BpReader.Get(varADIOS2, static_cast<T*>(array->GetVoidPointer(0)));
  }
  catch (const std::exception& ex)
  {
    vtkErrorMacro("Fail to populate data array from variable " << varName << ex.what());
  }
  return array;
}

//----------------------------------------------------------------------------
template <typename T>
void vtkADIOS2CoreImageReader::GatherTimeStepsFromADIOSTimeArray()
{
  try
  {
    auto varADIOS2 = this->Impl->AdiosIO.InquireVariable<T>(this->TimeStepArray);

    size_t nSteps = varADIOS2.Steps();
    size_t stepsStart = varADIOS2.StepsStart();
    std::vector<int> dims = parseDimensions(this->Impl->AvailVars[this->TimeStepArray]["Shape"]);
    int multiplication{ 1 };
    std::for_each(dims.begin(), dims.end(), [&multiplication](int v) { multiplication *= v; });

    this->Impl->TimeSteps.clear();
    // A temporary vector to hold time steps since ADIOS requires the variable and array should have
    // same type
    std::vector<T> tempTimeSteps(nSteps, 0);
    if (multiplication == 1 || multiplication == static_cast<int>(nSteps))
    {
      // From Chuck: We should be able to read all steps at once but because of an ADIOS2 bug
      // we can only read one at a time
      for (size_t s = 0; s < nSteps; ++s)
      {
        varADIOS2.SetStepSelection({ stepsStart + s, 1 });
        this->Impl->BpReader.Get(varADIOS2, tempTimeSteps.data() + s);
      }
      this->Impl->BpReader.PerformGets();

      for (const auto v : tempTimeSteps)
      {
        this->Impl->TimeSteps.push_back(static_cast<double>(v));
      }
    }
    else
    {
      for (int i = 0; i < static_cast<int>(nSteps); i++)
      {
        this->Impl->TimeSteps.push_back(static_cast<double>(stepsStart + i));
      }
    }

    this->Impl->TimeStepsReverseMap.clear();
    for (size_t i = 0; i < this->Impl->TimeSteps.size(); ++i)
    {
      this->Impl->TimeStepsReverseMap[this->Impl->TimeSteps[i]] = i;
    }
  }
  catch (const std::exception& ex)
  {
    vtkErrorMacro("Fail to gather time steps from time array " << this->TimeStepArray << ex.what());
  }
}
