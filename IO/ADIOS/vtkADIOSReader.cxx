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
#include <map>
#include <sstream>
#include <stdexcept>
#include <limits>

#include "vtkObjectFactory.h"
#include "vtkType.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMPIController.h"
#include "vtkMPI.h"
#include "vtkNew.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkDataArray.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkFieldData.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkADIOSReader.h"
#include "vtkADIOSDirTree.h"
#include "ADIOSScalar.h"
#include "ADIOSVarInfo.h"
#include "ADIOSDefs.h"
#include "FunctionPointers.h"

#define TEST_OBJECT_TYPE(subDir, objType, blockId) \
  if(!subDir) \
    { \
    return NULL; \
    } \
 \
  const ADIOSScalar *v = subDir->GetScalar("DataObjectType"); \
  if(!(v && v->GetValues<vtkTypeUInt8>(this->RequestStepIndex)[blockId] == \
            objType)) \
    { \
    return NULL; \
    }

//----------------------------------------------------------------------------
typedef std::map<std::string, vtkADIOSDirTree> SubDirMap;
typedef std::map<std::string, const ADIOSVarInfo*> VarMap;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkADIOSReader);

//----------------------------------------------------------------------------

vtkADIOSReader::vtkADIOSReader()
: FileName(NULL), ReadMethod(vtkADIOSReader::BP), ReadMethodArguments(NULL),
  Tree(NULL), Reader(NULL), Controller(NULL), NumberOfPieces(-1)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkADIOSReader::~vtkADIOSReader()
{
  if(this->Tree)
    {
    delete this->Tree;
    }

  if(this->Reader)
    {
    delete this->Reader;
    }

  while(!this->PostReadOperations.empty())
    {
    delete this->PostReadOperations.front();
    this->PostReadOperations.pop();
    }
  this->SetFileName(NULL);
  this->SetReadMethodArguments(NULL);
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
template<typename TObjectFun, typename TObjectData, typename TReturn>
void vtkADIOSReader::AddPostReadOperation(TObjectData* obj,
  TReturn (TObjectFun::*fun)())
{
  this->PostReadOperations.push(
    new MemberFunction0Args<TObjectFun, TReturn>(obj, fun));
}

template<typename TObjectFun, typename TObjectData, typename TReturn,
  typename TArg1Fun, typename TArg1Data>
void vtkADIOSReader::AddPostReadOperation(TObjectData* obj,
  TReturn (TObjectFun::*fun)(TArg1Fun), TArg1Data arg1)
{
  this->PostReadOperations.push(
    new MemberFunction1Arg<TObjectFun, TReturn, TArg1Fun>(
      obj, fun, arg1));
}

template<typename TObjectFun, typename TObjectData, typename TReturn,
  typename TArg1Fun, typename TArg1Data,
  typename TArg2Fun, typename TArg2Data>
void vtkADIOSReader::AddPostReadOperation(TObjectData* obj,
  TReturn (TObjectFun::*fun)(TArg1Fun, TArg2Fun),
  TArg1Data arg1, TArg2Data arg2)
{
  this->PostReadOperations.push(
    new MemberFunction2Args<TObjectFun, TReturn, TArg1Fun, TArg2Fun>(
      obj, fun, arg1, arg2));
}

template<typename TObjectFun, typename TObjectData, typename TReturn,
  typename TArg1Fun, typename TArg1Data,
  typename TArg2Fun, typename TArg2Data,
  typename TArg3Fun, typename TArg3Data>
void vtkADIOSReader::AddPostReadOperation(TObjectData* obj,
  TReturn (TObjectFun::*fun)(TArg1Fun, TArg2Fun, TArg3Fun),
  TArg1Data arg1, TArg2Data arg2, TArg3Data arg3)
{
  this->PostReadOperations.push(
    new MemberFunction3Args<TObjectFun, TReturn, TArg1Fun, TArg2Fun, TArg3Fun>(
      obj, fun, arg1, arg2, arg3));
}

//----------------------------------------------------------------------------
void vtkADIOSReader::SetController(vtkMultiProcessController *controller)
{
  vtkMPIController *mpiController = vtkMPIController::SafeDownCast(controller);
  if(controller && !mpiController)
    {
    vtkErrorMacro("ADIOS Reader can only be used with an MPI controller");
    return;
    }

  vtkSetObjectBodyMacro(Controller, vtkMultiProcessController, controller);

  if(mpiController)
    {
    vtkMPICommunicator *comm = static_cast<vtkMPICommunicator *>(
      this->Controller->GetCommunicator());
    ADIOSReader::SetCommunicator(*comm->GetMPIComm()->GetHandle());
    }
}

//----------------------------------------------------------------------------
int vtkADIOSReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkADIOSReader::ProcessRequest(vtkInformation* request,
  vtkInformationVector** input, vtkInformationVector* output)
{
  // Make sure the ADIOS subsystem is initialized before processing any
  // sort of request.
  if(!this->Reader)
    {
    ADIOSReader::SetReadMethod(
      static_cast<ADIOS::ReadMethod>(this->ReadMethod),
      this->ReadMethodArguments ? this->ReadMethodArguments : "");
    this->Reader = new ADIOSReader;
    }

  if(!this->OpenAndReadMetadata())
    {
    return 0;
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, input, output) ? 1 : 0;
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, input, output) ? 1 : 0;
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, input, output) ? 1 : 0;
    }

  return this->Superclass::ProcessRequest(request, input, output);
}

