/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriter2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLWriter2.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <numeric>

vtkCxxSetObjectMacro(vtkXMLWriter2, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkXMLWriter2::vtkXMLWriter2()
  : Controller(nullptr)
  , NumberOfGhostLevels(0)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkXMLWriter2::~vtkXMLWriter2()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
vtkTypeBool vtkXMLWriter2::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->Artifacts.clear();
    if (!this->RequestData(request, inputVector, outputVector))
    {
      this->DeleteArtifacts();
    }
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // Create data object output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkXMLWriter2::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  using SDDP = vtkStreamingDemandDrivenPipeline;

  // setup pipeline request
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(SDDP::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->NumberOfGhostLevels);
  inInfo->Set(SDDP::UPDATE_NUMBER_OF_PIECES(),
    this->Controller ? this->Controller->GetNumberOfProcesses() : 1);
  inInfo->Set(
    SDDP::UPDATE_PIECE_NUMBER(), this->Controller ? this->Controller->GetLocalProcessId() : 0);
  return 1;
}

//----------------------------------------------------------------------------
std::tuple<std::string, std::string, std::string> vtkXMLWriter2::SplitFileName(
  const std::string& inputName)
{
  // if it's a relative path, convert to full path first to avoid issues like
  // paraview/paraview#20840.
  std::string unixPath = vtksys::SystemTools::CollapseFullPath(inputName);
  vtksys::SystemTools::ConvertToUnixSlashes(unixPath);
  const auto path = vtksys::SystemTools::GetFilenamePath(unixPath);
  const auto fname = vtksys::SystemTools::GetFilenameName(unixPath);
  const auto fnameNoExt = vtksys::SystemTools::GetFilenameWithoutLastExtension(fname);
  const auto artifactsDir = (fname == fnameNoExt ? fname + std::string("_data") : fnameNoExt);
  return std::make_tuple(path, fname, artifactsDir);
}

//----------------------------------------------------------------------------
void vtkXMLWriter2::AddArtifact(const std::string& fname, bool isDir)
{
  this->Artifacts.emplace_back(fname, isDir);
}

//----------------------------------------------------------------------------
void vtkXMLWriter2::AddRootArtifact(const std::string& fname, bool isDir)
{
  if (this->Controller == nullptr || this->Controller->GetLocalProcessId() == 0)
  {
    this->AddArtifact(fname, isDir);
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriter2::DeleteArtifacts()
{
  vtkLogScopeF(INFO, "DeleteArtifacts");
  for (const auto& pair : this->Artifacts)
  {
    if (pair.second)
    {
      vtksys::SystemTools::RemoveADirectory(pair.first);
    }
    else
    {
      vtksys::SystemTools::RemoveFile(pair.first);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkXMLWriter2::MakeDirectory(const std::string& dirname) const
{
  int status = 0;
  if (this->Controller == nullptr || this->Controller->GetLocalProcessId() == 0)
  {
    status = vtksys::SystemTools::MakeDirectory(dirname) ? 1 : 0;
  }
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->Controller->Broadcast(&status, 1, 0);
  }
  return (status == 1);
}

//----------------------------------------------------------------------------
int vtkXMLWriter2::ExclusiveScanSum(vtkMultiProcessController* controller, int count)
{
  if (controller == nullptr || controller->GetNumberOfProcesses() <= 1)
  {
    return 0;
  }

  const int myRank = controller->GetLocalProcessId();
  const int numRanks = controller->GetNumberOfProcesses();

  std::vector<int> gatheredResult(numRanks);
  // need to use AllGather since vtkMultiProcessController does not support MPI_Scan equivalent yet.
  controller->AllGather(&count, gatheredResult.data(), 1);
  return std::accumulate(gatheredResult.begin(), std::next(gatheredResult.begin(), myRank), 0);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkXMLWriter2::Gather(vtkMultiProcessController* controller,
  const std::vector<std::string>& values, int destinationRank)
{
  if (controller == nullptr || controller->GetNumberOfProcesses() <= 1)
  {
    return values;
  }

  assert(destinationRank >= 0 && destinationRank < controller->GetNumberOfProcesses());

  vtkMultiProcessStream local;
  local << static_cast<int>(values.size());
  for (auto& value : values)
  {
    local << value;
  }

  std::vector<vtkMultiProcessStream> recvBuffer;
  controller->Gather(local, recvBuffer, destinationRank);

  if (controller->GetLocalProcessId() == destinationRank)
  {
    std::vector<std::string> result;
    for (auto& stream : recvBuffer)
    {
      int count;
      stream >> count;
      for (int cc = 0; cc < count; ++cc)
      {
        std::string fname;
        stream >> fname;
        result.push_back(fname);
      }
    }
    return result;
  }
  else
  {
    return {};
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriter2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "NumberOfGhostLevels: " << this->NumberOfGhostLevels << endl;
}
