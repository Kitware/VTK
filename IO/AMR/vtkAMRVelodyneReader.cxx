#include "vtkAMRVelodyneReader.h"
#include "vtkOverlappingAMR.h"

#include "vtkAMRBox.h"
#include "vtkAlgorithm.h"
#include "vtkByteSwap.h"
#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

#include "vtkCellData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedShortArray.h"

#include <cassert>
#include <cfloat>
#include <cmath>
#include <map>
#include <vector>
#define H5_USE_16_API
#include "vtk_hdf5.h"

#include "vtkAMRVelodyneReaderInternal.h"
vtkStandardNewMacro(vtkAMRVelodyneReader);

//-------------------------------------------------------------------
vtkAMRVelodyneReader::vtkAMRVelodyneReader()
{
  this->IsReady = false;
  this->Internal = new vtkAMRVelodyneReaderInternal;
  this->currentIndex = 0;
  this->Initialize();
}

vtkAMRVelodyneReader::~vtkAMRVelodyneReader()
{
  delete this->Internal;
  Internal = nullptr;
  this->Metadata = nullptr;
  for (unsigned int i = 0; i < this->amrVector.size(); i++)
  {
    this->amrVector[i]->Delete();
    this->amrVector[i] = nullptr;
  }
  this->amrVector.clear();
}

void vtkAMRVelodyneReader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkAMRVelodyneReader::SetFileName(const char* fileName)
{
  assert("pre: Internal Velodyne Reader is nullptr" && (this->Internal != nullptr));
  if (fileName && *fileName &&
    ((this->FileName == nullptr) || strcmp(fileName, this->FileName) != 0))
  {
    if (this->FileName)
    {
      delete[] this->FileName;
      this->FileName = nullptr;
    }

    this->FileName = new char[strlen(fileName) + 1];
    strcpy(this->FileName, fileName);
    this->FileName[strlen(fileName)] = '\0';

    this->IsReady = true;
    this->Internal->SetFileName(this->FileName);
    this->LoadedMetaData = false;

    this->SetUpDataArraySelections();
  }
  this->Modified();
}

void vtkAMRVelodyneReader::UpdateFileName(int index)
{
  const char* fN = this->fileList[index].c_str();
  if (this->FileName)
  {
    delete[] this->FileName;
    this->FileName = nullptr;
  }

  this->FileName = new char[strlen(fN) + 1];
  strcpy(this->FileName, fN);
  this->FileName[strlen(fN)] = '\0';
  this->Internal->SetFileName(this->FileName);
  this->Metadata = this->amrVector[index];
  this->currentIndex = static_cast<unsigned int>(index);
}

int vtkAMRVelodyneReader::RequestInformation(
  vtkInformation* rqst, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(rqst, inputVector, outputVector);
  this->FillMetaData();
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  assert("pre: output information object is nullptr" && (info != nullptr));
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->timeList.data(),
    static_cast<int>(this->timeList.size()));
  double timeRange[2];
  timeRange[0] = this->timeList.front();
  timeRange[1] = this->timeList.back();
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

int vtkAMRVelodyneReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  double requestedTime = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  int length = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int closestStep = 0;
  double minDist = -1;
  for (int cnt = 0; cnt < length; cnt++)
  {
    double tdist = (this->timeList[cnt] - requestedTime > requestedTime - this->timeList[cnt])
      ? this->timeList[cnt] - requestedTime
      : requestedTime - this->timeList[cnt];
    if (minDist < 0 || tdist < minDist)
    {
      minDist = tdist;
      closestStep = cnt;
    }
  }
  this->UpdateFileName(closestStep);
  this->ReadMetaData();
  if (!this->Metadata->HasChildrenInformation())
  {
    vtkTimerLog::MarkStartEvent("vtkAMRVelodyneReader::GenerateParentChildInformation");
    this->Metadata->GenerateParentChildInformation();
    vtkTimerLog::MarkEndEvent("vtkAMRVelodyneReader::GenerateParentChildInformation");
  }

  vtkInformation* dummy;
  dummy = nullptr;
  vtkInformationVector** dummyV;
  dummyV = nullptr;
  this->Modified();
  return this->Superclass::RequestData(dummy, dummyV, outputVector);
}

void vtkAMRVelodyneReader::ReadMetaData()
{
  assert("pre: Internal Velodyne Reader is nullptr" && (this->Internal != nullptr));
  this->Internal->ReadMetaData();
}

int vtkAMRVelodyneReader::GetNumberOfBlocks()
{
  assert("pre: Internal Velodyne Reader is nullptr" && (this->Internal != nullptr));
  if (!this->IsReady)
  {
    return 0;
  }
  this->Internal->ReadMetaData();
  return (this->Internal->nBlocks);
}

int vtkAMRVelodyneReader::GetNumberOfLevels()
{
  assert("pre: Internal Velodyne Reader is nullptr" && (this->Internal != nullptr));
  if (!this->IsReady)
  {
    return 0;
  }
  this->Internal->ReadMetaData();
  return (this->Internal->nLevels);
}

int vtkAMRVelodyneReader::GetBlockLevel(const int blockIdx)
{
  assert("pre: Internal Velodyne Reader is nullptr" && (this->Internal != nullptr));
  if (!this->IsReady)
  {
    return -1;
  }
  this->Internal->ReadMetaData();
  return (this->Internal->Blocks[blockIdx].Level);
}