//----------------------------------------------------------------------------
bool vtkADIOSReader::RequestInformation(vtkInformation *vtkNotUsed(req),
  vtkInformationVector **vtkNotUsed(input), vtkInformationVector *output)
{
  vtkInformation* outInfo = output->GetInformationObject(0);
  outInfo->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

  // Rank 0 reads attributes and time steps and sends to all other ranks
  if(this->Controller->GetLocalProcessId() == 0)
    {
    // 1: Retrieve the necessary attributes
    const std::vector<ADIOSAttribute*>& attrs = this->Reader->GetAttributes();
    typedef std::vector<ADIOSAttribute*>::const_iterator AttrIt;
    for(AttrIt a = attrs.begin(); a != attrs.end(); ++a)
      {
      if((*a)->GetName() == "::NumberOfPieces")
        {
        this->NumberOfPieces = (*a)->GetValue<int>();
        }
      }

    // 2: Make sure we have the ones we need
    if(this->NumberOfPieces == -1)
      {
      vtkWarningMacro(<< "NumberOfPieces attribute not present.  Assuming 1");
      this->NumberOfPieces = 1;
      }

    // 3: Retrieve the time steps
    const ADIOSScalar *varTimeSteps = this->Tree->GetScalar("TimeStamp");
    this->TimeSteps.clear();
    this->TimeSteps.resize(varTimeSteps->GetNumSteps());
    for(int t = 0; t < varTimeSteps->GetNumSteps(); ++t)
      {
      const std::vector<double>& values = varTimeSteps->GetValues<double>(t);
      this->TimeSteps[t] = values[0];
      }
    }

  // 4: Communicate metadata to all other ranks
  int msg1[2];
  if(this->Controller->GetLocalProcessId() == 0)
    {
    msg1[0] = this->NumberOfPieces;
    msg1[1] = this->TimeSteps.size();
    }
  this->Controller->Broadcast(msg1, 2, 0);
  if(this->Controller->GetLocalProcessId() != 0)
    {
    this->NumberOfPieces = msg1[0];
    this->TimeSteps.resize(msg1[1]);
    }
  this->Controller->Broadcast(&(*this->TimeSteps.begin()),
    this->TimeSteps.size(), 0);

  // Populate the inverse lookup, i.e. time step value to time step index
  this->TimeStepsIndex.clear();
  for(size_t i = 0; i < this->TimeSteps.size(); ++i)
    {
    this->TimeStepsIndex[this->TimeSteps[i]] = i;
    }

  // Copy the necessary values to the output info
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
    &(*this->TimeSteps.begin()), this->TimeSteps.size());

  double tRange[2];
  tRange[0] = *this->TimeSteps.begin();
  tRange[1] = *this->TimeSteps.rbegin();
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);

  return true;
}

