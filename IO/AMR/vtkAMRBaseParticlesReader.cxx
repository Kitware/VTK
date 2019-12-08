/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRBaseParticlesReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRBaseParticlesReader.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyData.h"

#include <cassert>

vtkAMRBaseParticlesReader::vtkAMRBaseParticlesReader() = default;

//------------------------------------------------------------------------------
vtkAMRBaseParticlesReader::~vtkAMRBaseParticlesReader()
{
  this->ParticleDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->ParticleDataArraySelection->Delete();
}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::GetNumberOfParticleArrays()
{
  assert(
    "pre: ParticleDataArraySelection is nullptr!" && (this->ParticleDataArraySelection != nullptr));
  return this->ParticleDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkAMRBaseParticlesReader::GetParticleArrayName(int index)
{
  assert("pre: array inded out-of-bounds!" && (index >= 0) &&
    (index < this->ParticleDataArraySelection->GetNumberOfArrays()));

  return this->ParticleDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::GetParticleArrayStatus(const char* name)
{
  assert("pre: array name is nullptr" && (name != nullptr));
  return this->ParticleDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::SetParticleArrayStatus(const char* name, int status)
{

  if (status)
  {
    this->ParticleDataArraySelection->EnableArray(name);
  }
  else
  {
    this->ParticleDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::SelectionModifiedCallback(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkAMRBaseParticlesReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::Initialize()
{
  this->SetNumberOfInputPorts(0);
  this->Frequency = 1;
  this->FilterLocation = 0;
  this->NumberOfBlocks = 0;
  this->Initialized = false;
  this->InitialRequest = true;
  this->FileName = nullptr;
  this->Controller = vtkMultiProcessController::GetGlobalController();

  for (int i = 0; i < 3; ++i)
  {
    this->MinLocation[i] = this->MaxLocation[i] = 0.0;
  }

  this->ParticleDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkAMRBaseParticlesReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->ParticleDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::InitializeParticleDataSelections()
{
  if (!this->InitialRequest)
  {
    return;
  }

  this->ParticleDataArraySelection->DisableAllArrays();
  this->InitialRequest = false;
}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::SetFileName(const char* fileName)
{

  if (this->FileName != nullptr)
  {
    if (strcmp(this->FileName, fileName) != 0)
    {
      this->Initialized = false;
      delete[] this->FileName;
      this->FileName = nullptr;
    }
    else
    {
      return;
    }
  }

  this->FileName = new char[strlen(fileName) + 1];
  strcpy(this->FileName, fileName);

  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkAMRBaseParticlesReader::IsParallel()
{
  if (this->Controller != nullptr && this->Controller->GetNumberOfProcesses() > 1)
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRBaseParticlesReader::IsBlockMine(const int blkIdx)
{
  if (!this->IsParallel())
  {
    return true;
  }

  int myRank = this->Controller->GetLocalProcessId();
  if (myRank == this->GetBlockProcessId(blkIdx))
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::GetBlockProcessId(const int blkIdx)
{
  if (!this->IsParallel())
  {
    return 0;
  }

  int N = this->Controller->GetNumberOfProcesses();
  return (blkIdx % N);
}

//------------------------------------------------------------------------------
bool vtkAMRBaseParticlesReader::CheckLocation(const double x, const double y, const double z)
{
  if (!this->FilterLocation)
  {
    return true;
  }

  double coords[3];
  coords[0] = x;
  coords[1] = y;
  coords[2] = z;

  for (int i = 0; i < 3; ++i)
  {
    if (this->MinLocation[i] > coords[i] || coords[i] > this->MaxLocation[i])
    {
      return false;
    }
  } // END for all dimensions

  return true;
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // STEP 0: Get the output object
  vtkInformation* outInf = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* mbds =
    vtkMultiBlockDataSet::SafeDownCast(outInf->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: output multi-block dataset object is nullptr" && (mbds != nullptr));

  // STEP 1: Read Meta-Data
  this->ReadMetaData();

  // STEP 2: Read blocks
  mbds->SetNumberOfBlocks(this->NumberOfBlocks);
  unsigned int blkidx = 0;
  for (; blkidx < static_cast<unsigned int>(this->NumberOfBlocks); ++blkidx)
  {
    if (this->IsBlockMine(blkidx))
    {
      vtkPolyData* particles = this->ReadParticles(blkidx);
      assert("particles dataset should not be nullptr!" && (particles != nullptr));

      mbds->SetBlock(blkidx, particles);
      particles->Delete();
    }
    else
    {
      mbds->SetBlock(blkidx, nullptr);
    }
  } // END for all blocks

  // STEP 3: Synchronize
  if (this->IsParallel())
  {
    this->Controller->Barrier();
  }

  return 1;
}