int vtkAMRVelodyneReader::FillMetaData()
{
  assert("pre: Internal Velodyne Reader is nullptr" && (this->Internal != nullptr));
  // assert( "pre: metadata object is nullptr" && (this->Metadata != nullptr) );
  if (this->IsFileRead(this->FileName))
  {
    return (1);
  }

  this->ReadMetaData();
  vtkOverlappingAMR* cAMR = vtkOverlappingAMR::New();
  cAMR->Initialize(this->Internal->nLevels, this->Internal->blocksPerLevel.data());
  cAMR->SetGridDescription(VTK_XYZ_GRID);
  cAMR->SetOrigin(this->Internal->globalOrigin.data());
  int dims[3];
  double spacing[3];
  for (int i = 0; i < this->Internal->nBlocks; i++)
  {
    vtkAMRVelodyneReaderInternal::Block& theBlock = this->Internal->Blocks[i];
    int level = theBlock.Level;
    int id = theBlock.Index;
    CalculateBlockDims(this->Internal->blockDims.data(), theBlock.isFull, dims);
    CalculateSpacing(this->Internal->rootDX.data(), level, spacing);
    vtkAMRBox box(
      theBlock.Origin, dims, spacing, this->Internal->globalOrigin.data(), VTK_XYZ_GRID);
    cAMR->SetSpacing(level, spacing);
    cAMR->SetAMRBox(level, id, box);
    cAMR->SetAMRBlockSourceIndex(level, id, i);
  }
  cAMR->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->Internal->dataTime);
  this->amrVector.push_back(cAMR);
  this->timeList.push_back(this->Internal->dataTime);
  this->MarkFileAsRead(this->FileName);
  return 1;
}

vtkUniformGrid* vtkAMRVelodyneReader::GetAMRGrid(const int blockIdx)
{
  if (!this->IsReady)
  {
    return nullptr;
  }
  vtkAMRVelodyneReaderInternal::Block& theBlock = this->Internal->Blocks[blockIdx];
  int dims[3];
  CalculateBlockDims(this->Internal->blockDims.data(), theBlock.isFull, dims);
  vtkUniformGrid* ug = vtkUniformGrid::New();
  ug->SetDimensions(dims);
  ug->SetOrigin(theBlock.Origin);
  int level = theBlock.Level;
  double spacing[3];
  CalculateSpacing(this->Internal->rootDX.data(), level, spacing);
  ug->SetSpacing(spacing);
  return ug;
}

void vtkAMRVelodyneReader::GetAMRGridData(
  const int blockIdx, vtkUniformGrid* block, const char* field)
{
  assert("pre: Internal Velodyne Reader is nullptr" && (this->Internal != nullptr));
  this->Internal->ReadMetaData();
  this->Internal->GetBlockAttribute(field, blockIdx, block);
}

void vtkAMRVelodyneReader::SetUpDataArraySelections()
{
  if (this->IsFileRead(this->FileName))
  {
    return;
  }
  this->Internal->ReadMetaData();
  int numAttr = static_cast<int>(this->Internal->AttributeNames.size());
  for (int i = 0; i < numAttr; i++)
  {
    this->CellDataArraySelection->AddArray(this->Internal->AttributeNames[i].c_str());
  }
}

void vtkAMRVelodyneReader::CalculateSpacing(double* dx, int lvl, double* spacing)
{
  for (int i = 0; i < 3; i++)
  {
    spacing[i] = dx[i] / (static_cast<double>(std::pow(2, lvl)));
  }
}

void vtkAMRVelodyneReader::CalculateBlockDims(int* bDims, bool isFull, int* curDims)
{
  if (isFull)
  {
    for (int i = 0; i < 3; i++)
    {
      curDims[i] = 2 * bDims[i] + 1;
    }
  }
  else
  {
    for (int i = 0; i < 3; i++)
    {
      curDims[i] = bDims[i] + 1;
    }
  }
}

void vtkAMRVelodyneReader::MarkFileAsRead(char* fN)
{
  std::string tmp(fN);
  bool res = this->LoadedHash.insert(std::make_pair(tmp, true)).second;
  if (res)
  {
    this->fileList.push_back(tmp);
  }
}

bool vtkAMRVelodyneReader::IsFileRead(char* fN)
{
  if (this->LoadedHash.empty())
  {
    return false;
  }
  std::string tmp(fN);
  auto it = this->LoadedHash.find(tmp);
  if (it == this->LoadedHash.end())
  {
    return false;
  }

  return it->second;
}

bool vtkAMRVelodyneReader::IsFileRead(const char* fN)
{
  if (this->LoadedHash.empty())
  {
    return false;
  }
  std::string tmp(fN);
  auto it = this->LoadedHash.find(tmp);
  if (it == this->LoadedHash.end())
  {
    return false;
  }
  return it->second;
}

vtkOverlappingAMR* vtkAMRVelodyneReader::GetOutput()
{
  this->FillMetaData();
  vtkOverlappingAMR* amr = this->amrVector[this->currentIndex];
  amr->GenerateParentChildInformation();
  return amr;
}