//----------------------------------------------------------------------------
bool vtkADIOSReader::RequestUpdateExtent(vtkInformation *vtkNotUsed(req),
  vtkInformationVector **vtkNotUsed(input), vtkInformationVector *output)
{
  vtkInformation* outInfo = output->GetInformationObject(0);

  this->RequestNumberOfPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  this->RequestPiece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  this->RequestStep = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  std::map<double, size_t>::const_iterator idx = this->TimeStepsIndex.find(
    this->RequestStep);
  if(idx == this->TimeStepsIndex.end())
    {
    vtkWarningMacro(<< "Requested time step does not exist");
    return false;
    }
  this->RequestStepIndex = idx->second;

  return true;
}

//----------------------------------------------------------------------------
bool vtkADIOSReader::RequestData(vtkInformation *vtkNotUsed(req),
  vtkInformationVector **vtkNotUsed(input), vtkInformationVector *outputVector)
{
  // Get the output pipeline information and data object.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),
    this->RequestStep);

  // Set up multi-piece for paraview
  vtkNew<vtkMultiPieceDataSet> outputPieces;
  output->SetNumberOfBlocks(1);
  output->SetBlock(0, outputPieces.GetPointer());

  // Make sure the multi-piece has the "global view"
  outputPieces->SetNumberOfPieces(
    std::max(this->NumberOfPieces, this->RequestNumberOfPieces));

  // Cut out early if there's too many request pieces
  if(this->RequestPiece >= this->NumberOfPieces)
    {
    return true;
    }

  // Determine the range of blocks to be read
  int blockStart, blockEnd;
  int blocksPerProc = this->NumberOfPieces > this->RequestNumberOfPieces ?
    this->NumberOfPieces / this->RequestNumberOfPieces : 1;
  int blocksLeftOver = this->NumberOfPieces % blocksPerProc;
  if(this->RequestPiece < blocksLeftOver)
    {
    blockStart = (blocksPerProc + 1) * this->RequestPiece;
    blockEnd = blockStart + blocksPerProc;
    }
  else
    {
    blockStart = blocksPerProc * this->RequestPiece + blocksLeftOver;
    blockEnd = blockStart + blocksPerProc-1;
    }

  //if(this->Controller->GetLocalProcessId() == 0)
  //  {
  //  vtkWarningMacro(<< "Reading data for time " << this->RequestStep);
  //  }

  // Loop through the assigned blocks
  bool readSuccess = true;
  for(int blockId = blockStart; blockId <= blockEnd; ++blockId)
    {
    vtkDataObject *block;
    try
      {
      int objType = this->Tree->GetDir("/")->GetScalar("DataObjectType")
        ->GetValues<vtkTypeUInt8>(this->RequestStepIndex)[blockId];
      switch(objType)
        {
        case VTK_IMAGE_DATA:
          block = this->ReadObject<vtkImageData>("/", blockId); break;
        case VTK_POLY_DATA:
          block = this->ReadObject<vtkPolyData>("/", blockId); break;
        case VTK_UNSTRUCTURED_GRID:
          block = this->ReadObject<vtkUnstructuredGrid>("/", blockId); break;
        default:
          vtkErrorMacro(<< "Piece " << blockId << ": Unsupported object type");
          readSuccess = false;
          continue;
        }
      }
    catch(const std::runtime_error &e)
      {
      vtkErrorMacro(<< "Piece " << blockId << ": " << e.what());
      readSuccess = false;
      continue;
      }
    outputPieces->SetPiece(blockId, block);
    if(block)
      {
      block->Delete();
      }
    }

  // After all blocks have been scheduled, wait for the reads to process
  this->WaitForReads();

  // After reads have completed, execute all of the pending post-read
  // operations
  while(!this->PostReadOperations.empty())
    {
    this->PostReadOperations.front()->ExecVoid();
    delete this->PostReadOperations.front();
    this->PostReadOperations.pop();
    }

  return readSuccess;
}

//----------------------------------------------------------------------------
void vtkADIOSReader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << this->FileName << std::endl;
  os << indent << "Tree: " << std::endl;
  this->Tree->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
bool vtkADIOSReader::OpenAndReadMetadata(void)
{
  if(this->Reader->IsOpen())
    {
    return true;
    }

  if(!this->FileName)
    {
    return false;
    }

  try
    {
    this->Reader->OpenFile(this->FileName);
    this->Tree = new vtkADIOSDirTree;
    this->Tree->BuildDirTree(*this->Reader);
    }
  catch(const std::runtime_error&)
    {
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkADIOSReader::WaitForReads(void)
{
  this->Reader->ReadArrays();
}

//----------------------------------------------------------------------------
template<>
vtkImageData* vtkADIOSReader::ReadObject<vtkImageData>(
  const std::string& path, int blockId)
{
  vtkADIOSDirTree *subDir = this->Tree->GetDir(path);
  TEST_OBJECT_TYPE(subDir, VTK_IMAGE_DATA, blockId)

  // Avoid excessive validation and assume that if we have a vtkDataObjectField
  // then the remainder of the subdirectory will be in proper form

  vtkImageData *data = vtkImageData::New();
  this->ReadObject(subDir, data, blockId);

  return data;
}

//----------------------------------------------------------------------------
template<>
vtkPolyData* vtkADIOSReader::ReadObject<vtkPolyData>(
  const std::string& path, int blockId)
{
  vtkADIOSDirTree *subDir = this->Tree->GetDir(path);
  TEST_OBJECT_TYPE(subDir, VTK_POLY_DATA, blockId)

  // Avoid excessive validation and assume that if we have a vtkDataObjectField
  // then the remainder of the subdirectory will be in proper form

  vtkPolyData *data = vtkPolyData::New();
  this->ReadObject(subDir, data, blockId);

  return data;
}

//----------------------------------------------------------------------------
template<>
vtkUnstructuredGrid* vtkADIOSReader::ReadObject<vtkUnstructuredGrid>(
  const std::string& path, int blockId)
{
  vtkADIOSDirTree *subDir = this->Tree->GetDir(path);
  TEST_OBJECT_TYPE(subDir, VTK_UNSTRUCTURED_GRID, blockId)

  // Avoid excessive validation and assume that if we have a vtkDataObjectField
  // then the remainder of the subdirectory will be in proper form

  vtkUnstructuredGrid *data = vtkUnstructuredGrid::New();
  this->ReadObject(subDir, data, blockId);

  return data;
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const ADIOSVarInfo* info,
  vtkDataArray* data, int blockId)
{
  std::vector<size_t> dims;
  info->GetDims(dims, blockId);
  if(dims.size() == 2)
    {
    data->SetNumberOfComponents(dims[0]);
    data->SetNumberOfTuples(dims[1]);
    }
  else if(dims.size() == 1)
    {
    data->SetNumberOfComponents(1);
    data->SetNumberOfTuples(dims[0]);
    }
  else
    {
    throw std::runtime_error(info->GetName() +
      ": Num dims != 2 for data array");
    }

  // Only queue the read if there's data to be read
  if(dims[0] != 0 && dims[1] != 0)
    {
    this->Reader->ScheduleReadArray(info->GetId(), data->GetVoidPointer(0),
      this->RequestStepIndex, blockId);
    }
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const vtkADIOSDirTree *subDir,
  vtkCellArray* data, int blockId)
{
  data->SetNumberOfCells(
    subDir->GetScalar("NumberOfCells")->
      GetValues<vtkIdType>(this->RequestStepIndex)[blockId]);
  this->ReadObject((*subDir)["IndexArray"], data->GetData(), blockId);
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const vtkADIOSDirTree *subDir,
  vtkFieldData* data, int blockId)
{
  for(std::map<std::string, const ADIOSVarInfo*>::const_iterator a =
    subDir->Arrays.begin(); a != subDir->Arrays.end(); ++a)
    {
    vtkDataArray *da = vtkDataArray::CreateDataArray(a->second->GetType());

    da->SetName(a->first.c_str());
    this->ReadObject(a->second, da, blockId);
    this->AddPostReadOperation(data, &vtkFieldData::AddArray, da);
    this->AddPostReadOperation(da, &vtkObjectBase::Delete);
    }
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const vtkADIOSDirTree *subDir,
  vtkDataSetAttributes* data, int blockId)
{
  for(std::map<std::string, const ADIOSVarInfo*>::const_iterator a =
    subDir->Arrays.begin(); a != subDir->Arrays.end(); ++a)
    {
    const std::string& name = a->first;
    vtkDataArray *da = vtkDataArray::CreateDataArray(a->second->GetType());

    da->SetName(name.c_str());
    this->ReadObject(a->second, da, blockId);

    if(name == "Scalars_")
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::SetScalars, da);
      }
    else if(name == "Vectors_")
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::SetVectors, da);
      }
    else if(name == "Normals_")
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::SetNormals, da);
      }
    else if(name == "TCoords_")
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::SetTCoords, da);
      }
    else if(name == "Tensors_")
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::SetTensors, da);
      }
    else if(name == "GlobalIds_")
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::SetGlobalIds,
        da);
      }
    else if(name == "PedigreeIds_")
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::SetPedigreeIds,
        da);
      }
    else
      {
      this->AddPostReadOperation(data, &vtkDataSetAttributes::AddArray, da);
      }
    this->AddPostReadOperation(da, &vtkObjectBase::Delete);
    }
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const vtkADIOSDirTree *subDir,
  vtkDataSet* data, int blockId)
{
  const vtkADIOSDirTree *d;

  if((d = subDir->GetDir("FieldData")))
    {
    this->ReadObject(d, data->GetFieldData(), blockId);
    }
  if((d = subDir->GetDir("CellData")))
    {
    this->ReadObject(d, data->GetCellData(), blockId);
    }
  if((d = subDir->GetDir("PointData")))
    {
    this->ReadObject(d, data->GetPointData(), blockId);
    }
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const vtkADIOSDirTree *subDir,
  vtkImageData* data, int blockId)
{
  const int &rsi = this->RequestStepIndex;

  data->SetOrigin(
    subDir->GetScalar("OriginX")->GetValues<double>(rsi)[blockId],
    subDir->GetScalar("OriginY")->GetValues<double>(rsi)[blockId],
    subDir->GetScalar("OriginZ")->GetValues<double>(rsi)[blockId]);
  data->SetSpacing(
    subDir->GetScalar("SpacingX")->GetValues<double>(rsi)[blockId],
    subDir->GetScalar("SpacingY")->GetValues<double>(rsi)[blockId],
    subDir->GetScalar("SpacingZ")->GetValues<double>(rsi)[blockId]);
  data->SetExtent(
    subDir->GetScalar("ExtentXMin")->GetValues<int>(rsi)[blockId],
    subDir->GetScalar("ExtentXMax")->GetValues<int>(rsi)[blockId],
    subDir->GetScalar("ExtentYMin")->GetValues<int>(rsi)[blockId],
    subDir->GetScalar("ExtentYMax")->GetValues<int>(rsi)[blockId],
    subDir->GetScalar("ExtentZMin")->GetValues<int>(rsi)[blockId],
    subDir->GetScalar("ExtentZMax")->GetValues<int>(rsi)[blockId]);

  this->ReadObject(subDir->GetDir("DataSet"), static_cast<vtkDataSet*>(data),
    blockId);
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const vtkADIOSDirTree *subDir,
  vtkPolyData* data, int blockId)
{
  const ADIOSVarInfo *v;
  if((v = (*subDir)["Points"]))
    {
    vtkPoints *p = vtkPoints::New();
    this->ReadObject(v, p->GetData(), blockId);
    this->AddPostReadOperation(data, &vtkPolyData::SetPoints, p);
    this->AddPostReadOperation(p, &vtkObjectBase::Delete);
    }

  const vtkADIOSDirTree *d;
  if((d = subDir->GetDir("Verticies")))
    {
    vtkCellArray *cells = vtkCellArray::New();
    this->ReadObject(d, cells, blockId);
    this->AddPostReadOperation(data, &vtkPolyData::SetVerts, cells);
    this->AddPostReadOperation(cells, &vtkObjectBase::Delete);
    }
  if((d = subDir->GetDir("Lines")))
    {
    vtkCellArray *cells = vtkCellArray::New();
    this->ReadObject(d, cells, blockId);
    this->AddPostReadOperation(data, &vtkPolyData::SetLines, cells);
    this->AddPostReadOperation(cells, &vtkObjectBase::Delete);
    }
  if((d = subDir->GetDir("Polygons")))
    {
    vtkCellArray *cells = vtkCellArray::New();
    this->ReadObject(d, cells, blockId);
    this->AddPostReadOperation(data, &vtkPolyData::SetPolys, cells);
    this->AddPostReadOperation(cells, &vtkObjectBase::Delete);
    }
  if((d = subDir->GetDir("Strips")))
    {
    vtkCellArray *cells = vtkCellArray::New();
    this->ReadObject(d, cells, blockId);
    this->AddPostReadOperation(data, &vtkPolyData::SetStrips, cells);
    this->AddPostReadOperation(cells, &vtkObjectBase::Delete);
    }

  this->ReadObject(subDir->GetDir("DataSet"), static_cast<vtkDataSet*>(data),
    blockId);
}

//----------------------------------------------------------------------------
void vtkADIOSReader::ReadObject(const vtkADIOSDirTree *subDir,
  vtkUnstructuredGrid* data, int blockId)
{
  const ADIOSVarInfo *v;
  if((v = (*subDir)["Points"]))
    {
    vtkPoints *p = vtkPoints::New();
    this->ReadObject(v, p->GetData(), blockId);
    this->AddPostReadOperation(data, &vtkUnstructuredGrid::SetPoints, p);
    this->AddPostReadOperation(p, &vtkObjectBase::Delete);
    }

  const ADIOSVarInfo *vCta = (*subDir)["CellTypes"];
  const ADIOSVarInfo *vCla = (*subDir)["CellLocations"];
  const vtkADIOSDirTree *dCa = subDir->GetDir("Cells");
  if(vCta && vCla && dCa)
    {
    vtkUnsignedCharArray *cta = vtkUnsignedCharArray::New();
    vtkIdTypeArray *cla = vtkIdTypeArray::New();
    vtkCellArray *ca = vtkCellArray::New();
    this->ReadObject(vCta, cta, blockId);
    this->ReadObject(vCla, cla, blockId);
    this->ReadObject(dCa, ca, blockId);
    this->AddPostReadOperation(data, &vtkUnstructuredGrid::SetCells, cta, cla,
      ca);
    this->AddPostReadOperation(cta, &vtkObjectBase::Delete);
    this->AddPostReadOperation(cla, &vtkObjectBase::Delete);
    this->AddPostReadOperation(ca, &vtkObjectBase::Delete);
    }

  this->ReadObject(subDir->GetDir("DataSet"), static_cast<vtkDataSet*>(data),
    blockId);
}

//----------------------------------------------------------------------------
//Cleanup
#undef TEST_OBJECT_TYPE
